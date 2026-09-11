"""Promote closed PC-relative tables and NUL-terminated text literals."""
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
RANGES = (
    (0x0008F8, 0x00091C, "three 12-byte selector records", (0x0008BE,), bytes.fromhex("70003E3CE000")),
    (0x000A70, 0x000A90, "eight 4-byte parameter records", (0x000998,), bytes.fromhex("202020202020202000")),
    (0x000A90, 0x000A99, "space-filled NUL-terminated text literal", (0x0009CC,), b"New Data\0"),
    (0x000A99, 0x000AA2, "NUL-terminated text literal", (0x0009F4,), bytes.fromhex("202F00")),
    (0x000DF0, 0x000E00, "four 4-byte branch-selector records", (0x000DCE,), bytes.fromhex("363900FF19A8")),
    (0x000E40, 0x000E80, "sixteen 4-byte VDP selector records", (0x000E12,), bytes.fromhex("0839000000FF")),
    (0x0087F4, 0x008814, "eight 4-byte status records", (0x0087AC, 0x008978), bytes.fromhex("41F900FF1668")),
    (0x0096F8, 0x009708, "sixteen-byte nibble lookup", (0x009B12,), bytes.fromhex("97CB222E0008")),
    (0x01953C, 0x01955C, "four 8-byte transform records", (0x019620,), bytes.fromhex("3D7C00140006")),
    (0x01E064, 0x01E074, "eight-word coordinate lookup", (0x01E034, 0x01E04E), bytes.fromhex("0C6E00020096")),
    (0x025C24, 0x025C64, "four 16-byte transform records", (0x025BEC,), bytes.fromhex("0839000300FF1A20")),
    (0x0288A2, 0x0288C2, "four 8-byte transform records", (0x02887E,), bytes.fromhex("4EB90000E682")),
    (0x03E430, 0x03E436, "three-word status lookup", (0x03E3F6,), bytes.fromhex("42467E044A2E00056700003A")),
    (0x03E492, 0x03E498, "three-word status lookup", (0x03E458,), bytes.fromhex("42467E044A2E00056700003A")),
    (0x03E4F4, 0x03E4FA, "three-word status lookup", (0x03E4BA,), bytes.fromhex("4EB900002B28")),
)
CONSUMERS = {
    address: bytes.fromhex(data) for address, data in (
        (0x0008BE, "41FA0038243C87808780224B"),
        (0x000998, "43FA00D63002610000943003"),
        (0x0009CC, "41FA00C2303C048CD047"),
        (0x0009F4, "41FA00A3"),
        (0x000DCE, "41FA00203230000038300002"),
        (0x000E12, "41FA002C1230300018303001"),
        (0x0087AC, "43FB0046121110305000B400"),
        (0x008978, "43FAFE7A14310000"),
        (0x009B12, "43FAFBE41A3150006B000090"),
        (0x019620, "41FAFF1A2D700000"),
        (0x01E034, "41FB002E30103200"),
        (0x01E04E, "41FB001C30103200"),
        (0x025BEC, "41FA00362D700000"),
        (0x02887E, "41FA002222300000"),
        (0x03E3F6, "43FA0038301E6100"),
        (0x03E458, "43FA0038301E6100"),
        (0x03E4BA, "43FA0038301E6100"),
    )
}


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_unknown(entries, start, end, classification, reason):
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
                "source": "M12_AUTO54_pc_island_tables",
                "confidence": "CONFIRMED",
                "classification": classification,
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
    for start, end, classification, consumers, marker in RANGES:
        for consumer in consumers:
            expected = CONSUMERS[consumer]
            if rom[consumer:consumer + len(expected)] != expected:
                raise ValueError(f"consumer contract changed at 0x{consumer:06X}")
        if rom[end:end + len(marker)] != marker:
            raise ValueError(f"boundary marker changed at 0x{end:06X}")
        records.append({"start": start, "end": end, "bytes": end - start,
                        "classification": classification,
                        "consumers": list(consumers)})
    return {"ranges": records,
            "reason": "exact PC-relative consumers and fixed record/terminator boundaries"}


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
    for start, end, classification, consumers, marker in RANGES:
        current = split_unknown(
            current, start, end, classification,
            "static PC-relative consumer(s) at "
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
    manifest["transaction"] = "M12-AUTO54 PC-relative island tables"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto54-pc-island-tables.v1",
        "contract": contract, "metrics": manifest["metrics"],
        "promoted_bytes": sum(end - start for start, end, *_ in RANGES),
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
