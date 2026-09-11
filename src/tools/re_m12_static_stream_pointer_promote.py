"""Promote graphics streams with exact static ROM-pointer provenance."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import zlib


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("re_auto_promote", ROOT / "re_auto_promote.py")
AUTO = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUTO)

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
STREAMS = (
    (0x176340, 0x1768C5, 3392, (0x03B99C,), "RESOURCE_TABLE"),
    (0x1794CA, 0x17A74F, 7296, (0x03B9C0,), "RESOURCE_TABLE"),
    (0x17C700, 0x17D3F0, 5696, (0x03B9D6,), "RESOURCE_TABLE"),
    (0x17E610, 0x180C56, 16064, (0x03B9EC,), "RESOURCE_TABLE"),
    (0x1698FE, 0x169D29, 16390, (0x008A1E,), "DIRECT_LOADER"),
    (0x196300, 0x196B87, 6406, (0x005248, 0x007A5E), "DIRECT_LOADER"),
    (0x0541FE, 0x0551B5, 8902, (0x006046,), "DIRECT_LOADER"),
    (0x163EB8, 0x16406F, 736, (0x0467B8,), "REPEATED_POINTER"),
    (0x165434, 0x165834, 1760, (0x04441A,), "REPEATED_POINTER"),
)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end, classification, reason):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            replacement.append({
                "start": start, "end": end,
                "kind": "LOCAL_ROM_DERIVED_ASSET",
                "source": "M12_AUTO51_static_stream_pointer",
                "confidence": "CONFIRMED", "classification": classification,
                "emitted_artifact_type": "rom_asset", "ownership_reason": reason,
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"range overlaps non-UNKNOWN 0x{entry['start']:06X}.."
                             f"0x{entry['end']:06X}")
    raise ValueError(f"range is not wholly UNKNOWN 0x{start:06X}..0x{end:06X}")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "DATA_KNOWN", "HEADER_VECTOR_ASM",
             "STRUCTURED_DATA_CONFIRMED", "PADDING_ALIGNMENT_CONFIRMED",
             "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def parse_contract(rom):
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    records = []
    for start, _, _, pointers, family in STREAMS:
        for pointer_address in pointers:
            if int.from_bytes(rom[pointer_address:pointer_address + 4], "big") != start:
                raise ValueError(f"pointer changed at 0x{pointer_address:06X}")
        records.append({"start": start, "pointer_addresses": pointers, "family": family})
    return {
        "stream_count": len(STREAMS), "streams": records,
        "reason": "static long pointer resolves to stream start and exact graphics parser closes stream end",
    }


def verify_census(path):
    report = json.loads(Path(path).read_text())
    by_start = {int(item["start"]): item for item in report["records"]}
    result = []
    for start, end, output_size, _, _ in STREAMS:
        item = by_start.get(start)
        if item is None or int(item["end"]) != end:
            raise ValueError(f"census boundary mismatch at 0x{start:06X}")
        if int(item["decompressed_bytes"]) != output_size:
            raise ValueError(f"census output mismatch at 0x{start:06X}")
        result.append({"start": start, "end": end,
                       "compressed_bytes": end - start,
                       "decompressed_bytes": output_size})
    return result


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    contract = parse_contract(rom)
    census = verify_census(args.census)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    for start, end, _, pointers, family in STREAMS:
        current = split_unknown(
            current, start, end, "GRAPHICS_COMPRESSED_STREAM",
            f"{family.lower()} pointer(s) at " + ", ".join(f"0x{x:06X}" for x in pointers) +
            "; exact graphics decompressor boundary")
    output.mkdir(parents=True)
    materialized = output / "materialized"
    sources = {index: AUTO.resolve_artifact(baseline_path, entry["artifact"])
               for index, entry in enumerate(current)
               if entry.get("emitted_artifact_type") == "asm" and entry.get("artifact")}
    entries = AUTO.materialize(materialized, current, rom, sources)
    baseline_rebuilt = baseline_path.parent / "rebuilt.rom"
    if baseline_rebuilt.read_bytes() != rom:
        raise ValueError("baseline rebuilt ROM is not canonical")
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO51 static stream pointers"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto51-static-stream-pointers.v1",
              "contract": contract, "census": census, "metrics": manifest["metrics"],
              "promoted_bytes": sum(end - start for start, end, *_ in STREAMS),
              "full_rom": {"size": len(rebuilt),
                           "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                           "sha1": hashlib.sha1(rebuilt).hexdigest(),
                           "sha256": hashlib.sha256(rebuilt).hexdigest()}}
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--census", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
