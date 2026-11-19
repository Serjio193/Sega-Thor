#!/usr/bin/env python3
"""
Extract ROM World Interactive Objects: Sliding/Retracting Wall Trap, Falling Rock Boulder
& Impact AABB (0x0171C4..0x0173D6, 530 bytes).

Disassembles 137 M68K instructions across 3 routines, validates opcodes and RAM/SFX
constants, and generates evidence JSON for Milestone M15.521.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_world_sliding_falling_traps_m15_521.json"

RANGE_START = 0x0171C4
RANGE_END = 0x0173D6
RANGE_SIZE = RANGE_END - RANGE_START  # 530 bytes

ROUTINE_BOUNDS = [
    ("AI_World_SlidingRetractingWallBridgeTrap", 0x0171C4, 0x0172C2, 67,
     "Sliding/Retracting Wall or Bridge Trap: sub-mode 0 initializes timer +$2=+$A0, z=+$8C, anchor x=+$A6, offset +$AA=0; sub-mode 1 restores x=+$A6, decrements offset +$AA by 8 until 0, applies directional offset based on facing +$16==1, counts timer +$2, checks persistent bit via 0x018030 (if clear transitions to sub-mode 2, sets timer +$2=+$9E, plays SFX #$700391 via 0x00E7E8); sub-mode 2 sets +$37:3, increments offset +$AA by 8 up to $28 while clearing +$37:3, applies directional offset, animates via 0x00BC8E, checks persistent bit via 0x018030 / timer +$2 to transition back to sub-mode 1 with timer +$2=+$A0."),
    ("AI_World_FallingRockBoulderTrap", 0x0172C2, 0x017344, 32,
     "Falling Rock / Boulder Trap: sub-mode 0 initializes +$66=$FF, z=+$8C, sets aerial flag +$38:6; active tick: if z != 0 calls impact AABB 0x017344, applies gravity -$C000 to vz (+$56), integrates z; on floor impact (z < 0) clamps z=0, first bounce (+$A6==0) sets +$A6=$FF, z=1, bounce vz=+$30000, triggers screen shake $FF17C4=3 and rock impact SFX #$800386 via 0x00E7FC; second landing clears vz=0; when z==0 increments fade timer +$2E unless anchor deltas +$72|+$76 != 0."),
    ("AI_World_FallingRockImpactAABB", 0x017344, 0x0173D6, 38,
     "Falling Rock / Hazard Impact AABB: tests contact vs Ali via 0x00B922 [-10..+10, -10..+10, 0..24]; if Ali is guarded (+$38:3) or invulnerable (+$82!=0) ignores hit; on vulnerable Ali sets attacker ptr +$7E=a6, damage $8007 (7 HP), clears +$82, +$83, +$9D, imparts launch vz=+$20000, raises z+=1, flips direction bit 1 (+$16^2 -> +$84), computes directional knockback via 0x00E2D4 ($310/$300); plays SFX #$900237 via 0x00E7FC if Ali type==2, else #$800346 via 0x00E7E8."),
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key ROM addresses, offsets, opcodes and table words."""
    # Routine 1: Sliding/Retracting Wall or Bridge Trap
    assert u16_be(rom, 0x017230) == 0x6100       # bsr.w $18030
    assert u32_be(rom, 0x01724C) == 0x00700391   # slide extend SFX ID
    assert u32_be(rom, 0x017252) == 0x0000E7E8   # jsr $E7E8.l (play sound)
    assert u32_be(rom, 0x017294) == 0x0000BC8E   # jsr $BC8E.l (animation tick)
    assert u16_be(rom, 0x017298) == 0x6100       # bsr.w $18030

    # Routine 2: Falling Rock Boulder Trap
    assert u16_be(rom, 0x0172E2) == 0x6100       # bsr.w $17344 (impact AABB)
    assert u32_be(rom, 0x0172EC) == 0x0000C000   # gravity -$C000
    assert u32_be(rom, 0x01730E) == 0x00030000   # bounce vz +$00030000
    assert u16_be(rom, 0x017316) == 0x0003       # screen shake amplitude 3
    assert u32_be(rom, 0x017318) == 0x00FF17C4   # screen shake RAM variable $FF17C4
    assert u32_be(rom, 0x01731E) == 0x00800386   # heavy rock impact SFX ID
    assert u32_be(rom, 0x017324) == 0x0000E7FC   # jsr $E7FC.l

    # Routine 3: Falling Rock Impact AABB
    assert u32_be(rom, 0x017356) == 0x0000B922   # jsr $B922.l (AABB test vs Ali)
    assert u16_be(rom, 0x017374) == 0x8007       # damage word $8007 (7 HP)
    assert u32_be(rom, 0x017386) == 0x00020000   # launch vz +$00020000
    assert u16_be(rom, 0x01739E) == 0x0310       # speed x magnitude $310
    assert u16_be(rom, 0x0173A2) == 0x0300       # speed y magnitude $300
    assert u32_be(rom, 0x0173A6) == 0x0000E2D4   # jsr $E2D4.l (directional velocity)
    assert u32_be(rom, 0x0173BC) == 0x00800346   # rock strike SFX ID 1
    assert u32_be(rom, 0x0173C2) == 0x0000E7E8   # jsr $E7E8.l
    assert u32_be(rom, 0x0173CA) == 0x00900237   # rock strike SFX ID 2
    assert u32_be(rom, 0x0173D0) == 0x0000E7FC   # jsr $E7FC.l


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

    evidence = {
        "metadata": {
            "milestone": "M15.521",
            "rom_file": "Beyond Oasis (USA).md",
            "range_start": f"0x{RANGE_START:06X}",
            "range_end": f"0x{RANGE_END:06X}",
            "range_size_bytes": RANGE_SIZE,
            "instruction_count": len(instructions),
            "routine_count": len(routines),
        },
        "routines": routines,
        "instructions": instructions,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(json.dumps(evidence, indent=2))
    print(f"Wrote evidence JSON to {OUTPUT_PATH}")
    print(f"Total instructions extracted: {len(instructions)}")
    print(f"Routines defined: {len(routines)}")


if __name__ == "__main__":
    extract()
