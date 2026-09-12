"""Promote the remaining exact screen-family descriptor candidate."""
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
RECORD_START = 0x02E1D8
RECORD_SIZE = 0x16
CALL_SITE = RECORD_START + 0x1A
CALL_BYTES = bytes.fromhex("4EB90000D406")
STREAM = (0x2119D2, 0x211F79, 18712)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] > RECORD_START or entry["end"] < RECORD_START + RECORD_SIZE:
            continue
        replacement = []
        if entry["start"] < RECORD_START:
            replacement.append({**entry, "end": RECORD_START})
        replacement.append({
            "start": RECORD_START, "end": RECORD_START + RECORD_SIZE,
            "kind": "STRUCTURED_DATA_CONFIRMED",
            "source": "M12_GFX_MAX_SCREEN_DESCRIPTOR_CANDIDATE",
            "confidence": "CONFIRMED",
            "classification": "GRAPHICS_DESCRIPTOR_RECORD",
            "emitted_artifact_type": "rom_asset",
            "ownership_reason": (
                "same 22-byte screen descriptor family, exact +4 ROM pointer, "
                "screen resource IDs, and exact downstream graphics stream boundary"),
        })
        if RECORD_START + RECORD_SIZE < entry["end"]:
            replacement.append({**entry, "start": RECORD_START + RECORD_SIZE})
        return renumber(entries[:index] + replacement + entries[index + 1:])
    raise ValueError("candidate descriptor is not wholly UNKNOWN")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "DATA_KNOWN", "HEADER_VECTOR_ASM",
             "STRUCTURED_DATA_CONFIRMED", "PADDING_ALIGNMENT_CONFIRMED",
             "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def verify_contract(rom, census):
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    pointer = int.from_bytes(rom[RECORD_START + 4:RECORD_START + 8], "big")
    ids = tuple(rom[RECORD_START + 8:RECORD_START + 12])
    if pointer != STREAM[0] or any(value >= 108 for value in ids):
        raise ValueError("candidate descriptor fields do not match the closed family")
    if rom[CALL_SITE:CALL_SITE + len(CALL_BYTES)] != CALL_BYTES:
        raise ValueError("candidate does not have the exact D406 continuation")
    records = {int(item["start"]): item for item in census["records"]}
    stream = records.get(STREAM[0])
    if stream is None or int(stream["end"]) != STREAM[1]:
        raise ValueError("candidate stream boundary is not present in the exact census")
    if int(stream["decompressed_bytes"]) != STREAM[2]:
        raise ValueError("candidate stream output size changed")
    return {"record_start": RECORD_START, "record_end": RECORD_START + RECORD_SIZE,
            "call_site": CALL_SITE, "pointer": pointer, "resource_ids": list(ids),
            "stream_start": STREAM[0], "stream_end": STREAM[1],
            "stream_bytes": STREAM[1] - STREAM[0],
            "decompressed_bytes": STREAM[2]}


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    census = json.loads(Path(args.census).read_text())
    contract = verify_contract(rom, census)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = split_unknown(renumber(baseline["entries"]))
    output.mkdir(parents=True)
    materialized = output / "materialized"
    sources = {index: AUTO.resolve_artifact(baseline_path, entry["artifact"])
               for index, entry in enumerate(current)
               if entry.get("emitted_artifact_type") == "asm" and entry.get("artifact")}
    entries = AUTO.materialize(materialized, current, rom, sources)
    rebuilt = baseline_path.parent / "rebuilt.rom"
    if rebuilt.read_bytes() != rom:
        raise ValueError("baseline rebuilt ROM is not canonical")
    shutil.copyfile(rebuilt, materialized / "rebuilt.rom")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-GFX-MAX screen descriptor candidate"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt_bytes = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-gfx-max-screen-descriptor-candidate.v1",
        "contract": contract, "metrics": manifest["metrics"],
        "promoted_bytes": RECORD_SIZE,
        "full_rom": {"size": len(rebuilt_bytes),
                     "crc32": f"{zlib.crc32(rebuilt_bytes) & 0xFFFFFFFF:08X}",
                     "sha1": hashlib.sha1(rebuilt_bytes).hexdigest(),
                     "sha256": hashlib.sha256(rebuilt_bytes).hexdigest()},
    }
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
