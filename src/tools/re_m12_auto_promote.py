"""Materialize exact code islands and verified local-ROM resource streams."""
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

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"


def overlap(left, right):
    return left[0] < right[1] and right[0] < left[1]


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split(entries, start, end, kind, source, reason):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN" or not (entry["start"] <= start <= end <= entry["end"]):
            continue
        replacement = []
        if entry["start"] < start:
            replacement.append({"start": entry["start"], "end": start, "kind": "UNKNOWN",
                                "source": "canonical_local_rom", "confidence": "ROM_HASH_VERIFIED",
                                "emitted_artifact_type": "blob"})
        replacement.append({"start": start, "end": end, "kind": kind, "source": source,
                            "confidence": "CONFIRMED", "classification": kind,
                            "emitted_artifact_type": "asm" if kind == "CODE_VERIFIED" else "rom_asset",
                            "ownership_reason": reason})
        if end < entry["end"]:
            replacement.append({"start": end, "end": entry["end"], "kind": "UNKNOWN",
                                "source": "canonical_local_rom", "confidence": "ROM_HASH_VERIFIED",
                                "emitted_artifact_type": "blob"})
        return renumber(entries[:index] + replacement + entries[index + 1:])
    raise ValueError(f"range 0x{start:06X}..0x{end:06X} is not UNKNOWN")


def validate_baseline(manifest, rom):
    if hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM SHA-256 mismatch")
    entries = sorted(manifest["entries"], key=lambda item: item["start"])
    cursor = 0
    for entry in entries:
        if entry["start"] != cursor or entry["end"] <= entry["start"]:
            raise ValueError("baseline manifest is not contiguous")
        cursor = entry["end"]
    if cursor != len(rom):
        raise ValueError("baseline manifest does not cover ROM")


def selected(code_report, asset_report):
    code = [item for item in code_report if item.get("exact") and int(item.get("called_by", 0)) > 0]
    assets = list(asset_report["records"])
    chosen = []
    for item in sorted(code, key=lambda value: (int(value["start"]), -int(value["end"]))):
        candidate = (int(item["start"]), int(item["end"]))
        conflicts = [old for old in chosen if overlap(candidate, old[0])]
        if conflicts:
            largest = max([candidate] + [old[0] for old in conflicts],
                          key=lambda value: value[1] - value[0])
            if largest != candidate:
                continue
            chosen = [old for old in chosen if old[0] not in [x[0] for x in conflicts]]
        chosen.append((candidate, item))
    code = [item for _, item in sorted(chosen, key=lambda value: value[0])]
    ranges = sorted([(int(item["start"]), int(item["end"])) for item in code] +
                    [(int(item["start"]), int(item["end"])) for item in assets])
    if any(overlap(left, right) for left, right in zip(ranges, ranges[1:])):
        raise ValueError("promotion candidates overlap")
    return sorted(code, key=lambda item: int(item["start"])), sorted(assets, key=lambda item: int(item["start"])), ranges


def source_map(entries, sources):
    return {index: sources[entry["start"]] for index, entry in enumerate(entries)
            if entry.get("emitted_artifact_type") == "asm"}


def metrics(entries, rom_size):
    result = auto.FULL.metrics(entries, rom_size)
    result["SOURCE_OWNED_BYTES"] = sum(entry["size"] for entry in entries
                                       if entry["kind"] in ("CODE_VERIFIED", "HEADER_VECTOR_ASM",
                                                             "STRUCTURED_DATA_CONFIRMED",
                                                             "PADDING_ALIGNMENT_CONFIRMED",
                                                             "LOCAL_ROM_DERIVED_ASSET"))
    result["SOURCE_OWNED_PERCENT"] = 100.0 * result["SOURCE_OWNED_BYTES"] / rom_size
    return result


def run(args):
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    validate_baseline(baseline, rom)
    code_report = json.loads(Path(args.code_results).read_text())
    asset_report = json.loads(Path(args.asset_results).read_text())
    codes, assets, ranges = selected(code_report, asset_report)
    current = renumber(copy.deepcopy(baseline["entries"]))
    sources = {entry["start"]: baseline_path.parent / entry["artifact"]
               for entry in baseline["entries"]
               if entry.get("emitted_artifact_type") == "asm"}
    output.mkdir(parents=True)
    staging = output / "staging"
    staging.mkdir()
    for item in codes:
        start, end = int(item["start"]), int(item["end"])
        asm = staging / f"code_{start:06X}.asm"
        data = staging / f"code_{start:06X}.json"
        binary = staging / f"code_{start:06X}.bin"
        emitted = auto.run([args.range_tool, rom_path, hex(start), hex(end), asm, data])
        if emitted.returncode:
            raise ValueError(f"range decode failed at 0x{start:06X}")
        assembled = auto.run([args.assembler, "-m68000", "-no-opt", "-Fbin", "-o", binary, asm])
        if assembled.returncode or binary.read_bytes() != rom[start:end]:
            raise ValueError(f"exactness failed at 0x{start:06X}")
        current = split(current, start, end, "CODE_VERIFIED", "M12_AUTO_exact_static_island",
                        "Ghidra bounded entry with static caller, exact decoder and vasm round-trip")
        sources[start] = asm
    asset_bytes = 0
    for item in assets:
        start, end = int(item["start"]), int(item["end"])
        current = split(current, start, end, "LOCAL_ROM_DERIVED_ASSET",
                        "M12_AUTO_resource_boundary_scan",
                        f"resource table index {item['index']}; decompressor consumed exact stream")
        asset_bytes += end - start
        padding = int(item.get("padding_after_stream", 0))
        if padding:
            current = split(current, end, end + padding, "PADDING_ALIGNMENT_CONFIRMED",
                            "M12_AUTO_resource_boundary_scan",
                            f"one-byte alignment before resource table index {int(item['index']) + 1}")
    materialized = output / "materialized"
    entries = auto.materialize(materialized, current, rom, source_map(current, sources))
    matched, difference, reason, detail = auto.verify_full(materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = auto.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"] = metrics(entries, len(rom))
    manifest["transaction"] = "M12-AUTO exact static islands and compressed resources"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    full = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto-promotion.v1", "codes": len(codes),
              "code_bytes": sum(int(item["size"]) for item in codes), "assets": len(assets),
              "asset_bytes": asset_bytes, "ranges": len(ranges), "metrics": manifest["metrics"],
              "full_rom": {"size": len(full), "crc32": f"{zlib.crc32(full) & 0xFFFFFFFF:08X}",
                           "sha1": hashlib.sha1(full).hexdigest(), "sha256": hashlib.sha256(full).hexdigest()}}
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("assembler", "rom", "manifest", "range-tool", "code-results", "asset-results", "output"):
        parser.add_argument("--" + name, required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
