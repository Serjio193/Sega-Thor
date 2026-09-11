"""Promote graphics streams selected by the exact level resource table."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import zlib


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("re_auto_promote", ROOT / "re_auto_promote.py")
AUTO = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUTO)

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
TABLE_START = 0x3F306
TABLE_STRIDE = 0x20
TABLE_COUNT = 99
TABLE_END = TABLE_START + TABLE_STRIDE * TABLE_COUNT
FIELD1_OFFSET = 4

# These are the exact decoder boundaries recorded by the independent graphics
# census.  Only entries wholly UNKNOWN in the input map are promoted here.
STREAMS = (
    (0x2DC334, 0x2DC7F0, 15, 3072),
    (0x2B15DA, 0x2B45D8, 17, 18720),
    (0x29EE9A, 0x2A0922, 23, 10688),
    (0x1650DC, 0x165433, 33, 1760),
    (0x1658DC, 0x165E12, 34, 2272),
    (0x165E72, 0x16610C, 35, 1536),
    (0x1665A0, 0x166C3D, 37, 2752),
    (0x166CFC, 0x1670C0, 38, 1536),
    (0x16722E, 0x167707, 39, 2048),
    (0x167728, 0x16779D, 40, 256),
    (0x28D158, 0x28EA71, 43, 12448),
    (0x16378E, 0x1638CC, 44, 736),
    (0x1638CC, 0x163A69, 45, 736),
    (0x163A6A, 0x163BDA, 46, 736),
    (0x163BDA, 0x163D42, 47, 736),
    (0x163D42, 0x163EB7, 48, 736),
    (0x164070, 0x1641D0, 49, 736),
    (0x164270, 0x16460C, 50, 1920),
    (0x29D148, 0x29E2D8, 53, 7584),
    (0x26DCDE, 0x26E55C, 54, 5024),
    (0x26ECD0, 0x270584, 57, 11200),
    (0x16785A, 0x167A6D, 60, 768),
    (0x164896, 0x164D10, 61, 2720),
    (0x2DF6A4, 0x2E0134, 63, 4128),
    (0x27AA2A, 0x27B32A, 65, 4480),
    (0x2DD302, 0x2DF25F, 67, 14368),
    (0x27C206, 0x27E112, 68, 13920),
    (0x28003E, 0x28161F, 69, 13088),
    (0x161364, 0x1621B9, 73, 7776),
    (0x162470, 0x163716, 74, 8384),
    (0x2E1870, 0x2E2FE6, 77, 14784),
    (0x2E3AB6, 0x2E5FCF, 79, 16832),
    (0x2EF4F4, 0x2F125E, 89, 15328),
    (0x168D34, 0x16943C, 94, 3584),
    (0x2F6986, 0x2F784F, 95, 5984),
)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end):
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
                "source": "M12_AUTO20_level_resource_table_graphics",
                "confidence": "CONFIRMED",
                "classification": "TABLE_SELECTED_GRAPHICS_STREAM",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "exact 99-row table field1 pointer, exact graphics decoder census "
                    "boundary, and no overlap with existing ownership"
                ),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"stream overlaps non-UNKNOWN 0x{entry['start']:06X}.."
                             f"0x{entry['end']:06X}")
    raise ValueError(f"stream is not wholly UNKNOWN 0x{start:06X}..0x{end:06X}")


def parse_census(paths):
    result = {}
    for path in paths:
        data = json.loads(Path(path).read_text())
        rows = data.get("records", data.get("streams", []))
        for row in rows:
            key = (int(row["start"]), int(row["end"]))
            previous = result.get(key)
            if previous and previous.get("decompressed_bytes") != row.get("decompressed_bytes"):
                raise ValueError(f"conflicting census output size at 0x{key[0]:06X}")
            result[key] = row
    return result


def validate(rom, census):
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    if rom[TABLE_END:TABLE_END + 10] != b"\0" * 10:
        raise ValueError("level resource table terminator contract changed")
    table_fields = []
    for index in range(TABLE_COUNT):
        address = TABLE_START + index * TABLE_STRIDE + FIELD1_OFFSET
        table_fields.append(int.from_bytes(rom[address:address + 4], "big"))
    result = []
    for start, end, row, decompressed_bytes in STREAMS:
        if row >= TABLE_COUNT or table_fields[row] != start:
            raise ValueError(f"field1 pointer contract changed for table row {row}")
        census_row = census.get((start, end))
        if census_row is None:
            raise ValueError(f"census boundary missing for 0x{start:06X}")
        if census_row.get("compressed_bytes", end - start) != end - start:
            raise ValueError(f"census byte count changed at 0x{start:06X}")
        if census_row.get("decompressed_bytes") != decompressed_bytes:
            raise ValueError(f"census output size changed at 0x{start:06X}")
        result.append({"start": start, "end": end, "table_row": row,
                      "decompressed_bytes": decompressed_bytes})
    return result


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET", "DATA_KNOWN"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count, "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    census = parse_census(args.census)
    streams = validate(rom, census)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    for stream in streams:
        current = split_unknown(current, stream["start"], stream["end"])
    output.mkdir(parents=True)
    materialized = output / "materialized"
    sources = {index: baseline_path.parent / entry["artifact"]
               for index, entry in enumerate(current)
               if entry.get("emitted_artifact_type") == "asm"}
    entries = AUTO.materialize(materialized, current, rom, sources)
    matched, difference, reason, detail = AUTO.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO20 table-selected graphics streams"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto20-table-graphics.v1", "streams": streams,
              "metrics": manifest["metrics"],
              "promoted_stream_bytes": sum(item["end"] - item["start"] for item in streams),
              "full_rom": {"size": len(rebuilt), "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                           "sha1": hashlib.sha1(rebuilt).hexdigest(),
                           "sha256": hashlib.sha256(rebuilt).hexdigest()}}
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--census", action="append", required=True)
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
