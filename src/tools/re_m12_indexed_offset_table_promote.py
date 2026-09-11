"""Promote the exact 16-word indexed offset table at 0x00AD56."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import struct
import zlib


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("re_auto_promote", ROOT / "re_auto_promote.py")
AUTO = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUTO)

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
TABLE_START = 0x00AD56
TABLE_END = 0x00AD76
BASE_INSTRUCTION = 0x00B1DA
BASE_BYTES = bytes.fromhex("4BFAFB7A")
INDEX_INSTRUCTION = 0x00B242
INDEX_BYTES = bytes.fromhex("02440F00EE4C38354000")
VALUES = (1, 2, 3, 4, 2, 4, 6, 8, 3, 6, 9, 12, 4, 8, 12, 16)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def selector_offsets(mask=0x0F00, shift=7):
    return tuple((value & mask) >> shift for value in range(0, 0x1000, 0x100))


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
                "source": "M12_AUTO45_indexed_offset_table",
                "confidence": "CONFIRMED",
                "classification": "INDEXED_WORD_OFFSET_TABLE",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "PC-relative LEA fixes the table base; AND #$0F00 and LSR #7 "
                    "close the 16 even word offsets used by MOVE.W (A5,D4.W)"
                ),
            })
            if TABLE_END < entry["end"]:
                replacement.append({**entry, "start": TABLE_END})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if TABLE_START < entry["end"] and entry["start"] < TABLE_END:
            raise ValueError("indexed offset table overlaps existing ownership")
    raise ValueError("indexed offset table is not wholly UNKNOWN")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "DATA_KNOWN", "HEADER_VECTOR_ASM",
             "STRUCTURED_DATA_CONFIRMED", "PADDING_ALIGNMENT_CONFIRMED",
             "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def parse_contract(rom):
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    if rom[BASE_INSTRUCTION:BASE_INSTRUCTION + len(BASE_BYTES)] != BASE_BYTES:
        raise ValueError("PC-relative table-base contract changed")
    if rom[INDEX_INSTRUCTION:INDEX_INSTRUCTION + len(INDEX_BYTES)] != INDEX_BYTES:
        raise ValueError("indexed table consumer contract changed")
    values = tuple(struct.unpack(">H", rom[address:address + 2])[0]
                   for address in range(TABLE_START, TABLE_END, 2))
    if values != VALUES:
        raise ValueError("indexed offset table bytes changed")
    offsets = selector_offsets()
    if offsets != tuple(range(0, TABLE_END - TABLE_START, 2)):
        raise ValueError("selector does not close the 16-word table")
    return {"start": TABLE_START, "end": TABLE_END,
            "bytes": TABLE_END - TABLE_START, "entry_count": len(values),
            "values": values, "selector_offsets": offsets,
            "consumer": INDEX_INSTRUCTION,
            "reason": "A5=0x00AD56; (D4 & 0x0F00) >> 7; MOVE.W (A5,D4.W),D4"}


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
    sources = {index: AUTO.resolve_artifact(baseline_path, entry["artifact"])
               for index, entry in enumerate(current)
               if entry.get("emitted_artifact_type") == "asm"}
    entries = AUTO.materialize(materialized, current, rom, sources)
    baseline_rebuilt = baseline_path.parent / "rebuilt.rom"
    if baseline_rebuilt.read_bytes() != rom:
        raise ValueError("baseline rebuilt ROM is not canonical")
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO45 exact indexed offset table"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto45-indexed-offset-table.v1",
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
    parser.add_argument("--assembler", required=False)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
