"""Promote the exact five-entry relative selector table at 0x15A9A6."""
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
TABLE_START = 0x15A9A6
TABLE_END = 0x15A9B0
SELECTOR_OFFSETS = (0, 2, 4, 6, 8)
TABLE_VALUES = (0x000A, 0x001C, 0x0034, 0x0052, 0x0076)
TARGETS = (0x15A9B0, 0x15A9C4, 0x15A9DE, 0x15A9FE, 0x15AA24)
TABLE_SHA256 = "b40ff9c28102d01b6e8c555fe1d6ab133e69f16e075436d0bb8ebcdf317cb1d8"
CONSUMER = 0x003EFA
CONSUMER_BYTES = bytes.fromhex(
    "41 F9 00 15 A9 A6 2A 3C 00 15 AA 32 E2 8D")


def be16(rom, offset):
    return int.from_bytes(rom[offset:offset + 2], "big")


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
                "source": "M12_AUTO39_relative_selector_table",
                "confidence": "CONFIRMED",
                "classification": "RELATIVE_SELECTOR_OFFSET_TABLE_5X16",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "0x003EFA selects only five even byte offsets and applies "
                    "each table word as a relative A0 offset"),
            })
            if TABLE_END < entry["end"]:
                replacement.append({**entry, "start": TABLE_END})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if TABLE_START < entry["end"] and entry["start"] < TABLE_END:
            raise ValueError("relative selector table overlaps existing ownership")
    raise ValueError("relative selector table is not wholly UNKNOWN")


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
    if rom[CONSUMER:CONSUMER + len(CONSUMER_BYTES)] != CONSUMER_BYTES:
        raise ValueError("relative selector consumer changed")
    values = tuple(be16(rom, TABLE_START + offset) for offset in SELECTOR_OFFSETS)
    if values != TABLE_VALUES:
        raise ValueError("relative selector table bytes changed")
    targets = tuple(TABLE_START + offset + value
                    for offset, value in zip(SELECTOR_OFFSETS, values))
    if targets != TARGETS:
        raise ValueError("relative selector target contract changed")
    if hashlib.sha256(rom[TABLE_START:TABLE_END]).hexdigest() != TABLE_SHA256:
        raise ValueError("relative selector table hash changed")
    return {"start": TABLE_START, "end": TABLE_END,
            "bytes": TABLE_END - TABLE_START, "consumer": CONSUMER,
            "selector_offsets": list(SELECTOR_OFFSETS),
            "values": list(values), "targets": list(targets)}


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
    manifest["transaction"] = "M12-AUTO39 relative selector table"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report.update({
        "schema": "oasis.m68k.m12-auto39-relative-selector-promotion.v1",
        "metrics": manifest["metrics"],
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
