"""Promote the exact sentinel-terminated threshold table at 0x05D906."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import zlib


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("re_auto_promote", ROOT / "re_auto_promote.py")
AUTO = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUTO)

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
TABLE_START = 0x05D906
TABLE_END = 0x05D918
VALUES = (0x0019, 0x004E, 0x0089, 0x00D2, 0x0138, 0x01DF, 0x034C, 0x0A27, 0xFFFF)


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
                "source": "M12_AUTO31_threshold_table",
                "confidence": "CONFIRMED",
                "classification": "SENTINEL_TERMINATED_THRESHOLD_TABLE",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": "0x010666 compares sequential words and stops at FFFF",
            })
            if TABLE_END < entry["end"]:
                replacement.append({**entry, "start": TABLE_END})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if TABLE_START < entry["end"] and entry["start"] < TABLE_END:
            raise ValueError("threshold table overlaps existing ownership")
    raise ValueError("threshold table is not wholly UNKNOWN")


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
    values = tuple(struct.unpack(">H", rom[address:address + 2])[0]
                   for address in range(TABLE_START, TABLE_END, 2))
    if values != VALUES or values[-1] != 0xFFFF:
        raise ValueError("threshold table bytes or sentinel changed")
    return {"start": TABLE_START, "end": TABLE_END, "bytes": TABLE_END - TABLE_START,
            "entry_count": len(values), "sentinel": 0xFFFF, "values": values,
            "consumer_address": 0x010666,
            "reason": "CMP.W (A0)+,D1; BLS return; ADDQ.W #8,D0; loop"}


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
    manifest["transaction"] = "M12-AUTO31 exact threshold table"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto31-threshold-table-promotion.v1",
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
