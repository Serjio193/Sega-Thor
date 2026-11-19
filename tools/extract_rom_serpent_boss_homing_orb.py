#!/usr/bin/env python3
"""
Extract ROM Composite Serpent Boss State $12 Homing Orb Projectile, Ali Grab Pinning,
Homing Hit Callback & Smooth Turn-Rate Steering (0x0160E2..0x016294, 434 bytes).

Disassembles 124 M68K instructions across 5 routines with Capstone, validates opcodes/addresses,
and generates evidence JSON for Milestone M15.514.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_serpent_boss_homing_orb_m15_514.json"

RANGE_START = 0x0160E2
RANGE_END = 0x016294
RANGE_SIZE = RANGE_END - RANGE_START  # 434 bytes

ROUTINE_BOUNDS = [
    ("AI_SerpentBoss_HomingOrb_Tick", 0x0160E2, 0x016168, 38,
     "State $12 Homing Orb Projectile main loop: cancels/fadeouts on damage, lifetime expiry (+$A1), anim end (+$37:1), or wall contact; tests AABB vs Ali [-4..+4] after age >= 12; calls steering."),
    ("AI_SerpentBoss_SubStepAnchorIntegrator", 0x016168, 0x016184, 8,
     "Sub-Step Anchor Integrator: decrements timer +$2; accumulates +$A6/+$AA deltas into +$72/+$76, clears Zero flag in CCR."),
    ("AI_SerpentBoss_AliGrabPinning", 0x016184, 0x0161A2, 6,
     "Ali Grab Pinning: copies boss 32-bit XYZ to Ali $FF19E8 with Y offset of -8px (pins Ali in mouth/jaw)."),
    ("AI_SerpentBoss_HomingOrb_HitCallback", 0x0161A2, 0x0161FA, 17,
     "Homing Hit Callback: checks Ali invulnerability (+$38:3); applies attacker ptr, hitstun 8, reaction mode from orb facing +$7C, SFX $800216, copies XY velocity, applies damage (2 grounded, $8002+vz=$10000 airborne), zeroes orb velocities and triggers fadeout."),
    ("AI_SerpentBoss_SmoothHomingTurnRateSteering", 0x0161FA, 0x016294, 55,
     "Smooth Homing Turn-Rate Limiter & Vector Stepper: clamps turn-rate <= 8 units/frame toward Ali (skipped when speed == $30000); quantizes angle into 4-way facing +$7C; computes sin/cos from $5D706 scaled by speed >> 7; integrates into pos +$72/+$76 and vel +$4E/+$52."),
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key ROM addresses, offsets, opcodes and table words."""
    assert u16_be(rom, 0x0160E2) == 0x0C6E       # cmpi.w #$38, $2a(a6)
    assert u16_be(rom, 0x0160E4) == 0x0038
    assert u32_be(rom, 0x01611A) == 0x0000B852   # jsr $B852.l (AABB test)
    assert u32_be(rom, 0x016186) == 0x00FF19E8   # Ali entity pointer
    assert u32_be(rom, 0x0161C0) == 0x00800216   # SFX $800216 (orb hit)
    assert u32_be(rom, 0x0161C6) == 0x0000E7FC   # jsr $E7FC.l (play sound)
    assert u32_be(rom, 0x016204) == 0x00030000   # speed threshold $30000
    assert u32_be(rom, 0x01626C) == 0x0005D706   # sin/cos table address


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    # Disassemble all instructions
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
            "milestone": "M15.514",
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
