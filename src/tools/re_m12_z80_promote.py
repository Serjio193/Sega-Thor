"""Promote the exact 68000-uploaded Z80 music-driver image."""
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
LOADER_START = 0x6134E
UPLOAD_PC = 0x6135E
UPLOAD_START = 0x62E38
UPLOAD_END = 0x64E38
UPLOAD_DESTINATION = 0xA00000
UPLOAD_BYTES = bytes.fromhex(
    "43 f9 00 06 2e 38 45 f9 00 a0 00 00 30 3c 1f ff 14 d9 51 c8 ff fc")
DRIVER_SIGNATURE = b"Ancient Music   Driver -MD-     Z80 Program"


def renumber(entries):
    result = copy.deepcopy(entries)
    for index, entry in enumerate(result):
        entry["size"] = entry["end"] - entry["start"]
        entry["manifest_index"] = index
    return result


def verify_upload_contract(rom):
    if rom[LOADER_START:LOADER_START + len(UPLOAD_BYTES)] != UPLOAD_BYTES:
        raise ValueError("68000 Z80 upload loop bytes changed")
    if rom[UPLOAD_START:UPLOAD_START + 3] != bytes.fromhex("c3 9d 04"):
        raise ValueError("Z80 reset JP signature missing")
    if rom.find(DRIVER_SIGNATURE, UPLOAD_START, UPLOAD_END) < 0:
        raise ValueError("Ancient Music Driver signature missing")
    if UPLOAD_END - UPLOAD_START != 0x2000:
        raise ValueError("unexpected Z80 upload size")


def split_unknown(entries):
    for index, entry in enumerate(entries):
        if entry["kind"] != "UNKNOWN":
            continue
        if entry["start"] <= UPLOAD_START and UPLOAD_END <= entry["end"]:
            replacement = []
            if entry["start"] < UPLOAD_START:
                replacement.append({**entry, "end": UPLOAD_START})
            replacement.append({
                "start": UPLOAD_START,
                "end": UPLOAD_END,
                "kind": "CODE_VERIFIED",
                "source": "M12_AUTO2_68000_z80_upload",
                "confidence": "CONFIRMED",
                "classification": "Z80_ASM_SOURCE_OWNED",
                "architecture": "Z80",
                "emitted_artifact_type": "asm",
                "ownership_reason": (
                    "exact 68000 LEA/DBF upload loop copies this image to Z80 RAM; "
                    "reset vector and embedded Ancient driver signature agree"
                ),
            })
            if UPLOAD_END < entry["end"]:
                replacement.append({**entry, "start": UPLOAD_END})
            return renumber(entries[:index] + replacement + entries[index + 1:])
    raise ValueError("Z80 upload range is not wholly UNKNOWN")


def render_source(rom):
    lines = [
        "; Exact Z80 source image uploaded by the 68000 loader at $06135E.",
        "; This is a developer-only source representation; no production C++ migration.",
        "z80_driver_start:",
    ]
    block = rom[UPLOAD_START:UPLOAD_END]
    for offset in range(0, len(block), 16):
        values = ", ".join("$" + f"{value:02X}" for value in block[offset:offset + 16])
        lines.append(f"z80_{offset:04X}:    dc.b {values}")
    return "\n".join(lines) + "\n"


def source_map(entries, root, generated):
    result = {}
    for index, entry in enumerate(entries):
        if entry.get("emitted_artifact_type") != "asm":
            continue
        result[index] = generated if entry["start"] == UPLOAD_START else root / entry["artifact"]
    return result


def run(args):
    rom_path = Path(args.rom).resolve()
    rom = rom_path.read_bytes()
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    verify_upload_contract(rom)
    manifest_path = Path(args.manifest).resolve()
    current_root = manifest_path.parent
    baseline = json.loads(manifest_path.read_text())
    output = Path(args.output).resolve()
    if output.exists():
        raise ValueError("output directory must be new")
    current = split_unknown(baseline["entries"])
    output.mkdir(parents=True)
    generated = output / "z80_driver_source.inc"
    generated.write_text(render_source(rom))
    materialized = output / "materialized"
    entries = AUTO.materialize(
        materialized, current, rom, source_map(current, current_root, generated))
    matched, difference, reason, detail = AUTO.verify_full(
        materialized, rom, args.assembler, entries)
    if not matched:
        raise ValueError(f"full ROM verification failed: {reason} {detail} {difference}")
    manifest = AUTO.manifest_for(entries, len(rom))
    manifest["rom_sha256"] = ROM_SHA256
    z80_bytes = sum(
        entry["size"] for entry in entries
        if entry.get("classification") == "Z80_ASM_SOURCE_OWNED")
    manifest["metrics"]["Z80_SOURCE_OWNED_BYTES"] = z80_bytes
    manifest["metrics"]["SOURCE_OWNED_BYTES"] = sum(
        entry["size"] for entry in entries if entry["kind"] != "UNKNOWN")
    manifest["metrics"]["SOURCE_OWNED_PERCENT"] = (
        100.0 * manifest["metrics"]["SOURCE_OWNED_BYTES"] / len(rom))
    manifest["transaction"] = "M12-AUTO2 exact Z80 upload source"
    (materialized / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    rebuilt = (materialized / "rebuilt.rom").read_bytes()
    report = {
        "schema": "oasis.m68k.m12-auto2-z80-promotion.v1",
        "upload": {
            "source_pc": UPLOAD_PC,
            "rom_start": UPLOAD_START,
            "rom_end": UPLOAD_END,
            "destination": UPLOAD_DESTINATION,
            "bytes": UPLOAD_END - UPLOAD_START,
            "loader_sha256": hashlib.sha256(UPLOAD_BYTES).hexdigest(),
            "image_sha256": hashlib.sha256(rom[UPLOAD_START:UPLOAD_END]).hexdigest(),
            "signature": DRIVER_SIGNATURE.decode("ascii"),
        },
        "metrics": manifest["metrics"],
        "full_rom": {
            "size": len(rebuilt),
            "crc32": f"{zlib.crc32(rebuilt) & 0xFFFFFFFF:08X}",
            "sha1": hashlib.sha1(rebuilt).hexdigest(),
            "sha256": hashlib.sha256(rebuilt).hexdigest(),
        },
    }
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
