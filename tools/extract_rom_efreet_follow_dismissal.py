#!/usr/bin/env python3
"""
Extract ROM Efreet Idle/Wander/Follow AI, Vulnerability Scanner,
Dismissal & Hover Bobbing Subsystem (0x012988..0x012C4E, 710 bytes).

Disassembles exact M68K instructions across 0x012988..0x012C46 (702 bytes),
extracts the embedded 4-word bobbing table at 0x012C46..0x012C4E (8 bytes)
and external 3-facing hitbox table at 0x0586E8..0x05870C (36 bytes),
verifies key constants/branches, and emits evidence JSON for Milestone M15.491.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_efreet_follow_dismissal_m15_491.json"

RANGE_START = 0x012988
CODE_END = 0x012C46
TABLE_START = 0x012C46
TABLE_END = 0x012C4E
RANGE_END = 0x012C4E
RANGE_SIZE = RANGE_END - RANGE_START  # 710 bytes (702B code + 8B table)

HITBOX_TABLE_0586E8 = 0x0586E8
HITBOX_TABLE_ENTRIES = 3  # Facing 0 (Up), 1 (Right), 2 (Down); Facing 3 mirrors Facing 1

EXPECTED_BOBBING_WORDS = [0x7000, 0x3000, 0x9000, 0xD000]

ROUTINES = [
    {
        "name": "AI_Efreet_IdleWanderFollowAndVulnScan",
        "start": "0x012988",
        "end": "0x012BA8",
        "size_bytes": 0x012BA8 - 0x012988,
        "sub_entry_points": {
            "state_00_02_10_entry": "0x012988",
            "vuln_scanner_4dir": "0x0129BA",
            "elevation_and_bobbing_hook": "0x012A28",
            "state_00_wander_init": "0x012A3A",
            "state_00_wander_step": "0x012AA0",
            "wander_vel_oscillate": "0x012AC6",
            "state_02_diagonal_follow": "0x012AE6",
        },
        "description": (
            "Shared tick for States $00/$02/$10: checks $FF19A7 dismissal (-> 0x012BA8), "
            "dispatches $FF19A2 (1->0x012608, 2->0x0125CA, 3->0x01263C), and branches to "
            "0x012950 if State==$10. Otherwise saves +$16 in +$B2, scans 4 facings "
            "[f, (f+1)&3, (f-1)&3, f^2] via 0x00BAB8 with $0586E8 -> enters State $10 "
            "(0x012672) on first hit else restores +$16=+$B2. Calls 0x012BFE & 0x012C32. "
            "In State $00: if +$9E==0, calls 0x00E398 and computes Chebyshev max(|dx|,|dy|); "
            "<= $40 PRNG wander turn (0x00E268 >= $7FFF -> +/-(1) mod 4), <= $80 face Ali, "
            "> $80 enter State $02 (0x01268C); else sets +$9E=30 ($1E) and speed (0x100,0x100) "
            "via 0x00E2D4. While +$9E!=0, oscillates +$4E/+$52 by +/-$2000 (+$9E>$0F vs <=$0F) "
            "into +$72/+$76. In State $02: clears +$2A, computes ($28,$20) behind-Ali offset "
            "with Z-delta y-compensation, calls 0x00E45E (d4==0 -> State 0/+9E=0/stop; "
            "1<=d4<=4 -> 1x vel; d4>4 -> 2x vel) with 4-frame facing debounce +$AC."
        ),
    },
    {
        "name": "AI_Efreet_DismissalAndHoverBobbing",
        "start": "0x012BA8",
        "end": "0x012C4E",
        "size_bytes": 0x012C4E - 0x012BA8,
        "code_end": "0x012C46",
        "code_size_bytes": 0x012C46 - 0x012BA8,
        "table_start": "0x012C46",
        "table_end": "0x012C4E",
        "table_size_bytes": 0x012C4E - 0x012C46,
        "sub_entry_points": {
            "start_dismissal": "0x012BA8",
            "state_06_despawn": "0x012BF0",
            "track_target_elevation": "0x012BFE",
            "integrate_hover_bobbing": "0x012C32",
            "bobbing_delta_table": "0x012C46",
        },
        "description": (
            "0x012BA8 sets $FF19A7=$FF, State $06, +$2A=6, facing 2, +$38|=8, +$36&=~2, "
            "+$A2=$FF, plays SFX #$A0039E via 0x00E7FC, clears *(+$A6) if == $FFFE "
            "($FF1C1C flame slot), and clears +$06. 0x012BF0 (State $06) despawns +$00=0 "
            "on +$37 bit 1. 0x012BFE tracks ali_z14 + 16 ($10) using +$14(a6) (falling back "
            "to ali_z14 if < 0), stepping +$10.w by +1 if delta < 0 or -1 only if delta > 12 "
            "($0C) and +$10.w > 0. 0x012C32 reads +$30(a6) >> 1 into 4-word table at "
            "0x012C46..0x012C4E (0x7000, 0x3000, 0x9000, 0xD000), sign-extends, doubles, "
            "and adds to 32-bit +$10(a6)."
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
    """Assert key opcodes, constants, and tables in 0x012988..0x012C4E."""
    assert u32_be(rom, 0x01298A) == 0x00FF19A7  # tst.b $ff19a7.l
    assert u32_be(rom, 0x012994) == 0x00FF19A2  # move.b $ff19a2.l, d0
    assert u16_be(rom, 0x0129B2) == 0x0010      # cmpi.w #$10, $4(a6)
    assert u32_be(rom, 0x0129C4) == 0x000586E8  # lea.l $586e8.l, a1
    assert u16_be(rom, 0x012A4E) == 0x0040      # cmpi.w #$40, d4
    assert u16_be(rom, 0x012A54) == 0x0080      # cmpi.w #$80, d4
    assert u16_be(rom, 0x012A66) == 0x7FFF      # cmpi.w #$7fff, d0
    assert u16_be(rom, 0x012A82) == 0x001E      # move.b #$1e, $9e(a6)
    assert u16_be(rom, 0x012A8C) == 0x0100      # move.w #$100, d1
    assert u16_be(rom, 0x012A90) == 0x0100      # move.w #$100, d2
    assert u32_be(rom, 0x012AC8) == 0x00002000  # move.l #$2000, d2
    assert u16_be(rom, 0x012ACE) == 0x000F      # cmpi.b #$f, $9e(a6)
    assert u16_be(rom, 0x012B04) == 0x7028      # moveq #$28, d0 (40 px)
    assert u16_be(rom, 0x012B06) == 0x7820      # moveq #$20, d4 (32 px)
    assert u16_be(rom, 0x012B46) == 0x0004      # cmpi.w #$4, d4
    assert u16_be(rom, 0x012B64) == 0x0004      # move.b #$4, $ac(a6)
    assert u32_be(rom, 0x012BD2) == 0x00A0039E  # dismissal SFX #$A0039E
    assert u16_be(rom, 0x012BE0) == 0xFFFE      # cmpi.w #$fffe, $0(a0)
    assert u16_be(rom, 0x012C00) == 0x0010      # move.w #$10, d4 (16 px)
    assert u16_be(rom, 0x012C22) == 0x000C      # cmpi.w #$c, d0 (12 px)
    words = [u16_be(rom, TABLE_START + i * 2) for i in range(4)]
    assert words == EXPECTED_BOBBING_WORDS, f"Unexpected bobbing words: {words}"


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    assert len(rom[RANGE_START:RANGE_END]) == RANGE_SIZE
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    pos = RANGE_START
    for insn in md.disasm(rom[RANGE_START:CODE_END], RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size,
        })
        pos = insn.address + insn.size
    assert pos == CODE_END, f"Disassembly ended at 0x{pos:06X} != 0x{CODE_END:06X}"
    assert pos + 8 == RANGE_END

    bobbing_entries = []
    for idx in range(4):
        addr = TABLE_START + idx * 2
        uval = u16_be(rom, addr)
        sval = s16_be(rom, addr)
        bobbing_entries.append({
            "index": idx,
            "anim_offset_30": f"0x{idx * 4:02X}",
            "address": f"0x{addr:06X}",
            "raw_word_hex": f"0x{uval:04X}",
            "signed_word": sval,
            "doubled_fixed32": sval * 2,
            "doubled_fixed32_hex": f"0x{(sval * 2) & 0xFFFFFFFF:08X}",
        })

    hitbox_facings = []
    for f_idx in range(HITBOX_TABLE_ENTRIES):
        base = HITBOX_TABLE_0586E8 + f_idx * 12
        z_min, z_max, x_min, y_min, x_max, y_max = struct.unpack_from(">6h", rom, base)
        hitbox_facings.append({
            "facing": f_idx,
            "address": f"0x{base:06X}",
            "z_min": z_min,
            "z_max": z_max,
            "x_min": x_min,
            "y_min": y_min,
            "x_max": x_max,
            "y_max": y_max,
        })

    routines_meta = []
    for r in ROUTINES:
        r_start = int(r["start"], 16)
        r_end = int(r["end"], 16)
        r_insns = [ins for ins in instructions if r_start <= int(ins["address"], 16) < r_end]
        meta = {
            "name": r["name"],
            "start": r["start"],
            "end": r["end"],
            "size_bytes": r["size_bytes"],
            "instruction_count": len(r_insns),
            "sub_entry_points": r["sub_entry_points"],
            "description": r["description"],
        }
        if "table_start" in r:
            meta["code_end"] = r["code_end"]
            meta["code_size_bytes"] = r["code_size_bytes"]
            meta["table_start"] = r["table_start"]
            meta["table_end"] = r["table_end"]
            meta["table_size_bytes"] = r["table_size_bytes"]
        routines_meta.append(meta)

    evidence = {
        "milestone": "M15.491",
        "title": "ROM Efreet Idle/Wander/Follow AI, Vulnerability Scanner, Dismissal & Hover Bobbing Subsystem",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "code_end": f"0x{CODE_END:06X}",
            "code_size_bytes": CODE_END - RANGE_START,
            "table_size_bytes": TABLE_END - TABLE_START,
            "size_bytes": RANGE_SIZE,
        },
        "embedded_data_tables": [
            {
                "name": "EfreetHoverBobbingDeltaTable",
                "address": f"0x{TABLE_START:06X}",
                "end": f"0x{TABLE_END:06X}",
                "size_bytes": TABLE_END - TABLE_START,
                "encoding": "signed_16bit_words_doubled_into_fixed32_z",
                "entries": bobbing_entries,
            }
        ],
        "external_data_tables": {
            "vuln_scan_hitbox_table_0586e8": {
                "address": "0x0586E8",
                "end": "0x05870C",
                "size_bytes": 36,
                "format": "3x12B [z_min, z_max, x_min, y_min, x_max, y_max] (facing 3 mirrors facing 1 X)",
                "facings": hitbox_facings,
            }
        },
        "constants": {
            "vuln_scan_order": ["f", "(f+1)&3", "(f-1)&3", "f^2"],
            "vuln_trigger_state": "0x0010",
            "vuln_trigger_action": "0x000A",
            "wander_chebyshev_near_max_px": 64,
            "wander_chebyshev_follow_trigger_px": 128,
            "wander_prng_turn_threshold": "0x7FFF",
            "wander_duration_frames": 30,
            "wander_accel_min_timer_exclusive": 15,
            "wander_speed_base": "0x0100",
            "wander_vel_osc_step": "0x00002000",
            "follow_offset_primary_px": 40,
            "follow_offset_secondary_px": 32,
            "follow_fast_d4_min_exclusive": 4,
            "follow_facing_debounce_frames": 4,
            "dismissal_sfx": "0x00A0039E",
            "dismissal_state": "0x0006",
            "dismissal_action": "0x0006",
            "dismissal_facing": 2,
            "flame_slot_reserved_sentinel": "0xFFFE",
            "hover_target_offset_px": 16,
            "hover_downward_hysteresis_max_px": 12,
        },
        "ram_addresses": {
            "command_code_ff19a2": "0x00FF19A2",
            "dismiss_flag_ff19a7": "0x00FF19A7",
            "pool1_flame_slot_ff1c1c": "0x00FF1C1C",
            "ali_entity_base_ff19e8": "0x00FF19E8",
        },
        "external_targets": {
            "enemy_hitbox_scanner_3d": "0x00BAB8",
            "prng_helper": "0x00E268",
            "compute_8dir_velocity": "0x00E2D4",
            "ali_delta_facing_distance": "0x00E398",
            "homing_velocity_helper": "0x00E45E",
            "play_sfx": "0x00E7FC",
            "efreet_try_start_dash": "0x0125CA",
            "efreet_try_start_flame": "0x012608",
            "efreet_try_start_meltdown": "0x01263C",
            "efreet_enter_state_10": "0x012672",
            "efreet_enter_state_02": "0x01268C",
            "efreet_state_10_melee_scan": "0x012950",
        },
        "routines": routines_meta,
        "instruction_count": len(instructions),
        "instructions": instructions,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    ins_placeholder = "__INSTRUCTIONS_PLACEHOLDER__"
    raw_json = json.dumps({**evidence, "instructions": ins_placeholder}, indent=2)
    ins_lines = ",\n".join(f"    {json.dumps(item)}" for item in instructions)
    formatted_json = raw_json.replace(f'"{ins_placeholder}"', f"[\n{ins_lines}\n  ]") + "\n"
    assert json.loads(formatted_json) == evidence
    OUTPUT_PATH.write_text(formatted_json, encoding="utf-8")
    print(f"Extracted {len(instructions)} instructions + 8B table ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
