#!/usr/bin/env python3
"""
Extract ROM Efreet Dash Charge Attack, Flame Breath State Controller &
Radial Fireball Supernova Spawner (0x012680..0x012988, 776 bytes).

Disassembles exact M68K instructions, verifies opcodes/constants/jump targets,
and generates evidence JSON for Milestone M15.490.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_efreet_combat_fireball_m15_490.json"

RANGE_START = 0x012680
RANGE_END = 0x012988
RANGE_SIZE = RANGE_END - RANGE_START  # 776 bytes

ROUTINES = [
    {
        "name": "AI_Efreet_DashChargeStates",
        "start": "0x012680",
        "end": "0x0127A4",
        "size_bytes": 0x0127A4 - 0x012680,
        "sub_entry_points": {
            "state_04_recovery_to_follow": "0x012680",
            "return_to_idle_clear_cooldown": "0x01269A",
            "state_08_dash_startup": "0x0126AC",
            "state_12_dash_active": "0x0126CE",
            "state_14_dash_deceleration": "0x012702",
            "step_clamped_dash_velocity": "0x01274C",
        },
        "description": (
            "State $04 (0x012680) returns to State 2 on +$37 bit 1; helper 0x01269A resets "
            "State 0 (+$2A=0) and clears cooldown +$9E. State $08 (0x0126AC) accelerates via "
            "0x01274C until +$37 bit 1 -> State $12 (+$2A=$1A, +$2=$0A). State $12 (0x0126CE) "
            "sets +$90 = (($FF0DB7 + 1) >> 2), calls 0x00BCA4(d0=0), clears +$90, steps "
            "0x01274C, and decrements +$2 -> State $14 (+$2A=$1C) after 10 frames. State $14 "
            "(0x012702) decelerates by (0xC0,0xC0) via 0x00E2D4, integrates +$72/+$76, and "
            "enters idle with +$9E=$0E on +$37 bit 1. 0x01274C adds (0x100,0x100) via 0x00E2D4 "
            "and clamps +$4E/+$52 to [-$80000..+$80000]."
        ),
    },
    {
        "name": "AI_Efreet_FlameBreathStates",
        "start": "0x0127A4",
        "end": "0x012802",
        "size_bytes": 0x012802 - 0x0127A4,
        "sub_entry_points": {
            "state_0a_flame_breath_startup": "0x0127A4",
            "state_0e_flame_breath_active": "0x0127C0",
        },
        "description": (
            "State $0A (0x0127A4) waits for +$37 bit 1 to enter State $0E (+$2A=$18, +$2=$46 "
            "= 70 frames). State $0E (0x0127C0) decrements +$2; when +$2 >= $14 (20) and "
            "(+$2 & 3) == 0, spawns flame particles via 0x012DFA + SFX #$80034F, runs "
            "0x012CDC and 0x012EA8 every active frame, and on 0 frees the $FF1C1C slot "
            "pointed to by +$A6(a6), clears +$6(a6), and branches to 0x01269A."
        ),
    },
    {
        "name": "AI_Efreet_FireballSupernovaStates",
        "start": "0x012802",
        "end": "0x012914",
        "size_bytes": 0x012914 - 0x012802,
        "sub_entry_points": {
            "state_0c_supernova_windup": "0x012802",
            "state_18_supernova_spawner": "0x012818",
            "state_1a_supernova_recovery": "0x01284C",
            "state_18_spawn_fireball_branch": "0x012858",
        },
        "description": (
            "State $0C (0x012802) waits for +$37 bit 1 -> State $18 (+$2A=$1E). State $18 "
            "(0x012818, 0x012858) writes screen shake $FF17C4 = ($FF1971 & 1) + 3; while "
            "+$2 >= 0 after decrement, plays SFX #$B00224 and #$700383. On +$2 borrow, "
            "decrements +$AF (transitioning to State $1A +$2A=$20 on underflow), resets +$2=2, "
            "steps +$AE -= $20, looks up signed trig bytes at $05D706 scaled by $1000 "
            "(vx = cos*0x1000, vy = -sin*0x1000), allocates a $FF1C1C Pool 1 slot via "
            "0x00DA1A (reverting +$2/+$AE/+$AF if full), clones Efreet via 0x00E66E, sets "
            "State $16 (+$2A=$16), +$A2=$FF, priority (+$18 & $8000)|$07A8, offsets +$8/+$C "
            "by 2*vel. State $1A (0x01284C) exits to 0x01269A on +$37 bit 1."
        ),
    },
    {
        "name": "AI_Efreet_FireballFlightMeleeAndBuzzer",
        "start": "0x012914",
        "end": "0x012988",
        "size_bytes": 0x012988 - 0x012914,
        "sub_entry_points": {
            "state_16_fireball_flight": "0x012914",
            "state_10_melee_attack_scan": "0x012950",
            "insufficient_sp_buzzer": "0x012978",
        },
        "description": (
            "State $16 (0x012914..0x012950) maps velocity (+$4E, +$52) to cardinal facing via "
            "0x00E338 into +$16, executes 0x00BCA4(d0=2), clears +$16, despawns (+$0=0) on "
            "+$37 bit 1, or integrates +$4E/+$52 into +$72/+$76. State $10 (0x012950..0x012978, "
            "jumped from 0x0129B0) applies (($FF0DB7 + 1) >> 2) into +$90, calls 0x00BCA4(d0=0), "
            "clears +$90, and returns via 0x01269A on +$37 bit 1. 0x012978..0x012988 plays "
            "insufficient-SP/slot-full SFX #$00A00253 via $60004 and branches to 0x0129B0."
        ),
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key opcodes, constants, and jump targets across 0x012680..0x012988."""
    assert u16_be(rom, 0x0126C8) == 0x000A      # move.w #$a, $2(a6)
    assert u32_be(rom, 0x0126D2) == 0x00FF0DB7  # move.b $ff0db7.l, d0
    assert u32_be(rom, 0x0126E2) == 0x0000BCA4  # jsr $bca4.l
    assert u16_be(rom, 0x01270C) == 0x00C0      # move.w #$c0, d1
    assert u16_be(rom, 0x012746) == 0x000E      # move.b #$e, $9e(a6)
    assert u16_be(rom, 0x012756) == 0x0100      # move.w #$100, d1
    assert u32_be(rom, 0x012766) == 0x00080000  # move.l #$80000, d0
    assert u16_be(rom, 0x0127BA) == 0x0046      # move.w #$46, $2(a6)
    assert u16_be(rom, 0x0127D0) == 0x0014      # cmpi.w #$14, d0
    assert u32_be(rom, 0x0127E0) == 0x0080034F  # move.l #$80034f, d0
    assert u32_be(rom, 0x012826) == 0x00FF17C4  # move.w d0, $ff17c4.l
    assert u32_be(rom, 0x012838) == 0x00B00224  # move.l #$b00224, d0
    assert u32_be(rom, 0x012842) == 0x00700383  # move.l #$700383, d0
    assert u16_be(rom, 0x012878) == 0x0020      # subi.b #$20, d3
    assert u16_be(rom, 0x012880) == 0x1000      # move.w #$1000, d6
    assert u32_be(rom, 0x012892) == 0x0005D706  # lea.l $5d706.l, a0
    assert u16_be(rom, 0x0128E4) == 0x07A8      # ori.w #$7a8, d0
    assert u16_be(rom, 0x012924) == 0x7002      # moveq #$2, d0
    assert u32_be(rom, 0x01297A) == 0x00A00253  # move.l #$a00253, d0
    assert u32_be(rom, 0x012980) == 0x00060004  # jsr $60004.l


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
            "sub_entry_points": r["sub_entry_points"],
            "description": r["description"],
        })

    evidence = {
        "milestone": "M15.490",
        "title": "ROM Efreet Dash Charge Attack, Flame Breath State Controller & Radial Fireball Supernova Spawner",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE,
        },
        "embedded_data_tables": [],
        "constants": {
            "dash_accel_step": "0x0100",
            "dash_decel_step": "0x00C0",
            "dash_velocity_clamp": "0x00080000",
            "dash_active_frames": 10,
            "dash_cooldown_frames": 14,
            "flame_breath_duration_frames": 70,
            "flame_breath_emit_min_timer": 20,
            "flame_breath_emit_period_mask": "0x03",
            "supernova_angle_step": "0x20",
            "supernova_speed_scale": "0x1000",
            "supernova_spawn_interval_reload": 2,
            "supernova_trig_lut_external": "0x05D706",
            "fireball_attack_index": 2,
            "sfx": {
                "flame_breath_tick": "0x0080034F",
                "supernova_blast": "0x00B00224",
                "supernova_rumble": "0x00700383",
                "insufficient_sp_buzzer": "0x00A00253",
            },
        },
        "ram_addresses": {
            "efreet_level_byte": "0x00FF0DB7",
            "screen_shake_timer": "0x00FF17C4",
            "frame_counter_byte": "0x00FF1971",
            "pool1_general_base": "0x00FF1C1C",
        },
        "external_targets": {
            "entity_attack_scan": "0x00BCA4",
            "find_free_pool1_slot_18": "0x00DA1A",
            "compute_8dir_velocity": "0x00E2D4",
            "compute_cardinal_facing": "0x00E338",
            "clone_entity_record_188b": "0x00E66E",
            "play_sfx": "0x00E7FC",
            "flame_particle_update": "0x012CDC",
            "flame_particle_spawn": "0x012DFA",
            "flame_piece_render": "0x012EA8",
            "efreet_post_dispatch": "0x0129B0",
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
