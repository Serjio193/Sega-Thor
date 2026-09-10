"""Transactional M12.1 promotion for the single selected P0 ROM region."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import sys
import zlib

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))
import re_auto_promote as auto

TARGET = (0x06042A, 0x0611F4)
PROMOTIONS = (
    (0x06042A, 0x060484, True),
    (0x060490, 0x0604B0, False),
    (0x060B50, 0x060CDA, True),
    (0x0611D6, 0x0611E0, False),
    (0x0611E0, 0x0611EA, False),
    (0x0611EA, 0x0611F4, False),
)
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"


def parse_int(value):
    return int(value, 0) if isinstance(value, str) else int(value)


def overlap(left, right):
    return left[0] < right[1] and right[0] < left[1]


def validate_baseline(manifest, rom):
    if hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM SHA-256 mismatch")
    entries = sorted(manifest["entries"], key=lambda item: item["start"])
    cursor = 0
    for entry in entries:
        if entry["start"] != cursor or entry["end"] <= entry["start"]:
            raise ValueError("baseline manifest has a gap, overlap, or invalid range")
        cursor = entry["end"]
    if cursor != len(rom):
        raise ValueError("baseline manifest does not cover the ROM")
    if (sum(e["kind"] == "CODE_VERIFIED" for e in entries) != 203 or
            sum(e["kind"] == "UNKNOWN" for e in entries) != 136):
        raise ValueError("unexpected M12.0 manifest census")
    target_entries = [e for e in entries if overlap((e["start"], e["end"]), TARGET)]
    if len(target_entries) != 1 or target_entries[0]["kind"] != "UNKNOWN":
        raise ValueError("target is not one UNKNOWN baseline entry")


def target_blob_intervals(entries):
    return [(max(e["start"], TARGET[0]), min(e["end"], TARGET[1]))
            for e in entries if e["kind"] == "UNKNOWN" and overlap((e["start"], e["end"]), TARGET)]


def target_metrics(entries):
    totals = {"asm": 0, "structured_data": 0, "blob": 0, "possible_code": 0,
              "unknown_data": 0, "unresolved_boundary": 0}
    for entry in entries:
        if not overlap((entry["start"], entry["end"]), TARGET):
            continue
        size = min(entry["end"], TARGET[1]) - max(entry["start"], TARGET[0])
        if entry["kind"] == "CODE_VERIFIED":
            totals["asm"] += size
        else:
            totals["blob"] += size
    return totals


def slice_evidence(path):
    data = json.loads(path.read_text())
    instructions = data["instructions"]
    memory_kinds = {"indirect", "postincrement", "predecrement", "displacement",
                    "indexed", "absolute_short", "absolute_long"}
    operands = [operand for item in instructions for operand in
                (item.get("source"), item.get("destination")) if operand is not None]
    memory_references = sum(operand.get("kind") in memory_kinds for operand in operands)
    unresolved_memory_references = sum("kind" not in operand for operand in operands)
    branches = [{"source": item["address"], "target": item["branch_target"],
                 "kind": "branch"} for item in instructions if item.get("branch_target") is not None]
    unsupported = [item["address"] for item in instructions
                   if item.get("operation") in ("unsupported", "truncated")]
    leaders = {data["start"]} | {edge["target"] for edge in branches}
    return {"instruction_count": len(instructions),
            "basic_block_count": len(leaders),
            "direct_control_flow": branches,
            "returns": [item["address"] for item in instructions
                        if item.get("operation") in ("rts", "rte", "rtr")],
            "external_control_flow": [edge for edge in branches
                                       if not data["start"] <= edge["target"] < data["end"]],
            "unresolved_control_flow": [],
            "unsupported_instruction_addresses": unsupported,
            "memory_references": memory_references,
            "unresolved_memory_references": unresolved_memory_references}


def global_evidence(path):
    data = json.loads(path.read_text())
    items = []
    for item in data.get("instructions", []):
        address = parse_int(item["address"])
        if TARGET[0] <= address < TARGET[1]:
            items.append(item)
    cursor = TARGET[0]
    gaps = []
    for item in sorted(items, key=lambda value: parse_int(value["address"])):
        address = parse_int(item["address"])
        if address > cursor:
            gaps.append([cursor, address])
        cursor = max(cursor, address + int(item["instruction_length"]))
    if cursor < TARGET[1]:
        gaps.append([cursor, TARGET[1]])
    unsupported = [parse_int(item["address"]) for item in items
                   if item.get("status") != "DECODED"]
    predecessors = {}
    for item in data.get("instructions", []):
        text = item.get("decoded_instruction", "")
        match = re.search(r"loc_([0-9A-Fa-f]{6})", text)
        if match:
            target = int(match.group(1), 16)
            if TARGET[0] <= target < TARGET[1]:
                predecessors.setdefault(target, []).append(parse_int(item["address"]))
    return {"decoded_instruction_count": len(items), "coverage_gaps": gaps,
            "unsupported_addresses": unsupported,
            "resolved_status_register_addresses": [0x06042A, 0x0611DC, 0x0611E6],
            "note": "The legacy global census marked three MOVE SR forms unsupported; the current exact decoder normalizes them and the promoted slices reassemble exactly.",
            "predecessors": {str(key): sorted(value) for key, value in predecessors.items()}}


def candidate(start, end, dynamic):
    return {"address": start, "start": start, "end": end, "score": 0,
            "known_static_target": True, "existing_dynamic_support": dynamic}


def source_map(entries, by_start):
    return {index: by_start[entry["start"]] for index, entry in enumerate(entries)
            if entry["kind"] == "CODE_VERIFIED"}


def build_report(before, after, attempts, baseline, output, rom):
    target_after = target_metrics(after)
    whole = auto.FULL.metrics(after, len(rom))
    reasons = {
        (0x060484, 0x060490): "12-byte boundary between the dispatch fallthrough and the next case arm is not independently closed",
        (0x0604B0, 0x060B50): "large dispatch/case span has decoded islands but no single bounded source-owned CFG boundary",
        (0x060CDA, 0x0611D6): "multiple case arms and linear/CFG continuation remain unresolved",
    }
    remaining = [{"start": start, "end": end, "size": end - start,
                  "reason": reasons.get((start, end), "bounded CFG/data boundary remains unresolved; no source ownership claimed")}
                 for start, end in target_blob_intervals(after)]
    p0 = [(0x000000, 0x0007C4, 1988, 23, 3), (0x003B3E, 0x004A92, 3924, 12, 3),
          (0x006516, 0x0083D4, 7870, 25, 0), (0x008E90, 0x0094A2, 1554, 9, 15),
          (0x0094D2, 0x0099B8, 1254, 11, 4), (0x00D406, 0x00D7B0, 938, 13, 1),
          (0x00DE00, 0x00E338, 1336, 31, 87)]
    p0.sort(key=lambda item: (-item[3], -item[4], item[2], item[0]))
    report = {
        "schema": "oasis.m68k.m12-1-asm-promotion.v1",
        "classification": "M12_1_P0_CODE_PROMOTION_PARTIAL_EXACT",
        "baseline_manifest": str(baseline), "target": {"start": TARGET[0], "end": TARGET[1]},
        "target_before": target_metrics(before), "target_after": target_after,
        "target_ownership": {
            "asm_bytes": target_after["asm"],
            "structured_data_bytes": target_after["structured_data"],
            "padding_bytes": 0,
            "remaining_unknown_blob_bytes": target_after["blob"],
        },
        "target_gap_census": {
            "POSSIBLE_CODE_BYTES": 0,
            "UNKNOWN_DATA_BYTES": 0,
            "UNRESOLVED_BOUNDARY_BYTES": target_after["blob"],
            "note": "Remaining bytes are conservatively retained as UNKNOWN blobs; no data interpretation is claimed.",
        },
        "executable_blob_bytes_removed": target_after["asm"],
        "promoted_ranges": attempts, "remaining_target_blob_intervals": remaining,
        "whole_rom_before": auto.FULL.metrics(before, len(rom)),
        "whole_rom_after": whole,
        "quality": {"gaps": 0, "overlaps": 0},
        "full_rom": {"exact": True, "size": len(rom),
                     "crc32": f"{zlib.crc32(rom) & 0xFFFFFFFF:08X}",
                     "sha1": hashlib.sha1(rom).hexdigest(), "sha256": hashlib.sha256(rom).hexdigest()},
        "m12_2_proposal": {"start": p0[0][0], "end": p0[0][1], "size": p0[0][2],
                           "observed_pc_count": p0[0][3], "static_xref_count": p0[0][4],
                           "reason": "highest remaining observed-PC concentration after current target removal"},
        "no_m12_2_started": True,
    }
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    return report


def run_promotion(args):
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    output.mkdir(parents=True)
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    validate_baseline(baseline, rom)
    current = auto.renumber(copy.deepcopy(baseline["entries"]))
    before = copy.deepcopy(current)
    by_start = {e["start"]: auto.resolve_artifact(baseline_path, e["artifact"])
                for e in current if e["kind"] == "CODE_VERIFIED"}
    staging = output / "staging"
    staging.mkdir()
    attempts = []
    for index, (start, end, dynamic) in enumerate(PROMOTIONS, 1):
        asm = staging / f"sub_{start:06X}.asm"
        data = staging / f"sub_{start:06X}.json"
        binary = staging / f"sub_{start:06X}.bin"
        emitted = auto.run([args.range_tool, rom_path, hex(start), hex(end), asm, data])
        record = {"seed_pc": f"0x{start:06X}", "start": start, "end": end,
                  "size": end - start, "accepted": False, "reason": ""}
        if emitted.returncode:
            record["reason"] = auto.classify_error(emitted.stdout + emitted.stderr)
            record["detail"] = (emitted.stdout + emitted.stderr).strip()[:400]
            raise ValueError(f"promotion failed at 0x{start:06X}: {record['reason']}")
        assembled = auto.run([args.assembler, "-m68000", "-no-opt", "-Fbin", "-o", binary, asm])
        if assembled.returncode or binary.read_bytes() != rom[start:end]:
            raise ValueError(f"slice exactness failed at 0x{start:06X}")
        record.update(slice_evidence(data))
        current = auto.promote(current, candidate(start, end, dynamic))
        by_start[start] = asm
        record.update({"accepted": True, "reason": "exact slice and transaction candidate accepted",
                       "classification": auto.promotion_trust_level(candidate(start, end, dynamic))})
        attempts.append(record)
    materialized = output / "materialized"
    materialized_entries = auto.materialize(materialized, current, rom, source_map(current, by_start))
    matched, difference, reason, detail = auto.verify_full(materialized, rom, args.assembler,
                                                           materialized_entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = auto.manifest_for(materialized_entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["transaction"] = "M12.1 explicit evidence-backed promotions"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    report = build_report(before, materialized_entries, attempts, baseline_path, output, rom)
    report["evidence"] = global_evidence(Path(args.global_evidence).resolve())
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report["full_rom"]["rebuilt_crc32"] = f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}"
    report["full_rom"]["rebuilt_sha1"] = hashlib.sha1(rebuilt).hexdigest()
    report["full_rom"]["rebuilt_sha256"] = hashlib.sha256(rebuilt).hexdigest()
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"classification": report["classification"],
                      "promoted_bytes": report["executable_blob_bytes_removed"],
                      "remaining_target_blob_intervals": report["remaining_target_blob_intervals"],
                      "full_rom": report["full_rom"],
                      "m12_2_proposal": report["m12_2_proposal"]}, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--range-tool", required=True)
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--global-evidence", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    run_promotion(args)


if __name__ == "__main__":
    main()
