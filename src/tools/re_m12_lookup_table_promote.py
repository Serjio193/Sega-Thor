"""Promote two exact ROM lookup tables with closed 68000 consumers."""
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
WORD_TABLE_START = 0x5D686
WORD_TABLE_END = 0x5D706
BYTE_TABLE_START = WORD_TABLE_END
BYTE_TABLE_END = 0x5D906
WORD_CONSUMER_START = 0x302E
WORD_CONSUMER_BYTES = bytes.fromhex(
    "48 E7 7F E0 34 19 36 1A 42 47 3C 3C 0E 00 61 00 00 26")
WORD_CALL_START = 0xCB2C
WORD_CALL_BYTES = bytes.fromhex(
    "32 3C 00 3F 41 F9 00 FF 13 4C 43 F9 00 FF 18 A6 45 F9 00 05 D6 86 "
    "4E B9 00 00 30 2E")
BYTE_CONSUMER_START = 0x106F2
BYTE_CONSUMER_BYTES = bytes.fromhex(
    "3D 41 00 16 EE 82 EE 83 32 00 06 01 00 40 D0 40 D2 41 "
    "41 F9 00 05 D7 06 10 30 00 00")
BYTE_BOUNDARY_START = 0x30200
BYTE_BOUNDARY_BYTES = bytes.fromhex(
    "30 11 D0 41 02 40 01 FE 32 80 43 F9 00 05 D7 06 "
    "10 31 00 00")


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end, classification, reason):
    if end <= start:
        raise ValueError("promotion range is empty")
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            replacement.append({
                "start": start, "end": end, "kind": "STRUCTURED_DATA_CONFIRMED",
                "source": "M12_AUTO5_lookup_tables", "confidence": "CONFIRMED",
                "classification": classification, "emitted_artifact_type": "rom_asset",
                "ownership_reason": reason,
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"range overlaps non-UNKNOWN 0x{entry['start']:06X}.."
                             f"0x{entry['end']:06X}")
    raise ValueError(f"range is not wholly UNKNOWN: 0x{start:06X}..0x{end:06X}")


def parse_tables(rom, verify_code=True):
    if verify_code:
        checks = ((WORD_CONSUMER_START, WORD_CONSUMER_BYTES),
                  (WORD_CALL_START, WORD_CALL_BYTES),
                  (BYTE_CONSUMER_START, BYTE_CONSUMER_BYTES),
                  (BYTE_BOUNDARY_START, BYTE_BOUNDARY_BYTES))
        for start, expected in checks:
            if rom[start:start + len(expected)] != expected:
                raise ValueError(f"lookup-table consumer contract changed at 0x{start:06X}")
    word_table = rom[WORD_TABLE_START:WORD_TABLE_END]
    if len(word_table) != 0x80 or word_table != b"\x0E\xEE" * 0x40:
        raise ValueError("word lookup table bytes or boundary changed")
    byte_table = rom[BYTE_TABLE_START:BYTE_TABLE_END]
    if len(byte_table) != 0x200:
        raise ValueError("byte lookup table boundary changed")
    if max(byte_table) != 0xFF or min(byte_table) != 0x00:
        raise ValueError("byte lookup table fingerprint changed")
    return {
        "word_table": {"start": WORD_TABLE_START, "end": WORD_TABLE_END,
                       "entries": 0x40, "consumer": WORD_CONSUMER_START,
                       "caller": WORD_CALL_START},
        "byte_table": {"start": BYTE_TABLE_START, "end": BYTE_TABLE_END,
                       "bytes": len(byte_table), "consumer": BYTE_CONSUMER_START},
    }


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def source_map(entries, root):
    return {index: root / entry["artifact"] for index, entry in enumerate(entries)
            if entry.get("emitted_artifact_type") == "asm"}


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    report = parse_tables(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    current = split_unknown(
        current, WORD_TABLE_START, WORD_TABLE_END, "FIXED_WORD_LOOKUP_TABLE",
        "exact bounded 68000 consumer reads one word per loop iteration and the "
        "largest caller count is 0x40 entries")
    current = split_unknown(
        current, BYTE_TABLE_START, BYTE_TABLE_END, "BYTE_LOOKUP_TABLE",
        "exact 68000 indexed byte lookups use the bounded ROM table at 0x5D706")
    output.mkdir(parents=True)
    materialized = output / "materialized"
    entries = AUTO.materialize(materialized, current, rom,
                               source_map(current, baseline_path.parent))
    matched, difference, reason, detail = AUTO.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO5 exact lookup tables"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report.update({
        "schema": "oasis.m68k.m12-auto5-lookup-table-promotion.v1",
        "metrics": manifest["metrics"],
        "promoted_word_table_bytes": WORD_TABLE_END - WORD_TABLE_START,
        "promoted_byte_table_bytes": BYTE_TABLE_END - BYTE_TABLE_START,
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
