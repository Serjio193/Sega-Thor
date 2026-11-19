#!/usr/bin/env python3
"""
Extract ROM World Interactive Objects: Patrolling/Pacing Trap, Persistent Gate & Hidden Bridge,
and Bomb-Destructible Barrier (0x01778C..0x017A96, 778 bytes).

Disassembles 204 M68K instructions across 3 routines, validates opcodes and RAM/SFX
constants, records embedded tables, and generates evidence JSON for Milestone M15.523.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_world_patrol_gate_bomb_m15_523.json"

RANGE_START = 0x01778C
RANGE_END = 0x017A96
RANGE_SIZE = RANGE_END - RANGE_START  # 778 bytes

ROUTINE_DEFS = [
    (
        "AI_World_PatrollingPacingTrap",
        0x01778C,
        0x0178EA,
        [
            (0x01778C, 0x01783E),
            (0x01784E, 0x0178EA),
        ],
        93,
        "Patrolling / Pacing Bridge Trap: sub-mode 0 saves home coordinates to +$A8/+$AA and height z=+$8C; sub-mode 1 scans boundary tiles via 0x00C1CE using 4x4-byte signed vector table at 0x01783E, computes directional velocity via 0x00E2D4 from speed +$A0, sets timer +$2=+$9E, anim 2, plays trap start SFX #$800399 via 0x00E7E8; sub-mode 2 animates via 0x00BC8E, accelerates then decelerates past midpoint +$9E/2 while integrating position deltas into +$72/+$76, flips direction bit +$17:1 at timer expire, sets anim 4, enters sub-mode 3; sub-mode 3 homes back to +$A8/+$AA via 0x00E45E, on arrival flips +$17:1, anim 0, returns to sub-mode 1."
    ),
    (
        "AI_World_PersistentGateHiddenBridge",
        0x0178EA,
        0x0179D4,
        [
            (0x0178EA, 0x017920),
            (0x017926, 0x0179D4),
        ],
        59,
        "Persistent Gate & Hidden Bridge Controller: init sets +$66=1, anim 2*(+$8C), raises z+=1; dispatches via 3-entry jump table at 0x017920: Entry 0 queries persistent flag via 0x018030 to toggle visibility +$36:1, solid collision +$38:2, and elevation z=10000 vs 0, playing gate rumble SFX #$B00385 via $60004 on elevation change; Entry 1 waits 10 frames then checks living enemies 0x0101EC <= +$A0 to trigger gate unlock callback 0x017D06 with chime SFX #$A00228; Entry 2 counts 15-frame delay on +$2 before triggering 0x017D06."
    ),
    (
        "AI_World_BombDestructibleBarrier",
        0x0179D4,
        0x017A96,
        [
            (0x0179D4, 0x017A96),
        ],
        52,
        "Bomb-Destructible Barrier: sub-mode 0 initializes via 0x0178F2, if +$9E!=0 enters sub-mode 3 with flipped visibility +$36:1; sub-modes 1 and 3 on hit +$86!=0 check attacker ptr +$7E==$FF1AA4 and entity type==$1E (Bomb): if bomb, invokes persistent callback 0x017FF2, plays explosion SFX #$900202 via 0x00E7FC, transitions to destruction anim (sub-mode 2, despawns on anim done +$37:1) or instantly despawns +$0=0; if non-bomb, plays deflection clink SFX #$900238."
    ),
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key ROM addresses, offsets, opcodes and table words."""
    # Routine 1: Patrolling / Pacing Trap
    assert u32_be(rom, 0x0177FA) == 0x0000C1CE   # jsr $C1CE.l (tile collision scan)
    assert u32_be(rom, 0x01780E) == 0x0000E2D4   # jsr $E2D4.l (directional velocity)
    assert u32_be(rom, 0x017832) == 0x00800399   # trap start SFX ID
    assert u32_be(rom, 0x017838) == 0x0000E7E8   # jsr $E7E8.l (play sound)
    assert u32_be(rom, 0x017852) == 0x0000BC8E   # jsr $BC8E.l (animation tick)
    assert u32_be(rom, 0x0178C4) == 0x0000E45E   # jsr $E45E.l (homing step)

    # Tile scan table (16 bytes) at 0x01783E
    scan_table = rom[0x01783E:0x01784E]
    assert len(scan_table) == 16
    assert scan_table[0] == 0xFF and scan_table[1] == 0xE8

    # Routine 2: Persistent Gate & Hidden Bridge
    assert u16_be(rom, 0x017928) == 0x6100       # bsr.w $18030
    assert u16_be(rom, 0x017948) == 0x2710       # hidden elevation z = 10000 (0x2710)
    assert u32_be(rom, 0x01796A) == 0x00B00385   # gate rumble SFX ID
    assert u32_be(rom, 0x017970) == 0x00060004   # jsr $60004.l (sound driver)
    assert u32_be(rom, 0x01798A) == 0x000101EC   # jsr $101EC.l (living enemies count)
    assert u32_be(rom, 0x01799C) == 0x00A00228   # secret chime SFX ID
    assert u32_be(rom, 0x0179A2) == 0x00060004   # jsr $60004.l

    # Jump table at 0x017920
    assert u16_be(rom, 0x017920) == 0x0008       # Entry 0 offset (+8 -> 0x017928)
    assert u16_be(rom, 0x017922) == 0x0054       # Entry 1 offset (+0x54 -> 0x017976)
    assert u16_be(rom, 0x017924) == 0x0002       # Entry 2 offset (+2 -> 0x017926 rts)

    # Routine 3: Bomb-Destructible Barrier
    assert u32_be(rom, 0x017A10) == 0x00FF1AA4   # bomb attacker slot address
    assert u16_be(rom, 0x017A18) == 0x001E       # bomb entity type 0x1E
    assert u32_be(rom, 0x017A34) == 0x00900202   # bomb explosion SFX ID
    assert u32_be(rom, 0x017A3A) == 0x0000E7FC   # jsr $E7FC.l
    assert u32_be(rom, 0x017A46) == 0x00900238   # clink SFX ID
    assert u32_be(rom, 0x017A4C) == 0x0000E7FC   # jsr $E7FC.l
    assert u32_be(rom, 0x017A8A) == 0x00900202   # bomb explosion SFX ID


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    routines = []

    for name, r_start, r_end, code_ranges, exp_insns, desc in ROUTINE_DEFS:
        r_insns = []
        for c_start, c_end in code_ranges:
            c_bytes = rom[c_start:c_end]
            for insn in md.disasm(c_bytes, c_start):
                insn_entry = {
                    "address": f"0x{insn.address:06X}",
                    "mnemonic": insn.mnemonic,
                    "op_str": insn.op_str,
                    "bytes": insn.bytes.hex(),
                    "size": insn.size,
                }
                r_insns.append(insn_entry)
                instructions.append(insn_entry)

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
            "milestone": "M15.523",
            "rom_file": "Beyond Oasis (USA).md",
            "range_start": f"0x{RANGE_START:06X}",
            "range_end": f"0x{RANGE_END:06X}",
            "range_size_bytes": RANGE_SIZE,
            "instruction_count": len(instructions),
            "routine_count": len(routines),
        },
        "embedded_tables": {
            "patrol_tile_scan_table_0x01783e": [hex(b) for b in rom[0x01783E:0x01784E]],
            "gate_jump_table_0x017920": [
                f"0x{u16_be(rom, 0x017920):04X}",
                f"0x{u16_be(rom, 0x017922):04X}",
                f"0x{u16_be(rom, 0x017924):04X}",
            ],
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
