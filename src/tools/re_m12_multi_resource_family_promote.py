"""Promote two repeated graphics descriptor families and direct loader streams."""
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
FAMILY_A_RECORDS = (
    (0x02DAA4, 0x203BF6), (0x02DB24, 0x2051E4),
    (0x02DC4A, 0x207738), (0x02DCD8, 0x208C44),
    (0x02DD70, 0x20A824), (0x02DE3C, 0x20CCF8),
    (0x02DEF0, 0x20E5A2), (0x02DFA6, 0x20F826),
    (0x02E068, 0x2108D4), (0x02E0D4, 0x211518),
)
FAMILY_A_STREAMS = (
    (0x203BF6, 0x2051E4, 24644), (0x2051E4, 0x207595, 49220),
    (0x207738, 0x208C43, 24644), (0x208C44, 0x20A823, 37118),
    (0x20A824, 0x20CCF7, 41214), (0x20CCF8, 0x20E5A1, 28926),
    (0x20E5A2, 0x20F826, 27028), (0x20F826, 0x2108D3, 26692),
    (0x2108D4, 0x211517, 20548), (0x211518, 0x2119D2, 6424),
)
FAMILY_B_RECORDS = tuple((0x03DC22 + index * 0x10, pointer) for index, pointer in enumerate((
    0x2FD16C, 0x2FC682, 0x2FCF84, 0x2FD6C6, 0x2FCC2C,
    0x2FCA2C, 0x2FC87A, 0x2FD354, 0x2FD528, 0x2FCE04,
    0x2FD87A, 0x2FDC2A, 0x2FDA3C)))
FAMILY_B_STREAMS = (
    (0x2FC682, 0x2FC879, 1280), (0x2FC87A, 0x2FCA2B, 1280),
    (0x2FCA2C, 0x2FCC2C, 1280), (0x2FCC2C, 0x2FCE03, 1280),
    (0x2FCE04, 0x2FCF84, 1280), (0x2FCF84, 0x2FD16B, 1280),
    (0x2FD16C, 0x2FD354, 1280), (0x2FD354, 0x2FD528, 1280),
    (0x2FD528, 0x2FD6C5, 1280), (0x2FD6C6, 0x2FD879, 1280),
    (0x2FD87A, 0x2FDA3B, 1280), (0x2FDA3C, 0x2FDC2A, 1280),
    (0x2FDC2A, 0x2FDD91, 1280),
)
DIRECT_STREAMS = (
    (0x2F7A8C, 0x2F7D35, 3302, 0x03D250),
    (0x2F7D36, 0x2F9A7E, 15078, 0x03D2CA),
    (0x2FB266, 0x2FC681, 11334, 0x03D504),
)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end, classification, reason):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            kind = "STRUCTURED_DATA_CONFIRMED" if classification == "DESCRIPTOR" \
                else "LOCAL_ROM_DERIVED_ASSET"
            replacement.append({
                "start": start, "end": end, "kind": kind,
                "source": "M12_AUTO50_multi_resource_family",
                "confidence": "CONFIRMED", "classification": classification,
                "emitted_artifact_type": "rom_asset", "ownership_reason": reason,
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"range overlaps non-UNKNOWN 0x{entry['start']:06X}.."
                             f"0x{entry['end']:06X}")
    raise ValueError(f"range is not wholly UNKNOWN 0x{start:06X}..0x{end:06X}")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "DATA_KNOWN", "HEADER_VECTOR_ASM",
             "STRUCTURED_DATA_CONFIRMED", "PADDING_ALIGNMENT_CONFIRMED",
             "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def parse_contract(rom):
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    for start, pointer in FAMILY_A_RECORDS:
        if int.from_bytes(rom[start + 4:start + 8], "big") != pointer:
            raise ValueError(f"family A pointer changed at 0x{start:06X}")
    for start, pointer in FAMILY_B_RECORDS:
        if int.from_bytes(rom[start + 6:start + 10], "big") != pointer:
            raise ValueError(f"family B pointer changed at 0x{start:06X}")
    for start, _, _, pointer_address in DIRECT_STREAMS:
        if int.from_bytes(rom[pointer_address:pointer_address + 4], "big") != start:
            raise ValueError(f"direct loader pointer changed at 0x{pointer_address:06X}")
    return {
        "family_a": {"record_start": FAMILY_A_RECORDS[0][0], "record_size": 0x16,
                     "record_count": len(FAMILY_A_RECORDS), "stream_count": len(FAMILY_A_STREAMS)},
        "family_b": {"record_start": FAMILY_B_RECORDS[0][0], "record_size": 0x10,
                     "record_count": len(FAMILY_B_RECORDS), "stream_count": len(FAMILY_B_STREAMS)},
        "direct_stream_count": len(DIRECT_STREAMS),
        "reason": "repeated pointer-bearing records or exact direct loader source, plus exact decompressor boundaries",
    }


def verify_census(path, streams):
    report = json.loads(Path(path).read_text())
    by_start = {int(item["start"]): item for item in report["records"]}
    result = []
    for start, end, output_size, *_ in streams:
        item = by_start.get(start)
        if item is None or int(item["end"]) != end:
            raise ValueError(f"census boundary mismatch at 0x{start:06X}")
        if int(item["decompressed_bytes"]) != output_size:
            raise ValueError(f"census output mismatch at 0x{start:06X}")
        result.append({"start": start, "end": end,
                       "compressed_bytes": end - start,
                       "decompressed_bytes": output_size})
    return result


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    contract = parse_contract(rom)
    streams = FAMILY_A_STREAMS + FAMILY_B_STREAMS + DIRECT_STREAMS
    census = verify_census(args.census, streams)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    for start, _ in FAMILY_A_RECORDS:
        current = split_unknown(
            current, start, start + 0x16, "DESCRIPTOR",
            "repeated 22-byte record with exact +4 ROM pointer")
    for start, _ in FAMILY_B_RECORDS:
        current = split_unknown(
            current, start, start + 0x10, "DESCRIPTOR",
            "repeated 16-byte record with exact +6 ROM pointer")
    for start, end, _ in FAMILY_A_STREAMS + FAMILY_B_STREAMS:
        current = split_unknown(
            current, start, end, "GRAPHICS_COMPRESSED_STREAM",
            "descriptor pointer identifies stream; exact graphics decompressor end")
    for start, end, _, pointer_address in DIRECT_STREAMS:
        current = split_unknown(
            current, start, end, "GRAPHICS_DIRECT_LOADER_STREAM",
            f"exact source pointer at 0x{pointer_address:06X}; exact graphics decompressor end")
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
    manifest["transaction"] = "M12-AUTO50 multi resource families"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto50-multi-resource-family.v1",
              "contract": contract, "census": census, "metrics": manifest["metrics"],
              "promoted_bytes": len(FAMILY_A_RECORDS) * 0x16 +
              len(FAMILY_B_RECORDS) * 0x10 +
              sum(end - start for start, end, *_ in streams),
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
    parser.add_argument("--census", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
