"""Promote exact ROM callback entrypoints reached through field 86(A6)."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import zlib


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("re_auto_promote", ROOT / "re_auto_promote.py")
AUTO = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUTO)

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
CALLBACKS = (
    (0x010000, 0x010034, "field 86(A6) is initialized to 0x10000 and dispatched by JSR (A0)"),
    (0x030000, 0x030002, "field 86(A6) is initialized to 0x30000 and dispatches an RTS stub"),
)
CALLBACK_ENDS = {start: end for start, end, _ in CALLBACKS}
ASM = {
    0x010000: """    org $10000
sub_010000:
    move.w (A0)+,8(A6)
    move.w (A0)+,12(A6)
    move.w (A0)+,24(A6)
    move.w (A0)+,78(A6)
    move.w (A0)+,82(A6)
    move.w (A0)+,84(A6)
    bsr.w *-$6542
    move.w D4,20(A6)
    move.l 86(A6),D0
    beq.s loc_010032
    movem.l A0,-(A7)
    movea.l D0,A0
    jsr (A0)
    movem.l (A7)+,A0
loc_010032:
    rts
""",
    0x030000: """    org $30000
sub_030000:
    rts
""",
}


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end, reason):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            replacement.append({"start": start, "end": end, "kind": "CODE_VERIFIED",
                                "source": "M12_AUTO26_field86_callbacks",
                                "confidence": "CONFIRMED",
                                "classification": "INDIRECT_CALLBACK_ENTRYPOINT",
                                "trust_level": "EXACT_INDIRECT_CALLBACK",
                                "emitted_artifact_type": "asm",
                                "ownership_reason": reason})
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError("callback overlaps existing ownership")
    raise ValueError(f"callback is not wholly UNKNOWN: 0x{start:06X}..0x{end:06X}")


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    manifest_path = Path(args.manifest).resolve()
    baseline = json.loads(manifest_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    for start, end, reason in CALLBACKS:
        current = split_unknown(current, start, end, reason)
    output.mkdir(parents=True)
    staging = output / "staging"
    staging.mkdir()
    sources = {}
    for start, _, _ in CALLBACKS:
        path = staging / f"sub_{start:06X}.asm"
        path.write_text(ASM[start])
        binary = staging / f"sub_{start:06X}.bin"
        result = subprocess.run([args.assembler, "-m68000", "-no-opt", "-Fbin",
                                 "-o", str(binary), str(path)], capture_output=True)
        if result.returncode or binary.read_bytes() != rom[start:CALLBACK_ENDS[start]]:
            raise ValueError(f"callback vasm round-trip failed at 0x{start:06X}")
        sources[start] = path
    materialized = output / "materialized"
    source_map = {index: sources[entry["start"]] for index, entry in enumerate(current)
                  if entry["start"] in sources}
    for index, entry in enumerate(current):
        if entry.get("emitted_artifact_type") == "asm" and index not in source_map:
            source_map[index] = AUTO.resolve_artifact(manifest_path, entry["artifact"])
    entries = AUTO.materialize(materialized, current, rom, source_map)
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
    manifest["transaction"] = "M12-AUTO26 exact field-86 callback entrypoints"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto26-callback-promotion.v1",
              "callbacks": [{"start": s, "end": e, "reason": r}
                            for s, e, r in CALLBACKS], "metrics": manifest["metrics"],
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
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
