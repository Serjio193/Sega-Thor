"""Promote word-copy tables closed by the exact 68000 copy loop."""
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
ROUTINE = 0x002D66
ROUTINE_BYTES = bytes.fromhex("48E7011042471E1E47F900FF134CD6C71E1E36DE51CFFFFC4CDF08804E75")
TABLES = (
    (0x000472, 0x0004B4, 0x0089C4, bytes.fromhex("4DF9000004726100A38C")),
    (0x0031FC, 0x003218, 0x003260, bytes.fromhex("4DF9000031FC6100FAF0")),
)


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
                "source": "M12_AUTO59_bounded_word_copy",
                "confidence": "CONFIRMED",
                "classification": "BOUNDED_WORD_COPY_TABLE",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "exact routine 0x002D66 consumes the two-byte header and then "
                    "copies exactly D7+1 big-endian words; the header-derived end "
                    "closes each table independently"
                ),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"word-copy table overlaps non-UNKNOWN 0x{entry['start']:06X}")
    raise ValueError(f"word-copy table is not wholly UNKNOWN 0x{start:06X}..0x{end:06X}")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET", "DATA_KNOWN"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def parse_contract(rom, verify_identity=True):
    if verify_identity and (len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256):
        raise ValueError("canonical ROM identity mismatch")
    if rom[ROUTINE:ROUTINE + len(ROUTINE_BYTES)] != ROUTINE_BYTES:
        raise ValueError("word-copy routine contract changed")
    tables = []
    for start, end, caller, caller_bytes in TABLES:
        if rom[caller:caller + len(caller_bytes)] != caller_bytes:
            raise ValueError(f"word-copy caller contract changed at 0x{caller:06X}")
        offset = rom[start]
        words = rom[start + 1]
        computed_end = start + 2 + 2 * (words + 1)
        if computed_end != end:
            raise ValueError(f"word-copy bound changed at 0x{start:06X}")
        tables.append({"start": start, "end": end, "header_offset": offset,
                       "word_count": words + 1, "caller": caller})
    return {"routine": ROUTINE, "tables": tables}


def run(args):
    rom = Path(args.rom).resolve().read_bytes()
    contract = parse_contract(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    for start, end, _, _ in TABLES:
        current = split_unknown(current, start, end)
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
    manifest["transaction"] = "M12-AUTO59 bounded word-copy tables"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto59-bounded-word-copy.v1", "contract": contract,
              "promoted_bytes": sum(end - start for start, end, _, _ in TABLES),
              "metrics": manifest["metrics"],
              "full_rom": {"size": len(rebuilt), "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                           "sha1": hashlib.sha1(rebuilt).hexdigest(),
                           "sha256": hashlib.sha256(rebuilt).hexdigest()}}
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
