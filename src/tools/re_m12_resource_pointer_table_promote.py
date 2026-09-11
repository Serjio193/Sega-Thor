"""Promote the exact 108-entry compressed-resource pointer table at 0x05CE96."""
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
TABLE_SHA256 = "bf60ea487678ce60093ab0646031c0f53485f5be85cb38797ba746245f91e321"
TABLE_START = 0x05CE96
TABLE_END = 0x05D046
ENTRY_SIZE = 4
ENTRY_COUNT = 108
CONSUMER = 0x00D3B2
STREAM_FIRST = 0x1AD000
STREAM_LAST = 0x1E6EDA


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
                "source": "M12_AUTO34_resource_pointer_table",
                "confidence": "CONFIRMED",
                "classification": "COMPRESSED_RESOURCE_POINTER_TABLE",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": "0x00D3B2 reads 108 absolute pointers; next table starts at 0x05D046",
            })
            if TABLE_END < entry["end"]:
                replacement.append({**entry, "start": TABLE_END})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if TABLE_START < entry["end"] and entry["start"] < TABLE_END:
            raise ValueError("resource pointer table overlaps existing ownership")
    raise ValueError("resource pointer table is not wholly UNKNOWN")


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
    if len(actual) != ENTRY_SIZE * ENTRY_COUNT or hashlib.sha256(actual).hexdigest() != TABLE_SHA256:
        raise ValueError("resource pointer table bytes changed")
    values = tuple(struct.unpack(">I", actual[offset:offset + 4])[0]
                   for offset in range(0, len(actual), 4))
    if values[0] != 0 or values[1] != STREAM_FIRST or values[-1] != STREAM_LAST:
        raise ValueError("resource pointer endpoints changed")
    if any(left >= right for left, right in zip(values[1:], values[2:])):
        raise ValueError("resource pointer starts are not strictly increasing")
    return {"start": TABLE_START, "end": TABLE_END, "bytes": len(actual),
            "entry_size": ENTRY_SIZE, "entry_count": ENTRY_COUNT,
            "consumer_address": CONSUMER, "entry_zero": values[0],
            "first_stream": values[1], "last_stream": values[-1],
            "reason": "D0 << 2; A0 = 0x05CE96; MOVEA.L (A0,D0.W),A0; JSR 0x3820"}


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
    manifest["transaction"] = "M12-AUTO34 exact resource pointer table"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto34-resource-pointer-table-promotion.v1",
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
