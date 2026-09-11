"""Promote runtime-correlated count-bounded 6-byte record regions."""
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
AF16 = "0x00AF16"
ABB4 = "0x00ABB4"
RANGES = tuple((start, end, AF16) for start, end in (
    (0x15CFF6, 0x15D0FC), (0x15D116, 0x15D144), (0x15D15E, 0x15D18C),
    (0x15D1C0, 0x15D1D4), (0x15D208, 0x15D21C), (0x15D250, 0x15D264),
    (0x15D3B0, 0x15D4AC), (0x15D4CC, 0x15D500), (0x15D520, 0x15D53A),
    (0x15D574, 0x15D58E), (0x15D6B8, 0x15D6CC), (0x15DA0E, 0x15DA34),
    (0x15DA74, 0x15DA9A), (0x15DC2C, 0x15DC42), (0x15DC50, 0x15DC66),
    (0x15DCBC, 0x15DD04), (0x15DD1E, 0x15DD32), (0x15DD66, 0x15DD7A),
    (0x15DD94, 0x15DE24), (0x15DE3E, 0x15DE6C), (0x15DE86, 0x15DE9A),
    (0x15DECE, 0x15DF02), (0x174378, 0x1744A2),
)) + tuple((start, end, ABB4) for start, end in (
    (0x13D29A, 0x13D2CE), (0x13D2EE, 0x13D322), (0x13D342, 0x13D370),
    (0x13D38A, 0x13D3C4), (0x13D3E4, 0x13D418), (0x13D432, 0x13D466),
    (0x13D732, 0x13D74C), (0x13D774, 0x13D788), (0x13D7B6, 0x13D7DC),
    (0x13D816, 0x13D842), (0x13D882, 0x13D8A8), (0x13D936, 0x13D964),
    (0x13E4F4, 0x13E50E), (0x13E548, 0x13E562), (0x13ECE0, 0x13ED0E),
    (0x13ED28, 0x13ED5C), (0x13ED70, 0x13EDA4), (0x13EDBE, 0x13EE16),
    (0x13EE3C, 0x13EE94), (0x13EEBA, 0x13EF12), (0x13EF38, 0x13EF6C),
    (0x13EFFE, 0x13F032), (0x13F04C, 0x13F080), (0x13F09A, 0x13F0D4),
    (0x13F0F4, 0x13F152), (0x13F17E, 0x13F1BE), (0x13F1EA, 0x13F230),
    (0x13F250, 0x13F290),
))
NEW_RANGES = tuple((start, end, reader) for start, end, reader in RANGES if start >= 0x15D6B8 and reader == AF16) + tuple(
    (start, end, reader) for start, end, reader in RANGES if reader == ABB4 and start >= 0x13ECE0)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end, reader):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            replacement.append({
                "start": start, "end": end, "kind": "STRUCTURED_DATA_CONFIRMED",
                "source": "M12_AUTO44_runtime_count_bounded_record_regions",
                "confidence": "CONFIRMED",
                "classification": "RUNTIME_COUNT_BOUNDED_6BYTE_RECORD_STREAMS",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": f"runtime reader {reader}, exact count plus 6-byte record tiling",
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"region overlaps non-UNKNOWN 0x{entry['start']:06X}")
    raise ValueError(f"region is not wholly UNKNOWN 0x{start:06X}")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "DATA_KNOWN", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count, "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def validate_regions(rom, correlation, runtime):
    by_start = {int(item["region_start"], 16): item for item in correlation["regions"]}
    facts = {item["address"]: item for item in runtime["execution_facts"]}
    result = []
    for start, end, reader in RANGES:
        item = by_start.get(start)
        if not item or int(item["region_end"], 16) + 1 != end:
            raise ValueError(f"runtime boundary changed at 0x{start:06X}")
        if item["first_reader_pc"] != reader or item["observed_bytes"] != end - start:
            raise ValueError(f"runtime reader contract changed at 0x{start:06X}")
        if facts.get(reader, {}).get("evidence_type") != "CODE_EXECUTED_AT_ADDRESS":
            raise ValueError(f"reader {reader} is not runtime-executed")
        cursor = start
        counts = []
        while cursor < end:
            count = int.from_bytes(rom[cursor:cursor + 2], "big")
            size = 2 + 6 * (count + 1)
            if size <= 2 or cursor + size > end:
                raise ValueError(f"record tiling failed at 0x{cursor:06X}")
            counts.append(count)
            cursor += size
        if cursor != end:
            raise ValueError(f"record tiling did not close 0x{start:06X}")
        result.append({"start": start, "end": end, "bytes": end - start,
                       "reader": int(reader, 16), "stream_count": len(counts),
                       "counts": counts})
    return result


def run(args):
    rom_path = Path(args.rom).resolve(); rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    regions = validate_regions(
        rom, json.loads(Path(args.runtime_correlation).read_text()),
        json.loads(Path(args.runtime_evidence).read_text()))
    baseline_path = Path(args.manifest).resolve(); baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    for start, end, reader in NEW_RANGES:
        current = split_unknown(current, start, end, reader)
    materialized = output / "materialized"
    sources = AUTO.source_map(current, baseline_path)
    entries = AUTO.materialize(materialized, current, rom, sources)
    rebuilt = baseline_path.parent / "rebuilt.rom"
    if hashlib.sha256(rebuilt.read_bytes()).hexdigest() != ROM_SHA256:
        raise ValueError("baseline rebuilt ROM is not canonical")
    shutil.copyfile(rebuilt, materialized / "rebuilt.rom")
    manifest = AUTO.manifest_for(entries, len(rom)); manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO44 runtime count-bounded record regions"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto44-runtime-record-regions.v1", "regions": regions,
              "promoted_bytes": sum(end - start for start, end, _ in NEW_RANGES),
              "metrics": manifest["metrics"], "full_rom": {"size": len(rebuilt),
              "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
              "sha1": hashlib.sha1(rebuilt).hexdigest(), "sha256": hashlib.sha256(rebuilt).hexdigest()}}
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True); parser.add_argument("--manifest", required=True)
    parser.add_argument("--runtime-correlation", required=True); parser.add_argument("--runtime-evidence", required=True)
    parser.add_argument("--output", required=True); run(parser.parse_args())


if __name__ == "__main__":
    main()
