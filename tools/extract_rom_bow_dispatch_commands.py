#!/usr/bin/env python3
"""
Extract ROM Bow Spirit (0x1E) Master Tick, 9-Entry Jump Table, Command Starters
& States $0E/$0A/$0C (0x01387C..0x013B36, 698 bytes).

Disassembles exact M68K instructions across 0x01387C..0x0138E6 and
0x0138F8..0x013B36, extracts the 18-byte 9-entry signed word self-relative
jump table at 0x0138E6..0x0138F8, verifies key constants, and emits evidence
JSON for Milestone M15.497.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_bow_dispatch_commands_m15_497.json"

RANGE_START = 0x01387C
RANGE_END = 0x013B36
RANGE_SIZE = RANGE_END - RANGE_START  # 698 bytes

TABLE_START = 0x0138E6
TABLE_END = 0x0138F8
TABLE_ENTRIES = 9

ROUTINES = [
    {
        "name": "AI_Bow_MasterTickAndStateDispatcher",
        "start": "0x01387C",
        "end": "0x0138F8",
        "size_bytes": 0x0138F8 - 0x01387C,
        "code_end": "0x0138E6",
        "table_start": "0x0138E6",
        "table_end": "0x0138F8",
        "description": (
            "Loads a5=$FF19E8; bypasses to dispatcher 0x0138DA when +$A2!=0. "
            "Otherwise calls 0x00E682 & 0x011A4C, ticks 30-frame ($1E) SP drain "
            "timer +$8E(a6) to deduct 1 SP from +$8E(a5) (dismissing via 0x013F22 "
            "when 0), toggles +$36 bit 1 when SP<=1 (clears when >1), unconditionally "
            "clears +$86.w/+$83.b/+$82.b (Bow ignores damage/hitstun), and jumps via "
            "9-entry signed word self-relative table 0x0138E6..0x0138F8 ($00..$10)."
        ),
    },
    {
        "name": "AI_Bow_State0E_FinishAndPlaySfx",
        "start": "0x0138F8",
        "end": "0x013924",
        "size_bytes": 0x013924 - 0x0138F8,
        "description": (
            "Waits for +$37 bit 1, then sets +$18=$04F8, State $02, +$2A=$14, "
            "sets bit 0 of +$36(a6), and plays SFX #$80037E via 0x00E7FC."
        ),
    },
    {
        "name": "AI_Bow_TryStartCommand1_LungeBite",
        "start": "0x013924",
        "end": "0x013996",
        "size_bytes": 0x013996 - 0x013924,
        "command_code": 1,
        "description": (
            "Command 1 starter: scans nearest target via 0x014494 (rejecting to "
            "0x013A3A on Carry set or d0>$D8), consumes 4 SP via 0x011B28 (else "
            "0x013A3A), computes 3D launch vector (scale $80) toward (x, y, z+$4A>>2) "
            "via 0x00F10C into +$4E/+$52/+$56, sets facing +$16 via 0x00E338, enters "
            "State $08 (+$2A=$22, +$02=$0C), and plays SFX #$700221 via 0x00E7FC."
        ),
    },
    {
        "name": "AI_Bow_TryStartCommand2_UndergroundDive",
        "start": "0x013996",
        "end": "0x013A18",
        "size_bytes": 0x013A18 - 0x013996,
        "command_code": 2,
        "description": (
            "Command 2 starter: requires +$10(a5)==0, +$39(a5) bit 1 clear, terrain "
            "probe 0x009C40 bit 7 clear at Ali, and 9 SP via 0x011B28 (else 0x013A3A). "
            "Calls 0x013FDC, enters State $04 (+$2A=0), computes 2D homing velocity "
            "toward $FF1C1C +$4E/+$52 via 0x00E45E, sets +$38 bit 2, +$AC=$2710, "
            "+$02=$1E, latches Ali (x,y) into +$AE/+$B2, and plays SFX #$800258."
        ),
    },
    {
        "name": "AI_Bow_TryStartCommand3_PlantBloom",
        "start": "0x013A18",
        "end": "0x013A3A",
        "size_bytes": 0x013A3A - 0x013A18,
        "command_code": 3,
        "description": (
            "Command 3 starter: consumes 28 ($1C) SP via 0x011B28 (else 0x013A3A), "
            "enters State $0A (+$2A=$10), clears +$16, and sets +$38 bit 2."
        ),
    },
    {
        "name": "AI_Bow_CommandRejectReset",
        "start": "0x013A3A",
        "end": "0x013A5C",
        "size_bytes": 0x013A5C - 0x013A3A,
        "description": (
            "Shared reject/reset handler: sets +$2A=6, State $00, +$9E=4, plays "
            "SFX #$90037D via $060004, and branches to State 0 follow loop 0x013CDC."
        ),
    },
    {
        "name": "AI_Bow_State0A_BloomWindup",
        "start": "0x013A5C",
        "end": "0x013AE8",
        "size_bytes": 0x013AE8 - 0x013A5C,
        "description": (
            "Plays SFX #$800226 when +$30==8 and +$2E==0; while +$30 < $0C increments "
            "+$10 and calls 0x014200 (d7=7). On +$37 bit 1, enters State $0C (+$2A=$26), "
            "sets +$36 bit 1, computes duration +$02=($FF0DB9<<5)+$78, sets $FF17B8 bit 0, "
            "clears +$10, snaps (x,y) to $FF1C1C +$4E/+$52, calls 0x009D00, clears "
            "$FF1C1C +$00, and plays SFX #$800380."
        ),
    },
    {
        "name": "AI_Bow_State0C_BloomActiveAndCooldown",
        "start": "0x013AE8",
        "end": "0x013B36",
        "size_bytes": 0x013B36 - 0x013AE8,
        "description": (
            "Locks $FF199E = $FF00 | $FF1971 and keeps +$36 bit 1 set while "
            "+$8E(a5)!=0 and --+$02(a6)!=0; on SP==0 or timer==0, clears +$36 bit 1, "
            "clears $FF199E and $FF17B8 bit 0, enters State $02 (+$2A=$14), and plays "
            "SFX #$80037E."
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
    """Assert key opcodes, constants, and RAM addresses in 0x01387C..0x013B36."""
    assert u32_be(rom, 0x01387E) == 0x00FF19E8  # lea $ff19e8, a5
    assert u16_be(rom, 0x0138A2) == 0x001E      # 30-frame SP drain interval
    assert u16_be(rom, 0x0138CE) == 0x426E      # clr.w $86(a6)
    assert u16_be(rom, 0x013902) == 0x04F8      # +$18 = $04F8
    assert u32_be(rom, 0x01391A) == 0x0080037E  # SFX #$80037E
    assert u16_be(rom, 0x01392E) == 0x00D8      # max bite target distance 216
    assert u16_be(rom, 0x013940) == 0x0080      # 3D launch vector scale $80
    assert u32_be(rom, 0x01398C) == 0x00700221  # bite SFX #$700221
    assert u16_be(rom, 0x0139BA) == 0x7009      # 9 SP cost
    assert u16_be(rom, 0x0139F6) == 0x2710      # +$AC = $2710
    assert u32_be(rom, 0x013A0E) == 0x00800258  # dive SFX #$800258
    assert u16_be(rom, 0x013A18) == 0x701C      # 28 SP cost
    assert u32_be(rom, 0x013A4E) == 0x0090037D  # reject SFX #$90037D
    assert u32_be(rom, 0x013A6C) == 0x00800226  # windup SFX #$800226
    assert u32_be(rom, 0x013AA6) == 0x00FF0DB9  # Bow jewel rank $FF0DB9
    assert u16_be(rom, 0x013AAE) == 0x0078      # bloom base duration 120
    assert u32_be(rom, 0x013ADE) == 0x00800380  # bloom SFX #$800380
    assert u16_be(rom, 0x013AEA) == 0xFF00      # $FF00 mask for $FF199E


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
        for key in ("code_end", "table_start", "table_end", "command_code"):
            if key in r:
                entry[key] = r[key]
        routines_meta.append(entry)

    evidence = {
        "milestone": "M15.497",
        "title": "ROM Bow Spirit (0x1E) Master Tick, 9-Entry Jump Table, Command Starters & States $0E/$0A/$0C",
        "range": {"start": f"0x{RANGE_START:06X}", "end": f"0x{RANGE_END:06X}", "size_bytes": RANGE_SIZE},
        "constants": {
            "bow_entity_type": "0x001E",
            "sp_drain_interval_frames": 30,
            "state0e_priority": "0x04F8",
            "state0e_finish_sfx": "0x0080037E",
            "cmd1_max_target_dist": 216,
            "cmd1_sp_cost": 4,
            "cmd1_vector_scale": "0x0080",
            "cmd1_timer": 12,
            "cmd1_sfx": "0x00700221",
            "cmd2_sp_cost": 9,
            "cmd2_field_ac": "0x2710",
            "cmd2_timer": 30,
            "cmd2_sfx": "0x00800258",
            "cmd3_sp_cost": 28,
            "reject_sfx": "0x0090037D",
            "windup_sfx": "0x00800226",
            "bloom_base_duration_frames": 120,
            "bloom_sfx": "0x00800380",
            "bow_rank_ram": "0x00FF0DB9",
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
    OUTPUT_PATH.write_bytes(formatted_json.encode("utf-8"))
    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes, 9 jump entries) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
