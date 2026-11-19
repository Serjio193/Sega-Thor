#!/usr/bin/env python3
"""
Extract ROM Serpent Boss Phase 1 Body Contact AABB, Homing Orb Spawner,
PRNG Action Selector & 4-Limb Animation Broadcaster (0x016764..0x016A64, 768 bytes).

Disassembles 173 M68K instructions across 4 routines, validates opcodes and RAM/SFX
constants, and generates evidence JSON for Milestone M15.517.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_serpent_boss_phase1_actions_m15_517.json"

RANGE_START = 0x016764
RANGE_END = 0x016A64
RANGE_SIZE = RANGE_END - RANGE_START  # 768 bytes

ROUTINE_BOUNDS = [
    ("AI_SerpentBoss_Phase1BodyContactAABB", 0x016764, 0x0167BE, 22,
     "Phase 1 Body Contact AABB & Ali Knockback: checks collision box [-$40..+$40, -+$44..+$44] via 0x00B852; on non-immune Ali inflicts damage $8014 (20 HP), hitstun 5, vy=+$40000 (+4.0 px), vz=-1, attacker ptr a6."),
    ("AI_SerpentBoss_Phase1HomingOrbSpawner", 0x0167BE, 0x01685A, 31,
     "Phase 1 Homing Orb Spawner: allocates Pool 2 slot via $DA2A+$E66E; initializes State $12, tile $200, bounds 8x8x16, spawn pos (x-1, y+16), angle +$7A=d1, speed +$56=d2, anim $16, lifetime (prng&7)+$A1, vy=+$20000, SFX #$70035C."),
    ("AI_SerpentBoss_Phase1ActionSelector", 0x01685A, 0x016A46, 110,
     "Phase 1 Action Selector: broadcasts anim $28 via 0x016A46; checks lock $FF1987; evaluates Ali distance via 0x00E398: if d4<24 and d3>=32 enters State 2 Glide (limb anims $20/$1A/$1E/$1C or reversed, SFX #$70021A); else if d2>=0 rolls 0..99: 0..29 Fan Barrage (State $12, anim $C), 30..59 Arm Grab (State $0A, aim velocity to Ali, SFX #$700219), 60..79 State $04, 80..89 Dive Leap (State $0C, SFX #$700219), 90..99 State 0 Idle."),
    ("AI_SerpentBoss_4LimbAnimBroadcaster", 0x016A46, 0x016A64, 10,
     "4-Limb Animation Broadcaster: writes anim ID in d0 to +$2A across 4 sequential $BC-strided child slots."),
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key ROM addresses, offsets, opcodes and table words."""
    assert u16_be(rom, 0x016768) == 0x0040       # half-extent 0x40 (64)
    assert u32_be(rom, 0x016778) == 0x0000B852   # jsr $B852.l (AABB test)
    assert u16_be(rom, 0x01678E) == 0x8014       # damage word $8014 (20 HP)
    assert u32_be(rom, 0x0167AE) == 0x00040000   # knockback vy +$40000
    assert u32_be(rom, 0x01684C) == 0x0070035C   # orb launch SFX ID
    assert u32_be(rom, 0x016864) == 0x00FF1987   # dialogue/event lock flag
    assert u32_be(rom, 0x01690A) == 0x0070021A   # glide hiss SFX ID
    assert u32_be(rom, 0x016972) == 0x00FF2084   # arm/head child slot $FF2084
    assert u32_be(rom, 0x0169AA) == 0x00FF19F0   # Ali target X
    assert u32_be(rom, 0x0169B6) == 0x00FF19F4   # Ali target Y
    assert u32_be(rom, 0x0169D6) == 0x00700219   # roar/rumble SFX ID


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
            "milestone": "M15.517",
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
