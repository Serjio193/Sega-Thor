"""Promote descriptor-backed graphics streams with exact parser boundaries."""
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
RECORD_SIZE = 0x16
RECORDS = (
    (0x02CBA2, 0x1ED5EC), (0x02CC64, 0x1EF3AE),
    (0x02CDAA, 0x1F10DE), (0x02CE78, 0x1F2C3E),
    (0x02CF82, 0x1F4E64), (0x02D200, 0x1F683C),
    (0x02D2F2, 0x1F8BE4), (0x02D3E8, 0x1FA32A),
    (0x02D762, 0x1FB548), (0x02D7F2, 0x1FCC64),
)
STREAMS = (
    (0x1ED5EC, 0x1EF3AD, 36932), (0x1EF3AE, 0x1F10DE, 36984),
    (0x1F10DE, 0x1F2C3D, 36932), (0x1F2C3E, 0x1F4E63, 37020),
    (0x1F4E64, 0x1F66A0, 30906), (0x1F683C, 0x1F8BE3, 37058),
    (0x1F8BE4, 0x1FA329, 24770), (0x1FA32A, 0x1FB547, 24912),
    (0x1FB548, 0x1FCC64, 37144), (0x1FCC64, 0x1FF10E, 36932),
)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end, kind, source, classification, reason):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            replacement.append({
                "start": start, "end": end, "kind": kind, "source": source,
                "confidence": "CONFIRMED", "classification": classification,
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": reason,
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
    for start, pointer in RECORDS:
        if int.from_bytes(rom[start + 4:start + 8], "big") != pointer:
            raise ValueError(f"descriptor pointer changed at 0x{start:06X}")
        if start + RECORD_SIZE > len(rom):
            raise ValueError("descriptor exceeds ROM")
    return {
        "record_count": len(RECORDS), "record_size": RECORD_SIZE,
        "records": [{"start": start, "end": start + RECORD_SIZE, "pointer": pointer}
                    for start, pointer in RECORDS],
        "stream_count": len(STREAMS),
        "stream_starts": [start for start, _, _ in STREAMS],
        "unproven_adjacent_stream": "0x1F66A0..0x1F683C",
        "reason": "repeated descriptor layout, static pointer field, and exact graphics decompressor boundary",
    }


def verify_census(path):
    report = json.loads(Path(path).read_text())
    by_start = {int(item["start"]): item for item in report["records"]}
    expected = []
    for start, end, output_size in STREAMS:
        item = by_start.get(start)
        if item is None or int(item["end"]) != end:
            raise ValueError(f"census boundary mismatch at 0x{start:06X}")
        if int(item["decompressed_bytes"]) != output_size:
            raise ValueError(f"census output mismatch at 0x{start:06X}")
        expected.append({"start": start, "end": end,
                        "compressed_bytes": end - start, "decompressed_bytes": output_size})
    return expected


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
    source = "M12_AUTO49_descriptor_backed_graphics_streams"
    for start, pointer in RECORDS:
        current = split_unknown(
            current, start, start + RECORD_SIZE, "STRUCTURED_DATA_CONFIRMED", source,
            "GRAPHICS_DESCRIPTOR_RECORD",
            "repeated 22-byte descriptor family with exact +4 static ROM pointer")
    for start, end, _ in STREAMS:
        current = split_unknown(
            current, start, end, "LOCAL_ROM_DERIVED_ASSET", source,
            "GRAPHICS_COMPRESSED_STREAM",
            "descriptor pointer identifies stream; verified decompressor consumes exact end")
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
    manifest["transaction"] = "M12-AUTO49 descriptor-backed graphics streams"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto49-descriptor-streams.v1",
              "contract": contract, "census": census, "metrics": manifest["metrics"],
              "promoted_bytes": len(RECORDS) * RECORD_SIZE +
              sum(end - start for start, end, _ in STREAMS),
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
