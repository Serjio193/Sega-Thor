"""Promote exact direct graphics streams from bounded 68000 consumers."""
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
    0x004E0: bytes.fromhex("41 F9 00 15 23 40 28 49 61 00 33 36"),
    0x004E8: bytes.fromhex("61 00 33 36"),
    0x004F2: bytes.fromhex("61 00 33 2C"),
    0x004FC: bytes.fromhex("61 00 33 22"),
    0x0050A: bytes.fromhex("61 00 33 14"),
    0x00326A: bytes.fromhex("41 F9 00 15 00 00 43 F9 00 FF 2F A8 24 49 61 00 05 A6"),
    0x003356: bytes.fromhex("61 00 04 C8"),
    0x00335C: bytes.fromhex("61 00 04 C2"),
}
STREAMS = (
    {"start": 0x150000, "end": 0x1503D3, "consumer": 0x003278, "source_consumed": 979},
    {"start": 0x152340, "end": 0x152BA1, "consumer": 0x004E8, "source_consumed": 2145},
    {"start": 0x152BA1, "end": 0x152C57, "consumer": 0x004F2, "source_consumed": 182},
    {"start": 0x152C57, "end": 0x1532FB, "consumer": 0x004FC, "source_consumed": 1700},
    {"start": 0x1532FB, "end": 0x15335A, "consumer": 0x0050A, "source_consumed": 95},
    {"start": 0x180C56, "end": 0x180C9F, "consumer": 0x003356, "source_consumed": 73},
    {"start": 0x180C9F, "end": 0x1894EA, "consumer": 0x00335C, "source_consumed": 34891},
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
                "start": start, "end": end, "kind": "LOCAL_ROM_DERIVED_ASSET",
                "source": "M12_AUTO7_direct_graphics_consumers",
                "confidence": "CONFIRMED",
                "classification": "DIRECT_GRAPHICS_STREAM",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "exact 68000 LEA/BSR consumer or decoder-advanced A0 continuation; "
                    "local decoder reports the exact source-consumed boundary"),
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"stream overlaps non-UNKNOWN 0x{entry['start']:06X}.."
                             f"0x{entry['end']:06X}")
    raise ValueError(f"stream is not wholly UNKNOWN: 0x{start:06X}..0x{end:06X}")


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


def validate_contract(rom, decoder, rom_path, output):
    for start, expected in CONSUMERS.items():
        if rom[start:start + len(expected)] != expected:
            raise ValueError(f"direct consumer contract changed at 0x{start:06X}")
    output.mkdir(parents=True, exist_ok=True)
    result = []
    for stream in STREAMS:
        if stream["end"] - stream["start"] != stream["source_consumed"]:
            raise ValueError("stream geometry is internally inconsistent")
        image = output / f"{stream['start']:06X}.pgm"
        completed = subprocess.run(
            [str(decoder), str(rom_path), hex(stream["start"]), str(image)],
            text=True, capture_output=True, check=False)
        if completed.returncode:
            raise ValueError(f"decoder rejected 0x{stream['start']:06X}: "
                             f"{completed.stdout}{completed.stderr}")
        match = re.search(r"source_consumed=(\d+)", completed.stdout)
        if not match or int(match.group(1)) != stream["source_consumed"]:
            raise ValueError(f"decoder boundary changed at 0x{stream['start']:06X}")
        result.append({"start": stream["start"], "end": stream["end"],
                       "consumer": stream["consumer"],
                       "source_consumed": stream["source_consumed"]})
    return result


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
    decoder_output = output / "decoder"
    streams = validate_contract(rom, Path(args.decoder).resolve(), rom_path, decoder_output)
    current = renumber(baseline["entries"])
    for stream in streams:
        current = split_unknown(current, stream["start"], stream["end"])
    output.mkdir(parents=True, exist_ok=True)
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
    manifest["transaction"] = "M12-AUTO7 direct graphics consumers"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto7-direct-graphics.v1",
        "metrics": manifest["metrics"],
        "streams": streams,
        "promoted_stream_bytes": sum(item["end"] - item["start"] for item in streams),
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
    parser.add_argument("--decoder", required=True)
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
