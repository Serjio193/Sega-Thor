"""Promote the bounded CC-B0 relative-target tables."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import zlib


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location(
    "re_m12_ccb0_group_table_promote", ROOT / "re_m12_ccb0_group_table_promote.py")
GROUP = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GROUP)

ROM_SHA256 = GROUP.ROM_SHA256
CONSUMER = GROUP.CONSUMER
SELECTOR_CONTRACT = bytes.fromhex(
    "41 F9 00 04 37 1E 34 00 E0 4A D4 42 D4 42 20 70 20 00 "
    "32 00 02 41 00 FF D2 41 D0 C1 D0 D0")
TARGET_TABLE_BYTES = 0x100 * 2


def parse_targets(rom):
    if rom[CONSUMER:CONSUMER + len(SELECTOR_CONTRACT)] != SELECTOR_CONTRACT:
        raise ValueError("CC-B0 selector contract changed")
    groups = GROUP.parse_table(rom)
    targets = sorted(set(groups["targets"]))
    for target in targets:
        if target + TARGET_TABLE_BYTES > len(rom):
            raise ValueError(f"target table outside ROM at 0x{target:06X}")
    return {
        "consumer": CONSUMER,
        "selector_contract_bytes": len(SELECTOR_CONTRACT),
        "slot_count": 0x100,
        "slot_stride": 2,
        "group_table": groups,
        "targets": [{"start": target, "end": target + TARGET_TABLE_BYTES}
                    for target in targets],
    }


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_interval(entries, start, end):
    result = []
    cursor = start
    for entry in entries:
        if entry["end"] <= start or entry["start"] >= end:
            result.append(entry)
            continue
        if entry["start"] < start or entry["end"] > end:
            if entry["kind"] != "UNKNOWN":
                raise ValueError(f"target table conflicts at 0x{max(start, entry['start']):06X}")
        left = max(start, entry["start"])
        right = min(end, entry["end"])
        if cursor < left:
            raise ValueError("target-table interval is not covered by manifest")
        if entry["start"] < left:
            result.append({**entry, "end": left})
        if entry["kind"] == "UNKNOWN":
            result.append({
                "start": left,
                "end": right,
                "kind": "STRUCTURED_DATA_CONFIRMED",
                "source": "M12_AUTO21_CCB0_relative_target_tables",
                "confidence": "CONFIRMED",
                "classification": "CCB0_RELATIVE_TARGET_TABLE_256X16",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "the exact CC-B0 consumer masks D0 to one byte, scales it "
                    "by two, and consumes a signed relative word; this closes "
                    "256 two-byte slots from each selected longword target"
                ),
            })
        else:
            result.append({**entry, "start": left, "end": right})
        if right < entry["end"]:
            result.append({**entry, "start": right})
        cursor = right
    if cursor < end:
        raise ValueError("target-table interval ended before manifest coverage")
    return renumber(sorted(result, key=lambda item: item["start"]))


def source_owned(entries, rom_size):
    count = sum(entry["size"] for entry in entries if entry["kind"] != "UNKNOWN")
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    report = parse_targets(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    intervals = merge_intervals((item["start"], item["end"]) for item in report["targets"])
    for start, end in intervals:
        current = split_interval(current, start, end)
    output.mkdir(parents=True)
    materialized = output / "materialized"
    entries = GROUP.AUTO.materialize(
        materialized, current, rom, GROUP.AUTO.source_map(current, baseline_path))
    matched, difference, reason, detail = GROUP.AUTO.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = GROUP.AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO21 CC-B0 relative target tables"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report.update({
        "schema": "oasis.m68k.m12-auto21-ccb0-target-tables.v1",
        "merged_intervals": [{"start": start, "end": end}
                             for start, end in intervals],
        "promoted_unknown_bytes": sum(
            entry["size"] for entry in entries
            if entry.get("source") == "M12_AUTO21_CCB0_relative_target_tables"),
        "metrics": manifest["metrics"],
        "full_rom": {"size": len(rebuilt),
                     "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                     "sha1": hashlib.sha1(rebuilt).hexdigest(),
                     "sha256": hashlib.sha256(rebuilt).hexdigest()},
    })
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def merge_intervals(intervals):
    merged = []
    for start, end in sorted(intervals):
        if merged and start <= merged[-1][1]:
            merged[-1] = (merged[-1][0], max(merged[-1][1], end))
        else:
            merged.append((start, end))
    return merged


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
