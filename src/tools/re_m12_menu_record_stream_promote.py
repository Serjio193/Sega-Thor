"""Promote the exact menu offset table and count-bounded record streams."""
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
TABLE_START = 0x15B9D4
TABLE_END = 0x15B9DC
TABLE_OFFSETS = (0x0008, 0x00D4, 0x00DA, 0x00E0)
STREAM_HEADER_BYTES = 2
STREAM_RECORD_BYTES = 6
EXPECTED_STREAM_COUNTS = (0x21, 0, 0, 0)
RANGE_END = 0x15BAC2
RANGE_SHA256 = "32c1717073b8538ad8c032a51b62b2e0d08a9d133b5f57d2287f7e145a85d942"
CONSUMER_CONTRACTS = {
    0x004AF2: bytes.fromhex("41 F9 00 15 B9 D6 61 00 00 48"),
    0x004B08: bytes.fromhex("41 F9 00 15 B9 D8 D0 C1 61 00 01 16"),
    0x004B18: bytes.fromhex(
        "41 F9 00 15 B9 D4 32 3C 00 A0 38 3C 00 78 61 00 00 1A"),
    0x004B42: bytes.fromhex("D0 D0 36 3C 84 A8 61 00 6B E6 4E 75"),
    0x00B730: bytes.fromhex(
        "48 E7 07 80 06 41 00 80 06 44 00 80 3A 18 6B 00 00 54"
        "0C 42 00 50 64 00 00 4C 3C 10 3E"),
}


def be16(rom, offset):
    return int.from_bytes(rom[offset:offset + 2], "big")


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries):
    start = TABLE_START
    end = RANGE_END
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            replacement.append({
                "start": start, "end": end,
                "kind": "STRUCTURED_DATA_CONFIRMED",
                "source": "M12_AUTO38_menu_record_streams",
                "confidence": "CONFIRMED",
                "classification": "MENU_OFFSET_TABLE_AND_COUNT_BOUNDED_RECORD_STREAMS",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "three exact LEA/ADDA consumers select four streams; the shared "
                    "0x00B730 parser reads a count and advances six-byte records"),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError("menu record range overlaps existing ownership")
    raise ValueError("menu record range is not wholly UNKNOWN")


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
    for address, expected in CONSUMER_CONTRACTS.items():
        if rom[address:address + len(expected)] != expected:
            raise ValueError(f"menu record consumer changed at 0x{address:06X}")
    if tuple(be16(rom, TABLE_START + index * 2) for index in range(4)) != TABLE_OFFSETS:
        raise ValueError("menu offset table changed")
    streams = []
    for index, relative in enumerate(TABLE_OFFSETS):
        field = TABLE_START + index * 2
        start = field + relative
        count = be16(rom, start)
        if count != EXPECTED_STREAM_COUNTS[index]:
            raise ValueError(f"stream {index} count changed")
        end = start + STREAM_HEADER_BYTES + STREAM_RECORD_BYTES * (count + 1)
        streams.append({"index": index, "offset_field": field, "relative": relative,
                        "start": start, "end": end, "count": count,
                        "record_bytes": STREAM_RECORD_BYTES})
    if streams[0]["start"] != TABLE_END or streams[-1]["end"] != RANGE_END:
        raise ValueError("menu record stream boundary changed")
    if any(left["end"] != right["start"] for left, right in zip(streams, streams[1:])):
        raise ValueError("menu record streams are no longer contiguous")
    if hashlib.sha256(rom[TABLE_START:RANGE_END]).hexdigest() != RANGE_SHA256:
        raise ValueError("menu record bytes changed")
    return {"table_start": TABLE_START, "table_end": TABLE_END,
            "table_offsets": list(TABLE_OFFSETS), "streams": streams,
            "range_end": RANGE_END, "record_bytes": STREAM_RECORD_BYTES}


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    report = parse_contract(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = split_unknown(renumber(baseline["entries"]))
    output.mkdir(parents=True)
    materialized = output / "materialized"
    entries = AUTO.materialize(materialized, current, rom,
                               AUTO.source_map(current, baseline_path))
    matched, difference, reason, detail = AUTO.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO38 menu offset table and record streams"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report.update({
        "schema": "oasis.m68k.m12-auto38-menu-record-stream-promotion.v1",
        "metrics": manifest["metrics"],
        "promoted_bytes": RANGE_END - TABLE_START,
        "full_rom": {"size": len(rebuilt),
                     "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                     "sha1": hashlib.sha1(rebuilt).hexdigest(),
                     "sha256": hashlib.sha256(rebuilt).hexdigest()},
    })
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
