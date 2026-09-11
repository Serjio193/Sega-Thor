"""Promote the exact CC-B0 group pointer table."""
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
TABLE_START = 0x4371E
TABLE_COUNT = 0x20
TABLE_STRIDE = 4
TABLE_END = TABLE_START + TABLE_COUNT * TABLE_STRIDE
CONSUMER = 0xCCB0
CONSUMER_CONTRACT = bytes.fromhex("41 F9 00 04 37 1E 34 00 E0 4A D4 42 D4 42 20 70 20 00")


def be32(rom, offset):
    return int.from_bytes(rom[offset:offset + 4], "big")


def parse_table(rom):
    if len(rom) < TABLE_END:
        raise ValueError("CC-B0 table is outside the ROM")
    if rom[CONSUMER:CONSUMER + len(CONSUMER_CONTRACT)] != CONSUMER_CONTRACT:
        raise ValueError("CC-B0 consumer contract changed")
    values = [be32(rom, TABLE_START + index * TABLE_STRIDE)
              for index in range(TABLE_COUNT)]
    if any(value & 1 or value >= len(rom) for value in values):
        raise ValueError("group pointer table contains an invalid ROM target")
    return {"start": TABLE_START, "end": TABLE_END, "count": TABLE_COUNT,
            "stride": TABLE_STRIDE, "consumer": CONSUMER,
            "targets": values}


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
                "start": start, "end": end,
                "kind": "STRUCTURED_DATA_CONFIRMED",
                "source": "M12_AUTO14_CCB0_group_pointer_table",
                "confidence": "CONFIRMED",
                "classification": "GROUP_POINTER_TABLE_32X32",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "the exact CC-B0 consumer shifts its high-byte selector by "
                    "two address bits and reads this closed 32-entry longword "
                    "pointer table; nested target subtables remain unowned"),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError("CC-B0 table overlaps a non-UNKNOWN range")
    raise ValueError("CC-B0 table is not wholly UNKNOWN")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def verify_inherited_baseline(materialized, baseline_root, baseline_rebuilt,
                              rom, entries):
    rebuilt = baseline_rebuilt.read_bytes()
    if len(rebuilt) != len(rom) or hashlib.sha256(rebuilt).digest() != hashlib.sha256(rom).digest():
        raise ValueError("verified baseline rebuilt ROM is not canonical")
    for entry in entries:
        artifact = materialized / entry["artifact"]
        if entry.get("emitted_artifact_type") in ("rom_asset", "blob"):
            if artifact.read_bytes() != rom[entry["start"]:entry["end"]]:
                raise ValueError("materialized ROM asset differs from canonical slice")
        else:
            previous = baseline_root / "code" / f"sub_{entry['start']:06X}.asm"
            if not previous.exists() or artifact.read_bytes() != previous.read_bytes():
                raise ValueError("ASM artifact changed during inherited verification")
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")
    return {"mode": "INHERITED_BASELINE_FULL_ROM",
            "baseline_rebuilt_sha256": hashlib.sha256(rebuilt).hexdigest()}


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
    current = split_unknown(renumber(baseline["entries"]), TABLE_START, TABLE_END)
    output.mkdir(parents=True)
    materialized = output / "materialized"
    entries = AUTO.materialize(materialized, current, rom,
                               AUTO.source_map(current, baseline_path))
    if args.baseline_rebuilt:
        verification = verify_inherited_baseline(
            materialized, baseline_path.parent, Path(args.baseline_rebuilt), rom, entries)
    else:
        matched, difference, reason, detail = AUTO.verify_full(
            materialized, rom, args.assembler, entries)
        if not matched:
            raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
        verification = {"mode": "FRESH_ASSEMBLER_ROUND_TRIP"}
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO14 CC-B0 group pointer table"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report.update({
        "schema": "oasis.m68k.m12-auto14-ccb0-group-table.v1",
        "metrics": manifest["metrics"],
        "promoted_table_bytes": TABLE_END - TABLE_START,
        "verification": verification,
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
    parser.add_argument("--baseline-rebuilt", help="reuse a previously verified canonical rebuilt.rom when vasm is unavailable")
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
