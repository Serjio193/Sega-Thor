"""Promote graphics streams observed by the canonical runtime decoder."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import re
import zlib


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("re_auto_promote", ROOT / "re_auto_promote.py")
AUTO = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUTO)

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
DECODER_PC = "0x003830"
RUNTIME_STREAMS = (
    {"start": 0x15E052, "end": 0x160E19, "pointer_refs": (0x03F60A, 0x043880, 0x043B6A, 0x0461E6, 0x046224),
     "reason": "canonical runtime reader, local decoder boundary, and static pointer literals agree"},
    {"start": 0x2119D2, "end": 0x211F79, "pointer_refs": (0x02E1DC,),
     "reason": "canonical runtime reader, local decoder boundary, and static pointer literal agree"},
)
STATIC_STREAMS = (
    {"start": 0x167E48, "end": 0x16821F, "pointer_refs": (0x02D446, 0x02E206, 0x03F58A),
     "reason": "exact LEA consumers call D9A4, whose D9A4->37D2->3820 chain is byte-verified"},
    {"start": 0x168442, "end": 0x168492, "pointer_refs": (0x02D418,),
     "reason": "exact LEA consumer calls D9A4, whose D9A4->37D2->3820 chain is byte-verified"},
)
STREAMS = RUNTIME_STREAMS + STATIC_STREAMS
NEW_STREAMS = STATIC_STREAMS
STATIC_CONSUMERS = {
    0x02D444: bytes.fromhex("4DF900167E48 3C3C50C0 4EB90000D9A4"),
    0x02E204: bytes.fromhex("4DF900167E48 3C3C4000 4EB90000D9A4"),
    0x02D416: bytes.fromhex("4DF900168442 3C3C5000 4EB90000D9A4"),
    0x00D9B2: bytes.fromhex("4EB9000037D2"),
    0x0037D8: bytes.fromhex("61000046"),
}


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, stream):
    start, end = stream["start"], stream["end"]
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            replacement.append({
                "start": start, "end": end, "kind": "LOCAL_ROM_DERIVED_ASSET",
                "source": "M12_AUTO17_runtime_graphics_decoder",
                "confidence": "CONFIRMED",
                "classification": "EXACT_3820_GRAPHICS_STREAM",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": stream["reason"],
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"stream overlaps non-UNKNOWN 0x{entry['start']:06X}")
    raise ValueError(f"stream is not wholly UNKNOWN 0x{start:06X}..0x{end:06X}")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count, "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def validate_runtime(correlation, execution):
    regions = {int(item["region_start"], 16): item for item in correlation["regions"]}
    facts = {item["address"]: item for item in execution["execution_facts"]}
    if facts.get(DECODER_PC, {}).get("evidence_type") != "CODE_EXECUTED_AT_ADDRESS":
        raise ValueError("runtime evidence does not execute the exact decoder PC")
    result = []
    for stream in RUNTIME_STREAMS:
        region = regions.get(stream["start"])
        if region is None or int(region["region_end"], 16) + 1 != stream["end"]:
            raise ValueError(f"runtime boundary changed at 0x{stream['start']:06X}")
        if region["first_reader_pc"] != DECODER_PC or region["observed_bytes"] != stream["end"] - stream["start"]:
            raise ValueError(f"runtime reader contract changed at 0x{stream['start']:06X}")
        result.append({"start": stream["start"], "end": stream["end"],
                       "runtime_reader": int(DECODER_PC, 16),
                       "observed_bytes": region["observed_bytes"],
                       "pointer_refs": list(stream["pointer_refs"])})
    return result


def validate_static_consumers(rom):
    for address, expected in STATIC_CONSUMERS.items():
        if rom[address:address + len(expected)] != expected:
            raise ValueError(f"static graphics consumer contract changed at 0x{address:06X}")
    return [{"address": address, "bytes": expected.hex().upper()}
            for address, expected in STATIC_CONSUMERS.items()]


def validate_decoder(rom_path, decoder, output):
    output.mkdir(parents=True, exist_ok=True)
    result = []
    for stream in STREAMS:
        image = output / f"{stream['start']:06X}.pgm"
        completed = subprocess.run(
            [str(decoder), str(rom_path), hex(stream["start"]), str(image)],
            text=True, capture_output=True, check=False)
        if completed.returncode:
            raise ValueError(f"decoder rejected 0x{stream['start']:06X}: {completed.stdout}{completed.stderr}")
        match = re.search(r"source_consumed=(\d+)", completed.stdout)
        expected = stream["end"] - stream["start"]
        if not match or int(match.group(1)) != expected:
            raise ValueError(f"decoder boundary changed at 0x{stream['start']:06X}")
        result.append({"start": stream["start"], "end": stream["end"],
                       "source_consumed": expected, "decoder_output": str(image)})
    return result


def validate_pointers(rom):
    result = []
    for stream in STREAMS:
        expected = stream["start"].to_bytes(4, "big")
        for address in stream["pointer_refs"]:
            if rom[address:address + 4] != expected:
                raise ValueError(f"pointer contract changed at 0x{address:06X}")
        result.append({"start": stream["start"], "pointer_refs": list(stream["pointer_refs"])})
    return result


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    output.mkdir(parents=True)
    runtime = validate_runtime(
        json.loads(Path(args.runtime_correlation).read_text()),
        json.loads(Path(args.runtime_execution).read_text()))
    decoded = validate_decoder(rom_path, Path(args.decoder).resolve(), output / "decoder")
    static_consumers = validate_static_consumers(rom)
    pointers = validate_pointers(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    current = renumber(baseline["entries"])
    for stream in NEW_STREAMS:
        current = split_unknown(current, stream)
    materialized = output / "materialized"
    sources = {index: baseline_path.parent / entry["artifact"]
               for index, entry in enumerate(current)
               if entry.get("emitted_artifact_type") == "asm"}
    entries = AUTO.materialize(materialized, current, rom, sources)
    baseline_rebuilt = baseline_path.parent / "rebuilt.rom"
    rebuilt = baseline_rebuilt.read_bytes()
    if hashlib.sha256(rebuilt).hexdigest() != ROM_SHA256:
        raise ValueError("baseline rebuilt ROM is not canonical")
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO18 exact 0x3820 graphics consumers"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto18-graphics-consumers.v1",
              "streams": runtime, "static_consumers": static_consumers,
              "decoder": decoded, "pointers": pointers,
              "metrics": manifest["metrics"],
              "promoted_stream_bytes": sum(item["end"] - item["start"] for item in NEW_STREAMS),
              "full_rom": {"size": len(rebuilt), "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                           "sha1": hashlib.sha1(rebuilt).hexdigest(),
                           "sha256": hashlib.sha256(rebuilt).hexdigest()}}
    (output / "promotion_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--runtime-correlation", required=True)
    parser.add_argument("--runtime-execution", required=True)
    parser.add_argument("--decoder", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
