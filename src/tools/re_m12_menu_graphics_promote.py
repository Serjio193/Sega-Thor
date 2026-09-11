"""Promote three exact graphics streams used by the menu renderer."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import zlib


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("re_auto_promote", ROOT / "re_auto_promote.py")
AUTO = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUTO)

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
CONSUMERS = {
    0x004966: bytes.fromhex("41 F9 00 15 BA C2 30 3C 95 00 61 00 01 20"),
    0x004974: bytes.fromhex("41 F9 00 15 C2 38 30 3C A1 80 61 00 01 12"),
    0x004982: bytes.fromhex("41 F9 00 15 CA 9C 30 3C AE 00 61 00 01 04"),
}
STREAMS = (
    {"start": 0x15BAC2, "source_consumed": 1910, "consumer": 0x004966,
     "decompressed_bytes": 3200},
    {"start": 0x15C238, "source_consumed": 2147, "consumer": 0x004974,
     "decompressed_bytes": 3200},
    {"start": 0x15CA9C, "source_consumed": 1028, "consumer": 0x004982,
     "decompressed_bytes": 2656},
)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, stream):
    start = stream["start"]
    end = start + stream["source_consumed"]
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            replacement.append({
                "start": start, "end": end,
                "kind": "LOCAL_ROM_DERIVED_ASSET",
                "source": "M12_AUTO36_menu_graphics_consumers",
                "confidence": "CONFIRMED",
                "classification": "DIRECT_MENU_GRAPHICS_STREAM",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "exact 68000 LEA/BSR consumer and independent local decoder "
                    "report the source-consumed boundary"),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"menu stream overlaps non-UNKNOWN 0x{start:06X}")
    raise ValueError(f"menu stream is not wholly UNKNOWN: 0x{start:06X}..0x{end:06X}")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "DATA_KNOWN", "HEADER_VECTOR_ASM",
             "STRUCTURED_DATA_CONFIRMED", "PADDING_ALIGNMENT_CONFIRMED",
             "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count, "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def validate_contract(rom, decoder, rom_path, output):
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    for address, expected in CONSUMERS.items():
        if rom[address:address + len(expected)] != expected:
            raise ValueError(f"menu consumer contract changed at 0x{address:06X}")
    output.mkdir(parents=True)
    result = []
    for stream in STREAMS:
        image = output / f"{stream['start']:06X}.pgm"
        completed = subprocess.run(
            [str(decoder), str(rom_path), hex(stream["start"]), str(image)],
            text=True, capture_output=True, check=False)
        if completed.returncode:
            raise ValueError(f"decoder rejected 0x{stream['start']:06X}: "
                             f"{completed.stdout}{completed.stderr}")
        match = re.search(r"source_consumed=(\d+)", completed.stdout)
        size = re.search(r"decompressed_bytes=(\d+)", completed.stdout)
        if not match or int(match.group(1)) != stream["source_consumed"]:
            raise ValueError(f"decoder boundary changed at 0x{stream['start']:06X}")
        if not size or int(size.group(1)) != stream["decompressed_bytes"]:
            raise ValueError(f"decoder output changed at 0x{stream['start']:06X}")
        result.append({**stream, "end": stream["start"] + stream["source_consumed"]})
    return result


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    decoder_output = output / "decoder"
    streams = validate_contract(rom, Path(args.decoder).resolve(), rom_path, decoder_output)
    current = renumber(baseline["entries"])
    for stream in streams:
        current = split_unknown(current, stream)
    output.mkdir(parents=True, exist_ok=True)
    materialized = output / "materialized"
    entries = AUTO.materialize(materialized, current, rom, AUTO.source_map(current, baseline_path))
    matched, difference, reason, detail = AUTO.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO36 exact menu graphics streams"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto36-menu-graphics-promotion.v1",
              "streams": streams, "metrics": manifest["metrics"],
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
    parser.add_argument("--decoder", required=True)
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
