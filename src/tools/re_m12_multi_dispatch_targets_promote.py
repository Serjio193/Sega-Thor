"""Promote two overlapping, selector-bounded dispatch families."""
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
FIRST_CONSUMER = (0x03AA12, 0x03AA2A)
SECOND_CONSUMER = (0x03AA92, 0x03AAAA)
TABLE = (0x03B8A6, 0x03B8E2)
TABLE_VALUES = (
    0x03AAAE, 0x03AB98, 0x03AC16, 0x03AC6E, 0x03ACA8, 0x03AD0C,
    0x03ADB4, 0x03AAEE, 0x03ABDA, 0x03AC68, 0x03AC92, 0x03ACE4,
    0x03AD66, 0x03AE74, 0x03BA46,
)
TARGETS = (
    (0x03AAAE, 0x03AAEE), (0x03AAEE, 0x03AB98), (0x03AB98, 0x03ABDA),
    (0x03ABDA, 0x03AC16), (0x03AC16, 0x03AC68), (0x03AC68, 0x03AC6E),
    (0x03AC6E, 0x03AC92), (0x03AC92, 0x03ACA8), (0x03ACA8, 0x03ACE4),
    (0x03ACE4, 0x03AD0C), (0x03AD0C, 0x03AD66), (0x03AD66, 0x03ADB4),
    (0x03ADB4, 0x03AE74),
)
FIRST_BYTES = bytes.fromhex("303900FFAFAE02400007D040D04041FA0E84207000004E90")
SECOND_BYTES = bytes.fromhex("303900FFAFAE02400007D040D04041FA0E20207000004E90")


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
                "start": start, "end": end, "kind": kind, "source": source,
                "confidence": "CONFIRMED",
                "classification": "STATIC_MULTI_DISPATCH_TARGET" if kind == "CODE_VERIFIED"
                else "STATIC_MULTI_DISPATCH_POINTER_TABLE",
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
    if rom[FIRST_CONSUMER[0]:FIRST_CONSUMER[1]] != FIRST_BYTES:
        raise ValueError("first dispatch consumer changed")
    if rom[SECOND_CONSUMER[0]:SECOND_CONSUMER[1]] != SECOND_BYTES:
        raise ValueError("second dispatch consumer changed")
    values = tuple(int.from_bytes(rom[address:address + 4], "big")
                   for address in range(TABLE[0], TABLE[1], 4))
    if values != TABLE_VALUES:
        raise ValueError("multi-dispatch table changed")
    return {"consumers": [FIRST_CONSUMER, SECOND_CONSUMER], "table": TABLE,
            "table_values": values, "selector_offsets": list(range(0, 0x20, 4)),
            "targets": [{"start": start, "end": end, "bytes": end - start}
                        for start, end in TARGETS],
            "unbounded_target_left_unknown": "0x03BA46"}


def table_source(path):
    lines = ["; Exact overlapping dispatch tables; selector offsets are 0..0x1C.",
             "    org $03B8A6", "dispatch_03B8A6:"]
    lines.append("    dc.l " + ",".join(f"${value:08X}" for value in TABLE_VALUES))
    path.write_text("\n".join(lines) + "\n")


def assemble_target(args, rom_path, start, end, staging):
    asm = staging / f"sub_{start:06X}.asm"
    decoded = staging / f"sub_{start:06X}.json"
    binary = staging / f"sub_{start:06X}.bin"
    decoded_run = subprocess.run(
        [args.range_tool, str(rom_path), hex(start), hex(end), str(asm), str(decoded)],
        text=True, capture_output=True, check=False)
    if decoded_run.returncode:
        raise ValueError(f"exact range decode failed at 0x{start:06X}: "
                         f"{decoded_run.stdout}{decoded_run.stderr}")
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
    current = split_unknown(current, *TABLE, "STRUCTURED_DATA_CONFIRMED",
                            "M12_AUTO47_multi_dispatch_targets",
                            "two exact &7 consumers close the overlapping 15-entry table")
    for start, end in TARGETS:
        current = split_unknown(current, start, end, "CODE_VERIFIED",
                                "M12_AUTO47_multi_dispatch_targets",
                                "exact &7 consumer, absolute in-ROM table target, RTS boundary and vasm round-trip")
    output.mkdir(parents=True)
    staging = output / "staging"
    staging.mkdir()
    table = staging / "dispatch_03B8A6.asm"
    table_source(table)
    target_starts = {start for start, _ in TARGETS}
    sources = {}
    for index, entry in enumerate(current):
        if (entry.get("emitted_artifact_type") == "asm" and
                entry["start"] not in target_starts):
            sources[index] = AUTO.resolve_artifact(baseline_path, entry["artifact"])
    for start, end in TARGETS:
        index = next(i for i, entry in enumerate(current) if entry["start"] == start)
        sources[index] = assemble_target(args, rom_path, start, end, staging)
    materialized = output / "materialized"
    entries = AUTO.materialize(materialized, current, rom, sources)
    baseline_rebuilt = baseline_path.parent / "rebuilt.rom"
    if baseline_rebuilt.read_bytes() != rom:
        raise ValueError("baseline rebuilt ROM is not canonical")
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO47 static multi-dispatch target families"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {"schema": "oasis.m68k.m12-auto47-multi-dispatch-targets.v1",
              "contract": contract, "metrics": manifest["metrics"],
              "promoted_bytes": TABLE[1] - TABLE[0] +
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
