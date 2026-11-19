#!/usr/bin/env python3
"""
Extract ROM Shade Spirit Buzzer, Dark Hole / Fall-Rescue Drain & Teleport Recovery
(States $0C/$0E), and Shadow Grapple Extension & Enemy/Item Latch (States $06/$08)
(0x01319E..0x01341A, 636 bytes).

Disassembles exact M68K instructions, verifies opcodes/constants/jump targets,
and generates evidence JSON for Milestone M15.494.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_shade_rescue_grapple_hit_m15_494.json"

RANGE_START = 0x01319E
RANGE_END = 0x01341A
RANGE_SIZE = RANGE_END - RANGE_START  # 636 bytes

ROUTINES = [
    {
        "name": "AI_Shade_InsufficientSpBuzzer",
        "start": "0x01319E",
        "end": "0x0131AE",
        "size_bytes": 0x0131AE - 0x01319E,
        "description": (
            "Plays insufficient-SP buzzer SFX #$00A00253 via $60004 and branches "
            "to 0x0136A0."
        ),
    },
    {
        "name": "AI_Shade_State0C_DarkHoleRescueAndDrain",
        "start": "0x0131AE",
        "end": "0x01325E",
        "size_bytes": 0x01325E - 0x0131AE,
        "description": (
            "Zeroes +$72/+$76(a6). While $FF1985 != 1 and +$8E(a5) != 0, drains "
            "+$8E(a6) by 4 (clamped >= 0) and toggles Ali +$36(a5) bit 1 via $FF1971 "
            "bit 0. Once $FF1985 == 1 or +$8E(a5) == 0, clears +$36:1 and +$38:2,6 on "
            "Ali, restores +$06(a5) = $FF0D7E, resets Ali State 0 / Anim 0 / Facing 2 / "
            "+$82 = $0A, snaps Ali X/Y to Shade with +$10(a5)=0, probes 0x009D00 on Ali, "
            "calls 0x01378A, enters State $0E, clears $FF164F, sets $FF1896 = 1, calls "
            "0x002E1E, and sets $FF17BA = $FF."
        ),
    },
    {
        "name": "AI_Shade_State0E_FinishDarkHoleRecovery",
        "start": "0x01325E",
        "end": "0x0132AA",
        "size_bytes": 0x0132AA - 0x01325E,
        "description": (
            "Clears $FF1985, clears +$38(a5) bit 3 and $FF17B8 bit 2, resets State 0, "
            "clears $FF199E and $FF19A1, sets $FF19A0 = $FF, calls 0x002D84, clears "
            "$FF1896, and sets $FF164F = $FF if $FF16F1 bit 1 is set."
        ),
    },
    {
        "name": "AI_Shade_State06_08_GrappleExtendAndHitLatch",
        "start": "0x0132AA",
        "end": "0x01341A",
        "size_bytes": 0x01341A - 0x0132AA,
        "sub_entry_points": {
            "state_06_grapple_extend": "0x0132AA",
            "state_06_item_probe_match": "0x013310",
            "state_06_entity_contact": "0x01332A",
            "state_08_grapple_latch_hold": "0x0133D0",
            "grapple_retract_init_state_0a": "0x0133E6",
        },
        "description": (
            "State $06 (0x0132AA..0x0133D0) zeroes Ali +$72/+$76; checks (+$40|+$41) "
            "bit 6 (wall -> 0x0133E6) and bit 7 (entity -> 0x01332A), then probes "
            "0x013608 (on Carry clear & +$14 match: stores +$AA=a0, calls 0x013550, "
            "sets +$2A=4 & State $08); else decrements +$02 (-> State $08 +$2A=$12 "
            "on 0) and accelerates (0x80, 0x80) via 0x00E2D4 into +$4E/+$52 and "
            "+$72/+$76. Entity contact (0x01332A) latches +$37:7 grabbables matching "
            "+$14 via +$AA=a0 & 0x0134DE -> State $08, or strikes vulnerable enemies "
            "(+$38:3==0, +$82==0, +$83==0) with $FF184F=$5A, $FF1850=a0, +$7E=a6, "
            "+$86 = 1 + ($FF0DB8 >> 1) (plus big-endian high-byte bit 7 i.e. 0x8000 "
            "when +$10(a0) != 0), +$83=$0A, (0x500, 0x4C0) knockback, +$56=$28000, "
            "SFX #$700345 -> State $08 (+$2A=$12). State $08 (0x0133D0..0x01341A) "
            "calls 0x0134DE, zeroes Ali +$72/+$76, and on +$37 bit 1 enters State $0A "
            "(+$2A=6), negating +$4E/+$52, setting +$02 = 18 - +$02, and setting +$38:2."
        ),
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key opcodes, constants, and targets across 0x01319E..0x01341A."""
    assert u32_be(rom, 0x0131A0) == 0x00A00253  # move.l #$a00253, d0
    assert u32_be(rom, 0x0131BA) == 0x00FF1985  # cmpi.b #$1, $ff1985.l
    assert u16_be(rom, 0x0131CC) == 0x5940      # subq.w #$4, d0
    assert u32_be(rom, 0x013204) == 0x00FF0D7E  # move.w $ff0d7e.l, $6(a5)
    assert u16_be(rom, 0x01324A) == 0x0001      # move.w #$1, $ff1896.l
    assert u32_be(rom, 0x013252) == 0x00002E1E  # jsr $2e1e.l
    assert u32_be(rom, 0x01328E) == 0x00002D84  # jsr $2d84.l
    assert u16_be(rom, 0x0132DE) == 0x0080      # move.w #$80, d1
    assert u16_be(rom, 0x013360) == 0x005A      # move.b #$5a, $ff184f.l
    assert u16_be(rom, 0x01338E) == 0x0500      # move.w #$500, d1
    assert u16_be(rom, 0x013392) == 0x04C0      # move.w #$4c0, d2
    assert u32_be(rom, 0x0133A2) == 0x00028000  # move.l #$28000, $56(a0)
    assert u32_be(rom, 0x0133B8) == 0x00FF0DB8  # move.b $ff0db8.l, d0
    assert u32_be(rom, 0x0133C4) == 0x00700345  # move.l #$700345, d0
    assert u16_be(rom, 0x013408) == 0x0012      # move.w #$12, d1


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
        "milestone": "M15.494",
        "title": "ROM Shade Spirit Buzzer, Dark Hole Fall-Rescue Drain & Teleport Recovery, and Shadow Grapple Extension & Hit Latch",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE,
        },
        "embedded_data_tables": [],
        "constants": {
            "rescue_drain_step": 4,
            "ali_rescue_hitstop": 10,
            "grapple_accel_step": "0x0080",
            "grapple_knockback_vx": "0x0500",
            "grapple_knockback_vy": "0x04C0",
            "grapple_knockback_vz": "0x00028000",
            "grapple_enemy_hitstop": 10,
            "grapple_hud_timer": "0x5A",
            "grapple_airborne_word_flag": "0x8000",
            "grapple_retract_base_frames": 18,
            "sfx": {
                "insufficient_sp_buzzer": "0x00A00253",
                "grapple_enemy_hit": "0x00700345",
            },
        },
        "ram_addresses": {
            "shade_level_byte": "0x00FF0DB8",
            "ali_piece_flag_backup": "0x00FF0D7E",
            "hud_fade_flag_164f": "0x00FF164F",
            "game_flags_16f1": "0x00FF16F1",
            "frame_counter_1971": "0x00FF1971",
            "pause_lock_flags_17b8": "0x00FF17B8",
            "teleport_flag_17ba": "0x00FF17BA",
            "hud_enemy_timer_184f": "0x00FF184F",
            "hud_enemy_ptr_1850": "0x00FF1850",
            "warp_fade_state_1896": "0x00FF1896",
            "rescue_trigger_1985": "0x00FF1985",
            "spirit_hold_counter_199e": "0x00FF199E",
            "spirit_prev_a_19a0": "0x00FF19A0",
            "spirit_dtap_timer_19a1": "0x00FF19A1",
        },
        "external_targets": {
            "fade_in_helper": "0x002D84",
            "fade_out_helper": "0x002E1E",
            "terrain_ground_probe": "0x009D00",
            "compute_8dir_velocity": "0x00E2D4",
            "play_sfx": "0x00E7FC",
            "grapple_anchor_offset": "0x0134DE",
            "grapple_snap_item": "0x013550",
            "grapple_item_probe": "0x013608",
            "shade_idle_continuation": "0x0136A0",
            "shade_reset_idle_struct": "0x01378A",
            "sound_driver_entry": "0x060004",
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
    OUTPUT_PATH.write_text(formatted_json, encoding="utf-8")
    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
