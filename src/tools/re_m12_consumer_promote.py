"""Promote consumer-backed graphics streams and fixed parser records."""
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


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end, kind, classification, source, reason):
    if end <= start:
        raise ValueError("promotion range is empty")
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            replacement.append({
                "start": start, "end": end, "kind": kind, "source": source,
                "confidence": "CONFIRMED", "classification": classification,
                "emitted_artifact_type": "rom_asset", "ownership_reason": reason,
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"range overlaps non-UNKNOWN 0x{entry['start']:06X}.."
                             f"0x{entry['end']:06X}")
    raise ValueError(f"range is not wholly UNKNOWN: 0x{start:06X}..0x{end:06X}")


def source_map(entries, root):
    return {index: root / entry["artifact"] for index, entry in enumerate(entries)
            if entry.get("emitted_artifact_type") == "asm"}


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    pointer_report = json.loads(Path(args.pointer_results).read_text())
    if pointer_report.get("schema") != "oasis.m12.pointer-resource-boundary.v1":
        raise ValueError("unexpected pointer report schema")
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    accepted = []
    for record in pointer_report["records"]:
        start, end = int(record["start"]), int(record["end"])
        current = split_unknown(
            current, start, end, "LOCAL_ROM_DERIVED_ASSET",
            "CONSUMER_BACKED_GRAPHICS_STREAM", "M12_AUTO2_consumer_graphics",
            "exact ROM pointer is loaded by a bounded game consumer and the "
            "existing graphics decoder consumes a deterministic source range")
        accepted.append({"start": start, "end": end, "consumer": record["consumer"]})
    records = []
    record_start = 0x200009
    record_size = 0x4B8
    for index in range(8):
        start = record_start + index * record_size
        end = start + record_size
        current = split_unknown(
            current, start, end, "STRUCTURED_DATA_CONFIRMED",
            "FIXED_CHECKSUM_RECORD_1208", "M12_AUTO2_fixed_record_parser",
            "bounded parser validates the record signature/checksum and the exact "
            "copy routine transfers 302 longwords (1208 bytes)")
        records.append({"start": start, "end": end, "bytes": record_size})
    output.mkdir(parents=True)
    materialized = output / "materialized"
    entries = AUTO.materialize(materialized, current, rom, source_map(current, baseline_path.parent))
    matched, difference, reason, detail = AUTO.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO2 consumer-backed graphics and fixed records"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto2-consumer-promotion.v1",
        "pointer_report": str(Path(args.pointer_results).resolve()),
        "accepted_graphics": accepted,
        "fixed_records": records,
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
    parser.add_argument("--pointer-results", required=True)
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
