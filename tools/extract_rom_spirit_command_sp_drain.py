#!/usr/bin/env python3
"""
Extract ROM Spirit Button A Command Decoder, Shared SP Guard & Dytto Master Tick
(0x011A4C..0x011BA0, 340 bytes).

Disassembles exact M68K instructions, verifies key constants/addresses, and
generates evidence JSON for Milestone M15.485.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_spirit_command_sp_drain_m15_485.json"

RANGE_START = 0x011A4C
RANGE_END = 0x011BA0
RANGE_SIZE = RANGE_END - RANGE_START  # 340 bytes

ROUTINES = [
    {
        "name": "Spirit_PollButtonACommandInput",
        "start": "0x011A4C",
        "end": "0x011B28",
        "size_bytes": 0x011B28 - 0x011A4C,
        "description": (
            "Polls Button A ($FF165E & 0x40) unless cutscene/lock flags $FF1982, "
            "$FF1983, or $FF19A7 are non-zero. Tracks hold counter $FF199E (0x14 / "
            "20-frame threshold) and 6-frame double-tap countdown $FF19A1, latches "
            "$FF165E to $FF19A3 and $FF19FE to $FF19A4 on initial press, suppresses "
            "re-triggering with $FF19A0 = 0xFF, and writes command code $FF19A2 "
            "(0 = none, 1 = single tap, 2 = double tap, 3 = 20+ frame hold release)."
        ),
    },
    {
        "name": "Spirit_TryConsumePlayerSP",
        "start": "0x011B28",
        "end": "0x011B4E",
        "size_bytes": 0x011B4E - 0x011B28,
        "description": (
            "Shared spirit ability SP-cost guard. Reads Ali's SP +$8E(a5), subtracts "
            "requested cost d0.w, commits the updated SP and clears Carry (CCR &= ~1) "
            "if signed result >= 1, or leaves SP unchanged and sets Carry (CCR |= 1) "
            "if signed result < 1, preserving CCR across movem.l (a7)+, d1."
        ),
    },
    {
        "name": "AI_Dytto_MasterTickAndSpDrain",
        "start": "0x011B4E",
        "end": "0x011BA0",
        "size_bytes": 0x011BA0 - 0x011B4E,
        "description": (
            "Dytto master per-frame update entry point. Loads player base a5 = $FF19E8; "
            "when sub-entity flag +$A2(a6) != 0 skips directly to state dispatcher $11BA0. "
            "Otherwise calls Spirit_PollButtonACommandInput (0x011A4C) and helper 0x00E36E, "
            "ticks 45-frame (0x2D) drain timer +$8E(a6) to deduct 1 SP from Ali +$8E(a5) "
            "(branching to spirit dismissal $12314 if SP is 0), and toggles low-SP blink "
            "bit 1 on +$36(a6) when Ali's SP <= 1 (clearing bit 1 when SP > 1)."
        ),
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key opcodes, RAM addresses, and constants in 0x011A4C..0x011BA0."""
    assert u32_be(rom, 0x011A50) == 0x00FF1982  # tst.b $ff1982.l
    assert u32_be(rom, 0x011A5A) == 0x00FF1983  # tst.b $ff1983.l
    assert u32_be(rom, 0x011A64) == 0x00FF19A7  # tst.b $ff19a7.l
    assert u32_be(rom, 0x011A6E) == 0x00FF165E  # move.b $ff165e.l, d7
    assert u16_be(rom, 0x011A74) == 0x0040      # andi.b #$40, d7
    assert u16_be(rom, 0x011AEA) == 0x0014      # cmpi.w #$14, $ff199e.l
    assert u16_be(rom, 0x011AFC) == 0x0006      # move.b #$6, $ff19a1.l
    assert u32_be(rom, 0x011B22) == 0x00FF19A2  # move.b d0, $ff19a2.l
    assert u16_be(rom, 0x011B32) == 0x0001      # cmpi.w #$1, d1
    assert u16_be(rom, 0x011B3E) == 0x00FE      # andi.b #$fe, ccr
    assert u16_be(rom, 0x011B46) == 0x0001      # ori.b #$1, ccr
    assert u32_be(rom, 0x011B50) == 0x00FF19E8  # lea.l $ff19e8.l, a5
    assert u16_be(rom, 0x011B74) == 0x002D      # move.w #$2d, $8e(a6)
    assert u16_be(rom, 0x011B8C) == 0x0001      # cmpi.w #$1, $8e(a5)


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    disasm_bytes = 0
    for insn in md.disasm(slice_data, RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size,
        })
        disasm_bytes += insn.size

    assert disasm_bytes == RANGE_SIZE, f"Disassembled {disasm_bytes} != {RANGE_SIZE}"

    routines_meta = []
    for r in ROUTINES:
        r_start = int(r["start"], 16)
        r_end = int(r["end"], 16)
        r_insns = [
            ins for ins in instructions
            if r_start <= int(ins["address"], 16) < r_end
        ]
        routines_meta.append({
            "name": r["name"],
            "start": r["start"],
            "end": r["end"],
            "size_bytes": r["size_bytes"],
            "instruction_count": len(r_insns),
            "description": r["description"],
        })

    evidence = {
        "milestone": "M15.485",
        "title": "ROM Spirit Button A Command Decoder, Shared SP Guard & Dytto Master Tick / SP Drain",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE,
        },
        "constants": {
            "button_a_mask": "0x40",
            "hold_threshold_frames": 20,
            "double_tap_window_frames": 6,
            "sp_min_remaining": 1,
            "dytto_sp_drain_interval_frames": 45,
            "low_sp_blink_bit": 1,
            "command_codes": {
                "0": "none",
                "1": "single_tap",
                "2": "double_tap",
                "3": "hold_release_20_frames",
            },
            "external_targets": {
                "entity_collision_bound_helper": "0x00E36E",
                "dytto_state_dispatcher": "0x011BA0",
                "dytto_spirit_dismissal": "0x012314",
            },
        },
        "ram_addresses": {
            "player_input_byte": "0x00FF165E",
            "lock_flag_1982": "0x00FF1982",
            "lock_flag_1983": "0x00FF1983",
            "spirit_hold_counter_word": "0x00FF199E",
            "spirit_prev_button_a_byte": "0x00FF19A0",
            "spirit_double_tap_timer_byte": "0x00FF19A1",
            "spirit_command_code_byte": "0x00FF19A2",
            "spirit_latched_input_byte": "0x00FF19A3",
            "spirit_latched_facing_word": "0x00FF19A4",
            "lock_flag_19a7": "0x00FF19A7",
            "player_entity_base": "0x00FF19E8",
            "player_facing_word": "0x00FF19FE",
            "player_sp_offset": "+$8E(a5)",
            "spirit_render_flags_offset": "+$36(a6)",
            "spirit_sp_drain_timer_offset": "+$8E(a6)",
            "spirit_sub_entity_flag_offset": "+$A2(a6)",
        },
        "routines": routines_meta,
        "instruction_count": len(instructions),
        "instructions": instructions,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    # Emit compact single-line instruction objects so the full evidence JSON stays compact (<= 500 lines).
    placeholder = "__INSTRUCTIONS_PLACEHOLDER__"
    raw_json = json.dumps({**evidence, "instructions": placeholder}, indent=2)
    insn_lines = ",\n".join(f"    {json.dumps(item)}" for item in instructions)
    formatted_json = raw_json.replace(f'"{placeholder}"', f"[\n{insn_lines}\n  ]") + "\n"
    assert json.loads(formatted_json) == evidence
    OUTPUT_PATH.write_text(formatted_json, encoding="utf-8")
    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
