"""Promote the exact event dispatch table consumed by sub_00530C."""
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
TABLE_START = 0x00532C
TABLE_END = 0x005378
ENTRY_COUNT = (TABLE_END - TABLE_START) // 2
DEFAULT_TARGET = 0x00532A


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
                "start": start, "end": end, "kind": "STRUCTURED_DATA_CONFIRMED",
                "source": "M12_AUTO27_event_dispatch_table",
                "confidence": "CONFIRMED",
                "classification": "SIGNED_RELATIVE_EVENT_DISPATCH_TABLE",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": "sub_00530C masks the event selector to 38 entries, "
                "then adds each signed word relative to its table entry",
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError("dispatch table overlaps existing ownership")
    raise ValueError("dispatch table is not wholly UNKNOWN")


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
    words = [struct.unpack(">h", rom[address:address + 2])[0]
             for address in range(TABLE_START, TABLE_END, 2)]
    if len(words) != ENTRY_COUNT:
        raise ValueError("unexpected event table length")
    targets = [address + offset for address, offset in
               zip(range(TABLE_START, TABLE_END, 2), words)]
    if targets[-(ENTRY_COUNT - 17):] != [DEFAULT_TARGET] * (ENTRY_COUNT - 17):
        raise ValueError("event table default targets changed")
    if any(target < TABLE_END or target >= 0x00557A for target in targets[:17]):
        raise ValueError("event table handler target escaped the bounded cluster")
    return {"start": TABLE_START, "end": TABLE_END, "bytes": TABLE_END - TABLE_START,
            "entry_count": ENTRY_COUNT, "default_target": DEFAULT_TARGET,
            "signed_offsets": words, "resolved_targets": targets,
            "reason": "D0=(event_byte-0x1A), doubled; 38 signed table words; "
                      "entries 17..37 return through 0x532A"}


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    contract = parse_contract(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = split_unknown(renumber(baseline["entries"]), TABLE_START, TABLE_END)
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
    manifest["transaction"] = "M12-AUTO27 exact event dispatch table"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto27-event-dispatch-promotion.v1",
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
