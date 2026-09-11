"""Promote the closed PC-relative dispatch table at 0x00E2F2."""
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
DISPATCHER = (0x00E2D4, 0x00E2F0)
JUMP_TAIL = (0x00E2F0, 0x00E2F2)
TABLE = (0x00E2F2, 0x00E302)
STUB = (0x00E318, 0x00E31A)
SHARED_TARGET = (0x00E322, 0x00E332)
DISPATCHER_BYTES = bytes.fromhex(
    "48E7108048414241E08948424242E08A02430007D64341FB3006D0D0")
JUMP_TAIL_BYTES = bytes.fromhex("4ED0")
STUB_BYTES = bytes.fromhex("6008")
TABLE_VALUES = (0x0010, 0x0014, 0x0016, 0x0018,
                0x001C, 0x0026, 0x001C, 0x001E)
TABLE_TARGETS = (0x00E302, 0x00E306, 0x00E308, 0x00E30A,
                 0x00E30E, 0x00E318, 0x00E30E, 0x00E310)


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end, kind, source, classification, reason):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= start and end <= entry["end"]:
            replacement = []
            if entry["start"] < start:
                replacement.append({**entry, "end": start})
            replacement.append({
                "start": start, "end": end, "kind": kind, "source": source,
                "confidence": "CONFIRMED", "classification": classification,
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
    if rom[DISPATCHER[0]:DISPATCHER[1]] != DISPATCHER_BYTES:
        raise ValueError("PC-relative dispatcher contract changed")
    if rom[JUMP_TAIL[0]:JUMP_TAIL[1]] != JUMP_TAIL_BYTES:
        raise ValueError("indirect jump tail contract changed")
    if rom[STUB[0]:STUB[1]] != STUB_BYTES:
        raise ValueError("shared-target branch stub contract changed")
    values = tuple(int.from_bytes(rom[address:address + 2], "big")
                   for address in range(TABLE[0], TABLE[1], 2))
    if values != TABLE_VALUES:
        raise ValueError("PC-relative dispatch table bytes changed")
    targets = tuple(TABLE[0] + value for value in values)
    if targets != TABLE_TARGETS:
        raise ValueError("PC-relative dispatch targets changed")
    if not all(TABLE_TARGETS[0] <= target < 0x00E332 for target in targets):
        raise ValueError("dispatch target escaped the closed handler family")
    return {
        "dispatcher": DISPATCHER, "jump_tail": JUMP_TAIL, "table": TABLE,
        "table_values": values, "table_targets": targets, "stub": STUB,
        "shared_target": SHARED_TARGET,
        "reason": "D3 AND #7 selects eight PC-relative words; each resolves inside the handler family",
    }


def exact_source(path, start, body, rom_path, assembler):
    path.write_text("\n".join(["; Exact M12 PC-relative dispatch source.",
                                f"    org ${start:06X}",
                                f"sub_{start:06X}:", body, ""]) + "\n")
    binary = path.with_suffix(".bin")
    result = subprocess.run(
        [assembler, "-m68000", "-no-opt", "-Fbin", "-o", str(binary), str(path)],
        text=True, capture_output=True, check=False)
    if result.returncode or binary.read_bytes() != rom_path.read_bytes()[start:start + len(binary.read_bytes())]:
        raise ValueError(f"exact source round-trip failed at 0x{start:06X}: "
                         f"{result.stdout}{result.stderr}")
    return path


def assemble_range(args, rom_path, start, end, staging):
    asm = staging / f"sub_{start:06X}.asm"
    decoded = staging / f"sub_{start:06X}.json"
    binary = staging / f"sub_{start:06X}.bin"
    result = subprocess.run(
        [args.range_tool, str(rom_path), hex(start), hex(end), str(asm), str(decoded)],
        text=True, capture_output=True, check=False)
    if result.returncode:
        raise ValueError(f"exact range decode failed at 0x{start:06X}: "
                         f"{result.stdout}{result.stderr}")
    result = subprocess.run(
        [args.assembler, "-m68000", "-no-opt", "-Fbin", "-o", str(binary), str(asm)],
        text=True, capture_output=True, check=False)
    if result.returncode or binary.read_bytes() != rom_path.read_bytes()[start:end]:
        raise ValueError(f"exact target round-trip failed at 0x{start:06X}: "
                         f"{result.stdout}{result.stderr}")
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
    source = "M12_AUTO48_pc_relative_dispatch"
    current = split_unknown(current, *JUMP_TAIL, "CODE_VERIFIED", source,
                             "CODE_STATIC_SUPPORTED", "exact indirect jump after PC-relative table lookup")
    current = split_unknown(current, *TABLE, "STRUCTURED_DATA_CONFIRMED", source,
                             "PC_RELATIVE_DISPATCH_TABLE", "D3&7 closes eight exact word offsets to in-family targets")
    current = split_unknown(current, *STUB, "CODE_VERIFIED", source,
                             "CODE_STATIC_SUPPORTED", "selected table target branches to the exact shared continuation")
    current = split_unknown(current, *SHARED_TARGET, "CODE_VERIFIED", source,
                             "CODE_STATIC_SUPPORTED", "branch target, exact range decode and vasm round-trip")
    output.mkdir(parents=True)
    staging = output / "staging"
    staging.mkdir()
    jump = exact_source(staging / "jump_00E2F0.asm", JUMP_TAIL[0], "    jmp.l (A0)", rom_path, args.assembler)
    table_body = "    dc.w " + ",".join(f"${value:04X}" for value in TABLE_VALUES)
    table = exact_source(staging / "table_00E2F2.asm", TABLE[0], "dispatch_00E2F2:\n" + table_body,
                         rom_path, args.assembler)
    stub = exact_source(staging / "stub_00E318.asm", STUB[0], "    bra.s *+$A",
                        rom_path, args.assembler)
    shared = assemble_range(args, rom_path, *SHARED_TARGET, staging)
    sources = {index: AUTO.resolve_artifact(baseline_path, entry["artifact"])
               for index, entry in enumerate(current)
               if entry.get("emitted_artifact_type") == "asm" and entry.get("artifact")}
    for start, path in ((JUMP_TAIL[0], jump), (TABLE[0], table), (STUB[0], stub),
                        (SHARED_TARGET[0], shared)):
        sources[next(i for i, e in enumerate(current) if e["start"] == start)] = path
    materialized = output / "materialized"
    entries = AUTO.materialize(materialized, current, rom, sources)
    baseline_rebuilt = baseline_path.parent / "rebuilt.rom"
    if baseline_rebuilt.read_bytes() != rom:
        raise ValueError("baseline rebuilt ROM is not canonical")
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO48 PC-relative dispatch table"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto48-pc-relative-dispatch.v1",
              "contract": contract, "metrics": manifest["metrics"],
              "promoted_bytes": sum(end - start for start, end in
                                    (JUMP_TAIL, TABLE, STUB, SHARED_TARGET)),
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
