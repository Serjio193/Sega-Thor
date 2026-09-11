"""Promote save-slot ranges closed by the exact save serializers."""
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
RANGES = (
    (0x2025CD, 0x2026E1, "primary slot: six tags, 130 bytes, checksum"),
    (0x2026E1, 0x2026F5, "secondary slot prefix: six tags and four-byte value"),
)
CONTRACTS = {
    0x001DEC: bytes.fromhex(
        "43 F9 00 20 25 CD 45 FA E6 C6 72 05 10 11 B0 1A 66 2C "
        "54 89 51 C9 FF F6 24 48 32 3C 00 81 42 40 42 42 10 11 "
        "14 C0 D4 40 54 89"),
    0x001EDE: bytes.fromhex(
        "43 F9 00 20 26 E1 45 FA E5 DA 72 05 10 11 B0 1A 66 1E "
        "54 89 51 C9 FF F6 10 11 54 89 E1 88 10 11 54 89 E1 88 "
        "10 11 54 89 E1 88 10 11"),
}


def parse_contracts(rom):
    for address, expected in CONTRACTS.items():
        if rom[address:address + len(expected)] != expected:
            raise ValueError(f"save serializer contract changed at 0x{address:06X}")
    return [{"start": start, "end": end, "bytes": end - start, "shape": shape}
            for start, end, shape in RANGES]


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
                "source": "M12_AUTO23_save_serialization_slots",
                "confidence": "CONFIRMED",
                "classification": "SAVE_SLOT_SERIALIZATION_RANGE",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "exact save serializer/checker contracts establish the slot start, "
                    "fixed stride-2 layout, checksum or value suffix, and non-code extent"
                ),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"save range overlaps non-UNKNOWN 0x{entry['start']:06X}")
    raise ValueError(f"save range is not wholly UNKNOWN 0x{start:06X}..0x{end:06X}")


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    ranges = parse_contracts(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    for item in ranges:
        current = split_unknown(current, item["start"], item["end"])
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
    manifest["transaction"] = "M12-AUTO23 save serialization slots"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto23-save-slots.v1",
        "ranges": ranges,
        "metrics": manifest["metrics"],
        "full_rom": {"size": len(rebuilt),
                     "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                     "sha1": hashlib.sha1(rebuilt).hexdigest(),
                     "sha256": hashlib.sha256(rebuilt).hexdigest()},
    }
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
