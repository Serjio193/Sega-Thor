#!/usr/bin/env python3
"""
Extract ROM Bow Spirit (0x1E) Combat States $08, $04 Gravity Leap/Chomp & $06
(0x013B36..0x013CB4, 382 bytes).

Disassembles exact M68K instructions, verifies opcodes/constants/jump targets,
and generates evidence JSON for Milestone M15.498.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_bow_combat_states_m15_498.json"

RANGE_START = 0x013B36
RANGE_END = 0x013CB4
RANGE_SIZE = RANGE_END - RANGE_START  # 382 bytes

ROUTINES = [
    {
        "name": "AI_Bow_State08_ChompAttackAndTrail",
        "start": "0x013B36",
        "end": "0x013BF2",
        "size_bytes": 0x013BF2 - 0x013B36,
        "sub_entry_points": {
            "state_08_chomp_tick": "0x013B36",
            "state_08_exit_to_idle": "0x013BB2",
            "chomp_hit_callback": "0x013BDE",
        },
        "description": (
            "State $08 (0x013B36..0x013BF2): decrements +$02(a6) (-> exit 0x013BB2 on 0). "
            "If +$2A != $24, probes hitbox table $05887C via 0x00BBA8 and sets +$2A = $24 "
            "on Carry set. When +$2A == $24, computes +$90 = (($FF0DB9 + 1) >> 2), sets "
            "$FF1940 = 0x00013BDE, calls 0x00BCAE(d0=2), clears +$90, refunds +$02 += 1, "
            "and exits to 0x013BB2 on +$37 bit 1. Otherwise integrates +$4E/+$52 into "
            "+$72/+$76 and +$56 into +$10 (clamped >= 0) and updates trail via 0x014200 "
            "(d7=$0C). Exit 0x013BB2 resets State 0/Anim 0, +$9E=2, zeroes +$4E/+$52/+$56, "
            "clears +$38 bit 2, and calls 0x014200(d7=9). Callback 0x013BDE calls 0x00BE98 "
            "and zeroes enemy +$4E/+$52 when enemy +$56(a0) == 0."
        ),
    },
    {
        "name": "AI_Bow_State04_GravityLeapAndSeek",
        "start": "0x013BF2",
        "end": "0x013C7C",
        "size_bytes": 0x013C7C - 0x013BF2,
        "sub_entry_points": {
            "state_04_gravity_leap": "0x013BF2",
            "state_04_enter_state_06": "0x013C52",
        },
        "description": (
            "State $04 (0x013BF2..0x013C7C): subtracts $C000 from +$56(a6) and adds to "
            "+$10(a6), clamping both to 0 on borrow. Syncs anchor chain via 0x013FFC, loads "
            "a4 = $FF1C1C and calls 0x00E3A4 with (+$4E(a4), +$52(a4)) -> Manhattan "
            "distance d5. Transitions to State $06 (0x013C52) if d5 <= 6, d5 > +$AC(a6), "
            "or --(+$02(a6)) == 0; otherwise stores +$AC = d5 and integrates +$4E/+$52 into "
            "+$72/+$76. At 0x013C52 sets State $06 (+$2A=$12), clears +$16 and +$10, frees "
            "$FF1C1C slot ($0(a4) = 0), and plays SFX #$0080037E."
        ),
    },
    {
        "name": "AI_Bow_State06_PopupAndRestorePos",
        "start": "0x013C7C",
        "end": "0x013CB4",
        "size_bytes": 0x013CB4 - 0x013C7C,
        "description": (
            "State $06 (0x013C7C..0x013CB4): waits for +$37(a6) bit 1; when set, enters "
            "State $02 (+$2A=$14), restores +$08/+$0C(a6) from +$AE/+$B2(a6), clears "
            "+$10(a6) = 0, calls terrain probe 0x009D00, and plays SFX #$0080037E."
        ),
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key opcodes, constants, and jump targets across 0x013B36..0x013CB4."""
    assert u16_be(rom, 0x013B3E) == 0x0024      # cmpi.w #$24, $2a(a6)
    assert u32_be(rom, 0x013B46) == 0x0005887C  # lea.l $5887c.l, a1
    assert u32_be(rom, 0x013B4C) == 0x0000BBA8  # jsr $bba8.l
    assert u32_be(rom, 0x013B5E) == 0x00FF0DB9  # move.b $ff0db9.l, d0
    assert u32_be(rom, 0x013B6E) == 0x00013BDE  # move.l #$13bde, $ff1940.l
    assert u32_be(rom, 0x013B72) == 0x00FF1940  # ... dst $ff1940.l
    assert u32_be(rom, 0x013B78) == 0x0000BCAE  # jsr $bcae.l
    assert u16_be(rom, 0x013BC0) == 0x0002      # move.b #$2, $9e(a6)
    assert u32_be(rom, 0x013BF8) == 0x0000C000  # subi.l #$c000, d0
    assert u32_be(rom, 0x013C14) == 0x00FF1C1C  # lea.l $ff1c1c.l, a4
    assert u16_be(rom, 0x013C26) == 0x0006      # cmpi.w #$6, d5
    assert u16_be(rom, 0x013C5A) == 0x0012      # move.w #$12, $2a(a6)
    assert u32_be(rom, 0x013C72) == 0x0080037E  # move.l #$80037e, d0
    assert u16_be(rom, 0x013C8E) == 0x0014      # move.w #$14, $2a(a6)
    assert u32_be(rom, 0x013CA4) == 0x00009D00  # jsr $9d00.l
    assert u32_be(rom, 0x013CAA) == 0x0080037E  # move.l #$80037e, d0


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
        entry = {
            "name": r["name"],
            "start": r["start"],
            "end": r["end"],
            "size_bytes": r["size_bytes"],
            "instruction_count": len(r_insns),
        }
        if "sub_entry_points" in r:
            entry["sub_entry_points"] = r["sub_entry_points"]
        entry["description"] = r["description"]
        routines_meta.append(entry)

    evidence = {
        "milestone": "M15.498",
        "title": "ROM Bow Spirit (0x1E) Combat States $08, $04 Gravity Leap/Chomp & $06",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE,
        },
        "embedded_data_tables": [],
        "constants": {
            "chomp_anim_id": "0x0024",
            "chomp_attack_index": 2,
            "state08_trail_param_active": 12,
            "state08_trail_param_exit": 9,
            "state08_exit_substate_9e": 2,
            "gravity_step": "0x0000C000",
            "leap_target_proximity_px": 6,
            "state06_anim_burrow": "0x0012",
            "state02_anim_popup": "0x0014",
            "sfx": {
                "burrow_popup": "0x0080037E",
            },
        },
        "ram_addresses": {
            "bow_level_byte": "0x00FF0DB9",
            "hit_callback_ptr_1940": "0x00FF1940",
            "anchor_slot_ff1c1c": "0x00FF1C1C",
        },
        "external_targets": {
            "hitbox_table_05887c": "0x05887C",
            "terrain_ground_probe": "0x009D00",
            "proximity_box_scan": "0x00BBA8",
            "entity_attack_scan_pool1": "0x00BCAE",
            "hit_apply_handler": "0x00BE98",
            "manhattan_distance_delta": "0x00E3A4",
            "play_sfx": "0x00E7FC",
            "bow_anchor_sync": "0x013FFC",
            "bow_trail_chain_step": "0x014200",
        },
        "routines": routines_meta,
        "instruction_count": len(instructions),
        "instructions": instructions,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    placeholder = "__INSTRUCTIONS_PLACEHOLDER__"
    raw_json = json.dumps({**evidence, "instructions": placeholder}, indent=2)
    insn_lines = ",\n".join(f"    {json.dumps(item)}" for item in instructions)
    formatted_json = raw_json.replace(f'"{placeholder}"', f"[\n{insn_lines}\n  ]") + "\n"
    assert json.loads(formatted_json) == evidence
    OUTPUT_PATH.write_text(formatted_json, encoding="utf-8", newline="\n")
    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
