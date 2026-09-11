"""Promote five closed PC-relative lookup-table instances."""
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
TABLES = (
    (0x01E2A0, 0x01E2E0, (0x01E21E, 0x01E232), "four 16-byte fixed records"),
    (0x01FC98, 0x01FCC0, (0x01FC7A,), "twenty-word indexed coordinate table"),
    (0x027DBA, 0x027DDA, (0x027D96,), "four 8-byte indexed records"),
    (0x02959A, 0x0295BA, (0x029576,), "four 8-byte indexed records"),
    (0x02A3E0, 0x02A400, (0x02A3BC,), "four 8-byte indexed records"),
)
CONSUMER_BYTES = {
    0x01E21E: bytes.fromhex("41FA00A04EB90000E3E82D7C000300000056"),
    0x01E232: bytes.fromhex("41FA006C4EB90000E3E82D7C000600000056"),
    0x01FC7A: bytes.fromhex(
        "41FB001C3018D16E00083010D16E0010066E0001000C"),
    0x027D96: bytes.fromhex("41FA002222300000243000040C6E00020096"),
    0x029576: bytes.fromhex("41FA002222300000243000040C6E00020096"),
    0x02A3BC: bytes.fromhex("41FA002222300000243000040C6E00020096"),
}
TABLE_END_MARKERS = {
    0x01E2E0: bytes.fromhex("4EB90000E682"),
    0x01FCC0: bytes.fromhex("4EB90000E66E"),
    0x027DDA: bytes.fromhex("4EB90000E682"),
    0x0295BA: bytes.fromhex("4EB90000E682"),
    0x02A400: bytes.fromhex("4EB90000E682"),
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
            replacement.append({
                "start": start, "end": end,
                "kind": "STRUCTURED_DATA_CONFIRMED",
                "source": "M12_AUTO53_pc_lookup_family",
                "confidence": "CONFIRMED",
                "classification": "PC_RELATIVE_LOOKUP_TABLE",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": reason,
            })
            if end < entry["end"]:
                replacement.append({**entry, "start": end})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if start < entry["end"] and entry["start"] < end:
            raise ValueError(f"range overlaps non-UNKNOWN 0x{start:06X}..0x{end:06X}")
    raise ValueError(f"range is not wholly UNKNOWN 0x{start:06X}..0x{end:06X}")


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
    records = []
    for start, end, consumers, shape in TABLES:
        for consumer in consumers:
            expected = CONSUMER_BYTES[consumer]
            if rom[consumer:consumer + len(expected)] != expected:
                raise ValueError(f"consumer contract changed at 0x{consumer:06X}")
        marker = TABLE_END_MARKERS[end]
        if rom[end:end + len(marker)] != marker:
            raise ValueError(f"closed table boundary changed at 0x{end:06X}")
        records.append({"start": start, "end": end, "bytes": end - start,
                        "consumers": list(consumers), "shape": shape})
    return {"tables": records,
            "reason": "PC-relative LEA consumers use fixed indexed widths and the next bytes are exact code entry markers"}


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
    for start, end, consumers, shape in TABLES:
        current = split_unknown(
            current, start, end,
            f"{shape}; exact PC-relative consumer(s) at "
            + ", ".join(f"0x{x:06X}" for x in consumers))
    output.mkdir(parents=True)
    materialized = output / "materialized"
    sources = {index: AUTO.resolve_artifact(baseline_path, entry["artifact"])
               for index, entry in enumerate(current)
               if entry.get("emitted_artifact_type") == "asm" and entry.get("artifact")}
    entries = AUTO.materialize(materialized, current, rom, sources)
    baseline_rebuilt = baseline_path.parent / "rebuilt.rom"
    if baseline_rebuilt.read_bytes() != rom:
        raise ValueError("baseline rebuilt ROM is not canonical")
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO53 PC-relative lookup-table family"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto53-pc-lookup-family.v1",
        "contract": contract, "metrics": manifest["metrics"],
        "promoted_bytes": sum(end - start for start, end, *_ in TABLES),
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
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
