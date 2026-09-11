"""Promote exact small tables with closed consumers and fixed bounds."""
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
TABLES = (
    (0x3E0B8, 0x3E0D8, 0x3E516,
     "41 F9 00 03 E0 B8 43 F9 00 FF 13 8C 30 3C 00 07 22 D8 51 C8",
     "eight-longword startup copy table"),
    (0x522E, 0x5236, 0x51CE,
     "41 F9 00 00 52 2E 49 F9 00 FF 0D B6",
     "eight-byte fixed selector table"),
    (0x5CBD6, 0x5CBE8, 0x5120,
     "41 F9 00 05 CB D6 7E 05 10 18 1A 18 1C 18 48 80 48 85 48 86 E7 4D E7 4E 61 00 EF B0",
     "six three-byte source records"),
)


def parse_tables(rom):
    report = []
    for start, end, consumer, contract, shape in TABLES:
        expected = bytes.fromhex(contract)
        if rom[consumer:consumer + len(expected)] != expected:
            raise ValueError(f"consumer contract changed at 0x{consumer:06X}")
        if end <= start or end > len(rom):
            raise ValueError(f"table bounds invalid at 0x{start:06X}")
        report.append({"start": start, "end": end, "consumer": consumer,
                       "bytes": end - start, "shape": shape})
    return report


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
                "source": "M12_AUTO22_exact_small_tables",
                "confidence": "CONFIRMED",
                "classification": "EXACT_SMALL_LOOKUP_TABLE",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "exact consumer contract and fixed loop/index shape close the "
                    "table boundary without relying on neighboring UNKNOWN bytes"
                ),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"table overlaps non-UNKNOWN 0x{entry['start']:06X}")
    raise ValueError(f"table is not wholly UNKNOWN 0x{start:06X}..{end:06X}")


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    tables = parse_tables(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    for table in tables:
        current = split_unknown(current, table["start"], table["end"])
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
    manifest["metrics"]["SOURCE_OWNED_BYTES"] = sum(
        entry["size"] for entry in entries if entry["kind"] != "UNKNOWN")
    manifest["metrics"]["SOURCE_OWNED_PERCENT"] = (
        100.0 * manifest["metrics"]["SOURCE_OWNED_BYTES"] / len(rom))
    manifest["transaction"] = "M12-AUTO22 exact small tables"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto22-exact-small-tables.v1",
              "tables": tables, "metrics": manifest["metrics"],
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
