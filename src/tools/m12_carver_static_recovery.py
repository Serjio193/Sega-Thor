"""Deterministic whole-ROM static consumer recovery for M12 Carver.

This pass consumes existing static reports only. It records typed references,
parser-boundary observations and family-level blockers in the Stage 1
IntervalDB. It deliberately has no ownership writer: existing promoters and
their exact transactions remain authoritative.
"""

from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re
import zlib

from m12_carver import IntervalDB, canonical, bounds


SCHEMA = "oasis.m68k.m12-carver.static-recovery.v1"
STATIC_WORDS = ("re-slice", "mass-verify", "candidate-map", "ghidra-map",
                "re-program", "cfg-audit", "re-resolution", "reachable-closure",
                "re-explore", "promotion", "m12-auto", "static", "screen-resource",
                "pointer-resource")
EXCLUDED_WORDS = ("m12-carver", "global-sweep", "emulator-trace", "natural-reach",
                  "gpgx", "re-trace", "runtime")
HEX_RANGE = re.compile(r"^0x[0-9a-fA-F]+$")
UNKNOWN_KINDS = {"UNKNOWN", "UNKNOWN_DATA", "UNKNOWN_WITH_EVIDENCE"}


def number(value):
    if isinstance(value, bool) or value is None:
        return None
    if isinstance(value, int):
        return value
    if isinstance(value, str) and (HEX_RANGE.match(value.strip()) or
                                   value.strip().lstrip("-").isdigit()):
        try:
            return int(value, 0)
        except ValueError:
            return None
    return None


def walk(value):
    if isinstance(value, dict):
        yield value
        for child in value.values():
            yield from walk(child)
    elif isinstance(value, list):
        for child in value:
            yield from walk(child)


def static_schema(schema):
    text = str(schema).lower()
    return any(word in text for word in STATIC_WORDS) and not any(
        word in text for word in EXCLUDED_WORDS)


def discover(root, rom_sha256, output):
    artifacts = []
    for path in sorted(root.rglob("*.json"), key=lambda item: str(item).lower()):
        if path.resolve() == output.resolve():
            continue
        try:
            payload = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            continue
        if not isinstance(payload, dict) or not static_schema(payload.get("schema")):
            continue
        identity = payload.get("rom_sha256") or payload.get("canonical_rom", {}).get("sha256")
        if identity and str(identity).lower() != rom_sha256.lower():
            continue
        artifacts.append((path, payload))
    return artifacts


def target_values(value):
    if isinstance(value, list):
        for item in value:
            yield from target_values(item)
    elif isinstance(value, dict):
        parsed = number(value.get("target", value.get("address")))
        if parsed is not None:
            yield parsed
    else:
        parsed = number(value)
        if parsed is not None:
            yield parsed


def source_pc(item):
    for key in ("source_pc", "reader_pc", "pc", "instruction", "function", "source",
                "entry_point", "entry"):
        value = number(item.get(key))
        if value is not None:
            return value
    return None


def exact_contract(item):
    if item.get("exact_boundary") is True or item.get("verified") is True:
        return True
    statuses = (item.get("boundary_status"), item.get("contract_status"),
                item.get("status"), item.get("confidence"))
    return any(str(value).upper() in {"EXACT", "CLOSED", "CONFIRMED"}
               for value in statuses if value is not None)


def contract_text(item):
    keys = ("contract", "boundary_contract", "boundary_status", "size",
            "stride", "count", "sentinel", "decoder", "termination")
    return canonical({key: item[key] for key in keys if key in item})


def gap_for(db, start, end):
    return next((item for item in db.find_ranges(start, end)
                 if item["source_kind"] in UNKNOWN_KINDS), None)


def safe_bounds(item):
    try:
        return bounds(item)
    except (TypeError, ValueError):
        return None


def add_reference(db, references, seen, target, pc, mechanism, source_ref,
                  context, rom_end):
    if target is None or not (0 <= target < rom_end):
        return
    target_range = next((item for item in db.ranges
                         if item["start"] <= target < item["end"]), None)
    if mechanism == "CODE_TO_ROM_RANGE" and target_range:
        label = str(target_range.get("classification", "")).upper()
        if "TABLE" in label or "POINTER" in label:
            mechanism = "CODE_TO_POINTER_TABLE"
    key = (pc, target, mechanism, context.get("parser"), context.get("consumer"))
    if key in seen:
        return
    seen.add(key)
    evidence = {"type": mechanism, "producer": "m12_static_consumer_recovery",
                "confidence": "EVIDENCE_ONLY", "start": target, "end": target + 1,
                "source_ref": source_ref, "source_pc": pc,
                "parser": context.get("parser"), "consumer": context.get("consumer"),
                "details": {"mechanism": mechanism,
                            "constant_propagation": context.get("constant_propagation", False)}}
    evidence_id = db.add_evidence(evidence)
    source_node = f"code:{pc:06X}" if pc is not None else f"static:{source_ref}"
    target_node = target_range["id"] if target_range else f"rom:{target:06X}"
    db.add_edge(source_node, target_node, mechanism, [evidence_id])
    references.append({"id": evidence_id, "start": target, "end": target + 1,
                       "source_pc": pc, "mechanism": mechanism, "source_ref": source_ref,
                       "parser": context.get("parser"), "consumer": context.get("consumer"),
                       "target_range": target_node})


def slice_references(db, payload, source_ref, references, seen, rom_end):
    entry = number(payload.get("entry_point"))
    for item in payload.get("instructions", []):
        if not isinstance(item, dict):
            continue
        pc = source_pc(item) or entry
        context = {"parser": payload.get("parser"), "consumer": payload.get("consumer"),
                   "constant_propagation": bool(item.get("constant_propagation"))}
        for ref in item.get("memory_references", []):
            if not isinstance(ref, dict):
                continue
            target = number(ref.get("address", ref.get("target")))
            if target is None:
                continue
            mechanism = "CODE_TO_POINTER_TABLE" if "table" in str(ref).lower() else "CODE_TO_ROM_RANGE"
            add_reference(db, references, seen, target, pc, mechanism, source_ref, context, rom_end)
        for target in target_values(item.get("immediate_constants", [])):
            add_reference(db, references, seen, target, pc, "CODE_TO_ROM_RANGE",
                          source_ref, context, rom_end)
        target = number(item.get("direct_target"))
        if target is not None:
            add_reference(db, references, seen, target, pc, "CODE_TO_ROM_RANGE",
                          source_ref, context, rom_end)
    for item in payload.get("direct_control_flow", []):
        if not isinstance(item, dict):
            continue
        add_reference(db, references, seen, number(item.get("target")),
                      number(item.get("source")), "CODE_XREF", source_ref, {}, rom_end)


def generic_references(db, payload, source_ref, references, seen, rom_end):
    for item in walk(payload):
        pc = source_pc(item)
        if pc is None:
            continue
        parser = item.get("parser") or payload.get("parser")
        consumer = item.get("consumer") or payload.get("consumer")
        context = {"parser": parser if isinstance(parser, str) else None,
                   "consumer": consumer if isinstance(consumer, str) else None,
                   "constant_propagation": bool(item.get("constant_propagation"))}
        for key in ("targets", "pointer_targets", "rom_targets", "xrefs", "references",
                    "pointers", "target"):
            if key not in item:
                continue
            for target in target_values(item[key]):
                text = f"{key} {item.get('type', '')} {item.get('kind', '')}".lower()
                if "selector" in text:
                    mechanism = "SELECTOR_TO_TABLE"
                elif "table" in text or key in {"pointers", "pointer_targets"}:
                    mechanism = "TABLE_TO_ROM_RANGE"
                elif context["parser"]:
                    mechanism = "PARSER_TO_RANGE"
                else:
                    mechanism = "CODE_TO_ROM_RANGE"
                add_reference(db, references, seen, target, pc, mechanism,
                              source_ref, context, rom_end)


def table_references(db, payload, source_ref, references, seen, rom_end):
    """Recover explicit table fields without treating arbitrary bytes as owned."""
    for item in walk(payload):
        fields = item.get("fields")
        if not isinstance(fields, list):
            continue
        table_pc = number(item.get("address", item.get("start")))
        if table_pc is None:
            continue
        context = {"parser": item.get("parser") if isinstance(item.get("parser"), str) else None,
                   "consumer": item.get("consumer") if isinstance(item.get("consumer"), str) else None,
                   "constant_propagation": True}
        for target in target_values(fields):
            if target >= 0x1000:
                add_reference(db, references, seen, target, table_pc,
                              "TABLE_TO_ROM_RANGE", source_ref, context, rom_end)


def collect_contracts(payload, source_ref, contracts, contract_keys):
    for item in walk(payload):
        item_bounds = safe_bounds(item)
        if item_bounds is None:
            continue
        text = str(item).lower()
        if not any(word in text for word in ("contract", "boundary", "stride", "sentinel",
                                             "termination", "decoder", "count")):
            continue
        record = {"start": item_bounds[0], "end": item_bounds[1],
                  "exact": exact_contract(item), "source_ref": source_ref,
                  "contract": contract_text(item), "parser": item.get("parser"),
                  "consumer": item.get("consumer")}
        key = canonical(record)
        if key not in contract_keys:
            contract_keys.add(key)
            contracts.append(record)


def collect_candidates(payload, source_ref, candidates):
    for item in walk(payload):
        if not isinstance(item, dict) or not any(key in item for key in ("failure_reasons", "structural_classification")):
            continue
        item_bounds = safe_bounds(item)
        if item_bounds is None:
            entry = number(item.get("entry")) or 0
            item_bounds = (entry, entry + 1)
        candidates.append({"start": item_bounds[0], "end": item_bounds[1],
                           "source_ref": source_ref,
                           "boundary_status": item.get("boundary_status", "UNKNOWN"),
                           "structural_classification": item.get("structural_classification"),
                           "failure_reasons": sorted(item.get("failure_reasons", []))})


def baseline_blockers(root):
    path = root / "m12-carver-m12c3-global-sweep-d" / "global_sweep_report.json"
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}, {}
    mapping = {(item["start"], item["end"]): item.get("blocker", "G")
               for item in payload.get("blocker_census", {}).get("ranges", [])}
    return mapping, payload.get("conflicts", {})


def family_map(db, references, blockers):
    by_gap = defaultdict(list)
    for ref in references:
        gap = gap_for(db, ref["start"], ref["end"])
        if gap:
            by_gap[gap["id"]].append(ref)
    families = defaultdict(list)
    for gap in db.ranges:
        if gap["source_kind"] not in UNKNOWN_KINDS:
            continue
        refs = by_gap.get(gap["id"], [])
        context = next((ref.get("parser") for ref in refs if ref.get("parser")), None)
        context = context or next((ref.get("consumer") for ref in refs if ref.get("consumer")), None)
        context = context or (f"pc:{refs[0]['source_pc']:06X}" if refs and refs[0]["source_pc"] is not None else None)
        context = context or f"blocker:{blockers.get((gap['start'], gap['end']), 'G')}"
        family_id = "family:" + hashlib.sha256(context.encode()).hexdigest()[:16]
        families[family_id].append(gap)
    output = []
    for family_id, members in families.items():
        output.append({"id": family_id, "gap_ids": [item["id"] for item in members],
                       "start": min(item["start"] for item in members),
                       "end": max(item["end"] for item in members),
                       "bytes": sum(item["end"] - item["start"] for item in members),
                       "member_ranges": len(members), "common_ancestry": family_id[7:],
                       "promotion": "REJECTED_NO_EXACT_STATIC_CONTRACT"})
    return sorted(output, key=lambda item: (-item["bytes"], item["start"], item["id"])), by_gap


def run(manifest_path, rom_path, input_root, output):
    manifest_path, rom_path, input_root, output = map(lambda item: Path(item).resolve(),
                                                       (manifest_path, rom_path, input_root, output))
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    rom = rom_path.read_bytes()
    if len(rom) != int(manifest["rom_size"]) or hashlib.sha256(rom).hexdigest() != manifest["rom_sha256"]:
        raise ValueError("canonical ROM does not match manifest")
    output.mkdir(parents=True, exist_ok=False)
    db = IntervalDB.from_manifest(manifest, manifest_path)
    artifacts = discover(input_root, manifest["rom_sha256"], output / "static_recovery_report.json")
    references, contracts, candidates, seen, contract_keys = [], [], [], set(), set()
    for path, payload in artifacts:
        source_ref = str(path.relative_to(input_root))
        if payload.get("schema") == "oasis.m68k.re-slice.v1":
            slice_references(db, payload, source_ref, references, seen, db.rom_end)
        generic_references(db, payload, source_ref, references, seen, db.rom_end)
        table_references(db, payload, source_ref, references, seen, db.rom_end)
        collect_contracts(payload, source_ref, contracts, contract_keys)
        collect_candidates(payload, source_ref, candidates)
    fixed = db.fixed_point()
    blockers, baseline_conflicts = baseline_blockers(input_root)
    families, by_gap = family_map(db, references, blockers)
    census = Counter(ref["mechanism"] for ref in references)
    blocker_rows = []
    for gap in db.ranges:
        if gap["source_kind"] not in UNKNOWN_KINDS:
            continue
        refs = by_gap.get(gap["id"], [])
        if gap["conflicts"]:
            blocker = "F"
        elif any(ref.get("parser") for ref in refs):
            blocker = "C"
        else:
            blocker = blockers.get((gap["start"], gap["end"]), "G")
        family_id = next((family["id"] for family in families if gap["id"] in family["gap_ids"]), None)
        blocker_rows.append({"id": gap["id"], "start": gap["start"], "end": gap["end"],
                             "size": gap["end"] - gap["start"], "blocker": blocker,
                             "static_references": len(refs), "family_id": family_id})
    blocker_summary = {}
    for code in sorted({row["blocker"] for row in blocker_rows}):
        rows = [row for row in blocker_rows if row["blocker"] == code]
        blocker_summary[code] = {"ranges": len(rows), "bytes": sum(row["size"] for row in rows)}
    db_payload = db.interval_db()
    static_state = {"references": references, "contracts": contracts, "candidates": candidates}
    contract_kinds = Counter()
    exact_unknown = 0
    for item in contracts:
        text = item["contract"]
        if '"count"' in text and '"stride"' in text:
            kind = "count_times_stride"
        elif "sentinel" in text:
            kind = "sentinel_termination"
        elif "offset" in text:
            kind = "offset_table_bounded"
        elif "decoder" in text or "termination" in text:
            kind = "decoder_eos_or_termination"
        elif '"size"' in text:
            kind = "fixed_size"
        else:
            kind = "unresolved_contract_shape"
        contract_kinds[kind] += 1
        if item["exact"] and gap_for(db, item["start"], item["end"]):
            exact_unknown += item["end"] - item["start"]
    gap_details, _ = db.gap_report()
    report = {"schema": SCHEMA, "deterministic": True,
              "baseline": {"source_owned_bytes": db.manifest_source_owned_bytes,
                            "unknown_ranges": len(blocker_rows), "blockers": {
                                code: {"ranges": sum(1 for value in blockers.values() if value == code),
                                       "bytes": sum(g["end"] - g["start"] for g in db.ranges
                                                     if (g["start"], g["end"]) in blockers and
                                                     blockers[(g["start"], g["end"])] == code)}
                                for code in sorted(set(blockers.values()))}},
              "final": {"source_owned_bytes": db.source_owned_bytes(),
                        "gained_source_owned_bytes": db.source_owned_bytes() - db.manifest_source_owned_bytes,
                        "source_owned_unchanged": db.source_owned_bytes() == db.manifest_source_owned_bytes},
              "canonical_rom": {"size": len(rom), "sha256": hashlib.sha256(rom).hexdigest(),
                                "sha1": hashlib.sha1(rom).hexdigest(),
                                "crc32": f"{zlib.crc32(rom) & 0xFFFFFFFF:08X}"},
              "static_artifacts": {"count": len(artifacts), "paths": [str(path.relative_to(input_root)) for path, _ in artifacts]},
              "reference_census": {"total": len(references), "by_mechanism": dict(sorted(census.items())),
                                   "references": sorted(references, key=canonical)},
              "consumer_families": families,
              "parser_contracts": {"records": sorted(contracts, key=canonical),
                                   "by_contract": dict(sorted(contract_kinds.items())),
                                   "exact_unknown_boundary_bytes": exact_unknown},
              "container_bank_candidates": sorted(candidates, key=canonical),
              "code_data_separation": {"static_code_candidates": len(candidates),
                                        "exact_reassemblable_unknown_bytes": 0, "promoted_code_bytes": 0},
              "candidate_decisions": {"candidates_seen": len(candidates), "promoted": 0,
                                      "rejected": len(candidates), "reason": "no new exact ownership transaction"},
              "conflicts": {"resolved": 0, "remaining": int(baseline_conflicts.get("remaining", 0)) + len(db.conflicts),
                            "policy": "candidate overlap remains explicit and blocking"},
              "remaining_unknown": {"ranges": blocker_rows, "gap_details": gap_details,
                                    "by_blocker": blocker_summary,
                                    "largest_families": families[:20]},
              "fixed_point": fixed,
              "hashes": {"interval_db_sha256": hashlib.sha256(canonical(db_payload).encode()).hexdigest(),
                         "evidence_state_sha256": hashlib.sha256(canonical(static_state).encode()).hexdigest()},
              "stop_reason": "static consumer evidence reached fixed point; remaining ranges require exact ownership contracts or new evidence class"}
    (output / "interval_db.json").write_text(json.dumps(db_payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    (output / "static_recovery_report.json").write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return report
