"""Promote one exact fixed-stride ROM table with bounded 68000 consumers."""
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
TABLE_START = 0x5D046
TABLE_RECORD_SIZE = 0x20
TABLE_COUNT = 0x32
TABLE_END = TABLE_START + TABLE_RECORD_SIZE * TABLE_COUNT
NEXT_TABLE_BYTES = b"\x0E\xEE" * 0x40
CONSUMERS = (
    (0x00D72C, bytes.fromhex(
        "41 F9 00 05 D0 46 30 39 00 FF 16 F8 EB 48 D0 C0 7C 07 "
        "22 D8 51 CE FF FC")),
    (0x010086, bytes.fromhex(
        "54 88 30 18 33 C0 00 FF 16 F8 43 F9 00 05 D0 46 45 F9 "
        "00 FF 13 6C EB 48 D2 C0 72 07 24 D9 51 C9 FF FC")),
    (0x0100AA, bytes.fromhex(
        "54 88 30 18 33 C0 00 FF 16 F8 43 F9 00 05 D0 46 45 F9 "
        "00 FF 13 6C EB 48 D2 C0 72 07 24 D9 51 C9 FF FC")),
    (0x039100, bytes.fromhex(
        "41 F9 00 05 D0 46 30 3C 00 08 4E B9 00 00 E2 A2 50 40 "
        "EB 48 D0 C0 7C 07 43 F9 00 FF 13 6C 22 D8 51 CE FF FC")),
)


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
                "source": "M12_AUTO6_fixed_stride_table",
                "confidence": "CONFIRMED",
                "classification": "FIXED_STRIDE_LOOKUP_TABLE",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "four exact 68000 consumers use the same ROM base, select "
                    "32-byte records with index << 5, and copy eight longwords; "
                    "the bounded table ends at the next independently confirmed "
                    "lookup table"),
            })
            if TABLE_END < entry["end"]:
                replacement.append({**entry, "start": TABLE_END})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if TABLE_START < entry["end"] and entry["start"] < TABLE_END:
            raise ValueError("fixed-stride table overlaps a non-UNKNOWN range")
    raise ValueError("fixed-stride table is not wholly UNKNOWN")


def parse_table(rom, verify_code=True):
    if len(rom) < TABLE_END or TABLE_END + len(NEXT_TABLE_BYTES) > len(rom):
        raise ValueError("fixed-stride table is outside the ROM")
    if verify_code:
        for start, expected in CONSUMERS:
            if rom[start:start + len(expected)] != expected:
                raise ValueError(f"fixed-stride consumer contract changed at 0x{start:06X}")
    table = rom[TABLE_START:TABLE_END]
    if len(table) != TABLE_COUNT * TABLE_RECORD_SIZE:
        raise ValueError("fixed-stride table geometry changed")
    if rom[TABLE_END:TABLE_END + len(NEXT_TABLE_BYTES)] != NEXT_TABLE_BYTES:
        raise ValueError("next lookup-table boundary fingerprint changed")
    return {"start": TABLE_START, "end": TABLE_END,
            "record_size": TABLE_RECORD_SIZE, "records": TABLE_COUNT,
            "consumers": [start for start, _ in CONSUMERS]}


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
    report = parse_table(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = split_unknown(renumber(baseline["entries"]))
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
    manifest["transaction"] = "M12-AUTO6 fixed-stride table"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report.update({
        "schema": "oasis.m68k.m12-auto6-fixed-stride-table.v1",
        "metrics": manifest["metrics"],
        "promoted_table_bytes": TABLE_END - TABLE_START,
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
