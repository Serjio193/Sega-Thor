"""Promote the exact indexed text/script table consumed by the 68000 parser."""
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
TABLE_START = 0x51514
TABLE_COUNT = 0x62
PARSER_START = 0xC326
CALLER_START = 0xC2EC
SKIPPED_INDICES = (0x1B, 0x1C, 0x1D)
CALLER_BYTES = bytes.fromhex(
    "08 39 00 07 00 FF 0D 7A 67 30 0C B9 10 10 05 00 00 FF 16 5E 66 24 "
    "30 3C 00 00 61 00 00 1E 52 40 0C 40 00 1B 67 F8 0C 40 00 1C 67 F2 "
    "0C 40 00 1D 67 EC 0C 40 00 62 66 E2 4E 75")
PARSER_BYTES = bytes.fromhex(
    "30 39 00 FF 18 70 41 F9 00 05 15 14 D0 40 D0 C0 D0 D0")
ALLOWED_CONTROLS = {1, 2, 3, 5, 6, 7}


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end):
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
                "start": start,
                "end": end,
                "kind": "STRUCTURED_DATA_CONFIRMED",
                "source": "M12_AUTO3_indexed_script_table",
                "confidence": "CONFIRMED",
                "classification": "INDEXED_SCRIPT_STREAM",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "exact 68000 caller enumerates this table index and the exact "
                    "parser resolves a table-relative stream and stops at NUL"
                ),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"range overlaps non-UNKNOWN 0x{entry['start']:06X}.."
                             f"0x{entry['end']:06X}")
    raise ValueError(f"range is not wholly UNKNOWN: 0x{start:06X}..{end:06X}")


def parse_table(rom):
    if rom[CALLER_START:CALLER_START + len(CALLER_BYTES)] != CALLER_BYTES:
        raise ValueError("indexed-table caller contract changed")
    parser_offset = 0xC3F8 - PARSER_START
    if rom[PARSER_START + parser_offset:PARSER_START + parser_offset + len(PARSER_BYTES)] != PARSER_BYTES:
        raise ValueError("indexed-table parser contract changed")
    table_end = TABLE_START + TABLE_COUNT * 2
    if table_end > len(rom):
        raise ValueError("indexed table is outside the ROM")
    records = []
    for index in range(TABLE_COUNT):
        entry = TABLE_START + index * 2
        offset = int.from_bytes(rom[entry:entry + 2], "big", signed=True)
        target = entry + offset
        if target < table_end or target >= len(rom):
            raise ValueError(f"table index {index:02X} target is out of bounds")
        end = target
        controls = []
        while end < len(rom) and rom[end] != 0:
            value = rom[end]
            if value < 0x20:
                if value not in ALLOWED_CONTROLS:
                    raise ValueError(f"index {index:02X} has unknown control {value:02X}")
                controls.append(value)
            end += 1
        if end >= len(rom):
            raise ValueError(f"index {index:02X} has no NUL terminator")
        end += 1
        records.append({"index": index, "table_entry": entry, "offset": offset,
                        "start": target, "end": end, "controls": sorted(set(controls)),
                        "invoked_by_caller": index not in SKIPPED_INDICES})
    if any(left["end"] > right["start"] for left, right in zip(records, records[1:])):
        raise ValueError("indexed streams overlap")
    if any(left["start"] >= right["start"] for left, right in zip(records, records[1:])):
        raise ValueError("indexed stream targets are not monotonic")
    return {"table_start": TABLE_START, "table_end": table_end,
            "parser_start": PARSER_START, "caller_start": CALLER_START,
            "table_count": TABLE_COUNT, "skipped_indices": list(SKIPPED_INDICES),
            "records": records}


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
    current = renumber(baseline["entries"])
    current = split_unknown(current, report["table_start"], report["table_end"])
    for record in report["records"]:
        current = split_unknown(current, record["start"], record["end"])
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
    manifest["transaction"] = "M12-AUTO3 indexed script table and NUL-terminated streams"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report.update({
        "schema": "oasis.m68k.m12-auto3-indexed-script-promotion.v1",
        "metrics": manifest["metrics"],
        "promoted_table_bytes": report["table_end"] - report["table_start"],
        "promoted_stream_bytes": sum(item["end"] - item["start"] for item in report["records"]),
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
