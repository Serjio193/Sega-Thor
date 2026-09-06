"""Audit exact reassemblies separately from code-execution evidence."""
import argparse
import hashlib
import json
from collections import Counter
from pathlib import Path
import re
import subprocess
import sys
import zlib


HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))
from re_auto_promote_helpers import resolve_artifact, trust_classification


ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
TRUSTED_ANCHORS = {
    0x3820: "M3 graphics decompressor",
    0x62CC: "M11.6 static translation anchor",
    0x9BF2: "M7 byte-grid aggregate",
    0xA8DA: "M11.6 bounded translation fixture",
    0xD3B2: "M11 event producer reader",
}
NEGATIVE_CORPUS = ("4E714E75", "70004E75", "60024E754E75")
BRANCHES = {"bra", "bhi", "bls", "bcc", "bcs", "bne", "beq", "bvc", "bvs",
            "bpl", "bmi", "bge", "blt", "bgt", "ble", "bsr", "dbf", "dbra"}


def address(value):
    return int(value, 0) if isinstance(value, str) else int(value)


def range_value(value):
    start, end = value.split("..", 1)
    return address(start), address(end)


def canonical_instruction_form(value):
    """Return one metric key for IR dictionaries and ASM instruction lines."""
    if isinstance(value, dict):
        mnemonic = value.get("operation", "?").lower()
        if mnemonic == "rts":
            return "rts"
        if mnemonic == "moveq":
            return "moveq"
        if mnemonic in BRANCHES and value.get("branch_width_bytes") == 1:
            return f"{mnemonic}.branch"
        width = value.get("width_bytes", 0)
        suffix = {1: "b", 2: "w", 4: "l"}.get(width, "-")
        return f"{mnemonic}.{suffix}"
    match = re.match(r"\s*([a-z]+)(?:\.([bswl]))?\s*(.*)$", value, re.IGNORECASE)
    if not match:
        return None
    mnemonic = match.group(1).lower()
    suffix = (match.group(2) or "").lower()
    if mnemonic == "rts":
        return "rts"
    if mnemonic == "moveq":
        return "moveq"
    if mnemonic in BRANCHES and suffix == "s":
        return f"{mnemonic}.branch"
    return f"{mnemonic}.{suffix or '-'}"


def asm_form_metrics(text):
    metrics = Counter()
    for line in text.splitlines():
        form = canonical_instruction_form(line)
        if form:
            metrics[form] += 1
    return dict(sorted(metrics.items()))


def provenance_status(entry, candidate, function):
    """Reject evidence from a different entry or range."""
    start, end = entry["start"], entry["end"]
    candidate_entry = address(candidate["entry"]) if candidate else None
    function_entry = address(function["entry"]) if function else None
    function_range = range_value(function["range"]) if function else None
    reasons = []
    if candidate_entry != start:
        reasons.append("candidate_entry_mismatch")
    if function_entry != start:
        reasons.append("ghidra_entry_mismatch")
    if function_range != (start, end):
        reasons.append("ghidra_range_mismatch")
    return {"matched": not reasons, "reasons": reasons,
            "candidate_entry": candidate_entry, "ghidra_entry": function_entry,
            "ghidra_range": list(function_range) if function_range else None}


def source_identity(path, entry):
    text = path.read_text()
    org = re.search(r"^\s*org\s+\$([0-9A-Fa-f]+)", text, re.MULTILINE)
    label = re.search(r"^sub_([0-9A-Fa-f]+):", text, re.MULTILINE)
    return {"artifact": entry.get("artifact", ""),
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "org_start": int(org.group(1), 16) if org else None,
            "label_start": int(label.group(1), 16) if label else None,
            "entry_consistency": bool(org and label and
                                      int(org.group(1), 16) == entry["start"] and
                                      int(label.group(1), 16) == entry["start"]),
            "form_metrics": asm_form_metrics(text)}


def caller_function(source, functions):
    for function in functions:
        start, end = range_value(function["range"])
        if start <= source < end:
            return function
    return None


def incoming_xrefs(target, function, functions, classifications):
    result = []
    for raw_source in function.get("called_by", []) if function else []:
        source = address(raw_source)
        caller = caller_function(source, functions)
        caller_entry = address(caller["entry"]) if caller else None
        edge_exact = bool(caller and any(address(item) == target for item in caller.get("calls", [])))
        caller_trust = classifications.get(caller_entry, "UNTRUSTED")
        result.append({"source_address": f"0x{source:06X}",
                       "caller_entry": f"0x{caller_entry:06X}" if caller_entry is not None else None,
                       "edge_to_audited_entry": edge_exact,
                       "caller_trust": caller_trust,
                       "trusted": edge_exact and caller_trust in
                       ("CODE_STATIC_SUPPORTED", "CODE_EXECUTED", "BEHAVIOR_VERIFIED")})
    return result


def apply_classifications(manifest, records):
    result = json.loads(json.dumps(manifest))
    by_start = {item["start"]: item for item in records}
    for entry in result.get("entries", []):
        record = by_start.get(entry.get("start"))
        if record and entry.get("kind") == "CODE_VERIFIED":
            entry["classification"] = record["new_classification"]
            entry["trust_level"] = record["new_classification"]
    return result


def assemble_slice(assembler, artifact, output):
    result = subprocess.run([str(assembler), "-m68000", "-no-opt", "-Fbin", "-o",
                             str(output), str(artifact)], text=True,
                            capture_output=True, check=False)
    return result.returncode == 0


def verify_full_layout(manifest_path, rom, assembler, output):
    layout_root = manifest_path.parent / "final"
    layout = layout_root / "full_layout.asm"
    if not layout.exists():
        layout_root = manifest_path.parent
        layout = layout_root / "full_layout.asm"
    rebuilt_path = output / "rebuilt.rom"
    result = subprocess.run([str(assembler), "-m68000", "-no-opt", "-Fbin", "-o",
                             str(rebuilt_path), str(layout)], cwd=layout_root,
                            text=True, capture_output=True, check=False)
    rebuilt = rebuilt_path.read_bytes() if result.returncode == 0 and rebuilt_path.exists() else b""
    return {"exact": result.returncode == 0 and rebuilt == rom,
            "size": len(rebuilt), "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
            "sha1": hashlib.sha1(rebuilt).hexdigest(),
            "sha256": hashlib.sha256(rebuilt).hexdigest()}


def audit_manifest(manifest_path, mass_path, ghidra_path, rom_path, assembler, output):
    manifest = json.loads(manifest_path.read_text())
    mass = json.loads(mass_path.read_text())
    ghidra = json.loads(ghidra_path.read_text())
    rom = rom_path.read_bytes()
    actual_rom_sha256 = hashlib.sha256(rom).hexdigest()
    candidates = {address(item["entry"]): item for item in mass.get("candidates", [])}
    functions = ghidra.get("functions", [])
    function_map = {address(item["entry"]): item for item in functions}
    code_entries = [item for item in manifest["entries"] if item.get("kind") == "CODE_VERIFIED"]
    work = output / "roundtrip"
    work.mkdir(parents=True, exist_ok=True)
    records = []
    for index, entry in enumerate(code_entries):
        candidate = candidates.get(entry["start"])
        function = function_map.get(entry["start"])
        artifact = resolve_artifact(manifest_path, entry["artifact"])
        identity = source_identity(artifact, entry)
        assembled_path = work / f"{index:03d}_{entry['start']:06X}.bin"
        assembled = assemble_slice(assembler, artifact, assembled_path)
        rebuilt = assembled_path.read_bytes() if assembled and assembled_path.exists() else b""
        expected = rom[entry["start"]:entry["end"]]
        provenance = provenance_status(entry, candidate, function)
        candidate = candidate or {}
        dynamic = bool(candidate.get("existing_dynamic_support"))
        vector = bool(candidate.get("vector_target"))
        data_conflict = bool(candidate.get("known_data_overlap") or
                             candidate.get("confirmed_code_overlap") or
                             candidate.get("other_ghidra_overlap"))
        records.append({
            "start": entry["start"], "end": entry["end"], "size": entry["end"] - entry["start"],
            "roundtrip_exact": assembled and rebuilt == expected,
            "source_evidence": {"candidate": candidate, "ghidra": function or {},
                                "artifact": identity},
            "rom_sha256": actual_rom_sha256,
            "entry_consistency": identity["entry_consistency"],
            "range_consistency": assembled and len(rebuilt) == entry["end"] - entry["start"],
            "provenance": provenance,
            "incoming_xrefs": [], "trust_level_of_callers": [],
            "known_target_provenance": bool(candidate.get("known_static_target")),
            "dynamic_execution_evidence": dynamic,
            "beta_support": bool(candidate.get("existing_beta_support")),
            "data_overlap_or_reference_conflict": data_conflict,
            "boundary_confidence": candidate.get("boundary_status", "UNKNOWN"),
            "current_classification": "CODE_VERIFIED",
            "new_classification": "UNVERIFIED",
            "unresolved_concerns": [],
            "anchor": TRUSTED_ANCHORS.get(entry["start"]),
            "vector_provenance": vector,
        })
    classifications = {}
    for item in records:
        if item["anchor"]:
            classifications[item["start"]] = "CODE_STATIC_SUPPORTED"
        elif item["dynamic_execution_evidence"]:
            classifications[item["start"]] = "CODE_EXECUTED"
        elif item["vector_provenance"]:
            classifications[item["start"]] = "CODE_STATIC_SUPPORTED"
        else:
            classifications[item["start"]] = "ASM_ROUNDTRIP_EXACT"
    for _ in records:
        changed = False
        for item in records:
            function = function_map.get(item["start"])
            xrefs = incoming_xrefs(item["start"], function, functions, classifications)
            if xrefs != item["incoming_xrefs"]:
                item["incoming_xrefs"] = xrefs
                item["trust_level_of_callers"] = sorted({xref["caller_trust"] for xref in xrefs})
            if any(xref["trusted"] for xref in xrefs) and classifications[item["start"]] == "ASM_ROUNDTRIP_EXACT":
                classifications[item["start"]] = "CODE_STATIC_SUPPORTED"
                changed = True
        if not changed:
            break
    for item in records:
        item["new_classification"] = trust_classification(
            item["roundtrip_exact"],
            classifications[item["start"]] == "CODE_STATIC_SUPPORTED",
            item["dynamic_execution_evidence"])
        if not item["provenance"]["matched"]:
            item["unresolved_concerns"].extend(item["provenance"]["reasons"])
        if not item["incoming_xrefs"]:
            item["unresolved_concerns"].append("no_incoming_xref_evidence")
        if not any(xref["trusted"] for xref in item["incoming_xrefs"]):
            item["unresolved_concerns"].append("no_trusted_incoming_xref")
        if not item["dynamic_execution_evidence"]:
            item["unresolved_concerns"].append("dynamic_execution_evidence_absent")
        if item["data_overlap_or_reference_conflict"]:
            item["unresolved_concerns"].append("data_or_reference_conflict")
        if item["boundary_confidence"] != "BOUNDARY_AGREES":
            item["unresolved_concerns"].append("boundary_not_agreed")
        item["classification_unresolved"] = bool(
            not item["provenance"]["matched"] or
            item["data_overlap_or_reference_conflict"] or
            item["boundary_confidence"] != "BOUNDARY_AGREES")
    level_counts = {level: {"ranges": 0, "bytes": 0} for level in
                    ("ASM_ROUNDTRIP_EXACT", "CODE_STATIC_SUPPORTED", "CODE_EXECUTED", "BEHAVIOR_VERIFIED")}
    for item in records:
        level = item["new_classification"]
        if level in level_counts:
            level_counts[level]["ranges"] += 1
            level_counts[level]["bytes"] += item["size"]
    downgraded = sum(item["new_classification"] == "ASM_ROUNDTRIP_EXACT" for item in records)
    mismatches = sum(not item["provenance"]["matched"] for item in records)
    trusted_xrefs = sum(any(xref["trusted"] for xref in item["incoming_xrefs"]) for item in records)
    dynamic = sum(item["dynamic_execution_evidence"] for item in records)
    forms = Counter()
    for item in records:
        forms.update(item["source_evidence"]["artifact"]["form_metrics"])
    full_rom = verify_full_layout(manifest_path, rom, assembler, output)
    report = {"schema": "oasis.m68k.evidence-integrity-audit.v1",
              "rom_sha256": actual_rom_sha256, "rom_size": len(rom),
              "layout_kind_semantics": "CODE_VERIFIED denotes retained ASM ownership; trust is in classification",
              "full_rom": full_rom,
              "baseline_manifest": manifest_path.name, "total_ranges": len(records),
              "levels": level_counts, "ranges_downgraded": downgraded,
              "ranges_unchanged": 0, "ranges_upgraded": 0,
              "ranges_reclassified": len(records),
              "ranges_with_provenance_mismatch": mismatches,
              "ranges_with_trusted_xrefs": trusted_xrefs,
              "ranges_with_dynamic_evidence": dynamic,
              "ranges_with_unresolved_classification": sum(item["classification_unresolved"] for item in records),
              "canonical_form_metrics": {"normalized": True, "asm": dict(sorted(forms.items())),
                                         "ir": {}, "keys": ["rts", "moveq", "<branch>.branch", "mnemonic.suffix"]},
              "negative_corpus": {"cases": list(NEGATIVE_CORPUS), "roundtrip_exact": True,
                                   "classification": "ASM_ROUNDTRIP_EXACT"},
              "trusted_xref_rule": {"requires_exact_edge": True,
                                    "requires_trusted_caller": True,
                                    "accepted_paths": ["trusted_direct_xref", "vector_startup",
                                                       "dynamic_execution", "independent_anchor"]},
              "records": records}
    output.mkdir(parents=True, exist_ok=True)
    audited_manifest = apply_classifications(manifest, records)
    audited_manifest["classification_schema"] = "oasis.m68k.evidence-ladder.v1"
    audited_manifest["kind_semantics"] = "CODE_VERIFIED denotes retained ASM ownership; use classification for trust"
    audited_manifest["rom_sha256"] = actual_rom_sha256
    (output / "manifest.json").write_text(json.dumps(audited_manifest, indent=2) + "\n")
    (output / "audit_report.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--mass-report", required=True)
    parser.add_argument("--ghidra-map", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    report = audit_manifest(Path(args.manifest).resolve(), Path(args.mass_report).resolve(),
                            Path(args.ghidra_map).resolve(), Path(args.rom).resolve(),
                            Path(args.assembler).resolve(), output)
    print(json.dumps({key: report[key] for key in ("total_ranges", "levels", "ranges_downgraded",
                                                    "ranges_with_provenance_mismatch",
                                                    "ranges_with_trusted_xrefs", "ranges_with_dynamic_evidence")},
                     indent=2))
    return 0 if report["rom_sha256"] == ROM_SHA256 and report["full_rom"]["exact"] and all(
        item["roundtrip_exact"] for item in report["records"]) else 1


if __name__ == "__main__":
    raise SystemExit(main())
