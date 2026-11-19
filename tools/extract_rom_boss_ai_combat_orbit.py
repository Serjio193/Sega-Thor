#!/usr/bin/env python3
"""
Extract ROM Boss AI Combat States $10/$06/$08/$0A/$16/$18 & Orbit Target Helpers
(0x014A72..0x014D1A, 680 bytes).

Disassembles exact M68K instructions with Capstone, validates opcodes/constants,
and generates evidence JSON for Milestone M15.504.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_boss_ai_combat_orbit_m15_504.json"

RANGE_START = 0x014A72
RANGE_END = 0x014D1A
RANGE_SIZE = RANGE_END - RANGE_START  # 680 bytes

ROUTINES = [
    {
        "name": "BossAI_State10_CombatAttack",
        "start": "0x014A72",
        "end": "0x014ABC",
        "size_bytes": 0x014ABC - 0x014A72,
        "description": (
            "State $10: Decrements timer at +$2; when 0 calls High Orbit helper 0x014C1C. "
            "Steers towards target (+$A6, +$AA) via 0x0150A0. If +$30 == $18 and +$2E == 0, "
            "calls special projectile spawn 0x015214 and plays SFX #$0080034F via 0x00E7FC. "
            "When anim complete (+$37 bit 1 set), sets substate +$2A = 8 and transitions to 0x014958."
        ),
    },
    {
        "name": "BossAI_State06_OrbitApproach",
        "start": "0x014ABC",
        "end": "0x014AEA",
        "size_bytes": 0x014AEA - 0x014ABC,
        "description": (
            "State $06: Decrements timer at +$2; when 0 calls Low Orbit helper 0x014CA0. "
            "Steers towards target (+$A6, +$AA) via 0x0150A0. When anim complete (+$37:1), "
            "sets substate +$2A = 0 and transitions to State $08 (+$04 = 8)."
        ),
    },
    {
        "name": "BossAI_State08_OrbitCombatAttack",
        "start": "0x014AEA",
        "end": "0x014B50",
        "size_bytes": 0x014B50 - 0x014AEA,
        "description": (
            "State $08: Decrements timer at +$2; when 0 calls Low Orbit helper 0x014CA0. "
            "Steers via 0x0150A0. When +$32 == 12 and $FF1971 % 3 == 0, calls attack helper "
            "0x015136 and (if quotient & 3 == 0) plays SFX #$0080034F. When anim complete (+$37:1), "
            "sets substate +$2A = 4 and transitions to State $0A (+$04 = $0A)."
        ),
    },
    {
        "name": "BossAI_State0A_Retreat",
        "start": "0x014B50",
        "end": "0x014B7C",
        "size_bytes": 0x014B7C - 0x014B50,
        "description": (
            "State $0A: Decrements timer at +$2; when 0 calls Low Orbit helper 0x014CA0. "
            "Steers via 0x0150A0. When anim complete (+$37:1), sets substate +$2A = 8 "
            "and transitions to 0x014958."
        ),
    },
    {
        "name": "BossAI_State16_ChaseAliHitbox",
        "start": "0x014B7C",
        "end": "0x014BB8",
        "size_bytes": 0x014BB8 - 0x014B7C,
        "description": (
            "State $16: Steers directly towards Ali ($FF19E8) via 0x0150A0. Tests attack "
            "hitbox $58CE6 via 0x00BAAC. If carry set (hit landed) or timer +$2 expires, "
            "transitions to State $18 (+$04 = $18, +$2A = $1A, speed +$B0 = $00010000)."
        ),
    },
    {
        "name": "BossAI_State18_HitThrustAndCallback",
        "start": "0x014BB8",
        "end": "0x014C1C",
        "size_bytes": 0x014C1C - 0x014BB8,
        "sub_entry_points": {
            "hit_callback": "0x014C04",
        },
        "description": (
            "State $18: Steers towards Ali via 0x0150A0. Registers hit callback 0x014C04 at "
            "$FF1940 and calls 0x00BCE2. When anim complete (+$37:1), resets state via 0x014958, "
            "sets +$2A = 8, speed +$B0 = $00030000, timer +$2 = 20 ($14), and flips angle "
            "+$AF += $80. Callback 0x014C04 calls 0x00BEBA and applies knockback $00020000 to Ali +$52."
        ),
    },
    {
        "name": "BossAI_HighOrbitTargetHelper",
        "start": "0x014C1C",
        "end": "0x014CA0",
        "size_bytes": 0x014CA0 - 0x014C1C,
        "description": (
            "Computes high orbit target relative to Ali ($FF19E8) based on facing +$16: "
            "facing 0 -> (+0x60, -0x30), facing 1 -> (-0x60, -0x30), facing 2 -> (0, -0x68). "
            "Clamps Y to minimum 200 ($00C8). Computes Chebyshev distance max(|dx|, |dy|) and "
            "selects speed tier: > 24 -> $00030000, 17..24 -> $00020000, <= 16 -> $00018000; "
            "resets timer +$2 = 4."
        ),
    },
    {
        "name": "BossAI_LowOrbitTargetHelper",
        "start": "0x014CA0",
        "end": "0x014D1A",
        "size_bytes": 0x014D1A - 0x014CA0,
        "description": (
            "Computes low orbit target relative to Ali ($FF19E8) based on facing +$16: "
            "facing 0 -> (+0x48, -0x30), facing 1 -> (-0x48, -0x30), facing 2 -> (0, -0x38). "
            "Computes Chebyshev distance max(|dx|, |dy|) and selects speed tier: "
            "> 24 -> $00028000, 17..24 -> $00020000, <= 16 -> $00018000; resets timer +$2 = 4."
        ),
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key ROM addresses, offsets, SFX and threshold constants."""
    assert u32_be(rom, 0x014A9E) == 0x0080034F  # SFX ID in State $10
    assert u32_be(rom, 0x014B30) == 0x0080034F  # SFX ID in State $08
    assert u32_be(rom, 0x014B7E) == 0x00FF19E8  # Ali player pointer
    assert u32_be(rom, 0x014B90) == 0x00058CE6  # Hitbox table pointer
    assert u32_be(rom, 0x014BB0) == 0x00010000  # State $18 initial speed
    assert u32_be(rom, 0x014BCE) == 0x00014C04  # Hit callback address
    assert u32_be(rom, 0x014BF0) == 0x00030000  # State $18 post-hit speed
    assert u32_be(rom, 0x014C14) == 0x00020000  # Knockback velocity
    assert u16_be(rom, 0x014C50) == 0x00C8      # High orbit Y clamp ($C8)
    assert u32_be(rom, 0x014C62) == 0x00030000  # High orbit default speed
    assert u32_be(rom, 0x014CDC) == 0x00028000  # Low orbit default speed


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
        if "sub_entry_points" in r:
            entry["sub_entry_points"] = r["sub_entry_points"]
        routines_meta.append(entry)

    evidence = {
        "milestone": "M15.504",
        "title": "ROM Boss AI Combat States $10/$06/$08/$0A/$16/$18 & Orbit Target Helpers",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE,
        },
        "combat_states": {
            "state_10": {
                "name": "CombatAttack_Ranged",
                "state_id": "0x10",
                "projectile_spawn": "0x015214",
                "sfx": "0x0080034F",
                "trigger_condition": "+$30 == 0x18 and +$2E == 0",
                "next_substate": "0x08",
            },
            "state_06": {
                "name": "OrbitApproach",
                "state_id": "0x06",
                "next_state": "0x08",
                "next_substate": "0x00",
            },
            "state_08": {
                "name": "OrbitCombatAttack",
                "state_id": "0x08",
                "attack_helper": "0x015136",
                "sfx": "0x0080034F",
                "trigger_condition": "+$32 == 12 and $FF1971 % 3 == 0",
                "next_state": "0x0A",
                "next_substate": "0x04",
            },
            "state_0a": {
                "name": "Retreat",
                "state_id": "0x0A",
                "next_substate": "0x08",
            },
            "state_16": {
                "name": "ChaseAliHitbox",
                "state_id": "0x16",
                "hitbox_table": "0x00058CE6",
                "next_state_on_hit": "0x18",
                "next_substate_on_hit": "0x1A",
                "chase_speed": "0x00010000",
            },
            "state_18": {
                "name": "HitThrustAndCallback",
                "state_id": "0x18",
                "hit_callback": "0x014C04",
                "post_hit_speed": "0x00030000",
                "post_hit_timer": 20,
                "angle_flip_delta": "0x80",
                "ali_knockback_vel": "0x00020000",
            },
        },
        "orbit_helpers": {
            "high_orbit": {
                "function": "0x014C1C",
                "offsets": {
                    "facing_0": {"dx": "+0x60", "dy": "-0x30"},
                    "facing_1": {"dx": "-0x60", "dy": "-0x30"},
                    "facing_2": {"dx": "0", "dy": "-0x68"},
                },
                "y_clamp_min": "0x00C8",
                "speed_tiers": {
                    "dist_gt_24": "0x00030000",
                    "dist_17_to_24": "0x00020000",
                    "dist_le_16": "0x00018000",
                },
                "timer_reload": 4,
            },
            "low_orbit": {
                "function": "0x014CA0",
                "offsets": {
                    "facing_0": {"dx": "+0x48", "dy": "-0x30"},
                    "facing_1": {"dx": "-0x48", "dy": "-0x30"},
                    "facing_2": {"dx": "0", "dy": "-0x38"},
                },
                "y_clamp_min": None,
                "speed_tiers": {
                    "dist_gt_24": "0x00028000",
                    "dist_17_to_24": "0x00020000",
                    "dist_le_16": "0x00018000",
                },
                "timer_reload": 4,
            },
        },
        "constants": {
            "sfx_projectile": "0x0080034F",
            "high_orbit_y_clamp": "0x00C8",
            "state_18_rebound_timer": 20,
            "orbit_timer_reload": 4,
            "knockback_velocity": "0x00020000",
        },
        "ram_addresses": {
            "ali_entity_ptr": "0x00FF19E8",
            "frame_counter_prng": "0x00FF1971",
            "hit_callback_slot": "0x00FF1940",
        },
        "external_targets": {
            "steer_towards_target": "0x0150A0",
            "spawn_projectile_special": "0x015214",
            "orbit_attack_helper": "0x015136",
            "test_hitbox": "0x00BAAC",
            "apply_hitbox": "0x00BCE2",
            "target_hurt_reaction": "0x00BEBA",
            "state_reset_common": "0x014958",
            "sound_dispatch": "0x00E7FC",
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
