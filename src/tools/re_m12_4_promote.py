"""Transactional M12.4 promotion for the ROM-start P0 region."""
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

TARGET = (0x000000, 0x0007C4)
VECTOR = (0x000000, 0x000100)
HEADER = (0x000100, 0x000200)
STRUCTURED = (0x00029C, 0x000308)
BASELINE_COMMIT = "3e667c304382c5bce81d2e5a4ff75751139db314"
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"

CODE_RANGES = (
    (0x000200, 0x000204, "vector-referenced default exception loop"),
    (0x000204, 0x000206, "vector-handler RTE terminal"),
    (0x000206, 0x00020A, "vector-referenced default exception loop"),
    (0x00020A, 0x00020C, "vector-handler RTE terminal"),
    (0x00020C, 0x00020E, "vector-referenced RTE terminal"),
    (0x00020E, 0x00029C, "reset-vector entry; 3 static xrefs and observed startup CFG"),
    (0x000308, 0x00045A, "reset startup continuation; exact CFG stops before indirect JSR"),
    (0x0006F6, 0x0007C4, "3 static callers; bounded helper CFG meets trusted 0x0007C4 ASM"),
)

REMAINING = (
    (0x00045A, 0x00045E, "UNRESOLVED_BOUNDARY",
     "unresolved indirect JSR at 0x00045A; transfer ownership is not proven"),
    (0x00045E, 0x0004C6, "UNKNOWN_DATA",
     "indexed dispatch table/data mix; only part of its layout is directly proven"),
    (0x0004C6, 0x0006F6, "POSSIBLE_CODE",
     "code-like island has no observed, vector, or trusted static entry"),
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
    if (sum(e["kind"] == "CODE_VERIFIED" for e in entries) != 236 or
            sum(e["kind"] == "UNKNOWN" for e in entries) != 147):
        raise ValueError("unexpected M12.3 manifest census")
    target = [e for e in entries if overlap((e["start"], e["end"]), TARGET)]
    if len(target) != 1 or target[0]["kind"] != "UNKNOWN" or \
            (target[0]["start"], target[0]["end"]) != TARGET:
        raise ValueError("M12.4 target is not one UNKNOWN baseline entry")


def split_owned(entries, start, end, kind, source, reason):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN" or not (entry["start"] <= start and end <= entry["end"]):
            continue
        replacement = []
        if entry["start"] < start:
            replacement.append({"start": entry["start"], "end": start, "kind": "UNKNOWN",
                                "source": "canonical_local_rom", "confidence": "ROM_HASH_VERIFIED",
                                "emitted_artifact_type": "blob"})
        replacement.append({"start": start, "end": end, "kind": kind,
                            "source": "M12.4_exact_source_ownership", "confidence": "CONFIRMED",
                            "classification": kind, "emitted_artifact_type": "asm",
                            "ownership_reason": reason, "asm_source": str(source)})
        if end < entry["end"]:
            replacement.append({"start": end, "end": entry["end"], "kind": "UNKNOWN",
                                "source": "canonical_local_rom", "confidence": "ROM_HASH_VERIFIED",
                                "emitted_artifact_type": "blob"})
        return auto.renumber(entries[:index] + replacement + entries[index + 1:])
    raise ValueError(f"ownership range 0x{start:06X}..0x{end:06X} is not UNKNOWN")


def directive_text(rom, start, end, width):
    per_line = {1: 16, 2: 8, 4: 4}[width]
    directive = {1: "dc.b", 2: "dc.w", 4: "dc.l"}[width]
    values = [int.from_bytes(rom[offset:offset + width], "big")
              for offset in range(start, end, width)]
    lines = [f"; Exact source ownership for 0x{start:06X}..0x{end:06X}."]
    digits = width * 2
    for offset in range(0, len(values), per_line):
        chunk = values[offset:offset + per_line]
        lines.append("    " + directive + " " + ",".join(f"${value:0{digits}X}" for value in chunk))
    return "\n".join(lines) + "\n"


def write_directive(path, rom, start, end, width):
    path.write_text(directive_text(rom, start, end, width))


def source_map(entries, sources):
    return {index: sources[entry["start"]] for index, entry in enumerate(entries)
            if entry.get("emitted_artifact_type") == "asm"}


def target_metrics(entries):
    totals = {"header_vector_asm": 0, "asm": 0, "structured_data_asm": 0,
              "padding": 0, "blob": 0}
    kinds = {"HEADER_VECTOR_ASM": "header_vector_asm", "CODE_VERIFIED": "asm",
             "STRUCTURED_DATA_CONFIRMED": "structured_data_asm",
             "PADDING_ALIGNMENT_CONFIRMED": "padding", "UNKNOWN": "blob"}
    for entry in entries:
        if overlap((entry["start"], entry["end"]), TARGET):
            totals[kinds[entry["kind"]]] += min(entry["end"], TARGET[1]) - max(entry["start"], TARGET[0])
    return totals


def whole_metrics(entries, rom_size):
    result = auto.FULL.metrics(entries, rom_size)
    result["ranges"] = {
        "asm": sum(e["kind"] == "CODE_VERIFIED" for e in entries),
        "header_vector_asm": sum(e["kind"] == "HEADER_VECTOR_ASM" for e in entries),
        "structured_data_asm": sum(e["kind"] == "STRUCTURED_DATA_CONFIRMED" for e in entries),
        "padding_alignment": sum(e["kind"] == "PADDING_ALIGNMENT_CONFIRMED" for e in entries),
        "blob": sum(e["kind"] == "UNKNOWN" for e in entries), "gaps": 0, "overlaps": 0,
    }
    return result


def vector_evidence(rom):
    values = [int.from_bytes(rom[offset:offset + 4], "big") for offset in range(*VECTOR, 4)]
    target_kinds = []
    for value in values:
        if value < len(rom):
            target_kinds.append("ROM")
        elif 0x00FF0000 <= value <= 0x00FFFFFF:
            target_kinds.append("RAM")
        else:
            target_kinds.append("OTHER")
    if values[1] != 0x0000020E or any(value & 1 for value in values[1:] if value < len(rom)):
        raise ValueError("vector target alignment or reset target mismatch")
    return {"count": len(values), "values": values, "target_kinds": target_kinds,
            "reset_target": values[1], "byte_exact": True}


def next_m12_5():
    candidates = [(0x003B3E, 0x004A92, 12, 3), (0x008E90, 0x0094A2, 9, 15),
                  (0x0094D2, 0x0099B8, 11, 4), (0x00D406, 0x00D7B0, 13, 1)]
    start, end, observed, xrefs = max(candidates, key=lambda item: (item[1] - item[0], item[2], item[3]))
    return {"start": start, "end": end, "size": end - start, "observed_pc_count": observed,
            "static_xref_count": xrefs,
            "reason": "largest remaining source-owned byte-gain opportunity; not started"}


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
    sources = {entry["start"]: baseline_path.parent / entry["artifact"]
               for entry in baseline["entries"]
               if entry.get("emitted_artifact_type") == "asm"}
    output.mkdir(parents=True)
    staging = output / "staging"
    staging.mkdir()

    vector_source = staging / "vectors.asm"
    header_source = staging / "header.asm"
    data_source = staging / "startup_data.asm"
    write_directive(vector_source, rom, *VECTOR, 4)
    write_directive(header_source, rom, *HEADER, 1)
    write_directive(data_source, rom, *STRUCTURED, 2)
    vector_evidence(rom)
    current = split_owned(current, *VECTOR, "HEADER_VECTOR_ASM", vector_source,
                          "64 fixed vector longwords; vector entries are not executable instructions")
    sources[VECTOR[0]] = vector_source
    current = split_owned(current, *HEADER, "HEADER_VECTOR_ASM", header_source,
                          "fixed 0x100-byte Genesis header; field semantics remain bounded")
    sources[HEADER[0]] = header_source
    current = split_owned(current, *STRUCTURED, "STRUCTURED_DATA_CONFIRMED", data_source,
                          "PC-relative startup table consumed by MOVEM and postincrement loads")
    sources[STRUCTURED[0]] = data_source

    attempts = []
    for start, end, reason in CODE_RANGES:
        asm = staging / f"code_{start:06X}.asm"
        data = staging / f"code_{start:06X}.json"
        binary = staging / f"code_{start:06X}.bin"
        emitted = auto.run([args.range_tool, rom_path, hex(start), hex(end), asm, data])
        if emitted.returncode:
            raise ValueError(f"startup promotion failed at 0x{start:06X}: {emitted.stdout}{emitted.stderr}")
        assembled = auto.run([args.assembler, "-m68000", "-no-opt", "-Fbin", "-o", binary, asm])
        if assembled.returncode or binary.read_bytes() != rom[start:end]:
            raise ValueError(f"startup exactness failed at 0x{start:06X}")
        current = split_owned(current, start, end, "CODE_VERIFIED", asm, reason)
        sources[start] = asm
        decoded = json.loads(data.read_text())
        attempts.append({"start": start, "end": end, "size": end - start,
                         "classification": "68000_CODE_CONFIRMED", "reason": reason,
                         "instruction_count": len(decoded["instructions"]),
                         "returns": [x["address"] for x in decoded["instructions"]
                                     if x.get("operation") in ("rts", "rte", "rtr")],
                         "unresolved_control_flow": decoded.get("unresolved_control_flow", [])})

    materialized = output / "materialized"
    entries = auto.materialize(materialized, current, rom, source_map(current, sources))
    matched, difference, verify_reason, detail = auto.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {verify_reason} {detail} {difference}")
    manifest = auto.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["transaction"] = "M12.4 exact ROM-start vector/header/startup ownership"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    after = target_metrics(entries)
    full = {"exact": True, "size": len(rom), "crc32": f"{zlib.crc32(rom) & 0xFFFFFFFF:08X}",
            "sha1": hashlib.sha1(rom).hexdigest(), "sha256": hashlib.sha256(rom).hexdigest()}
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    full.update({"rebuilt_crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                 "rebuilt_sha1": hashlib.sha1(rebuilt).hexdigest(),
                 "rebuilt_sha256": hashlib.sha256(rebuilt).hexdigest()})
    remaining = [{"start": start, "end": end, "size": end - start,
                  "classification": classification, "reason": reason}
                 for start, end, classification, reason in REMAINING]
    gap_census = {"POSSIBLE_CODE_BYTES": sum(item["size"] for item in remaining
                                               if item["classification"] == "POSSIBLE_CODE"),
                  "UNKNOWN_DATA_BYTES": sum(item["size"] for item in remaining
                                             if item["classification"] == "UNKNOWN_DATA"),
                  "UNRESOLVED_BOUNDARY_BYTES": sum(item["size"] for item in remaining
                                                    if item["classification"] == "UNRESOLVED_BOUNDARY")}
    report = {
        "schema": "oasis.m68k.m12-4-asm-promotion.v1",
        "classification": "M12_4_ROM_START_PROMOTION_PARTIAL_EXACT",
        "baseline_commit": BASELINE_COMMIT,
        "target": {"start": TARGET[0], "end": TARGET[1], "size": TARGET[1] - TARGET[0]},
        "target_before": {"header_vector_asm": 0, "asm": 0, "structured_data_asm": 0,
                           "padding": 0, "blob": TARGET[1] - TARGET[0]},
        "target_after": after,
        "target_gap_census": gap_census,
        "executable_blob_bytes_removed": after["asm"],
        "nonexecutable_blob_bytes_removed": after["header_vector_asm"] + after["structured_data_asm"] + after["padding"],
        "vector": vector_evidence(rom),
        "header": {"start": HEADER[0], "end": HEADER[1], "size": HEADER[1] - HEADER[0],
                    "signature": rom[0x100:0x104].decode("ascii"), "byte_exact": True,
                    "field_semantics": "bounded fixed-format ownership; no semantic normalization"},
        "structured_data": {"start": STRUCTURED[0], "end": STRUCTURED[1], "size": STRUCTURED[1] - STRUCTURED[0],
                             "classification": "STRUCTURED_DATA_CONFIRMED",
                             "reason": "PC-relative startup table consumed by exact decoded loads"},
        "promoted_code_ranges": attempts, "remaining_target_intervals": remaining,
        "whole_rom_before": whole_metrics(before, len(rom)),
        "whole_rom_after": whole_metrics(entries, len(rom)),
        "quality": {"gaps": 0, "overlaps": 0}, "full_rom": full,
        "next_m12_5_queue": {"recomputed": True, "proposed_m12_5": next_m12_5()},
        "no_m12_5_started": True, "no_cpp_migration_started": True,
        "no_emulator_expansion_started": True,
    }
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"classification": report["classification"], "target_after": after,
                      "whole_rom_after": report["whole_rom_after"],
                      "next_m12_5": report["next_m12_5_queue"]["proposed_m12_5"]}, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("assembler", "rom", "manifest", "range-tool", "output"):
        parser.add_argument("--" + name, required=True)
    run_promotion(parser.parse_args())


if __name__ == "__main__":
    main()
