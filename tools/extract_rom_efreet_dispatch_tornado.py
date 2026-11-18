#!/usr/bin/env python3
"""
Extract ROM Dytto Water Tornado Steering/Bounce & Efreet Spirit Master
14-State Dispatcher / Command Starters (0x0123E2..0x012680, 670 bytes).

Disassembles exact M68K instructions across 0x0123E2..0x0125AE and
0x0125CA..0x012680, extracts the 14-entry signed word self-relative jump
table at 0x0125AE..0x0125CA, verifies key constants, and emits evidence JSON
for Milestone M15.489.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_efreet_dispatch_tornado_m15_489.json"

RANGE_START = 0x0123E2
RANGE_END = 0x012680
RANGE_SIZE = RANGE_END - RANGE_START  # 670 bytes

TABLE_START = 0x0125AE
TABLE_END = 0x0125CA
TABLE_ENTRIES = 14

ROUTINES = [
    {
        "name": "AI_Dytto_WaterTornadoSteeringAndTrigVelocity",
        "start": "0x0123E2",
        "end": "0x01254C",
        "size_bytes": 0x01254C - 0x0123E2,
        "description": (
            "Calls 0x0123AE (8px hover), loads angle +$4E, angular vel +$50, drift "
            "timer +$51, speed +$52. Handles +$40 bit 6 X-reflection ((128-ang)&$FF) "
            "and +$41 bit 6 Y-reflection ((256-ang)&$FF) with -vel; bounces inside "
            "camera [cam_x..cam_x+$140, cam_y..cam_y+$E0] ($FF1716/$FF173E via "
            "$FF16F0 bit 7) halving turn rate and setting timer=1. On Button A "
            "($FF165E bit 6), clears $FF199E and steers shortest-arc (max step 8) "
            "toward 0x00E706 angle; else ticks 7-frame PRNG drift (+/-4 via 0x00E268). "
            "Ramps speed <= $80000 by $2000, maps $20/$60/$A0/$E0 sectors to 4-way "
            "+$16, and integrates sin/cos ($05D706) velocity into +$72/+$76."
        ),
    },
    {
        "name": "AI_Efreet_MasterTickAndDispatcher",
        "start": "0x01254C",
        "end": "0x0125CA",
        "size_bytes": 0x0125CA - 0x01254C,
        "code_end": "0x0125AE",
        "table_start": "0x0125AE",
        "table_end": "0x0125CA",
        "description": (
            "Sets a5=$FF19E8; bypasses to state dispatcher 0x0125A2 when +$A2!=0. "
            "Otherwise calls 0x00E682, 0x011A4C, 0x00E36E, ticks 35-frame ($23) SP "
            "drain timer +$8E to deduct 1 SP from +$8E(a5) (dismissing via 0x012BA8 "
            "when 0), toggles +$36 bit 1 when SP<=1 (clears when >1), and jumps "
            "via 14-entry signed word self-relative table 0x0125AE..0x0125CA ($00..$1A)."
        ),
    },
    {
        "name": "AI_Efreet_TryStartDashCharge",
        "start": "0x0125CA",
        "end": "0x012608",
        "size_bytes": 0x012608 - 0x0125CA,
        "command_code": 2,
        "description": (
            "Dispatched on $FF19A2==2 (0x0129A0): consumes 8 SP via 0x011B28 "
            "(branching to buzzer 0x012978 on Carry set), enters State $08 (+$2A=$0C), "
            "copies Ali's facing +$16(a5), resolves directional velocity (0x100,0x100) "
            "via 0x00E2D4 into +$4E/+$52, and plays SFX #$800258 via 0x00E7FC."
        ),
    },
    {
        "name": "AI_Efreet_TryStartFlameBreath",
        "start": "0x012608",
        "end": "0x01263C",
        "size_bytes": 0x01263C - 0x012608,
        "command_code": 1,
        "description": (
            "Dispatched on $FF19A2==1 (0x012998): consumes 6 SP via 0x011B28 "
            "(else 0x012978), requires slot $FF1C1C +$00==0 (else 0x012978), reserves "
            "+$00=$FFFE and stores $FF1C1C into +$A6(a6), calls 0x012C4E, and enters "
            "State $0A (+$2A=2)."
        ),
    },
    {
        "name": "AI_Efreet_TryStartFireballMeltdown",
        "start": "0x01263C",
        "end": "0x012672",
        "size_bytes": 0x012672 - 0x01263C,
        "command_code": 3,
        "description": (
            "Dispatched on $FF19A2==3 (0x0129A8): consumes 25 ($19) SP via 0x011B28 "
            "(else 0x012978), enters State $0C (+$2A=8), sets facing 2, +$AE=$60 (96), "
            "clears +$02, and sets +$AF = $FF0DB7 + $10."
        ),
    },
    {
        "name": "AI_Efreet_StartHitReaction",
        "start": "0x012672",
        "end": "0x012680",
        "size_bytes": 0x012680 - 0x012672,
        "description": "Enters Efreet State $10 with action/anim +$2A=$0A.",
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key opcodes, constants, and RAM addresses in 0x0123E2..0x012680."""
    assert u16_be(rom, 0x012400) == 0x0080      # cmp/sub anchor 0x80
    assert u16_be(rom, 0x012408) == 0x0180      # 0x180 - angle
    assert u16_be(rom, 0x012452) == 0x0140      # viewport width 320
    assert u16_be(rom, 0x012476) == 0x00E0      # viewport height 224
    assert u32_be(rom, 0x0124DA) == 0x00080000  # max speed threshold
    assert u32_be(rom, 0x0124E2) == 0x00002000  # speed ramp step
    assert u32_be(rom, 0x01252E) == 0x0005D706  # sin/cos table
    assert u32_be(rom, 0x01254E) == 0x00FF19E8  # player base a5
    assert u16_be(rom, 0x012576) == 0x0023      # 35-frame SP drain interval
    assert u32_be(rom, 0x0125FE) == 0x00800258  # dash SFX #$800258
    assert u16_be(rom, 0x012622) == 0xFFFE      # reserved flame slot sentinel
    assert u32_be(rom, 0x012664) == 0x00FF0DB7  # Efreet rank byte $FF0DB7


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
        if "command_code" in r:
            entry["command_code"] = r["command_code"]
        routines_meta.append(entry)

    evidence = {
        "milestone": "M15.489",
        "title": "ROM Dytto Water Tornado Steering/Bounce & Efreet Spirit Master 14-State Dispatcher / Command Starters",
        "range": {"start": f"0x{RANGE_START:06X}", "end": f"0x{RANGE_END:06X}", "size_bytes": RANGE_SIZE},
        "constants": {
            "tornado_max_speed_ramp_threshold": "0x00080000",
            "tornado_speed_ramp_step": "0x00002000",
            "tornado_aim_max_turn_step": 8,
            "tornado_aim_post_timer": 10,
            "tornado_drift_interval_frames": 7,
            "tornado_viewport_w": 320,
            "tornado_viewport_h": 224,
            "efreet_sp_drain_interval_frames": 35,
            "efreet_dash_sp_cost": 8,
            "efreet_dash_sfx": "0x00800258",
            "efreet_flame_sp_cost": 6,
            "efreet_flame_slot_sentinel": "0xFFFE",
            "efreet_meltdown_sp_cost": 25,
            "efreet_meltdown_field_ae": "0x60",
            "efreet_meltdown_rank_offset": "0x10",
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
    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes, 14 jump entries) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
