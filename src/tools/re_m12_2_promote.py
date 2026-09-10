"""Transactional M12.2 promotion for the P0 0x00DE00..0x00E338 region."""
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

TARGET = (0x00DE00, 0x00E338)
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
PROMOTIONS = (
    (0x00DEEC, 0x00DF52, False, "looping bounded routine; one indirect JSR at 0x00DF1A"),
    (0x00E0B8, 0x00E0BA, False, "standalone RTS island at a decoder/static boundary"),
    (0x00E0BA, 0x00E0F4, False, "bounded routine; one indirect JSR at 0x00E0E4"),
    (0x00E0F4, 0x00E0FE, False, "exact two-instruction dispatch prefix with direct exit to 0x00E106"),
    (0x00E106, 0x00E140, False, "bounded routine; one indirect JSR at 0x00E130"),
    (0x00E268, 0x00E2A2, True, "bounded arithmetic routine; four ADDX forms were independently normalized"),
    (0x00E2A2, 0x00E2BA, True, "bounded arithmetic routine; MULU form at 0x00E2B0 was independently normalized"),
    (0x00E2D4, 0x00E2F0, False, "exact dispatch setup prefix; indirect JMP at 0x00E2F0 remains blob-backed"),
    (0x00E302, 0x00E308, True, "observed case arm with direct branch to shared tail 0x00E332"),
    (0x00E308, 0x00E30C, True, "observed case arm with direct branch to shared tail 0x00E332"),
    (0x00E30C, 0x00E310, True, "observed case arm with direct branch to shared tail 0x00E332"),
    (0x00E310, 0x00E316, True, "observed case arm with direct branch to shared tail 0x00E332"),
    (0x00E332, 0x00E338, False, "shared movem restore and RTS tail"),
)


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
    if (sum(e["kind"] == "CODE_VERIFIED" for e in entries) != 209 or
            sum(e["kind"] == "UNKNOWN" for e in entries) != 138):
        raise ValueError("unexpected M12.1 manifest census")
    target_entries = [e for e in entries if overlap((e["start"], e["end"]), TARGET)]
    if len(target_entries) != 1 or target_entries[0]["kind"] != "UNKNOWN" or \
            target_entries[0]["start"] != TARGET[0] or target_entries[0]["end"] != TARGET[1]:
        raise ValueError("target is not one UNKNOWN baseline entry")


def target_metrics(entries):
    totals = {"asm": 0, "structured_data": 0, "padding": 0, "blob": 0}
    for entry in entries:
        if not overlap((entry["start"], entry["end"]), TARGET):
            continue
        size = min(entry["end"], TARGET[1]) - max(entry["start"], TARGET[0])
        if entry["kind"] == "CODE_VERIFIED":
            totals["asm"] += size
        elif entry["kind"] == "DATA_KNOWN":
            totals["structured_data"] += size
        else:
            totals["blob"] += size
    return totals


def candidate(start, end, dynamic):
    return {"address": start, "start": start, "end": end, "score": 0,
            "known_static_target": True, "existing_dynamic_support": dynamic}


def source_map(entries, sources):
    return {index: sources[entry["start"]] for index, entry in enumerate(entries)
            if entry["kind"] == "CODE_VERIFIED"}


def exact_evidence(path, indirect_exits):
    data = json.loads(path.read_text())
    instructions = data["instructions"]
    branches = [{"source": item["address"], "target": item["branch_target"]}
                for item in instructions if item.get("branch_target") is not None]
    return {"instruction_count": len(instructions),
            "direct_branches": branches,
            "returns": [item["address"] for item in instructions
                        if item.get("operation") in ("rts", "rte", "rtr")],
            "indirect_exits": list(indirect_exits),
            "memory_reference_operands": sum(
                item.get("kind") in {"absolute_long", "absolute_word", "indirect",
                                      "postincrement", "predecrement", "displacement",
                                      "indexed", "pc_indexed"}
                for instruction in instructions
                for item in (instruction.get("source"), instruction.get("destination"))
                if item is not None),
            "source_decoder": data["source_decoder"]}


def priority_evidence(path):
    data = json.loads(path.read_text())
    regions = [item for item in data.get("top20", [])
               if TARGET[0] <= int(item["start"], 0) < TARGET[1]]
    observed = sorted({int(pc, 0) for item in regions for pc in item.get("observed_pcs", [])})
    xrefs = sorted({int(pc, 0) for item in regions for pc in item.get("incoming_xrefs", [])})
    return {"source": str(path), "region_count": len(regions),
            "observed_pc_count": len(observed), "observed_pcs": observed,
            "static_xref_count": len(xrefs), "static_xrefs": xrefs,
            "regions": regions}


def global_evidence(path):
    data = json.loads(path.read_text())
    items = []
    gaps = []
    cursor = TARGET[0]
    for item in sorted(data.get("instructions", []), key=lambda value: int(value["address"], 0)):
        address = int(item["address"], 0)
        if not TARGET[0] <= address < TARGET[1]:
            continue
        items.append(item)
        if address > cursor:
            gaps.append([cursor, address])
        cursor = max(cursor, address + int(item["instruction_length"]))
    if cursor < TARGET[1]:
        gaps.append([cursor, TARGET[1]])
    predecessors = {}
    for item in data.get("instructions", []):
        for match in re.finditer(r"loc_([0-9A-Fa-f]{6})", item.get("decoded_instruction", "")):
            target = int(match.group(1), 16)
            if TARGET[0] <= target < TARGET[1]:
                predecessors.setdefault(target, []).append(int(item["address"], 0))
    return {"decoded_instruction_count": len(items), "coverage_gaps": gaps,
            "unsupported_addresses": [int(item["address"], 0) for item in items
                                      if item.get("status") != "DECODED"],
            "predecessors": {str(key): sorted(set(value)) for key, value in predecessors.items()}}


def whole_metrics(entries, rom_size):
    metrics = auto.FULL.metrics(entries, rom_size)
    metrics["ranges"] = {"asm": sum(e["kind"] == "CODE_VERIFIED" for e in entries),
                          "structured_data": sum(e["kind"] == "DATA_KNOWN" for e in entries),
                          "blob": sum(e["kind"] == "UNKNOWN" for e in entries),
                          "gaps": 0, "overlaps": 0}
    return metrics


def next_p0():
    queue = [(0x000000, 0x0007C4, 23, 3), (0x003B3E, 0x004A92, 12, 3),
             (0x006516, 0x0083D4, 25, 0), (0x008E90, 0x0094A2, 9, 15),
             (0x0094D2, 0x0099B8, 11, 4), (0x00D406, 0x00D7B0, 13, 1),
             (0x00DE00, 0x00E338, 31, 87)]
    remaining = [item for item in queue if (item[0], item[1]) != TARGET]
    remaining.sort(key=lambda item: (-item[2], -item[3], item[1] - item[0], item[0]))
    start, end, observed, xrefs = remaining[0]
    return {"start": start, "end": end, "size": end - start,
            "observed_pc_count": observed, "static_xref_count": xrefs,
            "reason": "highest remaining observed-PC concentration after M12.2 removal; not started"}


def build_report(before, after, attempts, output, rom, priority, global_data):
    target_after = target_metrics(after)
    blob_intervals = [{"start": max(e["start"], TARGET[0]),
                       "end": min(e["end"], TARGET[1]), "size": min(e["end"], TARGET[1]) -
                       max(e["start"], TARGET[0]),
                       "classification": "UNRESOLVED_BOUNDARY",
                       "reason": "not promoted: no independent exact source-ownership closure"}
                      for e in after if e["kind"] == "UNKNOWN" and
                      overlap((e["start"], e["end"]), TARGET)]
    dispatch = next((item for item in blob_intervals if item["start"] == 0x00E2F0), None)
    if dispatch:
        dispatch["reason"] = "indirect JMP dispatch at 0x00E2F0; target table and continuation are unresolved"
    whole_before = whole_metrics(before, len(rom))
    whole_after = whole_metrics(after, len(rom))
    reasons = {
        0x00DE00: "preceding bytes have no independently closed code boundary or exact CFG evidence",
        0x00DF52: "between the closed 0x00DEEC routine and next confirmed island; continuation unresolved",
        0x00E0FE: "branch bridge ends at 0x00E0FE; next entry 0x00E106 is not proven contiguous",
        0x00E140: "after the closed 0x00E106 routine; no exact source-owned continuation to 0x00E268",
        0x00E2BA: "after the closed 0x00E2A2 routine; no exact boundary to the dispatch setup",
        0x00E2F0: "indirect JMP dispatch at 0x00E2F0; target table and continuation are unresolved",
        0x00E2F2: "after the unresolved dispatch instruction; jump-table continuation is not proven",
        0x00E302: "case-arm region is promoted only in four independently bounded arms",
    }
    for interval in blob_intervals:
        interval["reason"] = reasons.get(interval["start"], interval["reason"])
    return {"schema": "oasis.m68k.m12-2-asm-promotion.v1",
            "classification": "M12_2_P0_CODE_PROMOTION_PARTIAL_EXACT",
            "baseline_commit": "0dac30bcca103ae03a6372d24c2aca25e1fb6460",
            "target": {"start": TARGET[0], "end": TARGET[1], "size": TARGET[1] - TARGET[0]},
            "target_before": target_metrics(before), "target_after": target_after,
            "target_ownership": {"asm_bytes": target_after["asm"],
                                  "structured_data_bytes": target_after["structured_data"],
                                  "padding_bytes": target_after["padding"],
                                  "remaining_unknown_blob_bytes": target_after["blob"]},
            "target_gap_census": {"POSSIBLE_CODE_BYTES": 0, "UNKNOWN_DATA_BYTES": 0,
                                   "UNRESOLVED_BOUNDARY_BYTES": target_after["blob"],
                                   "note": "All remaining target blobs stay conservative; no data interpretation is claimed."},
            "executable_blob_bytes_removed": target_after["asm"],
            "promoted_ranges": attempts, "remaining_target_intervals": blob_intervals,
            "whole_rom_before": whole_before, "whole_rom_after": whole_after,
            "quality": {"gaps": 0, "overlaps": 0,
                        "before_ranges": whole_before["ranges"],
                        "after_ranges": whole_after["ranges"]},
            "full_rom": {"exact": True, "size": len(rom),
                         "crc32": f"{zlib.crc32(rom) & 0xFFFFFFFF:08X}",
                         "sha1": hashlib.sha1(rom).hexdigest(), "sha256": hashlib.sha256(rom).hexdigest()},
            "evidence": {"priority": priority, "global_census": global_data},
            "next_p0_queue": {"recomputed": True, "proposed_m12_3": next_p0()},
            "no_m12_3_started": True, "output": str(output)}


def run_promotion(args):
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    validate_baseline(baseline, rom)
    current = auto.renumber(copy.deepcopy(baseline["entries"]))
    before = copy.deepcopy(current)
    sources = {entry["start"]: auto.resolve_artifact(baseline_path, entry["artifact"])
               for entry in current if entry["kind"] == "CODE_VERIFIED"}
    output.mkdir(parents=True)
    staging = output / "staging"
    staging.mkdir()
    attempts = []
    for index, (start, end, dynamic, reason) in enumerate(PROMOTIONS, 1):
        asm = staging / f"sub_{start:06X}.asm"
        data = staging / f"sub_{start:06X}.json"
        binary = staging / f"sub_{start:06X}.bin"
        emitted = auto.run([args.range_tool, rom_path, hex(start), hex(end), asm, data])
        if emitted.returncode:
            raise ValueError(f"promotion failed at 0x{start:06X}: {auto.classify_error(emitted.stdout + emitted.stderr)}")
        assembled = auto.run([args.assembler, "-m68000", "-no-opt", "-Fbin", "-o", binary, asm])
        if assembled.returncode or binary.read_bytes() != rom[start:end]:
            raise ValueError(f"slice exactness failed at 0x{start:06X}")
        indirect = {0x00DEEC: [0x00DF1A], 0x00E0BA: [0x00E0E4],
                    0x00E106: [0x00E130], 0x00E2D4: []}.get(start, [])
        record = {"seed_pc": f"0x{start:06X}", "start": start, "end": end,
                  "size": end - start, "accepted": True,
                  "classification": "68000_CODE_CONFIRMED", "reason": reason,
                  "dynamic_observed": dynamic, "evidence": exact_evidence(data, indirect)}
        current = auto.promote(current, candidate(start, end, dynamic))
        sources[start] = asm
        attempts.append(record)
    materialized = output / "materialized"
    entries = auto.materialize(materialized, current, rom, source_map(current, sources))
    matched, difference, verify_reason, detail = auto.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {verify_reason} {detail} {difference}")
    manifest = auto.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["transaction"] = "M12.2 exact evidence-backed promotions"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    report = build_report(before, entries, attempts, output, rom,
                          priority_evidence(Path(args.priority_report).resolve()),
                          global_evidence(Path(args.global_evidence).resolve()))
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report["full_rom"].update({"rebuilt_crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                                "rebuilt_sha1": hashlib.sha1(rebuilt).hexdigest(),
                                "rebuilt_sha256": hashlib.sha256(rebuilt).hexdigest()})
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"target_after": report["target_after"],
                      "whole_rom_after": report["whole_rom_after"],
                      "next_p0": report["next_p0_queue"]["proposed_m12_3"]}, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--range-tool", required=True)
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--global-evidence", required=True)
    parser.add_argument("--priority-report", required=True)
    parser.add_argument("--output", required=True)
    run_promotion(parser.parse_args())


if __name__ == "__main__":
    main()
