"""Promote exact sequential graphics chains from closed 68000 consumers."""

import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parent))
import re_m12_direct_graphics_promote as previous


ROM_SHA256 = previous.ROM_SHA256
AUTO = previous.AUTO
CONSUMERS = {
    0x03C074: bytes.fromhex("41F90017216826494EB900003820"),
    0x03C268: bytes.fromhex("41F9001894EA43F900FF3FAC24494EB900003820"),
    0x03C27C: bytes.fromhex("26494EB900003820"),
    0x03C284: bytes.fromhex("28494EB900003820"),
    0x03C5BC: bytes.fromhex("41F90018CF9843F900FF3FAC24494EB900003820"),
    0x03C5D0: bytes.fromhex("26494EB900003820"),
    0x03C5D8: bytes.fromhex("28494EB900003820"),
    0x03C5E0: bytes.fromhex("23C900FF17A24EB900003820"),
}
ANCHORS = (
    {"start": 0x172168, "end": 0x1742DC, "consumer": 0x03C07C, "source_consumed": 8564},
    {"start": 0x1894EA, "end": 0x18955A, "consumer": 0x03C276, "source_consumed": 112},
    {"start": 0x18CF98, "end": 0x18D01B, "consumer": 0x03C5CA, "source_consumed": 131},
)
STREAMS = (
    {"start": 0x18955A, "end": 0x18A4BE, "consumer": 0x03C27E, "source_consumed": 3940},
    {"start": 0x18A4BE, "end": 0x18CC6E, "consumer": 0x03C286, "source_consumed": 10160},
    {"start": 0x18D01B, "end": 0x18D727, "consumer": 0x03C5D2, "source_consumed": 1804},
    {"start": 0x18D727, "end": 0x18EA9A, "consumer": 0x03C5DA, "source_consumed": 4979},
    {"start": 0x18EA9A, "end": 0x18EB1F, "consumer": 0x03C5E6, "source_consumed": 133},
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
                "source": "M12_AUTO8_direct_graphics_chain",
                "confidence": "CONFIRMED",
                "classification": "DIRECT_GRAPHICS_STREAM",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "exact 68000 sequential graphics consumer and decoder-advanced A0; "
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
    return {"SOURCE_OWNED_BYTES": count, "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def validate_contract(rom, decoder, rom_path, output):
    for start, expected in CONSUMERS.items():
        if rom[start:start + len(expected)] != expected:
            raise ValueError(f"direct chain contract changed at 0x{start:06X}")
    output.mkdir(parents=True, exist_ok=True)
    result = []
    for stream in ANCHORS + STREAMS:
        if stream["end"] - stream["start"] != stream["source_consumed"]:
            raise ValueError("stream geometry is internally inconsistent")
        completed = subprocess.run(
            [str(decoder), str(rom_path), hex(stream["start"]),
             str(output / f"{stream['start']:06X}.pgm")],
            text=True, capture_output=True, check=False)
        if completed.returncode:
            raise ValueError(f"decoder rejected 0x{stream['start']:06X}: "
                             f"{completed.stdout}{completed.stderr}")
        match = re.search(r"source_consumed=(\d+)", completed.stdout)
        if not match or int(match.group(1)) != stream["source_consumed"]:
            raise ValueError(f"decoder boundary changed at 0x{stream['start']:06X}")
        result.append(stream)
    return result[:len(ANCHORS)], result[len(ANCHORS):]


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
    anchors, streams = validate_contract(
        rom, Path(args.decoder).resolve(), rom_path, output / "decoder")
    current = renumber(baseline["entries"])
    for stream in streams:
        current = split_unknown(current, stream["start"], stream["end"])
    materialized = output / "materialized"
    entries = AUTO.materialize(materialized, current, rom,
                               previous.source_map(current, baseline_path.parent))
    matched, difference, reason, detail = AUTO.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO8 direct graphics chains"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto8-direct-graphics-chain.v1",
              "metrics": manifest["metrics"], "anchors": anchors, "streams": streams,
              "promoted_stream_bytes": sum(item["end"] - item["start"] for item in streams),
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
