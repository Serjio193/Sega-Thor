"""Transactional M12.5 promotion of exact ASM islands around 0x003B3E."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import sys
import zlib

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))
import re_auto_promote as auto

TARGET = (0x003B3E, 0x004A92)
BASELINE_COMMIT = "33cb6d9985b3a87cd9eb92d4ad0883a736bde9ff"
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"

# Independent nested entry points from the derived Ghidra map.  The exact
# range tool and vasm must close every interval.  The enclosing routines stay
# blob-backed when their whole CFG has gaps or overlap.
PROMOTIONS = (
    (0x003D06, 0x003D4C, "nested entry; exact bounded decoder and vasm round-trip"),
    (0x003E18, 0x003E6A, "nested entry; exact bounded decoder and vasm round-trip"),
    (0x003E6A, 0x003F84, "nested entry; exact bounded decoder and vasm round-trip"),
    (0x004026, 0x00405E, "leaf entry; Ghidra boundary agrees with bounded decoder"),
    (0x00405E, 0x004066, "nested entry; exact bounded decoder and vasm round-trip"),
    (0x004066, 0x0040A6, "leaf entry; Ghidra boundary agrees with bounded decoder"),
    (0x0040A6, 0x0040CE, "leaf entry; Ghidra boundary agrees with bounded decoder"),
    (0x0040DE, 0x0040EA, "nested entry; exact bounded decoder and vasm round-trip"),
    (0x0040EA, 0x004158, "nested entry; Ghidra boundary agrees with bounded decoder"),
    (0x004158, 0x00417C, "leaf entry; Ghidra boundary agrees with bounded decoder"),
    (0x00417C, 0x00419A, "nested entry; Ghidra boundary agrees with bounded decoder"),
    (0x00419A, 0x0041BE, "nested entry; Ghidra boundary agrees with bounded decoder"),
    (0x0041BE, 0x0041E2, "nested entry; Ghidra boundary agrees with bounded decoder"),
    (0x0041E2, 0x004222, "nested entry; exact bounded decoder and vasm round-trip"),
    (0x004222, 0x004250, "leaf entry; exact bounded decoder and vasm round-trip"),
    (0x004250, 0x00426C, "nested entry; exact bounded decoder and vasm round-trip"),
    (0x00426C, 0x004296, "nested entry; exact bounded decoder and vasm round-trip"),
    (0x004296, 0x0042A6, "leaf entry; Ghidra boundary agrees with bounded decoder"),
    (0x0042A6, 0x0042D2, "leaf entry; exact bounded decoder and vasm round-trip"),
    (0x0042D2, 0x0042E8, "nested entry; exact bounded decoder and vasm round-trip"),
    (0x0042E8, 0x004388, "nested entry; Ghidra boundary agrees with bounded decoder"),
    (0x004388, 0x0043FE, "nested entry; exact bounded decoder and vasm round-trip"),
    (0x0043FE, 0x004416, "leaf entry; Ghidra boundary agrees with bounded decoder"),
)


def overlap(left, right):
    return left[0] < right[1] and right[0] < left[1]


def validate_ranges():
    previous = TARGET[0]
    for start, end, _ in PROMOTIONS:
        if not (TARGET[0] <= start < end <= TARGET[1]) or start < previous:
            raise ValueError("M12.5 promotion ranges are invalid or overlap")
        previous = end


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
    if manifest.get("rom_sha256") not in (None, ROM_SHA256):
        raise ValueError("baseline manifest ROM identity mismatch")
    if sum(e["kind"] == "CODE_VERIFIED" for e in entries) != 244:
        raise ValueError("unexpected M12.4 code census")
    target = [e for e in entries if overlap((e["start"], e["end"]), TARGET)]
    if len(target) != 1 or target[0]["kind"] != "UNKNOWN":
        raise ValueError("M12.5 target is not one UNKNOWN baseline entry")


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_owned(entries, start, end, source, reason):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN" or not (entry["start"] <= start and end <= entry["end"]):
            continue
        replacement = []
        if entry["start"] < start:
            replacement.append({"start": entry["start"], "end": start, "kind": "UNKNOWN",
                                "source": "canonical_local_rom", "confidence": "ROM_HASH_VERIFIED",
                                "emitted_artifact_type": "blob"})
        replacement.append({"start": start, "end": end, "kind": "CODE_VERIFIED",
                            "source": "M12.5_exact_source_ownership", "confidence": "CONFIRMED",
                            "classification": "68000_CODE_CONFIRMED",
                            "emitted_artifact_type": "asm", "asm_source": str(source),
                            "ownership_reason": reason})
        if end < entry["end"]:
            replacement.append({"start": end, "end": entry["end"], "kind": "UNKNOWN",
                                "source": "canonical_local_rom", "confidence": "ROM_HASH_VERIFIED",
                                "emitted_artifact_type": "blob"})
        return renumber(entries[:index] + replacement + entries[index + 1:])
    raise ValueError(f"ownership range 0x{start:06X}..0x{end:06X} is not UNKNOWN")


def target_metrics(entries):
    result = {"asm": 0, "blob": 0}
    for entry in entries:
        if overlap((entry["start"], entry["end"]), TARGET):
            size = min(entry["end"], TARGET[1]) - max(entry["start"], TARGET[0])
            result["asm" if entry["kind"] == "CODE_VERIFIED" else "blob"] += size
    return result


def source_map(entries, sources):
    return {index: sources[entry["start"]] for index, entry in enumerate(entries)
            if entry.get("emitted_artifact_type") == "asm"}


def run_promotion(args):
    validate_ranges()
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    validate_baseline(baseline, rom)
    current = renumber(copy.deepcopy(baseline["entries"]))
    sources = {entry["start"]: baseline_path.parent / entry["artifact"]
               for entry in baseline["entries"]
               if entry.get("emitted_artifact_type") == "asm"}
    output.mkdir(parents=True)
    staging = output / "staging"
    staging.mkdir()
    attempts = []
    for start, end, reason in PROMOTIONS:
        asm = staging / f"code_{start:06X}.asm"
        data = staging / f"code_{start:06X}.json"
        binary = staging / f"code_{start:06X}.bin"
        emitted = auto.run([args.range_tool, rom_path, hex(start), hex(end), asm, data])
        if emitted.returncode:
            raise ValueError(f"M12.5 decode failed at 0x{start:06X}: {emitted.stdout}{emitted.stderr}")
        assembled = auto.run([args.assembler, "-m68000", "-no-opt", "-Fbin", "-o", binary, asm])
        if assembled.returncode or binary.read_bytes() != rom[start:end]:
            raise ValueError(f"M12.5 exactness failed at 0x{start:06X}")
        current = split_owned(current, start, end, asm, reason)
        sources[start] = asm
        decoded = json.loads(data.read_text())
        attempts.append({"start": start, "end": end, "size": end - start,
                         "instruction_count": len(decoded["instructions"]),
                         "classification": "68000_CODE_CONFIRMED", "reason": reason,
                         "unresolved_control_flow": decoded.get("unresolved_control_flow", [])})

    materialized = output / "materialized"
    entries = auto.materialize(materialized, current, rom, source_map(current, sources))
    matched, difference, verify_reason, detail = auto.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {verify_reason} {detail} {difference}")
    manifest = auto.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["transaction"] = "M12.5 exact nested ASM islands"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    full = {"exact": True, "size": len(rom),
            "crc32": f"{zlib.crc32(rom) & 0xFFFFFFFF:08X}",
            "sha1": hashlib.sha1(rom).hexdigest(), "sha256": hashlib.sha256(rom).hexdigest(),
            "rebuilt_crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
            "rebuilt_sha1": hashlib.sha1(rebuilt).hexdigest(),
            "rebuilt_sha256": hashlib.sha256(rebuilt).hexdigest()}
    remaining = [{"start": max(e["start"], TARGET[0]), "end": min(e["end"], TARGET[1]),
                  "size": min(e["end"], TARGET[1]) - max(e["start"], TARGET[0])}
                 for e in entries if e["kind"] == "UNKNOWN" and overlap((e["start"], e["end"]), TARGET)]
    report = {"schema": "oasis.m68k.m12-5-asm-promotion.v1",
              "classification": "M12_5_EXACT_NESTED_ISLANDS_PARTIAL",
              "baseline_commit": BASELINE_COMMIT,
              "target": {"start": TARGET[0], "end": TARGET[1], "size": TARGET[1] - TARGET[0]},
              "promoted_code_ranges": attempts, "remaining_target_intervals": remaining,
              "target_after": target_metrics(entries), "whole_rom_after": auto.FULL.metrics(entries, len(rom)),
              "quality": {"gaps": 0, "overlaps": 0}, "full_rom": full,
              "failed_closed_regions": [
                  {"range": "0x003B3E..0x003D06", "reason": "enclosing routine has gaps/overlap"},
                  {"range": "0x004416..0x00494C", "reason": "no independently closed island selected"},
                  {"range": "0x00494C..0x004A92", "reason": "enclosing routine has gaps/overlap"}],
              "next_queue": {"start": 0x008E90, "end": 0x0094A2, "size": 0x0094A2 - 0x008E90,
                             "observed_pc_count": 9, "static_xref_count": 15},
              "no_cpp_migration_started": True, "no_emulator_expansion_started": True}
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"classification": report["classification"], "target_after": report["target_after"],
                      "whole_rom_after": report["whole_rom_after"], "full_rom": full}, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("assembler", "rom", "manifest", "range-tool", "output"):
        parser.add_argument("--" + name, required=True)
    run_promotion(parser.parse_args())


if __name__ == "__main__":
    main()
