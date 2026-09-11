"""Promote statically selected CC-B0 relative-pointer slots."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import zlib


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location(
    "re_m12_ccb0_group_table_promote",
    ROOT / "re_m12_ccb0_group_table_promote.py")
GROUP = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GROUP)

ROM_SHA256 = GROUP.ROM_SHA256
CALLERS = (
    (0x2E188, 0x2E17C, 0x0006), (0x2E19A, 0x2E18E, 0x0006),
    (0x2F3A0, 0x2F390, 0x0201), (0x2F3B2, 0x2F3A6, 0x0203),
    (0x2F3C4, 0x2F3B8, 0x0208), (0x2F844, 0x2F838, 0x0207),
    (0x2FF66, 0x2FF56, 0x0305), (0x3001C, 0x30002, 0x0401),
    (0x3015C, 0x30150, 0x000C), (0x30546, 0x3053A, 0x0502),
    (0x3135C, 0x31350, 0x0606), (0x3136E, 0x31362, 0x0609),
    (0x32F3E, 0x32F32, 0x0804),
    (0x34314, 0x34308, 0x0A01), (0x3673A, 0x3672A, 0x0C03),
    (0x36750, 0x36740, 0x0C05),
)
JMP = bytes.fromhex("4E F9 00 00 CA 24")


def parse_slots(rom):
    table = GROUP.parse_table(rom)
    seen = {}
    for caller, d0_address, d0 in CALLERS:
        expected = bytes((0x30, 0x3C, d0 >> 8, d0 & 0xFF))
        if rom[d0_address:d0_address + 4] != expected:
            raise ValueError(f"D0 contract changed at 0x{d0_address:06X}")
        if rom[caller:caller + len(JMP)] != JMP:
            raise ValueError(f"CC-B0 jump contract changed at 0x{caller:06X}")
        group = d0 >> 8
        slot = table["targets"][group] + 2 * (d0 & 0xFF)
        if slot + 2 > len(rom):
            raise ValueError(f"selected slot is outside ROM at 0x{slot:06X}")
        relative_value = int.from_bytes(rom[slot:slot + 2], "big", signed=True)
        resolved_target = slot - 2 * (d0 & 0xFF) + relative_value
        if resolved_target & 1 or not 0 <= resolved_target < len(rom):
            raise ValueError(f"selected relative pointer is invalid at 0x{slot:06X}")
        seen[slot] = {"slot": slot, "d0": d0, "caller": caller,
                      "d0_address": d0_address, "group": group,
                      "index": d0 & 0xFF, "relative_value": relative_value,
                      "resolved_target": resolved_target}
    return table, [seen[slot] for slot in sorted(seen)]


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def split_slot(entries, slot):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= slot and slot + 2 <= entry["end"]:
            replacement = []
            if entry["start"] < slot:
                replacement.append({**entry, "end": slot})
            replacement.append({
                "start": slot, "end": slot + 2,
                "kind": "STRUCTURED_DATA_CONFIRMED",
                "source": "M12_AUTO15_CCB0_selected_relative_pointer_slots",
                "confidence": "CONFIRMED",
                "classification": "SELECTED_RELATIVE_POINTER_SLOT_16BIT",
                "emitted_artifact_type": "rom_asset",
                "ownership_reason": (
                    "an exact constant-D0 caller reaches this 16-bit slot after "
                    "the closed CC-B0 group-table lookup and consumes it as a "
                    "signed relative pointer"),
            })
            if slot + 2 < entry["end"]:
                replacement.append({**entry, "start": slot + 2})
            return renumber(entries[:index] + replacement + entries[index + 1:])
        if entry["start"] < slot + 2 and slot < entry["end"]:
            raise ValueError(f"selected slot overlaps non-UNKNOWN at 0x{slot:06X}")
    raise ValueError(f"selected slot is not wholly UNKNOWN at 0x{slot:06X}")


def source_owned(entries, rom_size):
    kinds = {"CODE_VERIFIED", "HEADER_VECTOR_ASM", "STRUCTURED_DATA_CONFIRMED",
             "PADDING_ALIGNMENT_CONFIRMED", "LOCAL_ROM_DERIVED_ASSET"}
    count = sum(entry["size"] for entry in entries if entry["kind"] in kinds)
    return {"SOURCE_OWNED_BYTES": count,
            "SOURCE_OWNED_PERCENT": 100.0 * count / rom_size,
            "ROM_SIZE": rom_size}


def verify_inherited(materialized, baseline_root, baseline_rebuilt, rom, entries):
    rebuilt = baseline_rebuilt.read_bytes()
    if len(rebuilt) != len(rom) or hashlib.sha256(rebuilt).hexdigest() != ROM_SHA256:
        raise ValueError("verified baseline rebuilt ROM is not canonical")
    for entry in entries:
        artifact = materialized / entry["artifact"]
        if entry.get("emitted_artifact_type") in ("rom_asset", "blob"):
            expected = rom[entry["start"]:entry["end"]]
            if artifact.read_bytes() != expected:
                raise ValueError("materialized ROM artifact differs from canonical slice")
        else:
            previous = baseline_root / "code" / f"sub_{entry['start']:06X}.asm"
            if not previous.exists() or artifact.read_bytes() != previous.read_bytes():
                raise ValueError("inherited ASM artifact changed")
    shutil.copyfile(baseline_rebuilt, materialized / "rebuilt.rom")
    return {"mode": "INHERITED_BASELINE_FULL_ROM",
            "baseline_rebuilt_sha256": hashlib.sha256(rebuilt).hexdigest()}


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    table, slots = parse_slots(rom)
    baseline_path = Path(args.manifest).resolve()
    baseline = json.loads(baseline_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = renumber(baseline["entries"])
    for selected in slots:
        current = split_slot(current, selected["slot"])
    output.mkdir(parents=True)
    materialized = output / "materialized"
    entries = GROUP.AUTO.materialize(
        materialized, current, rom,
        GROUP.AUTO.source_map(current, baseline_path))
    if args.baseline_rebuilt:
        verification = verify_inherited(
            materialized, baseline_path.parent, Path(args.baseline_rebuilt), rom, entries)
    else:
        matched, difference, reason, detail = GROUP.AUTO.verify_full(
            materialized, rom, args.assembler, entries)
        if not matched:
            raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
        verification = {"mode": "FRESH_ASSEMBLER_ROUND_TRIP"}
    manifest = GROUP.AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    manifest["metrics"].update(source_owned(entries, len(rom)))
    manifest["transaction"] = "M12-AUTO15 CC-B0 selected relative-pointer slots"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto15-ccb0-selected-slots.v1",
        "table": table, "selected_slots": slots,
        "metrics": manifest["metrics"], "promoted_slot_bytes": 2 * len(slots),
        "verification": verification,
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
    parser.add_argument("--assembler", required=True)
    parser.add_argument("--baseline-rebuilt")
    parser.add_argument("--output", required=True)
    run(parser.parse_args())


if __name__ == "__main__":
    main()
