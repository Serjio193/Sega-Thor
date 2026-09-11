"""Promote the bounded eight-record table selected at ROM 0x03B8DE."""
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
TABLE_START = 0x03B8DE
TABLE_END = 0x03B95E
RECORD_WIDTH = 0x10
RECORD_COUNT = 8
CONSUMERS = {
    0x03A91C: bytes.fromhex("0C79000700FFAFAE"),
    0x03A9E2: bytes.fromhex("0C79000800FFAFCC"),
    0x03A9EE: bytes.fromhex("41FA0EEE303900FFAFAEE940"),
    0x03AA18: bytes.fromhex("02400007"),
}


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
                "start": start,
                "end": end,
                "kind": "STRUCTURED_DATA_CONFIRMED",
                "source": "M12_AUTO55_table_03B8DE",
                "confidence": "CONFIRMED",
                "classification": "GRAPHICS_RESOURCE_DESCRIPTOR_TABLE",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "eight fixed 16-byte records selected by the exact 0..7 "
                    "selector and 0x10-byte PC-relative stride"
                ),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"range overlaps non-UNKNOWN 0x{start:06X}..0x{end:06X}")
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
    for address, expected in CONSUMERS.items():
        if rom[address:address + len(expected)] != expected:
            raise ValueError(f"selector consumer contract changed at 0x{address:06X}")
    if TABLE_END - TABLE_START != RECORD_WIDTH * RECORD_COUNT:
        raise ValueError("table geometry is inconsistent")
    records = []
    for index in range(RECORD_COUNT):
        start = TABLE_START + index * RECORD_WIDTH
        fields = [int.from_bytes(rom[start + offset:start + offset + 4], "big")
                  for offset in range(0, RECORD_WIDTH, 4)]
        records.append({"index": index, "start": start, "fields": fields})
    return {
        "table": {"start": TABLE_START, "end": TABLE_END,
                  "bytes": TABLE_END - TABLE_START,
                  "record_width": RECORD_WIDTH, "record_count": RECORD_COUNT},
        "records": records,
        "consumers": [f"0x{address:06X}" for address in CONSUMERS],
        "reason": "exact PC-relative table base, bounded selector 0..7, and fixed 16-byte stride",
    }


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    contract = parse_contract(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = split_unknown(renumber(baseline["entries"]), TABLE_START + 4, TABLE_END)
    sources = {index: AUTO.resolve_artifact(baseline_path, entry["artifact"])
               for index, entry in enumerate(current)
               if entry.get("emitted_artifact_type") == "asm" and entry.get("artifact")}
    output.mkdir(parents=True)
    materialized = output / "materialized"
    entries = AUTO.materialize(materialized, current, rom, sources)
    baseline_rebuilt = baseline_path.parent / "rebuilt.rom"
    if baseline_rebuilt.read_bytes() != rom:
        raise ValueError("baseline rebuilt ROM is not canonical")
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO55 bounded 0x03B8DE descriptor table"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto55-table-03b8de.v1",
        "contract": contract,
        "metrics": manifest["metrics"],
        "promoted_bytes": TABLE_END - (TABLE_START + 4),
        "full_rom": {"size": len(rebuilt),
                     "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                     "sha1": hashlib.sha1(rebuilt).hexdigest(),
                     "sha256": hashlib.sha256(rebuilt).hexdigest()},
    }
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
