"""Global deterministic runtime-evidence sweep for M12 Carver.

The sweep consumes only evidence already materialized under an input root.  It
normalizes runtime reads and execution facts into typed spans, feeds every
recognized existing producer into :class:`IntervalDB`, and emits a complete
whole-ROM coverage and blocker census.  It never changes the source manifest
or infers SOURCE_OWNED from observation alone.
"""

from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import shutil
import zlib

from m12_carver import IntervalDB, canonical, intersects, parse_int


ROM_END = 0x300000
REPORT_SCHEMA = "oasis.m68k.m12-carver.global-sweep.v1"
KNOWN_PREFIXES = (
    "oasis.m68k.m12-", "oasis.m12.graphics-", "oasis.gpgx.",
    "oasis.m68k.emulator-trace.", "oasis.m68k.natural-reach.",
    "oasis.m68k.re-trace.", "oasis.m68k.re-slice.",
    "oasis.m68k.re-mass-verify.", "oasis.m68k.re-candidate-map.",
    "oasis.m68k.ghidra-map.", "oasis.m68k.screen-resource-boundary.",
    "oasis.m12.pointer-resource-boundary.", "oasis.hybrid-poc.",
)
SKIP_PREFIXES = ("oasis.m68k.m12-carver.",)
RUNTIME_SCHEMAS = {
    "oasis.gpgx.rom.reader.correlation.v1",
    "oasis.gpgx.runtime.execution.evidence.v1",
    "oasis.m68k.emulator-trace.v1",
    "oasis.m68k.natural-reach.v1",
    "oasis.m68k.re-trace.v1",
}


def _int(value):
    return parse_int(value)


def _span(item):
    if not isinstance(item, dict):
        return None
    if "start" in item and "end" in item:
        return _int(item["start"]), _int(item["end"])
    if "region_start" in item and "region_end" in item:
        return _int(item["region_start"]), _int(item["region_end"])
    if "address" in item:
        start = _int(item["address"])
        return start, start + 1
    return None


def _valid_span(span):
    return span and 0 <= span[0] < span[1] <= ROM_END


def _rom_matches(payload, rom_sha256):
    values = []
    for key in ("rom_sha256", "canonical_rom_sha256"):
        value = payload.get(key)
        if value:
            values.append(str(value).lower())
    provenance = payload.get("provenance")
    if isinstance(provenance, dict) and provenance.get("canonical_rom_sha256"):
        values.append(str(provenance["canonical_rom_sha256"]).lower())
    return not values or all(value == rom_sha256.lower() for value in values)


def _recognized(schema):
    return (schema and schema.startswith(KNOWN_PREFIXES)
            and not schema.startswith(SKIP_PREFIXES))


def discover(input_root, rom_sha256, output=None):
    """Return deterministic, ROM-compatible evidence artifacts.

    Identical payloads are retained as separate artifacts for repetition and
    scenario accounting, but ingestion later deduplicates their content.
    """
    result = []
    output = Path(output).resolve() if output else None
    for path in sorted(Path(input_root).resolve().rglob("*.json"), key=str):
        if output and (path == output or output in path.parents):
            continue
        try:
            payload = json.loads(path.read_text())
        except (OSError, ValueError):
            continue
        if not isinstance(payload, dict):
            continue
        schema = str(payload.get("schema", ""))
        if _recognized(schema) and _rom_matches(payload, rom_sha256):
            result.append({"path": str(path), "payload": payload,
                           "schema": schema,
                           "digest": hashlib.sha256(canonical(payload).encode()).hexdigest()})
    return result


def _scenario(payload, path):
    metadata = payload.get("metadata", {})
    if isinstance(metadata, dict) and metadata.get("scenario"):
        return str(metadata["scenario"])
    if payload.get("scenario_id"):
        variant = payload.get("variant_id")
        return str(payload["scenario_id"]) + (f"/{variant}" if variant else "")
    if payload.get("provenance", {}).get("analysis_meta"):
        return "gpgx/automatic_realtime_analysis"
    return Path(path).stem


def _runtime_records(artifact):
    payload, path, schema = artifact["payload"], artifact["path"], artifact["schema"]
    scenario = _scenario(payload, path)
    records = []

    def add(start, end, access_type, reader=None, width=0, order=None,
            frame=None, parser=None, consumer=None, destination_domain=None,
            exact=False, source_kind="runtime", src_before=None,
            src_after=None, consumed_bytes=None):
        if not _valid_span((start, end)):
            return
        record = {
            "id": f"global-runtime:{hashlib.sha256(canonical({
                'source': path, 'scenario': scenario, 'start': start,
                'end': end, 'type': access_type, 'reader': reader,
                'width': width, 'order': order, 'frame': frame,
            }).encode()).hexdigest()[:24]}",
            "start": start, "end": end, "type": "GLOBAL_RUNTIME_SPAN",
            "producer": "m12_carver_global_sweep", "confidence": "OBSERVED",
            "runtime": True, "scenario_id": scenario, "reader_pc": reader,
            "access_type": access_type, "width_bytes": width,
            "order": order, "frame": frame, "parser": parser,
            "consumer": consumer, "destination_domain": destination_domain,
            "caller_ancestry": None, "parser_entry": parser,
            "decoder_entry": None,
            "exact_boundary": exact, "source_kind": source_kind,
            "src_before": src_before, "src_after": src_after,
            "consumed_bytes": consumed_bytes, "source_ref": path,
        }
        records.append(record)

    if schema == "oasis.gpgx.rom.reader.correlation.v1":
        for order, item in enumerate(payload.get("regions", [])):
            span = _span(item)
            if span:
                add(span[0], span[1], "read", item.get("first_reader_pc"),
                    item.get("first_access_width", 0), order=order,
                    source_kind="rom_read")
    elif schema == "oasis.gpgx.runtime.execution.evidence.v1":
        facts = payload.get("execution_facts", [])
        addresses = [item.get("address") for item in facts if isinstance(item, dict)]
        if not addresses:
            addresses = payload.get("executed_addresses", [])
        for order, address in enumerate(addresses):
            start = _int(address)
            add(start, start + 1, "execute", start, order=order,
                source_kind="code_execution")
    elif schema == "oasis.m68k.emulator-trace.v1":
        events = payload.get("events", [])
        for order, item in enumerate(events):
            if not isinstance(item, dict):
                continue
            address = item.get("address")
            if address is not None:
                start = _int(address)
                width = _int(item.get("width_bytes", 0) or 0)
                add(start, start + max(width, 1), item.get("kind", "read"),
                    item.get("pc"), width, order=order, source_kind="rom_access")
            pc = item.get("pc")
            if pc is not None and item.get("kind") == "instruction":
                start = _int(pc)
                width = _int(item.get("instruction_size", 0) or 0)
                add(start, start + max(width, 1), "execute", start, width,
                    order=order, source_kind="code_execution")
    elif schema == "oasis.m68k.natural-reach.v1":
        pcs = list(payload.get("previous_pcs", []))
        entry = payload.get("entry", {})
        if isinstance(entry, dict) and entry.get("pc") is not None:
            pcs.append(entry["pc"])
        pcs.extend(payload.get("target_addresses", []))
        for order, pc in enumerate(sorted({_int(value) for value in pcs})):
            add(pc, pc + 1, "execute", pc, order=order,
                source_kind="code_execution")
    elif schema == "oasis.m68k.re-trace.v1":
        for order, item in enumerate(payload.get("executed_instructions", [])):
            if isinstance(item, dict) and item.get("pc") is not None:
                pc = _int(item["pc"])
                add(pc, pc + 1, "execute", pc, order=order,
                    source_kind="code_execution")
        for order, item in enumerate(payload.get("memory_reads", [])):
            if isinstance(item, dict) and item.get("address") is not None:
                start = _int(item["address"])
                width = _int(item.get("width_bytes", 0) or 0)
                add(start, start + max(width, 1), "read", item.get("instruction"),
                    width, order=order, source_kind="rom_read")
    return records


def _merge_runtime(records):
    """Merge adjacent observations and count equivalent repetitions."""
    grouped = defaultdict(list)
    for record in records:
        key = tuple(record.get(field) for field in
                    ("scenario_id", "reader_pc", "access_type", "width_bytes",
                     "parser", "consumer", "destination_domain"))
        grouped[key].append(record)
    merged = []
    for key, items in sorted(grouped.items(), key=lambda pair: canonical(pair[0])):
        items.sort(key=lambda item: (item.get("order") is None,
                                     item.get("order") or 0, item["start"],
                                     item["end"], item["id"]))
        current = None
        for item in items:
            if current and current["end"] == item["start"]:
                current["end"] = item["end"]
                current["src_after"] = item["src_after"] if current["exact_boundary"] else None
                if current["consumed_bytes"] is not None and item["consumed_bytes"] is not None:
                    current["consumed_bytes"] += item["consumed_bytes"]
                else:
                    current["consumed_bytes"] = None
                current["observation_count"] += 1
                current["exact_boundary"] &= item["exact_boundary"]
                continue
            if current:
                merged.append(current)
            current = dict(item)
            current["observation_count"] = 1
        if current:
            merged.append(current)
    for item in merged:
        identity = {key: item.get(key) for key in
                    ("scenario_id", "start", "end", "reader_pc", "access_type",
                     "width_bytes", "parser", "consumer", "destination_domain")}
        item["id"] = "global-runtime-span:" + hashlib.sha256(
            canonical(identity).encode()).hexdigest()[:24]
        item["repetition_count"] = item.pop("observation_count")
    collapsed = {}
    for item in merged:
        key = tuple(item.get(field) for field in
                    ("scenario_id", "start", "end", "reader_pc", "access_type",
                     "width_bytes", "parser", "consumer", "destination_domain"))
        previous = collapsed.get(key)
        if previous is None:
            collapsed[key] = item
        else:
            previous["repetition_count"] += item["repetition_count"]
            previous["exact_boundary"] &= item["exact_boundary"]
    return sorted(collapsed.values(), key=lambda item: (item["start"], item["end"], item["id"]))


def _union_bytes(spans, start=0, end=ROM_END):
    intervals = sorted((max(start, item["start"]), min(end, item["end"]))
                       for item in spans if item["start"] < end and item["end"] > start)
    total, right = 0, start
    for left, current_end in intervals:
        if current_end <= right:
            continue
        total += current_end - max(left, right)
        right = current_end
    return total


def _intersections(spans, start, end):
    return sorted({(max(start, item["start"]), min(end, item["end"]))
                   for item in spans if item["start"] < end and item["end"] > start})


def _coverage_map(db, runtime, candidates):
    code = [item for item in runtime if item["access_type"] == "execute"]
    reads = [item for item in runtime if item["access_type"] in ("read", "write")]
    result = []
    for item in db.ranges:
        observed = _union_bytes(runtime, item["start"], item["end"])
        candidate_hits = [candidate for candidate in candidates
                          if intersects((item["start"], item["end"]),
                                        (candidate["start"], candidate["end"]))]
        status = "SOURCE_OWNED" if item["source_owned"] else (
            "CANDIDATE_OBSERVED" if candidate_hits and observed else
            "UNKNOWN_OBSERVED_RUNTIME" if observed else "UNKNOWN_NEVER_OBSERVED")
        result.append({"range_id": item["id"], "start": item["start"],
                       "end": item["end"], "status": status,
                       "source_owned": item["source_owned"],
                       "observed_bytes": observed,
                       "candidate_bytes_observed": _union_bytes(
                           [span for span in runtime if any(
                               intersects((span["start"], span["end"]),
                                          (candidate["start"], candidate["end"]))
                               for candidate in candidate_hits)],
                           item["start"], item["end"]),
                       "executed_as_code": bool(_intersections(code, item["start"], item["end"])),
                       "read_as_data": bool(_intersections(reads, item["start"], item["end"])),
                       "parser_decoder": sorted({str(span["parser"]) for span in runtime
                                                  if span["parser"] and intersects(
                                                      (item["start"], item["end"]),
                                                      (span["start"], span["end"]))}),
                       "destination_domains": sorted({str(span["destination_domain"])
                                                       for span in runtime if span["destination_domain"]
                                                       and intersects((item["start"], item["end"]),
                                                                       (span["start"], span["end"]))})})
    return result


def _blocker(gap, db, runtime, candidates):
    related = db._related_evidence(gap)
    spans = [item for item in runtime if intersects((gap["start"], gap["end"]),
                                                     (item["start"], item["end"]))]
    candidate_hit = any(intersects((gap["start"], gap["end"]),
                                   (item["start"], item["end"])) for item in candidates)
    if gap["conflicts"]:
        return "F", "candidate overlaps confirmed range or has a blocking conflict"
    if any("code" in str(item.get("type", "")).lower() and
           "data" in str(item.get("type", "")).lower() for item, _ in related):
        return "E", "code/data evidence conflict"
    if spans:
        known_consumer = any(item.get("consumer") or item.get("parser") for item, _ in related)
        exact = any(item.get("exact_boundary") for item in spans)
        if not known_consumer:
            return "B", "runtime-observed but consumer/parser is unknown"
        if not exact:
            return "C", "known consumer/parser but exact source boundary is unknown"
        return "D", "known boundary but semantic type remains unresolved"
    if related:
        return "G", "static evidence exists but no available scenario observed the range"
    return "A", "never observed at runtime by the available scenarios"


def _scenario_summary(runtime, artifacts, db):
    result = []
    runtime_artifacts = [item for item in artifacts if item["schema"] in RUNTIME_SCHEMAS]
    scenarios = sorted({_scenario(item["payload"], item["path"])
                        for item in runtime_artifacts})
    for scenario in scenarios:
        spans = [item for item in runtime if item["scenario_id"] == scenario]
        result.append({"scenario_id": scenario, "artifact_count": sum(
            _scenario(item["payload"], item["path"]) == scenario
            for item in runtime_artifacts),
            "span_count": len(spans), "observed_rom_bytes": _union_bytes(spans),
            "unknown_bytes_observed": sum(_union_bytes(
                spans, item["start"], item["end"])
                for item in db.ranges if not item["source_owned"]),
            "executed_code_bytes": _union_bytes(
                [span for span in spans if span["access_type"] == "execute"]),
            "repetition_count": sum(item["repetition_count"] for item in spans)})
    return result


def _clusters(runtime):
    fields = ("reader_pc", "parser", "consumer", "destination_domain")
    result = {}
    for field in fields:
        groups = defaultdict(list)
        for item in runtime:
            value = item.get(field)
            if value not in (None, ""):
                groups[str(value)].append(item)
        result[field] = [{"key": key, "span_count": len(items),
                          "rom_bytes": _union_bytes(items)}
                         for key, items in sorted(groups.items())]
    return result


def _evidence_clusters(db):
    result = {}
    for field in ("parser", "consumer", "structural_format", "destination_domain"):
        groups = defaultdict(list)
        for item in db.evidence.values():
            value = item.get(field)
            if value not in (None, ""):
                groups[str(value)].append(item)
        result[field] = [{"key": key, "evidence_count": len(items),
                          "rom_bytes": _union_bytes([
                              {"start": item["start"], "end": item["end"]}
                              for item in items if "start" in item and "end" in item])}
                         for key, items in sorted(groups.items())]
    return result


def _destination_stats(db):
    groups = defaultdict(list)
    for item in db.evidence.values():
        value = item.get("destination_domain") or item.get("destination")
        if value:
            groups[str(value)].append(item)
    return [{"domain": key, "evidence_count": len(items),
             "rom_bytes": _union_bytes([{"start": item["start"], "end": item["end"]}
                                         for item in items if "start" in item and "end" in item])}
            for key, items in sorted(groups.items())]


def _checkpoint_catalog(artifacts):
    result = []
    for item in artifacts:
        payload = item["payload"]
        if not payload.get("state_checkpoints_sha256") and not payload.get("video_sequence_sha256"):
            continue
        result.append({"artifact": item["path"], "schema": item["schema"],
                       "scenario_id": payload.get("scenario"),
                       "target": payload.get("target_hex") or payload.get("target"),
                       "mode": payload.get("mode"),
                       "requested_frames": payload.get("requested_frames"),
                       "scenario_completed": payload.get("scenario_completed"),
                       "state_checkpoints_sha256": payload.get("state_checkpoints_sha256"),
                       "video_sequence_sha256": payload.get("video_sequence_sha256")})
    return sorted(result, key=canonical)


def run(manifest_path, rom_path, input_root, output):
    output = Path(output).resolve()
    manifest_path = Path(manifest_path).resolve()
    rom = Path(rom_path).read_bytes()
    manifest = json.loads(manifest_path.read_text())
    if len(rom) != ROM_END or hashlib.sha256(rom).hexdigest() != manifest.get("rom_sha256"):
        raise ValueError("canonical ROM identity does not match manifest")
    output.mkdir(parents=True, exist_ok=False)
    artifacts = discover(input_root, manifest["rom_sha256"], output)
    db = IntervalDB.from_manifest(manifest, manifest_path)
    seen_payloads = set()
    adapter_counts = Counter()
    for artifact in artifacts:
        if artifact["digest"] in seen_payloads:
            continue
        seen_payloads.add(artifact["digest"])
        adapter_counts[db.ingest(artifact["payload"], artifact["path"])] += 1
    raw_runtime = [record for artifact in artifacts for record in _runtime_records(artifact)]
    runtime = _merge_runtime(raw_runtime)
    db.ingest({"adapter": "runtime_pc_read", "records": runtime}, "global_runtime_span_aggregation")
    report = db.report()
    candidates = list(db.candidates.values())
    coverage = _coverage_map(db, runtime, candidates)
    blockers = []
    for gap in report["gaps"]:
        kind, reason = _blocker(gap, db, runtime, candidates)
        blockers.append({"range_id": gap["id"], "start": gap["start"],
                         "end": gap["end"], "size": gap["size"],
                         "blocker": kind, "reason": reason})
    blocker_totals = Counter(item["blocker"] for item in blockers)
    exact = [item for item in runtime if item["exact_boundary"] and
             item["parser"] and item["consumer"]]
    domains = Counter(item["destination_domain"] or "UNKNOWN" for item in runtime)
    promotion = {"eligible": 0, "promoted": 0,
                 "rejected": len(candidates),
                 "reason": "no new existing exact-contract transaction was proven by the global inputs"}
    canonical_db = db.interval_db()
    output_report = {
        "schema": REPORT_SCHEMA, "deterministic": True,
        "baseline_source_owned_bytes": db.manifest_source_owned_bytes,
        "final_source_owned_bytes": db.source_owned_bytes(),
        "gained_source_owned_bytes": db.source_owned_bytes() - db.manifest_source_owned_bytes,
        "source_owned_unchanged": db.source_owned_bytes() == db.manifest_source_owned_bytes,
        "canonical_rom": {"size": len(rom), "crc32": f"{zlib.crc32(rom) & 0xFFFFFFFF:08X}",
                          "sha1": hashlib.sha1(rom).hexdigest(),
                          "sha256": hashlib.sha256(rom).hexdigest()},
        "artifacts": {"discovered": len(artifacts), "unique_payloads_ingested": len(seen_payloads),
                       "schemas": dict(sorted(Counter(item["schema"] for item in artifacts).items())),
                       "adapters": dict(sorted(adapter_counts.items()))},
        "runtime": {"raw_observations": len(raw_runtime), "aggregated_spans": len(runtime),
                     "total_rom_bytes_observed": _union_bytes(runtime),
                     "unknown_bytes_observed": sum(_union_bytes(
                         runtime, item["start"], item["end"])
                         for item in db.ranges if not item["source_owned"]),
                     "exact_boundary_recoveries": len(exact),
                     "destination_domains": dict(sorted(domains.items())),
                     "spans": runtime},
        "scenario_coverage": _scenario_summary(runtime, artifacts, db),
        "checkpoint_catalog": _checkpoint_catalog(artifacts),
        "whole_rom_coverage": {"total_bytes": ROM_END, "gaps": 0, "overlaps": 0,
                                "ranges": coverage,
                                "observed_bytes": _union_bytes(runtime),
                                "unknown_bytes_observed": sum(item["observed_bytes"] for item in coverage
                                                               if not item["source_owned"]),
                                "candidate_bytes_observed": sum(item["candidate_bytes_observed"] for item in coverage)},
        "parser_decoder_clusters": {"runtime": _clusters(runtime),
                                     "evidence": _evidence_clusters(db)},
        "destination_domain_statistics": {"runtime": dict(sorted(domains.items())),
                                           "evidence": _destination_stats(db)},
        "promotion": promotion,
        "conflicts": {"resolved": 0, "remaining": len(db.conflicts),
                       "blocking": len([item for item in db.conflicts.values() if item.get("blocking")])},
        "blocker_census": {"totals": dict(sorted(blocker_totals.items())),
                            "ranges": sorted(blockers, key=lambda item: (-item["size"], item["start"]))},
        "largest_remaining_families": [{"blocker": key, "bytes": sum(
            item["size"] for item in blockers if item["blocker"] == key),
            "ranges": sum(item["blocker"] == key for item in blockers)}
            for key in sorted(blocker_totals, key=lambda value: (-sum(
                item["size"] for item in blockers if item["blocker"] == value), value))],
        "carver": report,
        "fixed_point": report["fixed_point"],
        "deterministic_db_sha256": hashlib.sha256(canonical(canonical_db).encode()).hexdigest(),
        "runner_availability": {"BizHawk": bool(shutil.which("EmuHawk")),
                                 "MAME": bool(shutil.which("mame")),
                                 "new_external_capture": False},
    }
    (output / "interval_db.json").write_text(json.dumps(canonical_db, indent=2, sort_keys=True) + "\n")
    (output / "global_sweep_report.json").write_text(json.dumps(output_report, indent=2, sort_keys=True) + "\n")
    return output_report
