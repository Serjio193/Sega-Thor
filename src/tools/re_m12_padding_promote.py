"""Promote exact erased-ROM alignment runs from the M12 full-ROM map."""
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
FILL_BYTE = 0xFF
MIN_RUN = 0x100
ALIGNMENT = 0x1000


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def discover(rom, entries):
    runs = []
    for entry in entries:
        if entry["kind"] != "UNKNOWN":
            continue
        position = entry["start"]
        while position < entry["end"]:
            if rom[position] != FILL_BYTE:
                position += 1
                continue
            end = position + 1
            while end < entry["end"] and rom[end] == FILL_BYTE:
                end += 1
            if (end - position >= MIN_RUN and end % ALIGNMENT == 0 and
                    (position == entry["start"] or rom[position - 1] != FILL_BYTE)):
                runs.append({"start": position, "end": end,
                             "bytes": end - position, "fill": FILL_BYTE})
            position = end
    return runs


def split_unknown(entries, start, end):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            replacement.append({
                "start": start,
                "end": end,
                "kind": "PADDING_ALIGNMENT_CONFIRMED",
                "source": "M12_AUTO9_erased_alignment_runs",
                "confidence": "CONFIRMED",
                "classification": "ERASED_ROM_ALIGNMENT_PADDING",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "the complete bounded range is byte-exact 0xFF fill, is at least "
                    "256 bytes, and terminates on a 4 KiB ROM alignment boundary"
                ),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"padding overlaps non-UNKNOWN range 0x{entry['start']:06X}.."
                             f"0x{entry['end']:06X}")
    raise ValueError(f"padding is not wholly UNKNOWN: 0x{start:06X}..0x{end:06X}")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def source_map(entries, root):
    return {index: root / entry["artifact"] for index, entry in enumerate(entries)
            if entry.get("emitted_artifact_type") == "asm"}


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    runs = discover(rom, current)
    for run_info in runs:
        current = split_unknown(current, run_info["start"], run_info["end"])
    output.mkdir(parents=True)
    materialized = output / "materialized"
    entries = AUTO.materialize(materialized, current, rom,
                               source_map(current, baseline_path.parent))
    matched, difference, reason, detail = AUTO.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO9 erased alignment padding"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto9-erased-alignment-padding.v1",
        "runs": runs,
        "promoted_padding_bytes": sum(item["bytes"] for item in runs),
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
