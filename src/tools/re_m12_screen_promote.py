"""Promote exact screen-descriptor graphics streams into the M12 ROM map."""
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


def merge_ranges(records):
    ranges = sorted((int(item["start"]), int(item["end"])) for item in records
                    if item.get("status") == "ACCEPTED")
    merged = []
    for start, end in ranges:
        if end <= start:
            raise ValueError("screen resource has an empty range")
        if merged and start < merged[-1][1]:
            merged[-1] = (merged[-1][0], max(merged[-1][1], end))
        else:
            merged.append((start, end))
    return merged


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
                "kind": "LOCAL_ROM_DERIVED_ASSET",
                "source": "M12_AUTO2_screen_descriptor_graphics",
                "confidence": "CONFIRMED",
                "classification": "SCREEN_DESCRIPTOR_PRIMARY_STREAM",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "screen-group relative pointer resolves to a 26-byte descriptor; "
                    "descriptor primary stream is consumed by the exact graphics decoder"
                ),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"screen stream overlaps non-UNKNOWN range "
                             f"0x{entry['start']:06X}..0x{entry['end']:06X}")
    raise ValueError(f"screen stream is not inside UNKNOWN: 0x{start:06X}..0x{end:06X}")


def split_descriptor(entries, start, end):
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
                "kind": "STRUCTURED_DATA_CONFIRMED",
                "source": "M12_AUTO2_screen_descriptor_parser",
                "confidence": "CONFIRMED",
                "classification": "SCREEN_DESCRIPTOR_26_BYTE",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "relative group pointer resolves to a fixed 26-byte descriptor "
                    "consumed by the screen dispatcher"
                ),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"descriptor overlaps non-UNKNOWN range "
                             f"0x{entry['start']:06X}..0x{entry['end']:06X}")
    raise ValueError(f"descriptor is not inside UNKNOWN: 0x{start:06X}..0x{end:06X}")


def source_map(entries, root):
    return {index: root / entry["artifact"] for index, entry in enumerate(entries)
            if entry.get("emitted_artifact_type") == "asm"}


def source_owned(entries, rom_size):
    owned_kinds = {
        "CODE_VERIFIED", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
        "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET",
    }
    count = sum(entry["size"] for entry in entries if entry["kind"] in owned_kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    manifest_path = Path(args.manifest).resolve()
    current_root = manifest_path.parent
    baseline = json.loads(manifest_path.read_text())
    results = json.loads(Path(args.screen_results).read_text())
    if results.get("schema") != "oasis.m68k.screen-resource-boundary.v1":
        raise ValueError("unexpected screen resource report schema")
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    descriptor_ranges = sorted({
        (int(use["descriptor"]), int(use["descriptor"]) + 26)
        for record in results["records"] if record.get("status") == "ACCEPTED"
        for use in record.get("uses", [])
    })
    for start, end in descriptor_ranges:
        current = split_descriptor(current, start, end)
    ranges = merge_ranges(results["records"])
    accepted = []
    for start, end in ranges:
        current = split_unknown(current, start, end)
        accepted.append({"start": start, "end": end, "bytes": end - start})
    output.mkdir(parents=True)
    materialized = output / "materialized"
    entries = AUTO.materialize(materialized, current, rom, source_map(current, current_root))
    matched, difference, reason, detail = AUTO.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO2 screen descriptor primary streams"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto2-screen-promotion.v1",
        "input_report": str(Path(args.screen_results).resolve()),
        "descriptors": results["descriptors"],
        "unique_streams": results["unique_streams"],
        "accepted_streams": results["accepted"],
        "descriptor_ranges": len(descriptor_ranges),
        "descriptor_bytes": 26 * len(descriptor_ranges),
        "materialized_ranges": accepted,
        "metrics": manifest["metrics"],
        "full_rom": {
            "size": len(rebuilt),
            "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
            "sha1": hashlib.sha1(rebuilt).hexdigest(),
            "sha256": hashlib.sha256(rebuilt).hexdigest(),
        },
    }
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--screen-results", required=True)
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
