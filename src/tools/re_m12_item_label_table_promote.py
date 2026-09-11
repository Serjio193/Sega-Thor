"""Promote the exact 64-record item-label table at 0x05CC16."""
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
TABLE_SHA256 = "e3e5ce6519727a44bf7d81452864fa62b0e4f6a8dcd87a41ab9a70a8fc5bad2d"
TABLE_START = 0x05CC16
TABLE_END = 0x05CE16
RECORD_SIZE = 8
RECORD_COUNT = 64
CONSUMERS = (0x0041A6, 0x0041CA)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= TABLE_START and TABLE_END <= entry["end"]:
            replacement = []
            if entry["start"] < TABLE_START:
                replacement.append({**entry, "end": TABLE_START})
            replacement.append({
                "start": TABLE_START, "end": TABLE_END,
                "kind": "STRUCTURED_DATA_CONFIRMED",
                "source": "M12_AUTO33_item_label_table",
                "confidence": "CONFIRMED",
                "classification": "FIXED_WIDTH_ITEM_LABEL_TABLE",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": "two exact consumers use 64 eight-byte records",
            })
            if TABLE_END < entry["end"]:
                replacement.append({**entry, "start": TABLE_END})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if TABLE_START < entry["end"] and entry["start"] < TABLE_END:
            raise ValueError("item label table overlaps existing ownership")
    raise ValueError("item label table is not wholly UNKNOWN")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "DATA_KNOWN", "HEADER_VECTOR_ASM",
             "STRUCTURED_DATA_CONFIRMED", "PADDING_ALIGNMENT_CONFIRMED",
             "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count, "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def parse_contract(rom):
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    actual = rom[TABLE_START:TABLE_END]
    if len(actual) != RECORD_SIZE * RECORD_COUNT or not all(0x20 <= b <= 0x7E for b in actual):
        raise ValueError("item label fixed-record contract failed")
    if hashlib.sha256(actual).hexdigest() != TABLE_SHA256:
        raise ValueError("item label table bytes changed")
    return {"start": TABLE_START, "end": TABLE_END, "bytes": len(actual),
            "record_size": RECORD_SIZE, "record_count": RECORD_COUNT,
            "consumer_addresses": CONSUMERS,
            "reason": "byte index from A4 is doubled, then shifted left three for 8-byte records"}


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    contract = parse_contract(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = split_unknown(renumber(baseline["entries"]))
    output.mkdir(parents=True)
    materialized = output / "materialized"
    entries = AUTO.materialize(materialized, current, rom, AUTO.source_map(current, baseline_path))
    matched, difference, reason, detail = AUTO.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO33 exact item label table"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto33-item-label-table-promotion.v1",
              "contract": contract, "metrics": manifest["metrics"],
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
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
