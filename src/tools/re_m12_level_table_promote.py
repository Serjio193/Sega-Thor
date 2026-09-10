"""Promote the exact nested pointer-record table and its bounded records."""
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
TABLE_START = 0x5D918
TABLE_COUNT = 0x20
TABLE_END = TABLE_START + TABLE_COUNT * 2
RECORD_AREA_START = 0x5DB44
RECORD_AREA_END = 0x5E1A0
OUTER_LOOKUP_START = 0x4D58
OUTER_LOOKUP_BYTES = bytes.fromhex(
    "41 F9 00 05 D9 18 30 39 00 FF 18 68 D0 40 D0 C0 D0 D0")
INNER_LOOKUP_START = 0x4E2A
INNER_LOOKUP_BYTES = bytes.fromhex(
    "41 F9 00 05 D9 18 30 39 00 FF 18 68 D0 40 D0 C0 D0 D0 "
    "54 88 30 39 00 FF 18 66 D0 40 D0 C0 D0 D0 30 39 00 FF")


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
                "start": start, "end": end, "kind": "STRUCTURED_DATA_CONFIRMED",
                "source": "M12_AUTO4_nested_level_table",
                "confidence": "CONFIRMED",
                "classification": "NESTED_POINTER_RECORD_TABLE",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "exact 68000 table lookups select this bounded pointer-record "
                    "family and each record has an exact NUL terminator"
                ),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"range overlaps non-UNKNOWN 0x{entry['start']:06X}.."
                             f"0x{entry['end']:06X}")
    raise ValueError(f"range is not wholly UNKNOWN: 0x{start:06X}..{end:06X}")


def parse_level_table(rom, verify_code=True):
    if verify_code:
        if rom[OUTER_LOOKUP_START:OUTER_LOOKUP_START + len(OUTER_LOOKUP_BYTES)] != OUTER_LOOKUP_BYTES:
            raise ValueError("outer level-table lookup contract changed")
        if rom[INNER_LOOKUP_START:INNER_LOOKUP_START + len(INNER_LOOKUP_BYTES)] != INNER_LOOKUP_BYTES:
            raise ValueError("inner level-table lookup contract changed")
    if TABLE_END > len(rom):
        raise ValueError("level table is outside the ROM")
    groups = []
    for index in range(TABLE_COUNT):
        entry = TABLE_START + index * 2
        offset = int.from_bytes(rom[entry:entry + 2], "big", signed=True)
        if offset == 0:
            continue
        start = entry + offset
        if start < TABLE_END or start >= len(rom):
            raise ValueError(f"group {index:02X} target is out of bounds")
        count = int.from_bytes(rom[start:start + 2], "big")
        end = start + 2 * (count + 2)
        if end > len(rom):
            raise ValueError(f"group {index:02X} extends beyond the ROM")
        groups.append({"index": index, "table_entry": entry, "start": start,
                       "count": count, "end": end})
    if any(a["start"] >= b["start"] for a, b in zip(groups, groups[1:])):
        raise ValueError("non-empty groups are not monotonic")
    if groups[0]["start"] != TABLE_END or groups[-1]["end"] != RECORD_AREA_START:
        raise ValueError("nested table boundary contract changed")
    for left, right in zip(groups, groups[1:]):
        if left["end"] != right["start"]:
            raise ValueError("nested group tables have a gap or overlap")
    pointers = []
    for group in groups:
        for entry in range(group["start"] + 2, group["end"], 2):
            offset = int.from_bytes(rom[entry:entry + 2], "big", signed=True)
            if offset == 0:
                continue
            target = entry + offset
            if target < RECORD_AREA_START or target >= RECORD_AREA_END:
                raise ValueError(f"inner pointer at {entry:06X} is out of bounds")
            end = rom.find(b"\0", target, RECORD_AREA_END)
            if end < 0:
                raise ValueError(f"record at {target:06X} has no NUL terminator")
            body = rom[target + 2:end]
            if not body or any(value < 0x20 for value in body):
                raise ValueError(f"record at {target:06X} is not a bounded label record")
            pointers.append({"group": group["index"], "entry": entry,
                             "target": target, "end": end + 1})
    records = []
    for target in sorted({item["target"] for item in pointers}):
        end = next(item["end"] for item in pointers if item["target"] == target)
        records.append({"start": target, "end": end})
    if any(a["end"] > b["start"] for a, b in zip(records, records[1:])):
        raise ValueError("nested level records overlap")
    if records[-1]["end"] != RECORD_AREA_END:
        raise ValueError("nested level record area does not end at FF filler")
    return {"table_start": TABLE_START, "table_end": TABLE_END,
            "record_area_start": RECORD_AREA_START, "record_area_end": RECORD_AREA_END,
            "groups": groups, "pointers": pointers, "records": records}


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
    report = parse_level_table(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    current = split_unknown(current, report["table_start"], report["table_end"])
    current = split_unknown(current, report["groups"][0]["start"], report["record_area_start"])
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
    manifest["transaction"] = "M12-AUTO4 nested level label table and records"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report.update({
        "schema": "oasis.m68k.m12-auto4-level-table-promotion.v1",
        "metrics": manifest["metrics"],
        "promoted_table_bytes": report["table_end"] - report["table_start"],
        "promoted_group_bytes": report["record_area_start"] - report["groups"][0]["start"],
        "promoted_record_bytes": sum(item["end"] - item["start"] for item in report["records"]),
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
