"""Promote the unknown prefix of a closed overlapping PC-relative word table."""
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
TABLE_START = 0x062DA8
TABLE_END = 0x062DC8
UNKNOWN_PREFIX_END = 0x062DC0
TABLE_COUNT = 16
TABLE_STRIDE = 2
CONSUMER = 0x061A22
CONSUMER_BYTES = bytes.fromhex("45FA138432320000024300F0EF4B82433D41000A")
OVERLAP_START = 0x062DC0
OVERLAP_END = 0x062DC8


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
                "source": "M12_AUTO56_pc_word_overlap",
                "confidence": "CONFIRMED",
                "classification": "PC_RELATIVE_WORD_LOOKUP_TABLE_PREFIX",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "the exact PC-relative consumer masks its selector to 0x0F, "
                    "doubles it, and reads 16 words from the 0x062DA8 base; "
                    "the final four words overlap the already confirmed table "
                    "at 0x062DC0"
                ),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError("promotion range partially overlaps an UNKNOWN entry")
    raise ValueError("promotion range is not wholly UNKNOWN")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET", "DATA_KNOWN"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def parse_contract(rom):
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    if rom[CONSUMER:CONSUMER + len(CONSUMER_BYTES)] != CONSUMER_BYTES:
        raise ValueError("PC-relative word-table consumer contract changed")
    words = [int.from_bytes(rom[TABLE_START + i * TABLE_STRIDE:TABLE_START + i * TABLE_STRIDE + 2], "big")
             for i in range(TABLE_COUNT)]
    if rom[OVERLAP_START:OVERLAP_END] != rom[TABLE_START + 24:TABLE_END]:
        raise ValueError("declared overlap does not match the final four words")
    return {
        "table_start": TABLE_START,
        "table_end": TABLE_END,
        "table_bytes": TABLE_END - TABLE_START,
        "table_count": TABLE_COUNT,
        "table_stride": TABLE_STRIDE,
        "unknown_prefix": [TABLE_START, UNKNOWN_PREFIX_END],
        "overlap": [OVERLAP_START, OVERLAP_END],
        "words": words,
        "consumer": CONSUMER,
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
    current = split_unknown(renumber(baseline["entries"]), TABLE_START, UNKNOWN_PREFIX_END)
    overlap_entry = next(
        entry for entry in current
        if entry["start"] <= OVERLAP_START and OVERLAP_END <= entry["end"]
    )
    if overlap_entry["kind"] != "STRUCTURED_DATA_CONFIRMED":
        raise ValueError("the declared overlap is not already source-owned")
    output.mkdir(parents=True)
    materialized = output / "materialized"
    sources = {index: AUTO.resolve_artifact(baseline_path, entry["artifact"])
               for index, entry in enumerate(current)
               if entry.get("emitted_artifact_type") == "asm" and entry.get("artifact")}
    entries = AUTO.materialize(materialized, current, rom, sources)
    baseline_rebuilt = baseline_path.parent / "rebuilt.rom"
    if baseline_rebuilt.read_bytes() != rom:
        raise ValueError("baseline rebuilt ROM is not canonical")
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO56 PC-relative overlapping word table"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto56-pc-word-overlap.v1",
        "contract": contract,
        "promoted_bytes": UNKNOWN_PREFIX_END - TABLE_START,
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
