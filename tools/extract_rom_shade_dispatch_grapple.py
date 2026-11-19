#!/usr/bin/env python3
"""
Extract ROM Shade Spirit Master Tick, 40-Frame SP Drain, Damage-to-SP
Absorption, 10-State Jump Table, Shadow Grapple Starter & Dark Hole / Anchor
Warp Starter (0x012F1E..0x01319E, 640 bytes).

Disassembles exact M68K instructions across 0x012F1E..0x013008 and
0x01301C..0x01319E, extracts the 20-byte 10-entry signed word self-relative
jump table at 0x013008..0x01301C, verifies key constants, and emits evidence
JSON for Milestone M15.493.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_shade_dispatch_grapple_m15_493.json"

RANGE_START = 0x012F1E
RANGE_END = 0x01319E
RANGE_SIZE = RANGE_END - RANGE_START  # 640 bytes

TABLE_START = 0x013008
TABLE_END = 0x01301C
TABLE_ENTRIES = 10

ROUTINES = [
    {
        "name": "AI_Shade_MasterTickAndSpAbsorption",
        "start": "0x012F1E",
        "end": "0x01301E",
        "size_bytes": 0x01301E - 0x012F1E,
        "code_head_end": "0x013008",
        "table_start": "0x013008",
        "table_end": "0x01301C",
        "state12_rts_addr": "0x01301C",
        "entrypoints": {
            "MasterEntry": "0x012F1E",
            "DamageAbsorptionCheck": "0x012F80",
            "StateDispatcher": "0x012FFC",
            "State12_NoOpReturn": "0x01301C",
        },
        "description": (
            "Loads a5=$FF19E8; returns immediately if +$A4!=0; bypasses to "
            "dispatcher 0x012FFC if +$A2!=0; clears $FF1989; calls 0x00E682 and "
            "0x011A4C; ticks 40-frame ($28) timer +$8E to deduct 1 SP from +$8E(a5) "
            "(dismissing via 0x0136CC on 0); toggles +$36 bit 1 when SP<=1. On "
            "+$86(a6)!=0 damage: clears +$86, sets +$83=max(8, ((+$83++ $82)*2)&$FF), "
            "clears +$82, subtracts (+$86&$0FFF) from +$8E(a5) clamped >=0, loads "
            "attacker +$7E->a0, sets +$39 bit 3 if d7 bit 13 set, enters State $04, "
            "copies +$84 facing (bchg #1 if bit 0 set), adds +$0C+=1 if facing!=0 "
            "and +$10+=3, calls 0x008EF8 (d0=$0C) & 0x013764, and dispatches via "
            "10-entry jump table 0x013008..0x01301C (State $12 targets rts at 0x01301C)."
        ),
    },
    {
        "name": "AI_Shade_TryStartShadowGrapple",
        "start": "0x01301E",
        "end": "0x013134",
        "size_bytes": 0x013134 - 0x01301E,
        "command_code": 1,
        "description": (
            "Command 1 Shadow Grapple starter: guards +$04(a5)==0 and terrain probe "
            "0x009C40 bit 7 clear, consumes 7 SP via 0x011B28 (else buzzer 0x01319E), "
            "sets $FF1982=$FF, +$38(a5)|=8, $FF17B8|=4, State $06 (+$2A=2), calls "
            "0x013764, snaps Shade to Ali, zeroes Ali & Shade +$72/+$76, integrates "
            "(0x1000,0x1000) into +$72/+$76 and (0x100,0x100) into +$4E/+$52 via "
            "0x00E2D4, sets +$10=1, +$02=$12, clears +$38 bit 2 & +$AA, allocates "
            "Pool 1 tether slot via 0x00DA1A (+$00=$FFFF, +$22=0x013576, +$32=$60, "
            "+$34=$FFFF, +$38=$0D, prio (parent_18&$8000)|$07A8, +$1A=*($03F4A6), "
            "doubly links +$A6), and plays SFX #$80037B."
        ),
    },
    {
        "name": "AI_Shade_TryStartShadowAnchorOrRescue",
        "start": "0x013134",
        "end": "0x01319E",
        "size_bytes": 0x01319E - 0x013134,
        "command_code": 3,
        "description": (
            "Command 3 Dark Hole / Anchor Warp starter: requires $FF1986!=0 (else "
            "0x01319E), sets +$38(a5)|=$4C, clears $FF198E, sets $FF17B8|=4, clears "
            "+$06(a5), snaps Shade to Ali and zeroes +$72/+$76, sets $FF1985=$FF, "
            "clears +$16, enters State $0C (+$2A=$0E), calls 0x013764, and plays "
            "SFX #$80037C."
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
    """Assert key opcodes, constants, and RAM addresses in 0x012F1E..0x01319E."""
    assert u32_be(rom, 0x012F20) == 0x00FF19E8  # lea $ff19e8, a5
    assert u32_be(rom, 0x012F38) == 0x00FF1989  # sf $ff1989
    assert u16_be(rom, 0x012F54) == 0x0028      # 40-frame SP drain interval
    assert u16_be(rom, 0x012F9C) == 0x0008      # min hitstun 8
    assert u16_be(rom, 0x012FAC) == 0x0FFF      # damage-to-SP mask
    assert u16_be(rom, 0x01301C) == 0x4E75      # rts for State $12
    assert u16_be(rom, 0x013036) == 0x7007      # 7 SP cost
    assert u32_be(rom, 0x013042) == 0x00FF1982  # st $ff1982
    assert u16_be(rom, 0x01308A) == 0x1000      # launch offset 0x1000
    assert u16_be(rom, 0x0130B8) == 0x0012      # grapple timer 18
    assert u32_be(rom, 0x0130D8) == 0x00013576  # tether callback
    assert u32_be(rom, 0x013114) == 0x0003F4A6  # sprite table ptr ptr
    assert u32_be(rom, 0x013126) == 0x0080037B  # grapple SFX #$80037B
    assert u32_be(rom, 0x013136) == 0x00FF1986  # tst $ff1986
    assert u32_be(rom, 0x013178) == 0x00FF1985  # st $ff1985
    assert u32_be(rom, 0x013192) == 0x0080037C  # anchor SFX #$80037C


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
        for key in ("code_head_end", "table_start", "table_end", "state12_rts_addr", "entrypoints", "command_code"):
            if key in r:
                entry[key] = r[key]
        routines_meta.append(entry)

    evidence = {
        "milestone": "M15.493",
        "title": "ROM Shade Spirit Master Tick, 40-Frame SP Drain, Damage-to-SP Absorption, 10-State Jump Table, Shadow Grapple & Anchor Warp Starters",
        "range": {"start": f"0x{RANGE_START:06X}", "end": f"0x{RANGE_END:06X}", "size_bytes": RANGE_SIZE},
        "constants": {
            "shade_sp_drain_interval_frames": 40,
            "damage_sp_mask": "0x0FFF",
            "damage_min_hitstun": 8,
            "damage_knockback_bit": 13,
            "damage_hit_state": "0x0004",
            "damage_hit_anim": "0x000C",
            "grapple_sp_cost": 7,
            "grapple_state": "0x0006",
            "grapple_anim": "0x0002",
            "grapple_launch_offset": "0x1000",
            "grapple_velocity": "0x0100",
            "grapple_timer_frames": 18,
            "grapple_tether_type": "0xFFFF",
            "grapple_tether_callback": "0x00013576",
            "grapple_tether_param_32": "0x0060",
            "grapple_tether_priority_base": "0x07A8",
            "grapple_sprite_ptr_ptr": "0x0003F4A6",
            "grapple_sfx": "0x0080037B",
            "anchor_ali_flags_38_mask": "0x4C",
            "anchor_state": "0x000C",
            "anchor_anim": "0x000E",
            "anchor_sfx": "0x0080037C",
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
    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes, 10 jump entries) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
