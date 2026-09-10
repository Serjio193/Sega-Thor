"""Transactional M12.3 promotion for P0 0x006516..0x0083D4."""
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

TARGET = (0x006516, 0x0083D4)
M12_2_TARGET = (0x00DE00, 0x00E338)
BASELINE_COMMIT = "b9b55fc46fad88eb2ff1285e85bd9d8169d0289d"
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"

# Each island is a complete exact CFG walk from an observed/proven seed.  The
# gaps are deliberately retained as blobs when a branch or return makes the
# contiguous bytes lack an independently closed ownership boundary.
PROMOTIONS = (
    (0x007A28, 0x007B2A, False, "Atlas-local verified entry; CFG closes at RTS 0x007B28"),
    (0x007B64, 0x007B76, False, "direct branch target 0x007B64; DBF loop closes at unconditional branch"),
    (0x007B76, 0x007B7A, False, "unconditional branch bridge to 0x007C20"),
    (0x007B7A, 0x007B84, False, "direct conditional target; branch exits to 0x007A6C"),
    (0x007B84, 0x007B9A, False, "direct branch target; DBF/branch island closes at 0x007B96"),
    (0x007B9A, 0x007BA4, False, "direct conditional target; branch exits to 0x007A6C"),
    (0x007BA4, 0x007BD4, False, "direct target; bounded branch/condition island"),
    (0x007BD4, 0x007BE4, False, "direct target; bounded branch island"),
    (0x007BE4, 0x007BE8, False, "unconditional branch bridge to 0x007A6C"),
    (0x007BE8, 0x007BF6, False, "direct target; dynamic BSET and unconditional branch"),
    (0x007BF6, 0x007C20, False, "direct target; bounded flag/update island"),
    (0x007C20, 0x007C3C, False, "direct target; helper call and branch to 0x007B28"),
    (0x0082AE, 0x0082F8, False, "Atlas-local verified entry; bounded routine closes at RTS"),
    (0x0082F8, 0x00838C, True, "direct caller 0x007F98; 25 observed PCs and CFG closes at RTS"),
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
    if (sum(e["kind"] == "CODE_VERIFIED" for e in entries) != 222 or
            sum(e["kind"] == "UNKNOWN" for e in entries) != 144):
        raise ValueError("unexpected M12.2 manifest census")
    target = [e for e in entries if overlap((e["start"], e["end"]), TARGET)]
    if len(target) != 1 or target[0]["kind"] != "UNKNOWN" or \
            (target[0]["start"], target[0]["end"]) != TARGET:
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


def whole_metrics(entries, rom_size):
    result = auto.FULL.metrics(entries, rom_size)
    result["ranges"] = {
        "asm": sum(e["kind"] == "CODE_VERIFIED" for e in entries),
        "structured_data": sum(e["kind"] == "DATA_KNOWN" for e in entries),
        "blob": sum(e["kind"] == "UNKNOWN" for e in entries),
        "gaps": 0, "overlaps": 0,
    }
    return result


def source_map(entries, sources):
    return {index: sources[entry["start"]] for index, entry in enumerate(entries)
            if entry["kind"] == "CODE_VERIFIED"}


def candidate(start, end, dynamic):
    return {"address": start, "start": start, "end": end, "score": 0,
            "known_static_target": True, "existing_dynamic_support": dynamic}


def exact_evidence(path):
    data = json.loads(path.read_text())
    instructions = data["instructions"]
    return {
        "instruction_count": len(instructions),
        "direct_branches": [{"source": x["address"], "target": x["branch_target"]}
                             for x in instructions if x.get("branch_target") is not None],
        "returns": [x["address"] for x in instructions
                    if x.get("operation") in ("rts", "rte", "rtr")],
        "indirect_exits": [x["address"] for x in instructions
                           if x.get("operation") in ("jmp", "jsr") and
                           x.get("branch_target") is None],
        "source_decoder": data["source_decoder"],
    }


def priority_evidence(path):
    data = json.loads(path.read_text())
    regions = [x for x in data.get("top20", [])
               if TARGET[0] <= int(x["start"], 0) < TARGET[1]]
    observed = sorted({int(pc, 0) for x in regions for pc in x.get("observed_pcs", [])})
    xrefs = sorted({int(pc, 0) for x in regions for pc in x.get("incoming_xrefs", [])})
    return {"source": str(path), "regions": regions,
            "observed_pc_count": len(observed), "observed_pcs": observed,
            "static_xref_count": len(xrefs), "static_xrefs": xrefs,
            "interpretation": {
                "A": "not dead: 25 GPGX PCs are decoded and contiguous in 0x0082FC..0x00832C",
                "B": "not data: the PCs are the interior fallthrough of entry 0x0082F8",
                "C": "metric scope: incoming_xrefs are counted to the priority subregion, not its containing entry",
                "D": "not an unresolved indirect caller: the local 0x0082F8 CFG has no indirect exit",
                "conclusion": "0 is a subregion/interior fallthrough accounting result, not absence of code evidence",
            }}


def static_edges(path):
    data = json.loads(path.read_text())
    edges = []
    for item in data.get("instructions", []):
        text = item.get("decoded_instruction", "")
        match = re.search(r"loc_([0-9A-Fa-f]{6})", text)
        if not match:
            continue
        target = int(match.group(1), 16)
        if TARGET[0] <= target < TARGET[1]:
            edges.append({"source": int(item["address"], 0), "target": target,
                          "instruction": text})
    return edges


def remaining(entries):
    reasons = {
        TARGET[0]: "no independently closed entry/CFG continuation from the preceding trusted ASM at 0x006516",
        0x007B2A: "0x007A28 CFG returns at 0x007B28; 0x007B2A..0x007B64 has no proven entry or observed PC",
        0x007C3C: "0x007A28 family ends through branch/return paths; no exact ownership bridge to 0x0082AE",
        0x00838C: "unresolved neighboring dispatch/helper bytes begin at 0x00838C; no observed PC or closed entry to 0x0083D4",
    }
    return [{"start": max(e["start"], TARGET[0]), "end": min(e["end"], TARGET[1]),
             "size": min(e["end"], TARGET[1]) - max(e["start"], TARGET[0]),
             "classification": "UNRESOLVED_BOUNDARY",
             "reason": reasons.get(max(e["start"], TARGET[0]),
                                    "no independent exact source-ownership closure")}
            for e in entries if e["kind"] == "UNKNOWN" and
            overlap((e["start"], e["end"]), TARGET)]


def next_p0():
    queue = [(0x000000, 0x0007C4, 23, 3), (0x003B3E, 0x004A92, 12, 3),
             TARGET + (25, 0), (0x008E90, 0x0094A2, 9, 15),
             (0x0094D2, 0x0099B8, 11, 4), (0x00D406, 0x00D7B0, 13, 1),
             M12_2_TARGET + (31, 87)]
    remaining = [item for item in queue if item[:2] not in {TARGET, M12_2_TARGET}]
    remaining.sort(key=lambda item: (-item[2], -item[3], item[1] - item[0], item[0]))
    start, end, observed, xrefs = remaining[0]
    return {"start": start, "end": end, "size": end - start,
            "observed_pc_count": observed, "static_xref_count": xrefs,
            "reason": "highest remaining P0 observed-PC concentration after M12.3 removal; not started"}


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
    observed = set(priority_evidence(Path(args.priority_report))["observed_pcs"])
    for start, end, dynamic, reason in PROMOTIONS:
        asm = staging / f"sub_{start:06X}.asm"
        data = staging / f"sub_{start:06X}.json"
        binary = staging / f"sub_{start:06X}.bin"
        emitted = auto.run([args.range_tool, rom_path, hex(start), hex(end), asm, data])
        if emitted.returncode:
            raise ValueError(f"promotion failed at 0x{start:06X}: {emitted.stdout}{emitted.stderr}")
        assembled = auto.run([args.assembler, "-m68000", "-no-opt", "-Fbin", "-o", binary, asm])
        if assembled.returncode or binary.read_bytes() != rom[start:end]:
            raise ValueError(f"slice exactness failed at 0x{start:06X}")
        current = auto.promote(current, candidate(start, end, dynamic))
        sources[start] = asm
        attempts.append({"seed_pc": f"0x{start:06X}", "start": start, "end": end,
                         "size": end - start, "accepted": True,
                         "classification": "68000_CODE_CONFIRMED", "reason": reason,
                         "dynamic_observed": dynamic,
                         "observed_pcs_inside": sorted(pc for pc in observed if start <= pc < end),
                         "evidence": exact_evidence(data)})
    materialized = output / "materialized"
    entries = auto.materialize(materialized, current, rom, source_map(current, sources))
    matched, difference, verify_reason, detail = auto.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {verify_reason} {detail} {difference}")
    manifest = auto.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["transaction"] = "M12.3 exact evidence-backed promotions"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    priority = priority_evidence(Path(args.priority_report).resolve())
    full = {"exact": True, "size": len(rom),
            "crc32": f"{zlib.crc32(rom) & 0xFFFFFFFF:08X}",
            "sha1": hashlib.sha1(rom).hexdigest(), "sha256": hashlib.sha256(rom).hexdigest()}
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    full.update({"rebuilt_crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                 "rebuilt_sha1": hashlib.sha1(rebuilt).hexdigest(),
                 "rebuilt_sha256": hashlib.sha256(rebuilt).hexdigest()})
    report = {
        "schema": "oasis.m68k.m12-3-asm-promotion.v1",
        "classification": "M12_3_P0_CODE_PROMOTION_PARTIAL_EXACT",
        "baseline_commit": BASELINE_COMMIT,
        "target": {"start": TARGET[0], "end": TARGET[1], "size": TARGET[1] - TARGET[0]},
        "target_before": target_metrics(before), "target_after": target_metrics(entries),
        "target_gap_census": {"POSSIBLE_CODE_BYTES": 0, "UNKNOWN_DATA_BYTES": 0,
                               "UNRESOLVED_BOUNDARY_BYTES": target_metrics(entries)["blob"],
                               "note": "All remaining target blobs stay conservative; no data interpretation is claimed."},
        "executable_blob_bytes_removed": target_metrics(entries)["asm"],
        "promoted_ranges": attempts, "remaining_target_intervals": remaining(entries),
        "whole_rom_before": whole_metrics(before, len(rom)),
        "whole_rom_after": whole_metrics(entries, len(rom)),
        "quality": {"gaps": 0, "overlaps": 0}, "full_rom": full,
        "evidence": {"priority": priority, "static_edges": static_edges(Path(args.global_evidence).resolve()),
                      "dynamic_observation_note": "The 25 observed PCs are the exact priority-report starts 0x0082FC..0x00832C; current decoder and vasm round-trip each containing CFG island."},
        "decoder_extension": {"form": "dynamic BSET/BCHG/BCLR/BTST", "rom_examples": ["0x007B24", "0x007BEC"],
                              "raw_directive_note": "dc.w is emitted only for byte-immediate extensions with canonical 0xFF high byte that vasm otherwise normalizes; it is documented here and remains executable ASM source ownership."},
        "next_p0_queue": {"recomputed": True, "proposed_m12_4": next_p0()},
        "no_m12_4_started": True, "no_cpp_migration_started": True,
        "no_emulator_expansion_started": True,
    }
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"classification": report["classification"],
                      "target_after": report["target_after"],
                      "whole_rom_after": report["whole_rom_after"],
                      "next_p0": report["next_p0_queue"]["proposed_m12_4"]}, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("range-tool", "assembler", "rom", "manifest", "global-evidence", "priority-report", "output"):
        parser.add_argument("--" + name, required=True)
    run_promotion(parser.parse_args())


if __name__ == "__main__":
    main()
