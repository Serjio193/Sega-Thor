"""Promote one caller-backed exact static code island from the M12 census."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import zlib


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("re_auto_promote", ROOT / "re_auto_promote.py")
AUTO = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUTO)

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
START = 0x0167BE
END = 0x01685A
CALLERS = (0x01672E, 0x016742)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def validate_contract(rom):
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    if START >= END or END > len(rom) or START & 1:
        raise ValueError("static island bounds are invalid")
    return {"start": START, "end": END, "bytes": END - START,
            "entry": START, "callers": list(CALLERS),
            "evidence": "Ghidra recognized function; two static caller xrefs; "
                        "exact range decoder and vasm round-trip"}


def promote(entries):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= START and END <= entry["end"]:
            replacement = []
            if entry["start"] < START:
                replacement.append({**entry, "end": START})
            replacement.append({
                "start": START, "end": END, "kind": "CODE_VERIFIED",
                "source": "M12_AUTO24_exact_static_caller_backed_island",
                "confidence": "CONFIRMED",
                "classification": "M12_EXACT_STATIC_ISLAND",
                "trust_level": "EXACT_STATIC_CALLER_BACKED",
                "emitted_artifact_type": "asm",
                "promotion_score": 0,
                "ownership_reason": (
                    "Ghidra function has two caller xrefs; bounded decoder emits "
                    "the complete range with no unsupported form or flow gap"
                ),
            })
            if END < entry["end"]:
                replacement.append({**entry, "start": END})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if START < entry["end"] and entry["start"] < END:
            raise ValueError("static island overlaps existing ownership")
    raise ValueError("static island is not wholly UNKNOWN")


def source_map(entries, baseline, generated):
    result = {}
    for index, entry in enumerate(entries):
        if entry["start"] == START:
            result[index] = generated
        elif entry.get("emitted_artifact_type") == "asm":
            result[index] = AUTO.resolve_artifact(baseline, entry["artifact"])
    return result


def normalize_asm(path):
    text = path.read_text()
    normalized = text.replace("    exg.w D4,A6", "    exg A4,A6")
    if normalized == text:
        raise ValueError("expected EXG operand normalization was not emitted")
    path.write_text(normalized)


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    island = validate_contract(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = promote(renumber(baseline["entries"]))
    output.mkdir(parents=True)
    staging = output / "staging"
    staging.mkdir()
    asm = staging / f"sub_{START:06X}.asm"
    decoded = staging / f"sub_{START:06X}.json"
    binary = staging / f"sub_{START:06X}.bin"
    emitted = AUTO.run([args.range_tool, rom_path, hex(START), hex(END), asm, decoded])
    if emitted.returncode:
        raise ValueError("exact range decode failed")
    normalize_asm(asm)
    assembled = AUTO.run([args.assembler, "-m68000", "-no-opt", "-Fbin", "-o", binary, asm])
    if assembled.returncode or binary.read_bytes() != rom[START:END]:
        raise ValueError("static island vasm round-trip failed")
    materialized = output / "materialized"
    entries = AUTO.materialize(materialized, current, rom,
                               source_map(current, baseline_path, asm))
    matched, difference, reason, detail = AUTO.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"]["SOURCE_OWNED_BYTES"] = sum(
        entry["size"] for entry in entries if entry["kind"] != "UNKNOWN")
    manifest["metrics"]["SOURCE_OWNED_PERCENT"] = (
        100.0 * manifest["metrics"]["SOURCE_OWNED_BYTES"] / len(rom))
    manifest["transaction"] = "M12-AUTO24 exact static caller-backed island"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto24-static-code.v1",
              "island": island, "metrics": manifest["metrics"],
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
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--range-tool", required=True)
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
