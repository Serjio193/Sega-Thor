"""Promote the bounded record table and its count-delimited source streams."""
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
TABLE_GAP = b"\0" * 10
TABLE_BASE = 0x3F2FA
STREAM_POINTER_FIELD = 0
STREAM_HEADER_BYTES = 2
STREAM_RECORD_BYTES = 6
MAX_STREAM_BYTES = 0x20000
CONTAINER_START = 0x58000
CONTAINER_END = 0x5D046
FIELD3_SELECTOR_OFFSETS = (-20, 0, 20, 40)
FIELD3_RECORD_BYTES = 44
MAX_FIELD3_RECORDS = 0x400

# These are exact byte contracts for the table selectors and the shared
# count/DBF consumers. They intentionally stop before unrelated instructions.
CONTRACTS = {
    0x8D06: bytes.fromhex(
        "48 E7 80 80 41 EE 00 02 30 3C 00 5C BD FC 00 FF 29 54"
        "65 04 30 3C 00 2B 30 FC 00 00 51 C8 FF FA 30 2E 00 00 41 F9"),
    0x8DA4: bytes.fromhex(
        "48 E7 80 80 41 F9 00 03 F2 FA 30 2E 00 00 55 40 E9 48"
        "41 F0 00 0C 2D 58 00 1A 2D 58 00 1E 2D 58 00 26"),
    0xAB82: bytes.fromhex(
        "36 2E 00 18 20 6E 00 1A 2A 2E 00 1E E2 8D 2C 45"),
    0xAF02: bytes.fromhex(
        "36 2E 00 18 20 6E 00 1A D0 C0 48 40 3A 10 67 00 00 E8"),
    0xB15E: bytes.fromhex(
        "08 05 00 09 66 00 01 2A 48 E7 D0 0C 9B CD 30 05 B0 6E 00 5C"
        "67 00 00 08 3D 40 00 5C 52 4D 3E 00 02 40 00 7F D0 40 36 39"
        "00 FF 19 46 08 00 00 1B 67 00 00 04 44 43 D2 43 D8 F9 00 FF"
        "19 48 02 47 08 00 48 40 BF 40 48 40 3A 2E 00 06 47 F9 00 03"),
    0xB28E: bytes.fromhex(
        "48 E7 90 04 3A 2E 00 06 41 F9 00 03 F2 FA E9 4D"),
}


def be16(rom, offset):
    return int.from_bytes(rom[offset:offset + 2], "big")


def be32(rom, offset):
    return int.from_bytes(rom[offset:offset + 4], "big")


def be16_signed(rom, offset):
    return int.from_bytes(rom[offset:offset + 2], "big", signed=True)


def verify_contracts(rom):
    if rom[TABLE_END:TABLE_END + len(TABLE_GAP)] != TABLE_GAP:
        raise ValueError("record table terminator gap changed")
    for address, expected in CONTRACTS.items():
        if rom[address:address + len(expected)] != expected:
            raise ValueError(f"record consumer contract changed at 0x{address:06X}")


def parse_table(rom):
    if len(rom) < TABLE_END:
        raise ValueError("record table is outside the ROM")
    verify_contracts(rom)
    streams = {}
    records = []
    for index in range(TABLE_COUNT):
        address = TABLE_START + index * TABLE_STRIDE
        fields = [be32(rom, address + 4 * field) for field in range(8)]
        pointer = fields[STREAM_POINTER_FIELD]
        stream = None
        if pointer:
            if pointer >= len(rom) - STREAM_HEADER_BYTES:
                raise ValueError(f"record {index} stream pointer is outside ROM")
            count = be16(rom, pointer)
            size = STREAM_HEADER_BYTES + STREAM_RECORD_BYTES * (count + 1)
            end = pointer + size
            if size > MAX_STREAM_BYTES or end > len(rom):
                raise ValueError(f"record {index} stream boundary is invalid")
            stream = streams.setdefault(pointer, {
                "start": pointer, "end": end, "count": count,
                "record_bytes": STREAM_RECORD_BYTES})
            if stream["end"] != end or stream["count"] != count:
                raise ValueError("duplicate stream pointer has inconsistent bounds")
        records.append({"index": index, "address": address,
                        "fields": fields, "stream": stream})
    ranges = []
    for stream in sorted(streams.values(), key=lambda item: item["start"]):
        if not ranges or stream["start"] > ranges[-1]["end"]:
            ranges.append({"start": stream["start"], "end": stream["end"]})
        else:
            ranges[-1]["end"] = max(ranges[-1]["end"], stream["end"])
    return {"table_start": TABLE_START, "table_end": TABLE_END,
            "table_stride": TABLE_STRIDE, "table_count": TABLE_COUNT,
            "selector_base": TABLE_BASE, "records": records,
            "streams": [streams[key] for key in sorted(streams)],
            "stream_ranges": ranges}


def parse_field3_lists(rom, records, container_start=CONTAINER_START,
                       container_end=CONTAINER_END):
    """Return only BCEA-selected, sentinel-terminated field3 list intervals."""
    bases = sorted({record["fields"][3] for record in records
                    if container_start <= record["fields"][3] < container_end})
    lists = []
    for base in bases:
        for selector_offset in FIELD3_SELECTOR_OFFSETS:
            slot = base + selector_offset
            if slot < container_start or slot + 2 > container_end:
                continue
            target = slot + be16_signed(rom, slot)
            if target < container_start or target >= container_end or target & 1:
                continue
            position = target
            for row_count in range(MAX_FIELD3_RECORDS + 1):
                if position + 2 > container_end:
                    break
                key = be16_signed(rom, position)
                if key < 0:
                    lists.append({
                        "base": base,
                        "selector_offset": selector_offset,
                        "slot": slot,
                        "target": target,
                        "rows": row_count,
                        "start": target,
                        "end": position + 2,
                    })
                    break
                if position + FIELD3_RECORD_BYTES > container_end:
                    break
                position += FIELD3_RECORD_BYTES
    ranges = []
    for item in sorted(lists, key=lambda value: (value["start"], value["end"])):
        if not ranges or item["start"] > ranges[-1]["end"]:
            ranges.append({"start": item["start"], "end": item["end"]})
        else:
            ranges[-1]["end"] = max(ranges[-1]["end"], item["end"])
    return {"bases": bases, "lists": lists, "ranges": ranges}


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end, classification, reason,
                  source="M12_AUTO11_record_stream_family"):
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
                "source": source,
                "confidence": "CONFIRMED",
                "classification": classification,
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": reason,
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"range overlaps non-UNKNOWN 0x{entry['start']:06X}.."
                             f"0x{entry['end']:06X}")
    raise ValueError(f"range is not wholly UNKNOWN: 0x{start:06X}..0x{end:06X}")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    report = parse_table(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    if not args.field3_only:
        current = split_unknown(
            current, report["table_start"], report["table_end"],
            "FIXED_STRIDE_RECORD_TABLE",
            "exact selector consumers use the 0x3F2FA base and the 32-byte "
            "record family is closed by the repeated layout and zero terminator")
        for stream in report["stream_ranges"]:
            current = split_unknown(
                current, stream["start"], stream["end"],
                "COUNT_BOUNDED_SIX_BYTE_RECORD_STREAM",
                "the shared 68000 consumer reads the leading count and advances "
                "exactly six bytes per DBF iteration")
    field3 = parse_field3_lists(rom, report["records"])
    for item in field3["ranges"]:
        current = split_unknown(
            current, item["start"], item["end"],
            "BCEA_FIELD3_SENTINEL_LIST",
            "BCEA applies the exact 0/1/2/3 selector transform, follows the "
            "signed relative pointer, advances 44-byte rows, and stops on a "
            "negative key sentinel",
            "M12_AUTO13_BCEA_field3_family")
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
    manifest["transaction"] = "M12-AUTO13 BCEA field3 sentinel lists"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report.update({
        "schema": "oasis.m68k.m12-auto13-field3-list-promotion.v1",
        "metrics": manifest["metrics"],
        "promoted_table_bytes": 0 if args.field3_only else TABLE_END - TABLE_START,
        "promoted_stream_bytes": 0 if args.field3_only else sum(
            item["end"] - item["start"] for item in report["stream_ranges"]),
        "field3_list_bytes": sum(item["end"] - item["start"]
                                  for item in field3["ranges"]),
        "full_rom": {"size": len(rebuilt),
                     "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                     "sha1": hashlib.sha1(rebuilt).hexdigest(),
                     "sha256": hashlib.sha256(rebuilt).hexdigest()},
    })
    report["field3_lists"] = field3
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--field3-only", action="store_true")
    run(parser.parse_args())


if __name__ == "__main__":
    main()
