#!/usr/bin/env python3
"""
Extract ROM Boss AI State $1A Death Cutscene Sequence, Explosion Shake
& Smooth Angle Steering Helper (0x014F60..0x015136, 470 bytes).

Disassembles exact M68K instructions with Capstone, validates opcodes/constants,
and generates evidence JSON for Milestone M15.506.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_boss_ai_death_steering_m15_506.json"

RANGE_START = 0x014F60
RANGE_END = 0x015136
RANGE_SIZE = RANGE_END - RANGE_START  # 470 bytes

ROUTINES = [
    {
        "name": "BossAI_State1A_DeathCutsceneSequence",
        "start": "0x014F60",
        "end": "0x01503C",
        "size_bytes": 0x01503C - 0x014F60,
        "description": (
            "State $1A death cutscene sequencer: increments timer +$2; at frame $B4 (180) "
            "clears $FF164F, sets $FF1896=6, calls palette effect 0x002E78 and shake helper 0x01503C; "
            "during frames < $F0 (240) invokes shake 0x01503C; at frame $12C (300) clears 17 Pool 2 "
            "slots ($FF1D94..), streams Nemesis art $167C54 via 0x00D9A4 into $8F00, spawns item Type "
            "$56 at $FF2D8C (tile $478, coords (0x100, 0x98), z-vel $10000, z-acc $331DC); at frame "
            "$168 (360) clears $FF1983, sets $FF1896=4, calls 0x002DB6 and 0x00DAC2 with d0=$11F, "
            "sets $FF1D0E:1 and $FF0DC3:2; at frame $1A4 (420) sets $FF164F=$FF and despawns boss slot $FF1CD8."
        ),
    },
    {
        "name": "BossAI_DeathExplosionShake",
        "start": "0x01503C",
        "end": "0x0150A0",
        "size_bytes": 0x0150A0 - 0x01503C,
        "description": (
            "Spawns death explosion particle type 1 via 0x00E90E at (0x100 - plane_b_x + plane_a_x + "
            "prng>>1, 0x00E8 - plane_b_y + plane_a_y + prng>>1 + $E0). Updates screen shake at $FF17C4 "
            "to oscillate between 2 and 3 based on frame counter ($FF1971 & 1) + 2."
        ),
    },
    {
        "name": "BossAI_SmoothAngleSteering",
        "start": "0x0150A0",
        "end": "0x015136",
        "size_bytes": 0x015136 - 0x0150A0,
        "description": (
            "Smooth angle turn-rate limiter and sin/cos velocity integrator: calls 0x00E6F0 for target "
            "angle, clamps angular turn step to <= 8 units into +$AE. Computes X/Y velocities via "
            "sin/cos table $5D706 with scaled speed +$B0>>7, storing into +$4E/+$52 and integrating "
            "+$72/+$76. Updates facing +$16 relative to Ali ($FF19E8) X: < $A0 -> 3 (left), "
            "< $160 -> 2 (down), >= $160 -> 1 (right)."
        ),
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key ROM addresses, offsets, cutscene frame thresholds and constants."""
    assert u16_be(rom, 0x014F6C) == 0x00B4  # Stage 1 frame 180
    assert u16_be(rom, 0x014F72) == 0x00F0  # Stage 2 frame 240
    assert u16_be(rom, 0x014F7A) == 0x012C  # Stage 3 frame 300
    assert u16_be(rom, 0x014F80) == 0x0168  # Stage 4 frame 360
    assert u16_be(rom, 0x014F86) == 0x01A4  # Stage 5 frame 420
    assert u32_be(rom, 0x014FA8) == 0x00FF1D94  # Pool 2 start address
    assert u32_be(rom, 0x014FBC) == 0x00167C54  # Item Nemesis art address
    assert u16_be(rom, 0x014FC2) == 0x8F00      # VRAM destination
    assert u32_be(rom, 0x014FCA) == 0x00FF2D8C  # Item spawn entity slot
    assert u16_be(rom, 0x014FD0) == 0x0056      # Item entity type (0x56)
    assert u32_be(rom, 0x015000) == 0x00FF1983  # Camera lock flag
    assert u32_be(rom, 0x015046) == 0x00FF173E  # Plane B scroll
    assert u32_be(rom, 0x015054) == 0x00FF1716  # Plane A scroll
    assert u32_be(rom, 0x01508E) == 0x00FF1971  # Frame counter PRNG
    assert u32_be(rom, 0x0150E2) == 0x0005D706  # Sin/Cos lookup table
    assert u32_be(rom, 0x01510A) == 0x00FF19E8  # Ali player pointer


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
            "description": r["description"],
        }
        routines_meta.append(entry)

    evidence = {
        "milestone": "M15.506",
        "title": "ROM Boss AI State $1A Death Cutscene Sequence, Explosion Shake & Smooth Angle Steering Helper",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE,
        },
        "cutscene_timeline": {
            "frame_180": {
                "threshold_hex": "0x00B4",
                "actions": [
                    "sf.b $FF164F",
                    "move.w #6, $FF1896",
                    "jsr 0x002E78",
                    "bra.w 0x01503C (screen shake)",
                ],
            },
            "frames_181_to_239": {
                "threshold_hex": "< 0x00F0",
                "actions": ["blt.w 0x01503C (screen shake)"],
            },
            "frame_300": {
                "threshold_hex": "0x012C",
                "actions": [
                    "clear 17 Pool 2 entity slots starting at $FF1D94 (dbra #16)",
                    "stream Nemesis art $167C54 to VRAM $8F00 via 0x00D9A4",
                    "spawn entity Type 0x56 at $FF2D8C via 0x008D06",
                    "set tile $0478, coords (256, 152), z_vel $10000, z_acc $331DC",
                ],
            },
            "frame_360": {
                "threshold_hex": "0x0168",
                "actions": [
                    "sf.b $FF1983 (clear camera lock)",
                    "move.w #4, $FF1896",
                    "jsr 0x002DB6",
                    "jsr 0x00DAC2 with d0=0x11F",
                    "bset.b #1, $FF1D0E (boss defeated event)",
                    "bset.b #2, $FF0DC3 (environmental flag)",
                ],
            },
            "frame_420": {
                "threshold_hex": "0x01A4",
                "actions": [
                    "st.b $FF164F",
                    "clr.w $FF1CD8 (despawn boss entity slot)",
                ],
            },
        },
        "explosion_particle_shake": {
            "spawn_fn": "0x00E90E",
            "particle_type": 1,
            "center_coords": {"x": 256, "y": 232},
            "y_offset": 224,
            "shake_register": "0x00FF17C4",
            "shake_formula": "($FF1971 & 1) + 2",
        },
        "smooth_angle_steering": {
            "target_angle_fn": "0x00E6F0",
            "max_turn_rate_step": 8,
            "sin_cos_table": "0x0005D706",
            "facing_thresholds_ali_x": {
                "lt_160": {"facing": 3, "name": "left"},
                "160_to_351": {"facing": 2, "name": "center_down"},
                "ge_352": {"facing": 1, "name": "right"},
            },
        },
        "constants": {
            "max_turn_step": 8,
            "item_entity_type": "0x0056",
            "item_tile_attr": "0x0478",
            "item_initial_x": 256,
            "item_initial_y": 152,
            "item_z_vel": "0x00010000",
            "item_z_acc": "0x000331DC",
            "event_flag_param": "0x011F",
        },
        "ram_addresses": {
            "screen_flag": "0x00FF164F",
            "palette_effect_param": "0x00FF1896",
            "pool2_slots_start": "0x00FF1D94",
            "item_entity_slot": "0x00FF2D8C",
            "camera_lock_flag": "0x00FF1983",
            "boss_entity_slot": "0x00FF1CD8",
            "plane_b_scroll": "0x00FF173E",
            "plane_a_scroll": "0x00FF1716",
            "screen_shake_disp": "0x00FF17C4",
            "frame_counter_prng": "0x00FF1971",
            "boss_defeated_flag": "0x00FF1D0E",
            "environmental_flag": "0x00FF0DC3",
            "ali_entity_ptr": "0x00FF19E8",
        },
        "external_targets": {
            "palette_effect_2e78": "0x002E78",
            "nemesis_stream_d9a4": "0x00D9A4",
            "entity_init_8d06": "0x008D06",
            "palette_restore_2db6": "0x002DB6",
            "event_dispatch_dac2": "0x00DAC2",
            "prng_e268": "0x00E268",
            "particle_spawn_e90e": "0x00E90E",
            "target_angle_e6f0": "0x00E6F0",
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
