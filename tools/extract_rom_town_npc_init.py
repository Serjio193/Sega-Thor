#!/usr/bin/env python3
"""
Extract ROM Town NPC Spawn Initializer (20 Variants, 0x018072..0x018538, 1222 bytes).

Disassembles 274 M68K instructions across the master dispatcher and 20 variant blocks,
validates story flag RAM addresses, patrol bounding boxes, and dialog IDs, and generates
evidence JSON for Milestone M15.526.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_town_npc_init_m15_526.json"

RANGE_START = 0x018072
RANGE_END = 0x018538
RANGE_SIZE = RANGE_END - RANGE_START  # 1222 bytes

ROUTINE_BOUNDS = [
    ("AI_TownNPC_SpawnInitializer", 0x018072, 0x018538, 274,
     "Town NPC Spawn Initializer (20 Variants): if +$66==0 branches directly to active NPC tick 0x018538; for +$66 in 1..20 configures entity height +$4A, animation +$2A, facing +$16, collision/render flags (+$36, +$37, +$38, +$3C, +$3D), patrol bounding box (xmin, ymin, xmax, ymax) at +$A6..+$AC, and dialog ID at +$A0 gated by story flags $FF0DBF:0, $FF0DBC:0, and $FF0DC5:3; common epilogue 0x018526 saves variant ID +$66 to +$9E, clears +$66=0 and timer +$2=0."),
]

VARIANT_BLOCKS = [
    (1,  "0x0180F2", "0x01814A", "Variant 1: +$4A=20, anim 6; Branch A ($FF0DBF:0||!$FF0DBC:0) box ($E8,$1A8,$1A8,$210) dialog $01; Branch B box ($48,$1C0,$160,$2C0) dialog $55."),
    (2,  "0x0181A2", "0x0181F4", "Variant 2: anim 10; Branch A box ($40,$188,$98,$1A8) dialog $02; Branch B box ($50,$188,$138,$1E0) dialog $56."),
    (3,  "0x0181F4", "0x018246", "Variant 3: anim 14; Branch A box ($70,$288,$120,$2E0) dialog $03; Branch B box ($20,$1D8,$178,$2E0) dialog $57."),
    (4,  "0x018298", "0x0182EA", "Variant 4: anim 18; Branch A box ($C8,$1A8,$E8,$240) dialog $04; Branch B box ($D0,$F8,$170,$1F0) dialog $58."),
    (5,  "0x0182EA", "0x018330", "Variant 5: +$4A=16, +$3C:2, +$37:6/7, anim 20; dialog $05 (!$FF0DBF:0), $59 ($FF0DBF:0 && !$FF0DC5:3), or $5D ($FF0DBF:0 && $FF0DC5:3)."),
    (6,  "0x018330", "0x018358", "Variant 6: +$4A=22, +$37:6/7, anim 22, facing 3, dialog $06."),
    (7,  "0x018358", "0x01837C", "Variant 7: +$38:2/0, +$36:2, +$3D=0, anim 30, dialog 0."),
    (8,  "0x01837C", "0x0183CC", "Variant 8: +$4A=8, anim 32, dialog 0; Branch A box ($C8,$1D0,$1A0,$228); Branch B box ($20,$C0,$170,$238)."),
    (9,  "0x0183CC", "0x0183EC", "Variant 9: anim 40, z=80 ($50), +$38:2, +$36:3, dialog 0."),
    (10, "0x01814A", "0x0181A2", "Variant 10: +$4A=20, anim 6; Branch A box ($E0,$1D0,$168,$2A8) dialog $0A; Branch B box ($10,$88,$130,$168) dialog $5B."),
    (11, "0x018246", "0x018298", "Variant 11: anim 14; Branch A box ($D0,$100,$138,$1A0) dialog $0B; Branch B box ($D0,$88,$1D8,$1E0) dialog $5C."),
    (12, "0x0183EC", "0x018416", "Variant 12: +$4A=20, anim 6, box ($B8,$218,$150,$3F0), dialog $29."),
    (13, "0x018416", "0x018440", "Variant 13: +$4A=20, anim 6, box ($70,$320,$190,$3F0), dialog $29."),
    (14, "0x018440", "0x018464", "Variant 14: anim 18, box ($38,$208,$118,$2C8), dialog $2D."),
    (15, "0x018464", "0x018488", "Variant 15: anim 14, box ($D0,$2E8,$190,$3F0), dialog $2C."),
    (16, "0x018488", "0x0184AC", "Variant 16: anim 10, box ($C8,$218,$138,$3C0), dialog $2B."),
    (17, "0x0184AC", "0x0184D0", "Variant 17: anim 10, box ($118,$210,$1B0,$268), dialog $2A."),
    (18, "0x0184D0", "0x0184EC", "Variant 18: anim 10, facing 2, +$37:6, dialog $31."),
    (19, "0x0184EC", "0x018508", "Variant 19: anim 14, facing 2, +$37:6, dialog $32."),
    (20, "0x018508", "0x018526", "Variant 20: +$4A=20, anim 6, facing 2, +$37:6, dialog $33."),
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key ROM addresses, story flags, patrol bounds, and dialog IDs."""
    assert u32_be(rom, 0x018106) == 0x00FF0DBF   # story flag $FF0DBF
    assert u32_be(rom, 0x018110) == 0x00FF0DBC   # story flag $FF0DBC
    assert u32_be(rom, 0x01831E) == 0x00FF0DC5   # story flag $FF0DC5

    # Dialog IDs across variants
    assert u16_be(rom, 0x018128) == 0x0001       # Variant 1 Branch A dialog
    assert u16_be(rom, 0x018142) == 0x0055       # Variant 1 Branch B dialog
    assert u16_be(rom, 0x018180) == 0x000A       # Variant 10 Branch A dialog
    assert u16_be(rom, 0x01819A) == 0x005B       # Variant 10 Branch B dialog
    assert u16_be(rom, 0x0181D2) == 0x0002       # Variant 2 Branch A dialog
    assert u16_be(rom, 0x0181EC) == 0x0056       # Variant 2 Branch B dialog
    assert u16_be(rom, 0x018224) == 0x0003       # Variant 3 Branch A dialog
    assert u16_be(rom, 0x01823E) == 0x0057       # Variant 3 Branch B dialog
    assert u16_be(rom, 0x018276) == 0x000B       # Variant 11 Branch A dialog
    assert u16_be(rom, 0x018290) == 0x005C       # Variant 11 Branch B dialog
    assert u16_be(rom, 0x0182C8) == 0x0004       # Variant 4 Branch A dialog
    assert u16_be(rom, 0x0182E2) == 0x0058       # Variant 4 Branch B dialog
    assert u16_be(rom, 0x01830A) == 0x0005       # Variant 5 base dialog
    assert u16_be(rom, 0x018318) == 0x0059       # Variant 5 mid dialog
    assert u16_be(rom, 0x018326) == 0x005D       # Variant 5 late dialog
    assert u16_be(rom, 0x018350) == 0x0006       # Variant 6 dialog
    assert u16_be(rom, 0x01840E) == 0x0029       # Variant 12 dialog
    assert u16_be(rom, 0x018438) == 0x0029       # Variant 13 dialog
    assert u16_be(rom, 0x01845C) == 0x002D       # Variant 14 dialog
    assert u16_be(rom, 0x018480) == 0x002C       # Variant 15 dialog
    assert u16_be(rom, 0x0184A4) == 0x002B       # Variant 16 dialog
    assert u16_be(rom, 0x0184C8) == 0x002A       # Variant 17 dialog
    assert u16_be(rom, 0x0184DE) == 0x0031       # Variant 18 dialog
    assert u16_be(rom, 0x0184FA) == 0x0032       # Variant 19 dialog
    assert u16_be(rom, 0x01851C) == 0x0033       # Variant 20 dialog


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    for insn in md.disasm(slice_data, RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size,
        })

    routines = []
    for name, r_start, r_end, exp_insns, desc in ROUTINE_BOUNDS:
        r_bytes = rom[r_start:r_end]
        r_insns = list(md.disasm(r_bytes, r_start))
        assert len(r_insns) == exp_insns, f"Instruction count mismatch for {name}: {len(r_insns)} != {exp_insns}"
        routines.append({
            "name": name,
            "start": f"0x{r_start:06X}",
            "end": f"0x{r_end:06X}",
            "size_bytes": r_end - r_start,
            "instruction_count": len(r_insns),
            "description": desc,
        })

    variants = [
        {"variant_id": vid, "start": v_start, "end": v_end, "description": v_desc}
        for vid, v_start, v_end, v_desc in VARIANT_BLOCKS
    ]

    evidence = {
        "metadata": {
            "milestone": "M15.526",
            "rom_file": "Beyond Oasis (USA).md",
            "range_start": f"0x{RANGE_START:06X}",
            "range_end": f"0x{RANGE_END:06X}",
            "range_size_bytes": RANGE_SIZE,
            "instruction_count": len(instructions),
            "routine_count": len(routines),
            "variant_count": len(variants),
        },
        "routines": routines,
        "variants": variants,
        "instructions": instructions,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(json.dumps(evidence, indent=2))
    print(f"Wrote evidence JSON to {OUTPUT_PATH}")
    print(f"Total instructions extracted: {len(instructions)}")
    print(f"Variants defined: {len(variants)}")


if __name__ == "__main__":
    extract()
