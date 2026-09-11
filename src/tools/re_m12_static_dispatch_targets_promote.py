"""Promote the closed targets of the 0x03B092 static dispatch family."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import zlib


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("re_auto_promote", ROOT / "re_auto_promote.py")
AUTO = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUTO)

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
CALLER = 0x03ADAC
DISPATCHER = (0x03B092, 0x03B0AA)
TABLE = (0x03B0AA, 0x03B0BA)
RETURN_TARGET = (0x03B0BA, 0x03B0BC)
TARGETS = ((0x03B0BC, 0x03B0E8), (0x03B0E8, 0x03B132),
           (0x03B132, 0x03B188))
CODE_RANGES = TARGETS + (RETURN_TARGET,)
CALLER_BYTES = bytes.fromhex("4EB90003B092")
DISPATCHER_BYTES = bytes.fromhex("303900FFAFB002400003D040D04041FA0008207000004ED0")
TABLE_VALUES = (0x03B0BC, 0x03B0E8, 0x03B132, 0x03B0BA)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end, kind, source, reason):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            replacement.append({
                "start": start,
                "end": end,
                "kind": kind,
                "source": source,
                "confidence": "CONFIRMED",
                "classification": "STATIC_DISPATCH_TARGET" if kind == "CODE_VERIFIED"
                else "STATIC_DISPATCH_POINTER_TABLE",
                "emitted_artifact_type": "asm" if kind == "CODE_VERIFIED" else "rom_asset",
                "ownership_reason": reason,
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"range overlaps non-UNKNOWN 0x{entry['start']:06X}.."
                             f"0x{entry['end']:06X}")
    raise ValueError(f"range is not wholly UNKNOWN: 0x{start:06X}..0x{end:06X}")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "DATA_KNOWN", "HEADER_VECTOR_ASM",
             "STRUCTURED_DATA_CONFIRMED", "PADDING_ALIGNMENT_CONFIRMED",
             "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def parse_contract(rom):
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    if rom[CALLER:CALLER + len(CALLER_BYTES)] != CALLER_BYTES:
        raise ValueError("static caller contract changed")
    if rom[DISPATCHER[0]:DISPATCHER[1]] != DISPATCHER_BYTES:
        raise ValueError("dispatcher contract changed")
    values = tuple(int.from_bytes(rom[address:address + 4], "big")
                   for address in range(TABLE[0], TABLE[1], 4))
    if values != TABLE_VALUES:
        raise ValueError("dispatch pointer table changed")
    return {"caller": CALLER, "dispatcher": DISPATCHER, "table": TABLE,
            "table_values": values, "return_target": RETURN_TARGET,
            "targets": [{"start": start, "end": end, "bytes": end - start}
                        for start, end in TARGETS],
            "reason": "direct JSR plus &3 selector and four in-family table targets"}


def table_source(path):
    lines = ["; Exact static dispatch table; targets are all inside the family.",
             "    org $03B0AA", "dispatch_03B0AA:"]
    lines.append("    dc.l " + ",".join(f"${value:08X}" for value in TABLE_VALUES))
    path.write_text("\n".join(lines) + "\n")


def dispatcher_source(path):
    path.write_text("\n".join([
        "; Exact dispatcher; indirect jump is bounded by the following four-entry table.",
        "    org $03B092",
        "dispatch_03B092:",
        "    move.w ($00FFAFB0).L,D0",
        "    andi.w #$3,D0",
        "    add.w D0,D0",
        "    add.w D0,D0",
        "    lea.l ($0008,PC),A0",
        "    movea.l 0(A0,D0.W),A0",
        "    jmp.l (A0)",
        ""]) + "\n")


def assemble_source(path, start, end, rom_path, assembler):
    binary = path.with_suffix(".bin")
    assembled = subprocess.run(
        [assembler, "-m68000", "-no-opt", "-Fbin", "-o", str(binary), str(path)],
        text=True, capture_output=True, check=False)
    if assembled.returncode or binary.read_bytes() != rom_path.read_bytes()[start:end]:
        raise ValueError(f"exact source round-trip failed at 0x{start:06X}: "
                         f"{assembled.stdout}{assembled.stderr}")
    return path


def assemble_range(args, rom_path, start, end, staging):
    asm = staging / f"sub_{start:06X}.asm"
    decoded = staging / f"sub_{start:06X}.json"
    binary = staging / f"sub_{start:06X}.bin"
    emitted = subprocess.run(
        [args.range_tool, str(rom_path), hex(start), hex(end), str(asm), str(decoded)],
        text=True, capture_output=True, check=False)
    if emitted.returncode:
        raise ValueError(f"exact range decode failed at 0x{start:06X}: "
                         f"{emitted.stdout}{emitted.stderr}")
    assembled = subprocess.run(
        [args.assembler, "-m68000", "-no-opt", "-Fbin", "-o", str(binary), str(asm)],
        text=True, capture_output=True, check=False)
    if assembled.returncode or binary.read_bytes() != rom_path.read_bytes()[start:end]:
        raise ValueError(f"exact target round-trip failed at 0x{start:06X}: "
                         f"{assembled.stdout}{assembled.stderr}")
    return asm


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    contract = parse_contract(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    dispatcher = split_unknown(
        current, *DISPATCHER, "CODE_VERIFIED", "M12_AUTO46_static_dispatch_targets",
        "direct caller, exact &3 dispatcher, bounded table and vasm round-trip")
    current = dispatcher
    current = split_unknown(current, *TABLE, "STRUCTURED_DATA_CONFIRMED",
                            "M12_AUTO46_static_dispatch_targets",
                            "bounded four-entry table selected by the exact dispatcher")
    current = split_unknown(current, *RETURN_TARGET, "CODE_VERIFIED",
                            "M12_AUTO46_static_dispatch_targets",
                            "fourth exact dispatch target is the in-family RTS terminal")
    for start, end in TARGETS:
        current = split_unknown(current, start, end, "CODE_VERIFIED",
                                "M12_AUTO46_static_dispatch_targets",
                                "direct caller, in-family dispatch target, exact decoder and vasm round-trip")
    output.mkdir(parents=True)
    staging = output / "staging"
    staging.mkdir()
    dispatcher_path = staging / "dispatch_03B092.asm"
    dispatcher_source(dispatcher_path)
    assemble_source(dispatcher_path, *DISPATCHER, rom_path, args.assembler)
    table = staging / "dispatch_03B0AA.asm"
    table_source(table)
    sources = {}
    for index, entry in enumerate(current):
        if (entry.get("emitted_artifact_type") == "asm" and
                entry["start"] not in {start for start, _ in CODE_RANGES} | {DISPATCHER[0]}):
            sources[index] = AUTO.resolve_artifact(baseline_path, entry["artifact"])
    sources[next(i for i, e in enumerate(current) if e["start"] == DISPATCHER[0])] = \
        dispatcher_path
    sources[next(i for i, e in enumerate(current) if e["start"] == TABLE[0])] = table
    for start, end in CODE_RANGES:
        sources[next(i for i, e in enumerate(current) if e["start"] == start)] = \
            assemble_range(args, rom_path, start, end, staging)
    materialized = output / "materialized"
    entries = AUTO.materialize(materialized, current, rom, sources)
    baseline_rebuilt = baseline_path.parent / "rebuilt.rom"
    if baseline_rebuilt.read_bytes() != rom:
        raise ValueError("baseline rebuilt ROM is not canonical")
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO46 static dispatch target family"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto46-static-dispatch-targets.v1",
              "contract": contract, "metrics": manifest["metrics"],
              "promoted_bytes": (DISPATCHER[1] - DISPATCHER[0]) + (TABLE[1] - TABLE[0]) +
              (RETURN_TARGET[1] - RETURN_TARGET[0]) +
              sum(end - start for start, end in TARGETS),
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
