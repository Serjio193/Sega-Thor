"""Promote an exact bounded static 68000 routine with a direct caller."""
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
ROUTINE_START = 0x003260
ROUTINE_END = 0x0032E8
CALLER = 0x003240
CALLER_BYTES = bytes.fromhex("08F9000200FF164D0839000200FF164D66F64EB900002B2833FC0EEE00FF1366")
SOURCE = ROOT / "re_m12_static_003260.asm"
ROUNDTRIP_SOURCE = ROOT / "re_m12_static_003260_roundtrip.asm"


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
        if entry["start"] <= ROUTINE_START and ROUTINE_END <= entry["end"]:
            replacement = []
            if entry["start"] < ROUTINE_START:
                replacement.append({**entry, "end": ROUTINE_START})
            replacement.append({
                "start": ROUTINE_START, "end": ROUTINE_END,
                "kind": "CODE_VERIFIED",
                "source": "M12_AUTO58_static_003260",
                "confidence": "CONFIRMED",
                "classification": "STATIC_EXACT_BOUNDED_ROUTINE",
                "trust_level": "STATIC_EXACT_BOUNDED_ROUTINE",
                "emitted_artifact_type": "asm",
                "ownership_reason": (
                    "the exact 68000 source round-trips to the canonical bytes; "
                    "the routine has a direct caller and an explicit RTS boundary"
                ),
            })
            if ROUTINE_END < entry["end"]:
                replacement.append({**entry, "start": ROUTINE_END})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if ROUTINE_START < entry["end"] and entry["start"] < ROUTINE_END:
            raise ValueError("static routine overlaps a non-UNKNOWN range")
    raise ValueError("static routine is not wholly UNKNOWN")


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
    if rom[CALLER:CALLER + len(CALLER_BYTES)] != CALLER_BYTES:
        raise ValueError("static routine caller contract changed")
    if not SOURCE.exists():
        raise ValueError("exact ASM source is missing")
    if not ROUNDTRIP_SOURCE.exists():
        raise ValueError("round-trip ASM wrapper is missing")
    return {"routine_start": ROUTINE_START, "routine_end": ROUTINE_END,
            "caller": CALLER, "source": str(SOURCE)}


def run(args):
    rom = Path(args.rom).resolve().read_bytes()
    contract = parse_contract(rom)
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
    for index, entry in enumerate(current):
        if entry["start"] == ROUTINE_START:
            sources[index] = SOURCE
    entries = AUTO.materialize(materialized, current, rom, sources)
    baseline_rebuilt = baseline_path.parent / "rebuilt.rom"
    if baseline_rebuilt.read_bytes() != rom:
        raise ValueError("baseline rebuilt ROM is not canonical")
    roundtrip = output / "routine.bin"
    assembled = AUTO.run([args.assembler, "-m68000", "-no-opt", "-Fbin", "-o",
                          roundtrip, ROUNDTRIP_SOURCE], cwd=ROOT, check=False)
    roundtrip_bytes = roundtrip.read_bytes()
    routine_offset = ROUTINE_START - 0x2CBC
    if (assembled.returncode or len(roundtrip_bytes) < routine_offset + ROUTINE_END - ROUTINE_START
            or roundtrip_bytes[routine_offset:routine_offset + ROUTINE_END - ROUTINE_START]
            != rom[ROUTINE_START:ROUTINE_END]):
        raise ValueError("static routine isolated ASM does not round-trip to canonical bytes")
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO58 static 0x003260 routine"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto58-static-003260.v1", "contract": contract,
              "promoted_bytes": ROUTINE_END - ROUTINE_START, "metrics": manifest["metrics"],
              "full_rom": {"size": len(rebuilt), "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
                           "sha1": hashlib.sha1(rebuilt).hexdigest(),
                           "sha256": hashlib.sha256(rebuilt).hexdigest()}}
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
