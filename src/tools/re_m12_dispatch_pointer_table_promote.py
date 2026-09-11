"""Promote the exact 89-entry state dispatch pointer table at 0x00DF54."""
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
TABLE_START = 0x00DF54
TABLE_END = 0x00E0B8
CONSUMERS = (0x00FD70, 0x00FDF4, 0x00FF7E)
POINTERS = (
    0x0115C8, 0x011B4E, 0x01254C, 0x012F1E, 0x01387C, 0x00E20A,
    0x01A184, 0x014848, 0x021F44, 0x0194AE, 0x00E0B8, 0x0254EA,
    0x00E25C, 0x015AE2, 0x018072, 0x00EA04, 0x01C772, 0x01AC90,
    0x00EDAA, 0x017D12, 0x00E0B8, 0x016BD0, 0x016C5C, 0x016CBE,
    0x0173D6, 0x01778C, 0x016D5C, 0x016DCE, 0x016F7C, 0x0170E0,
    0x0171C4, 0x00E0B8, 0x00E0B8, 0x021B2C, 0x0178EA, 0x0178EA,
    0x0178EA, 0x0178EA, 0x017908, 0x0179A8, 0x0179D4, 0x025B32,
    0x00E144, 0x01F5A4, 0x01EB54, 0x01DF1C, 0x01D140, 0x027C8A,
    0x0283E6, 0x00E9DE, 0x00E0B8, 0x00E0B8, 0x0172C2, 0x028810,
    0x028D8C, 0x02903C, 0x01CD5A, 0x020BDA, 0x020684, 0x01FA78,
    0x029452, 0x0269EE, 0x029E32, 0x019098, 0x01933E, 0x016EBC,
    0x015316, 0x02A652, 0x02A374, 0x02A9F4, 0x02AD2C, 0x02B194,
    0x02BDF2, 0x02C27A, 0x02BF3E, 0x023D02, 0x0246CC, 0x00E0B8,
    0x016ADE, 0x022CE0, 0x017E28, 0x02B582, 0x016A64, 0x016F10,
    0x01906C, 0x0247AA, 0x025054, 0x025114, 0x016CBE,
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
                "source": "M12_AUTO30_dispatch_pointer_table",
                "confidence": "CONFIRMED",
                "classification": "ABSOLUTE_STATE_DISPATCH_POINTER_TABLE",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": "three exact dispatch consumers read 89 longwords",
            })
            if TABLE_END < entry["end"]:
                replacement.append({**entry, "start": TABLE_END})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if TABLE_START < entry["end"] and entry["start"] < TABLE_END:
            raise ValueError("dispatch pointer table overlaps existing ownership")
    raise ValueError("dispatch pointer table is not wholly UNKNOWN")


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
    values = tuple(struct.unpack(">I", rom[address:address + 4])[0]
                   for address in range(TABLE_START, TABLE_END, 4))
    if values != POINTERS:
        raise ValueError("dispatch pointer table bytes changed")
    if len(values) != 89 or any(value & 1 for value in values):
        raise ValueError("dispatch pointer alignment contract failed")
    return {"start": TABLE_START, "end": TABLE_END, "bytes": TABLE_END - TABLE_START,
            "entry_count": len(values), "consumer_addresses": CONSUMERS,
            "selector_first": 0x16, "selector_last_table_entry": 0x6E,
            "values": values,
            "reason": "LEA DF54; add.w D6,D6; MOVEA.L -44(A0,D6.W),A0; JSR (A0)"}


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
    manifest["transaction"] = "M12-AUTO30 exact dispatch pointer table"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto30-dispatch-pointer-promotion.v1",
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
