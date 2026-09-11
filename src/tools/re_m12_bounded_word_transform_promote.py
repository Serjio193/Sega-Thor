"""Promote a bounded word table consumed by an exact DBF transform loop."""
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
TABLE_START = 0x03BD86
TABLE_WORDS = 0x40
TABLE_END = TABLE_START + 2 * TABLE_WORDS
ROUTINE = 0x03B7C0
ROUTINE_BYTES = bytes.fromhex(
    "47F900FF134CD6C602450007341E30020240000E36040243000E9640C7C587FC0007D043"
    "0240000E32003002024000E03604024300E09640C7C587FC0007D043024000E082403002"
    "02400E00360402430E009640C7C587FC0007D04302400E00824036C151CFFFA608F90000"
    "00FF164D4E75")
CALLERS = (
    (0x03A79E, bytes.fromhex(
        "3C3C00003E3C003F4DFA15DE61001014")),
    (0x03A842, bytes.fromhex(
        "383C0EEE3A3900FFAFCC3C3C00003E3C000F4DF90003BD8661000F64")),
    (0x03A98E, bytes.fromhex(
        "383900FFAFCA3A3900FFAFCC3C3C00003E3C003F2C7900FFAFA861000E16")),
    (0x03AA36, bytes.fromhex(
        "383900FFAFCA3A3900FFAFCC3C3C00003E3C003F2C7900FFAFA861000D6E")),
)
POINTER_INITIALIZER = (0x03A87A, bytes.fromhex("23FC0003BD8600FFAFA8"))


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
                "source": "M12_AUTO57_bounded_word_transform",
                "confidence": "CONFIRMED",
                "classification": "BOUNDED_WORD_TRANSFORM_TABLE",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "exact routine 0x03B7C0 reads one word per iteration and its "
                    "DBF D7 loop closes at 64 words; two direct and two RAM-pointer "
                    "callers establish the same ROM base and bounded consumer"
                ),
            })
            if TABLE_END < entry["end"]:
                replacement.append({**entry, "start": TABLE_END})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if TABLE_START < entry["end"] and entry["start"] < TABLE_END:
            raise ValueError("word-transform table overlaps a non-UNKNOWN range")
    raise ValueError("word-transform table is not wholly UNKNOWN")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET", "DATA_KNOWN"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def parse_contract(rom, verify_identity=True):
    if verify_identity and (len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256):
        raise ValueError("canonical ROM identity mismatch")
    if rom[ROUTINE:ROUTINE + len(ROUTINE_BYTES)] != ROUTINE_BYTES:
        raise ValueError("bounded word-transform routine contract changed")
    for address, expected in CALLERS + (POINTER_INITIALIZER,):
        if rom[address:address + len(expected)] != expected:
            raise ValueError(f"caller contract changed at 0x{address:06X}")
    return {
        "routine": ROUTINE,
        "table_start": TABLE_START,
        "table_end": TABLE_END,
        "word_count": TABLE_WORDS,
        "record_stride": 2,
        "callers": [address for address, _ in CALLERS],
        "pointer_initializer": POINTER_INITIALIZER[0],
    }


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
               if entry.get("emitted_artifact_type") == "asm" and entry.get("artifact")}
    entries = AUTO.materialize(materialized, current, rom, sources)
    baseline_rebuilt = baseline_path.parent / "rebuilt.rom"
    if baseline_rebuilt.read_bytes() != rom:
        raise ValueError("baseline rebuilt ROM is not canonical")
    (materialized / "rebuilt.rom").write_bytes(rom)
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO57 bounded word transform"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto57-bounded-word-transform.v1",
        "contract": contract,
        "promoted_bytes": TABLE_END - TABLE_START,
        "metrics": manifest["metrics"],
        "full_rom": {"size": len(rebuilt), "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                     "sha1": hashlib.sha1(rebuilt).hexdigest(),
                     "sha256": hashlib.sha256(rebuilt).hexdigest()},
    }
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
