"""Promote the exact Ancient Music Driver ROM data container."""
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
UPLOAD_START = 0x62E38
UPLOAD_END = 0x64E38
DATA_START = UPLOAD_END
DATA_END = 0x7D780
FF_END = 0x80000
TABLE_CONSUMER = 0x60B50
TABLE_BASE = 0x638D4
TABLES = (
    ("primary", TABLE_BASE, 0x638D4, 0x6395C, 34, 29),
    ("secondary", 0x639D4, 0x639D4, 0x63BCC, 126, 126),
    ("tertiary", 0x63C14, 0x63C14, 0x63C58, 17, 4),
)
CONSUMER_BYTES = bytes.fromhex(
    "41 fa 2d 82 02 40 00 ff e5 48 22 30 00 00 4a 81 "
    "67 00 06 7e d1 c1")


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def read_u32(rom, offset):
    return int.from_bytes(rom[offset:offset + 4], "big")


def validate_archive(rom):
    if rom[DATA_END:FF_END] != b"\xFF" * (FF_END - DATA_END):
        raise ValueError("sound-data terminal FF boundary changed")
    if rom[TABLE_CONSUMER:TABLE_CONSUMER + len(CONSUMER_BYTES)] != CONSUMER_BYTES:
        raise ValueError("exact 0x60B50 table consumer changed")
    groups = []
    for name, base, start, end, expected, active in TABLES:
        values = [read_u32(rom, offset) for offset in range(start, end, 4)]
        nonzero = [value for value in values if value]
        if len(values) != expected or len(nonzero) != expected:
            raise ValueError(f"{name} table shape changed")
        if nonzero != sorted(nonzero) or len(set(nonzero)) != len(nonzero):
            raise ValueError(f"{name} offsets are not strictly increasing")
        targets = [base + value for value in nonzero]
        active_targets = [target for target in targets if DATA_START <= target < DATA_END]
        if len(active_targets) != active:
            raise ValueError(f"{name} active target count changed")
        if name != "tertiary" and any(target < UPLOAD_START or target >= DATA_END
                                      for target in targets):
            raise ValueError(f"{name} target escaped the driver/data archive")
        if name == "tertiary" and any(target < DATA_START for target in targets[:active]):
            raise ValueError("tertiary active target escaped data")
        if name == "tertiary" and any(target < DATA_END for target in targets[active:]):
            raise ValueError("tertiary inactive target entered data")
        groups.append({"name": name, "base": base, "table_start": start,
                       "table_end": end, "entries": expected,
                       "active_data_targets": active,
                       "first_target": targets[0], "last_target": targets[-1]})
    return groups


def split_unknown(entries):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= DATA_START and DATA_END <= entry["end"]:
            replacement = []
            if entry["start"] < DATA_START:
                replacement.append({**entry, "end": DATA_START})
            replacement.append({
                "start": DATA_START, "end": DATA_END, "kind": "DATA_KNOWN",
                "source": "M12_AUTO19_ancient_music_driver_archive",
                "confidence": "CONFIRMED",
                "classification": "SOUND_DATA_CONTAINER_CONFIRMED",
                "emitted_artifact_type": "blob",
                "ownership_reason": (
                    "exact 0x60B50 consumer indexes the in-ROM Ancient Music Driver "
                    "offset table at 0x638D4; validated primary/secondary/tertiary "
                    "offset groups terminate at the exact 0x7D780 FF boundary"),
            })
            if DATA_END < entry["end"]:
                replacement.append({**entry, "start": DATA_END})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if DATA_START < entry["end"] and entry["start"] < DATA_END:
            raise ValueError("sound data overlaps a non-UNKNOWN manifest entry")
    raise ValueError("sound data range is not wholly UNKNOWN")


def verify_inherited(materialized, baseline_root, baseline_rebuilt, rom, entries):
    rebuilt = Path(baseline_rebuilt).read_bytes()
    if len(rebuilt) != len(rom) or hashlib.sha256(rebuilt).hexdigest() != ROM_SHA256:
        raise ValueError("verified baseline rebuilt ROM is not canonical")
    for entry in entries:
        artifact = materialized / entry["artifact"]
        if entry["kind"] == "DATA_KNOWN":
            if artifact.read_bytes() != rom[DATA_START:DATA_END]:
                raise ValueError("sound data artifact differs from canonical ROM")
        elif entry.get("emitted_artifact_type") == "asm":
            previous = baseline_root / entry["artifact"]
            if not previous.exists() or artifact.read_bytes() != previous.read_bytes():
                raise ValueError("inherited ASM artifact changed")
        elif artifact.read_bytes() != rom[entry["start"]:entry["end"]]:
            raise ValueError("inherited blob artifact differs from canonical ROM")
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    groups = validate_archive(rom)
    manifest_path = Path(args.manifest).resolve()
    baseline = json.loads(manifest_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = split_unknown(baseline["entries"])
    output.mkdir(parents=True)
    materialized = output / "materialized"
    sources = {index: manifest_path.parent / entry["artifact"]
               for index, entry in enumerate(current)
               if entry.get("emitted_artifact_type") == "asm"}
    entries = AUTO.materialize(materialized, current, rom, sources)
    verify_inherited(materialized, manifest_path.parent,
                     args.baseline_rebuilt, rom, entries)
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"]["SOURCE_OWNED_BYTES"] = sum(
        entry["size"] for entry in entries if entry["kind"] != "UNKNOWN")
    manifest["metrics"]["SOURCE_OWNED_PERCENT"] = (
        100.0 * manifest["metrics"]["SOURCE_OWNED_BYTES"] / len(rom))
    manifest["transaction"] = "M12-AUTO19 exact Ancient Music Driver data archive"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto19-sound-data-promotion.v1",
        "consumer": {"pc": TABLE_CONSUMER, "table_base": TABLE_BASE,
                     "bytes_sha256": hashlib.sha256(CONSUMER_BYTES).hexdigest()},
        "archive": {"start": DATA_START, "end": DATA_END,
                    "bytes": DATA_END - DATA_START, "tables": groups,
                    "terminal_ff_end": FF_END},
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
    parser.add_argument("--baseline-rebuilt", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
