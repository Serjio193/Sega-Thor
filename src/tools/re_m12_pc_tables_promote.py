"""Promote exact PC-relative lookup/data tables with closed consumers."""
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
    (0x008768, 0x008778, "16-entry byte lookup; D0 is masked with 0xF"),
    (0x0096E8, 0x0096F8, "shared 16-entry byte lookup; all consumers mask with 0xF"),
    (0x00A438, 0x00A480, "nine sequential 8-byte records; two DBF loops consume 6 then 3"),
    (0x00A480, 0x00A4C8, "nine sequential 8-byte records; two DBF loops consume 6 then 3"),
    (0x00B4C8, 0x00B4E6, "ten sequential 3-byte records; bounded byte selector and tail view"),
    (0x00B556, 0x00B56E, "eight sequential 3-byte records; selector rejects values >= 8"),
    (0x01FC02, 0x01FC0C, "ten-byte record copied by the first exact PC-relative consumer"),
    (0x01FC16, 0x01FC20, "ten-byte record copied by the second exact PC-relative consumer"),
    (0x029304, 0x02930C, "two 4-byte records; selector is masked with 0x4"),
    (0x02B0A, 0x02B28, "15-word VDP initialization table; exact DBF and copy bounds"),
    (0x03C1E8, 0x03C228, "32-word VDP initialization table; exact DBF copy bound"),
)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end, reason):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            replacement.append({
                "start": start, "end": end, "kind": "STRUCTURED_DATA_CONFIRMED",
                "source": "M12_AUTO25_exact_pc_consumer_tables",
                "confidence": "CONFIRMED", "classification": "PC_RELATIVE_LOOKUP_TABLE",
                "emitted_artifact_type": "rom_asset", "ownership_reason": reason,
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"range overlaps non-UNKNOWN 0x{entry['start']:06X}.."
                             f"0x{entry['end']:06X}")
    raise ValueError(f"range is not wholly UNKNOWN: 0x{start:06X}..0x{end:06X}")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "DATA_KNOWN", "HEADER_VECTOR_ASM",
             "STRUCTURED_DATA_CONFIRMED", "PADDING_ALIGNMENT_CONFIRMED",
             "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count, "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def parse_contracts(rom):
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    for start, end, _ in RANGES:
        if start >= end or end > len(rom) or start & 1 or end & 1:
            raise ValueError(f"invalid even table bounds 0x{start:06X}..0x{end:06X}")
    return [{"start": start, "end": end, "bytes": end - start, "reason": reason}
            for start, end, reason in RANGES]


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    contracts = parse_contracts(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    for item in contracts:
        current = split_unknown(current, item["start"], item["end"], item["reason"])
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
    manifest["transaction"] = "M12-AUTO25 exact PC-relative consumer tables"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto25-pc-table-promotion.v1",
              "contracts": contracts, "metrics": manifest["metrics"],
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
