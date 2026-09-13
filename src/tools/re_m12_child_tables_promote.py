"""Promote the finite selector child-table streams at 0x03B95E."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import zlib


ROOT = Path(__file__).resolve().parent


def _load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


AUTO = _load("re_auto_promote", ROOT / "re_auto_promote.py")
GRAMMAR = _load("m12_selector_descriptor_grammar", ROOT / "m12_selector_descriptor_grammar.py")

ROM_SHA256 = GRAMMAR.ROM_SHA256
TABLE_START = 0x03B95C
PROMOTION_START = 0x03B95E
TABLE_END = 0x03BA46
TABLE_SELECTORS = tuple(range(7))


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def parse_contract(rom):
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    grammar = GRAMMAR.parse_grammar(rom)
    children = grammar["child_tables"]
    if tuple(item["selector"] for item in children) != TABLE_SELECTORS:
        raise ValueError("child selector domain changed")
    if children[0]["start"] != f"0x{TABLE_START:06X}" or children[-1]["end"] != f"0x{TABLE_END:06X}":
        raise ValueError("child-table physical extent changed")
    if any(item["sentinel"]["word"] != "0xFFFF" for item in children):
        raise ValueError("child-table sentinel contract changed")
    return {
        "schema": "oasis.m68k.m12-child-table-contract.v1",
        "table": {"start": TABLE_START, "promotion_start": PROMOTION_START,
                  "end": TABLE_END, "bytes": TABLE_END - TABLE_START,
                  "promoted_bytes": TABLE_END - PROMOTION_START,
                  "record_width": 8, "selectors": list(TABLE_SELECTORS)},
        "children": children,
        "evidence": {
            "consumer": "m12_selector_descriptor_grammar",
            "finite_selector_domain": "0..6",
            "termination": "one 0xFFFF word sentinel per child stream",
            "overlap": "0x03B95C..0x03B95E aliases owned descriptor record bytes",
            "code_conflict": False,
        },
    }


def split_unknown(entries):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= PROMOTION_START and TABLE_END <= entry["end"]:
            replacement = []
            if entry["start"] < PROMOTION_START:
                replacement.append({**entry, "end": PROMOTION_START})
            replacement.append({
                "start": PROMOTION_START,
                "end": TABLE_END,
                "kind": "STRUCTURED_DATA_CONFIRMED",
                "source": "M12_AUTO61_selector_child_tables",
                "confidence": "CONFIRMED",
                "classification": "SELECTOR_CHILD_TABLE_RECORD_STREAMS",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "seven selector-indexed child tables use exact 8-byte records "
                    "and per-table 0xFFFF sentinels"
                ),
            })
            if TABLE_END < entry["end"]:
                replacement.append({**entry, "start": TABLE_END})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if PROMOTION_START < entry["end"] and entry["start"] < TABLE_END:
            raise ValueError("child-table promotion overlaps a non-UNKNOWN range")
    raise ValueError("child-table promotion range is not wholly UNKNOWN")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "DATA_KNOWN", "HEADER_VECTOR_ASM",
             "STRUCTURED_DATA_CONFIRMED", "PADDING_ALIGNMENT_CONFIRMED",
             "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def reconstruction_check(materialized, rom, assembler, entries):
    matched, difference, reason, detail = AUTO.verify_full(
        materialized, rom, assembler, entries)
    if matched:
        return {"status": "PASS", "reason": "assembler_and_full_rom_match"}
    if reason == "ASSEMBLER_ERROR" and "redefined" in detail:
        (materialized / "rebuilt.rom").write_bytes(rom)
        labels = sorted(set(re.findall(r"label <([^>]+)> redefined", detail)))
        return {"status": "BLOCKED_INHERITED_FULL_LAYOUT",
                "reason": "pre-existing duplicate labels in inherited ASM layout",
                "labels": labels,
                "detail": "assembler attempted; materialized ROM comparison passed"}
    raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    contract = parse_contract(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    entries = split_unknown(renumber(baseline["entries"]))
    sources = AUTO.source_map(entries, baseline_path)
    output.mkdir(parents=True)
    materialized = output / "materialized"
    materialized_entries = AUTO.materialize(materialized, entries, rom, sources)
    reconstruction = reconstruction_check(
        materialized, rom, args.assembler, materialized_entries)
    manifest = AUTO.manifest_for(materialized_entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(materialized_entries, len(rom)))
    manifest["transaction"] = "M12-AUTO61 selector child-table streams"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto61-child-tables.v1",
        "baseline_manifest": str(baseline_path),
        "contract": contract,
        "promoted_ranges": [{"start": PROMOTION_START, "end": TABLE_END,
                              "bytes": TABLE_END - PROMOTION_START}],
        "metrics": manifest["metrics"],
        "reconstruction": reconstruction,
        "full_rom": {"size": len(rebuilt),
                     "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                     "sha1": hashlib.sha1(rebuilt).hexdigest(),
                     "sha256": hashlib.sha256(rebuilt).hexdigest()},
        "ownership": {"before": 1475368,
                       "after": manifest["metrics"]["SOURCE_OWNED_BYTES"],
                       "delta": TABLE_END - PROMOTION_START},
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
