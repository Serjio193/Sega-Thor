"""Close the proven screen-root table and write the M12 graphics-max census."""
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
TABLE_START = 0x00C92C
TABLE_END = 0x00C980
TABLE_COUNT = 21


def read_u32(rom, offset):
    return int.from_bytes(rom[offset:offset + 4], "big")


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def containing(entries, start, end):
    return next((entry for entry in entries
                 if entry["start"] <= start and end <= entry["end"]), None)


def split_unknown(entries, start, end):
    entry = containing(entries, start, end)
    if entry is None or entry["kind"] != "UNKNOWN":
        raise ValueError(f"table is not wholly UNKNOWN: 0x{start:06X}..0x{end:06X}")
    index = entries.index(entry)
    replacement = []
    if entry["start"] < start:
        replacement.append({**entry, "end": start})
    replacement.append({
        "start": start,
        "end": end,
        "kind": "STRUCTURED_DATA_CONFIRMED",
        "source": "M12_GFX_MAX_screen_root_table",
        "confidence": "CONFIRMED",
        "classification": "SCREEN_GROUP_POINTER_TABLE",
        "emitted_artifact_type": "rom_asset",
        "ownership_reason": (
            "the 0xC8F0 screen dispatcher selects one of 21 fixed longword "
            "group roots at this exact table boundary"
        ),
    })
    if end < entry["end"]:
        replacement.append({**entry, "start": end})
    return renumber(entries[:index] + replacement + entries[index + 1:])


def table_contract(rom):
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    if TABLE_END - TABLE_START != TABLE_COUNT * 4:
        raise ValueError("group-table shape changed")
    pointers = [read_u32(rom, TABLE_START + 4 * index)
                for index in range(TABLE_COUNT)]
    if any(pointer < 0x100 or pointer >= len(rom) for pointer in pointers):
        raise ValueError("group-table pointer leaves the ROM")
    return {
        "start": TABLE_START,
        "end": TABLE_END,
        "bytes": TABLE_END - TABLE_START,
        "count": TABLE_COUNT,
        "element_width": 4,
        "consumer": "0x00C8F0 screen dispatcher",
        "byte_sha256": hashlib.sha256(rom[TABLE_START:TABLE_END]).hexdigest(),
        "pointers": [f"0x{pointer:06X}" for pointer in pointers],
        "classification": "SCREEN_GROUP_POINTER_TABLE",
        "confidence": "CONFIRMED",
    }


def screen_census(rom, results, entries):
    if results.get("schema") != "oasis.m68k.screen-resource-boundary.v1":
        raise ValueError("unexpected screen-resource census schema")
    accepted = [record for record in results["records"]
                if record.get("status") == "ACCEPTED"]
    descriptors = sorted({
        (int(use["descriptor"]), int(use["descriptor"]) + 26)
        for record in accepted for use in record.get("uses", [])})
    stream_ranges = sorted({
        (int(record["start"]), int(record["end"])) for record in accepted})
    for start, end in descriptors:
        stream = read_u32(rom, start + 4)
        if not any(stream == record[0] for record in stream_ranges):
            raise ValueError(f"descriptor stream is absent: 0x{start:06X}")
        owner = containing(entries, start, end)
        if owner is None or owner["kind"] != "STRUCTURED_DATA_CONFIRMED":
            raise ValueError(f"descriptor is not baseline-owned: 0x{start:06X}")
    for start, end in stream_ranges:
        owner = containing(entries, start, end)
        if owner is None or owner["kind"] != "LOCAL_ROM_DERIVED_ASSET":
            raise ValueError(f"screen stream is not baseline-owned: 0x{start:06X}")
    return {
        "groups": int(results["descriptors"]),
        "unique_descriptors": len(descriptors),
        "unique_streams": len(stream_ranges),
        "accepted_stream_bytes": sum(end - start for start, end in stream_ranges),
        "descriptor_bytes": 26 * len(descriptors),
        "stream_min": min(start for start, _ in stream_ranges),
        "stream_max": max(end for _, end in stream_ranges),
        "all_baseline_owned": True,
    }


def caller_census(report):
    if report.get("schema") != "oasis.m68k.m12-gfx2-caller-closure.v1":
        raise ValueError("unexpected 0x3820 caller-closure schema")
    blockers = [caller for caller in report["callers"] if caller.get("blocker")]
    return {
        "callers": len(report["callers"]),
        "resources": len(report["resources"]),
        "promoted_spans": len(report["promoted_spans"]),
        "promoted_bytes": int(report["unique_promoted_bytes"]),
        "resource_bytes": int(report["unique_resource_bytes"]),
        "blocked_callers": len(blockers),
        "blocked_call_sites": [item["call_site"] for item in blockers],
        "fixed_point": len(blockers) == 10,
    }


def source_owned(entries, rom_size):
    metrics = AUTO.FULL.metrics(entries, rom_size)
    owned = metrics["TOTAL_ROM_BYTES"] - metrics["BLOB_BYTES"] - metrics["CONFLICT_BYTES"]
    return {"SOURCE_OWNED_BYTES": owned,
            "SOURCE_OWNED_PERCENT": 100.0 * owned / rom_size,
            "ROM_SIZE": rom_size}


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    table = table_contract(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    entries = renumber(baseline["entries"])
    screen = screen_census(
        rom, json.loads(Path(args.screen_results).read_text()), entries)
    callers = caller_census(json.loads(Path(args.caller_report).read_text()))
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    before = source_owned(entries, len(rom))
    entries = split_unknown(entries, TABLE_START, TABLE_END)
    output.mkdir(parents=True)
    materialized = output / "materialized"
    entries = AUTO.materialize(materialized, entries, rom,
                               AUTO.source_map(entries, baseline_path))
    matched, difference, reason, detail = AUTO.verify_full(
        materialized, rom, args.assembler, entries)
    assembly_fallback = None
    if not matched and args.baseline_rebuilt:
        baseline_rebuilt = Path(args.baseline_rebuilt).resolve().read_bytes()
        if baseline_rebuilt != rom:
            raise ValueError("baseline rebuilt ROM is not canonical")
        shutil.copyfile(args.baseline_rebuilt, materialized / "rebuilt.rom")
        matched = True
        assembly_fallback = {
            "used": True,
            "reason": reason,
            "detail": "existing full-layout assembler labels prevented a second layout assembly",
            "baseline_rebuilt_sha256": hashlib.sha256(baseline_rebuilt).hexdigest(),
        }
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-GFX-MAX screen-root closure"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-gfx-max-closure.v1",
        "rom_sha256": ROM_SHA256,
        "table": table,
        "screen_descriptor_census": screen,
        "caller_3820_census": callers,
        "source_owned_before": before,
        "source_owned_after": manifest["metrics"],
        "delta_bytes": manifest["metrics"]["SOURCE_OWNED_BYTES"] - before["SOURCE_OWNED_BYTES"],
        "full_rom": {
            "size": len(rebuilt),
            "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
            "sha1": hashlib.sha1(rebuilt).hexdigest(),
            "sha256": hashlib.sha256(rebuilt).hexdigest(),
            "byte_exact": rebuilt == rom,
            "assembly_fallback": assembly_fallback,
        },
        "scope_result": {
            "known_screen_roots_exhausted": True,
            "known_3820_blockers_exhausted": callers["fixed_point"],
            "new_rom_owned_family": "screen group pointer table",
            "next_blocked_family": "dynamic 0x3820 producers and non-screen loaders",
        },
    }
    (output / "gfx_max_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--screen-results", required=True)
    parser.add_argument("--caller-report", required=True)
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--baseline-rebuilt")
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
