#!/usr/bin/env python3
"""
Extract ROM Dytto Spirit 16-State Dispatcher, Healing Cast Sub-Entity Spawner
& Jewel-Rank HP Recovery Subsystem (0x011BA0..0x011D8A, 490 bytes).

Disassembles exact M68K instructions across 0x011BA0..0x011BAC and
0x011BCC..0x011D8A, extracts the 16-entry relative jump table at
0x011BAC..0x011BCC, verifies key constants, and emits evidence JSON for
Milestone M15.486.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_dytto_healing_drop_m15_486.json"

RANGE_START = 0x011BA0
RANGE_END = 0x011D8A
RANGE_SIZE = RANGE_END - RANGE_START  # 490 bytes

TABLE_START = 0x011BAC
TABLE_END = 0x011BCC
TABLE_ENTRIES = 16

ROUTINES = [
    {
        "name": "AI_Dytto_StateDispatcher",
        "start": "0x011BA0",
        "end": "0x011BCC",
        "size_bytes": 0x011BCC - 0x011BA0,
        "code_end": "0x011BAC",
        "table_start": "0x011BAC",
        "table_end": "0x011BCC",
        "description": (
            "Reads state word +$04(a6) (d0 = $00..$1E in steps of 2), loads entry "
            "address a1 = 0x011BAC + d0, adds signed 16-bit displacement *(int16*)a1 "
            "(self-relative to each table slot address), and jumps to handler."
        ),
    },
    {
        "name": "AI_Dytto_TryStartHealingCast",
        "start": "0x011BCC",
        "end": "0x011BF2",
        "size_bytes": 0x011BF2 - 0x011BCC,
        "description": (
            "Attempts to start Dytto's healing drop cast: consumes 20 ($14) SP via "
            "0x011B28 (branching to error buzzer 0x01219A on Carry set), plays SFX "
            "#$800258 via 0x00E7FC, clears +$16(a6), enters State $18 (+$04=$18), "
            "and sets anim/action +$2A=$0E."
        ),
    },
    {
        "name": "AI_Dytto_State18_HealingCastAndSpawnOrb",
        "start": "0x011BF2",
        "end": "0x011C7A",
        "size_bytes": 0x011C7A - 0x011BF2,
        "description": (
            "On animation frame +$30(a6) == $28 (40), allocates Pool 1 slot via "
            "0x00DA1A, sets Type $18 and calls entity init 0x008D06, sets sprite "
            "table #$00141374, priority (+$18(a4) & $8000) | $07A8, +$36=8, +$38=$0D, "
            "+$A2=$FF, State $12 (+$2A=$20), copies parent (x,y,z) with +$10 += $20, "
            "and clears +$16. When +$30 != $28, returns parent to State 0 (0x0121AA) "
            "if +$37 bit 1 is set."
        ),
    },
    {
        "name": "AI_Dytto_State12_HealingOrbHomingAndHpRestore",
        "start": "0x011C7A",
        "end": "0x011D7C",
        "size_bytes": 0x011D7C - 0x011C7A,
        "description": (
            "Steps +$10(a6) toward +$10(a5)+$20 by clamped [-3..+3] while computing "
            "d6 = abs(target_z - z_before_step); homes 2D velocity toward Ali "
            "(+$08(a5), +$0C(a5)+6) via 0x00E45E (doubling velocities into +$72/+$76). "
            "While d6 > 4 or d4 > 2, spawns sparkle sub-entity (State $14, +$2A=$1C, "
            "PRNG jitter [-3..+4]) on even $FF1971 frames via 0x00DA1A+0x00E66E+0x00E268. "
            "On arrival (d6 <= 4 and d4 <= 2), despawns orb and restores Ali's HP "
            "+$8A(a5) by tier = ($FF0DB6+1)>>2 (0:max>>3, 1:max>>2, 2:max>>1, "
            "3:(max>>1)+(max>>2), 4:max), clamped to +$88(a5) unless $FF1987!=0, "
            "and plays #$D00355 via 0x060004."
        ),
    },
    {
        "name": "AI_Dytto_State14_HealingSparkleDespawn",
        "start": "0x011D7C",
        "end": "0x011D8A",
        "size_bytes": 0x011D8A - 0x011D7C,
        "description": (
            "Despawns healing trail sparkle sub-entity (clr.w +$00(a6)) when "
            "animation completion bit 1 of +$37(a6) is set."
        ),
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key opcodes, constants, and targets in 0x011BA0..0x011D8A."""
    assert u16_be(rom, 0x011BA0) == 0x302E  # move.w $4(a6), d0
    assert u16_be(rom, 0x011BCC) == 0x7014  # moveq #$14, d0 (20 SP)
    assert u32_be(rom, 0x011BD8) == 0x00800258  # SFX #$800258
    assert u16_be(rom, 0x011BF4) == 0x0028  # cmpi.w #$28, $30(a6)
    assert u32_be(rom, 0x011C22) == 0x00008D06  # jsr $8d06.l
    assert u32_be(rom, 0x011C28) == 0x00141374  # sprite table #$141374
    assert u16_be(rom, 0x011C34) == 0x8000  # andi.w #$8000, d0
    assert u16_be(rom, 0x011C38) == 0x07A8  # ori.w #$7a8, d0
    assert u16_be(rom, 0x011C50) == 0x0012  # State $12
    assert u16_be(rom, 0x011C56) == 0x0020  # Action $20
    assert u16_be(rom, 0x011C6E) == 0x0020  # addi.w #$20, $10(a6)
    assert u16_be(rom, 0x011C80) == 0x0020  # addi.w #$20, d6
    assert u16_be(rom, 0x011CB6) == 0x0006  # addi.w #$6, d2
    assert u16_be(rom, 0x011CCA) == 0x0004  # cmpi.w #$4, d6
    assert u16_be(rom, 0x011CD0) == 0x0002  # cmpi.w #$2, d4
    assert u32_be(rom, 0x011CD8) == 0x00FF1971  # btst.b #0, $ff1971.l
    assert u16_be(rom, 0x011CEE) == 0x0014  # State $14
    assert u16_be(rom, 0x011CF4) == 0x001C  # Action $1C
    assert u32_be(rom, 0x011D24) == 0x00FF0DB6  # move.b $ff0db6.l, d2
    assert u32_be(rom, 0x011D64) == 0x00FF1987  # tst.b $ff1987.l
    assert u32_be(rom, 0x011D70) == 0x00D00355  # SFX #$D00355
    assert u32_be(rom, 0x011D76) == 0x00060004  # jsr $60004.l


def disasm_range(md: capstone.Cs, rom: bytes, start: int, end: int) -> list[dict]:
    items = []
    pos = start
    for insn in md.disasm(rom[start:end], start):
        items.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size,
        })
        pos = insn.address + insn.size
    assert pos == end, f"Disassembly ended at 0x{pos:06X} != 0x{end:06X}"
    return items


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    assert len(rom[RANGE_START:RANGE_END]) == RANGE_SIZE
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    jump_table = []
    for i in range(TABLE_ENTRIES):
        slot_addr = TABLE_START + i * 2
        disp = s16_be(rom, slot_addr)
        target = slot_addr + disp
        jump_table.append({
            "index": i,
            "state": f"0x{i * 2:02X}",
            "entry_address": f"0x{slot_addr:06X}",
            "displacement_hex": f"0x{disp & 0xFFFF:04X}",
            "displacement": disp,
            "target": f"0x{target:06X}",
        })

    insns_head = disasm_range(md, rom, RANGE_START, TABLE_START)
    insns_tail = disasm_range(md, rom, TABLE_END, RANGE_END)
    instructions = insns_head + insns_tail

    routines_meta = []
    for r in ROUTINES:
        r_start = int(r["start"], 16)
        r_end = int(r["end"], 16)
        r_insns = [ins for ins in instructions if r_start <= int(ins["address"], 16) < r_end]
        entry = {
            "name": r["name"],
            "start": r["start"],
            "end": r["end"],
            "size_bytes": r["size_bytes"],
            "instruction_count": len(r_insns),
            "description": r["description"],
        }
        if "table_start" in r:
            entry["code_end"] = r["code_end"]
            entry["table_start"] = r["table_start"]
            entry["table_end"] = r["table_end"]
        routines_meta.append(entry)

    evidence = {
        "milestone": "M15.486",
        "title": "ROM Dytto Spirit 16-State Dispatcher, Healing Cast Sub-Entity Spawner & Jewel-Rank HP Recovery Subsystem",
        "range": {"start": f"0x{RANGE_START:06X}", "end": f"0x{RANGE_END:06X}", "size_bytes": RANGE_SIZE},
        "constants": {
            "healing_cast_sp_cost": 20,
            "cast_sfx_id": "0x00800258",
            "heal_sfx_id": "0x00D00355",
            "orb_spawn_anim_frame": 40,
            "orb_entity_type": "0x0018",
            "orb_sprite_table_ptr": "0x00141374",
            "orb_priority_mask": "0x8000",
            "orb_priority_vram_base": "0x07A8",
            "orb_z_offset": 32,
            "orb_z_step_clamp": 3,
            "orb_arrival_dz_max": 4,
            "orb_arrival_d4_max": 2,
            "sparkle_state": "0x0014",
            "sparkle_action": "0x001C",
            "jewel_rank_ram": "0x00FF0DB6",
            "hp_suppress_flag_ram": "0x00FF1987",
            "frame_counter_ram": "0x00FF1971",
        },
        "jump_table": {
            "address": f"0x{TABLE_START:06X}",
            "end": f"0x{TABLE_END:06X}",
            "size_bytes": TABLE_END - TABLE_START,
            "encoding": "signed_16bit_self_relative_to_entry_address",
            "entries": jump_table,
        },
        "routines": routines_meta,
        "instruction_count": len(instructions),
        "instructions": instructions,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    jt_placeholder = "__JUMP_TABLE_PLACEHOLDER__"
    ins_placeholder = "__INSTRUCTIONS_PLACEHOLDER__"
    raw_json = json.dumps(
        {
            **evidence,
            "jump_table": {**evidence["jump_table"], "entries": jt_placeholder},
            "instructions": ins_placeholder,
        },
        indent=2,
    )
    jt_lines = ",\n".join(f"      {json.dumps(item)}" for item in jump_table)
    ins_lines = ",\n".join(f"    {json.dumps(item)}" for item in instructions)
    formatted_json = (
        raw_json.replace(f'"{jt_placeholder}"', f"[\n{jt_lines}\n    ]")
        .replace(f'"{ins_placeholder}"', f"[\n{ins_lines}\n  ]")
        + "\n"
    )
    assert json.loads(formatted_json) == evidence
    OUTPUT_PATH.write_text(formatted_json, encoding="utf-8")
    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes, 16 jump entries) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
