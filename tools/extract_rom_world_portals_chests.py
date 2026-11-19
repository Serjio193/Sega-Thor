#!/usr/bin/env python3
"""
Extract ROM World Interactive Objects: Room Doors, Vortex Portals, Spirit Locks & Treasure Chests
(0x017A96..0x017E28, 914 bytes).

Disassembles 225 M68K instructions across 3 routines, validates opcodes and RAM/SFX
constants, and generates evidence JSON for Milestone M15.524.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_world_portals_chests_m15_524.json"

RANGE_START = 0x017A96
RANGE_END = 0x017E28
RANGE_SIZE = RANGE_END - RANGE_START  # 914 bytes

ROUTINE_BOUNDS = [
    ("AI_World_RoomDoorPortalWarpTrigger", 0x017A96, 0x017CA6, 127,
     "Room Door, Portal & Warp Trigger: type!=$7C tests AABB contact vs Ali via 0x00B922 [-3..+3, -1..+1, 0..1000]; if target room +$4E != $FF0E40 sets transition flags $FF17BB/$FF17BF, plays SFX #$800230 via $60004, executes special room $90B check via 0x00C2EC, jumps to 0x00CA24; same-room door teleports Ali to (+$52, +$54) via 0x009D00; vortex portal type==$7C tests cutscene locks $FF1985/$FF1987, filters Ali excluded states, exerts suction via 0x00E45E with thresholds 1/24/40/56 into warp state $6E with target (+$AE, +$B0, +$B2), anim $78, SFX #$800230."),
    ("AI_World_SpiritLockPersistentDespawner", 0x017CA6, 0x017D12, 28,
     "Spirit/Key Lock Trigger & Persistent Despawner: 0x017CA6 checks +$4C:7 and $FF1985 for visibility toggle, evaluates spirit sparkle $FF0DBA:1 contact via 0x00B922 [-10..+10, -6..+6, 0..4]: on contact with type $16 (Spirit Sparkle) reveals lock with anim 2 and SFX #$80022F via 0x00E7FC; 0x017D06 clears +$0 and invokes persistent callback 0x017FF2."),
    ("AI_World_TreasureChest", 0x017D12, 0x017E28, 70,
     "Treasure Chest: init sets +$66=$FF, height z=+$A0; auto-open bit 7 sets state 0x0A, anim+=2, timer=1; ballistic gravity -$C000 applies to z>0, floor impact z<0 triggers bounce vz=+$30000 with SFX #$400376 via 0x00E7FC; hit-to-open on damage +$86!=0 advances state 1, timer 2, anim+=2, plays open SFX #$900232 via 0x00E7FC; timer expiry invokes persistent callback 0x017FF2 (if +$9E!=0), spawns loot item +$8C via 0x00F53C, applies parameters +$A2/+$A4, and despawns chest."),
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key ROM addresses, offsets, opcodes and table words."""
    # Routine 1: Room Door & Portal Trigger
    assert u32_be(rom, 0x017AB4) == 0x0000B922   # jsr $B922.l (AABB test)
    assert u32_be(rom, 0x017ACA) == 0x00FF0E40   # current room RAM $FF0E40
    assert u32_be(rom, 0x017B14) == 0x0000C2EC   # jsr $C2EC.l
    assert u32_be(rom, 0x017B22) == 0x00800230   # door transition SFX ID
    assert u32_be(rom, 0x017B28) == 0x00060004   # jsr $60004.l
    assert u32_be(rom, 0x017B3E) == 0x0000CA24   # jmp $CA24.l
    assert u32_be(rom, 0x017B6C) == 0x00009D00   # jsr $9D00.l
    assert u32_be(rom, 0x017C2C) == 0x0000E45E   # jsr $E45E.l (homing suction)

    # Routine 2: Spirit Lock & Persistent Despawner
    assert u32_be(rom, 0x017CDE) == 0x0000B922   # jsr $B922.l
    assert u32_be(rom, 0x017CFA) == 0x0080022F   # spirit unlock SFX ID
    assert u32_be(rom, 0x017D00) == 0x0000E7FC   # jsr $E7FC.l
    assert u16_be(rom, 0x017D0C) == 0x6100       # bsr.w $17FF2

    # Routine 3: Treasure Chest
    assert u32_be(rom, 0x017D4A) == 0x0000C000   # gravity -$C000
    assert u32_be(rom, 0x017D6C) == 0x00030000   # bounce vz +$00030000
    assert u32_be(rom, 0x017D74) == 0x00400376   # chest bounce SFX ID
    assert u32_be(rom, 0x017D7A) == 0x0000E7FC   # jsr $E7FC.l
    assert u32_be(rom, 0x017DAE) == 0x00900232   # chest open SFX ID
    assert u32_be(rom, 0x017DB4) == 0x0000E7FC   # jsr $E7FC.l
    assert u32_be(rom, 0x017DD4) == 0x0000F53C   # jsr $F53C.l (loot spawner)


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
            "milestone": "M15.524",
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
