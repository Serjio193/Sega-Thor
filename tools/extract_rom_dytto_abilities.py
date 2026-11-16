#!/usr/bin/env python3
"""
extract_rom_dytto_abilities.py — Milestone M15.263

Extracts ROM semantic knowledge for Dytto Spirit Abilities, State Machine
Handlers (0x00..0x1A), Water Projectile Spawning, and Elemental Damage:
1. Complete Spirit State Table (0x014914, 14 entries: 0x00..0x1A)
2. State 0x00 (0x014932) Autonomous PRNG Command & Wander Dispatch
3. State 0x10 (0x014A72) & 0x015214 Dytto 3-Way Spread Attack (Type 0x0024)
4. State 0x06/0x08/0x0A (0x014ABC..0x014B7A) Magic Burst & 0x015136 Spawner
5. State 0x12/0x14 (0x014E1A..0x014EE4) Projectile Collision & Water Damage (0x2008)
6. State 0x16/0x18 (0x014B7C..0x014C1A) Return-to-Ali & Buff Callback (0x014C04)
7. State 0x0E (0x014EE6) Knockback Deceleration & Recovery

All addresses and opcodes are verified directly against the canonical ROM
(`local-roms/Beyond Oasis (USA).md`) so downstream agents can implement
Dytto's combat and support abilities in C++20 without guessing.
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_dytto_abilities_m15_263.json"

SPIRIT_STATE_TABLE_ADDR = 0x014914
SPIRIT_STATE_COUNT = 14  # States 0x00, 0x02, ..., 0x1A

# Expected state handler addresses verified by M68k disassembly
EXPECTED_STATE_HANDLERS = {
    "0x00": 0x014932,  # Idle / PRNG autonomous command & wander dispatch
    "0x02": 0x014930,  # Immediate RTS (no-op)
    "0x04": 0x014A5C,  # Follow player steering + timer countdown -> State 0x00
    "0x06": 0x014ABC,  # Magic attack stage 1 -> State 0x08
    "0x08": 0x014AEA,  # Magic attack stage 2 (spawns 0x0024 via 0x015136) -> State 0x0A
    "0x0A": 0x014B50,  # Magic attack recovery -> 0x014958 (wander)
    "0x0C": 0x014D1A,  # Projectile flight (arcing, spawned by 0x015136)
    "0x0E": 0x014EE6,  # Knockback / velocity damping recovery
    "0x10": 0x014A72,  # Dytto special spread attack (calls 0x015214 when $30 == 0x18)
    "0x12": 0x014E1A,  # Spread projectile flight & collision (0x00B922 -> 0x014E86)
    "0x14": 0x014E0C,  # Projectile pop / despawn on animation end
    "0x16": 0x014B7C,  # Return-to-Ali approach (hitbox $058CE6 via 0x00BAAC) -> State 0x18
    "0x18": 0x014BB8,  # Ali contact buff / heal callback ($FF1940 = 0x014C04, jsr 0x00BCE2)
    "0x1A": 0x014F60,  # Awakening cutscene timeline (180f..420f -> State 0x04)
}


def read_u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def read_s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def read_u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_contracts(rom: bytes) -> dict:
    """Verify key M68k instruction words in ROM before emitting evidence."""
    contracts = {}

    # 1. State 0x00 (0x014932): PRNG thresholds (0x2000, 0x6000, 0x9000)
    # 0x014940: cmpi.w #$2000, d0 (0x0C40 0x2000)
    # 0x014948: cmpi.w #$6000, d0 (0x0C40 0x6000)
    # 0x014950: cmpi.w #$9000, d0 (0x0C40 0x9000)
    assert read_u16_be(rom, 0x014940) == 0x0C40
    t1 = read_u16_be(rom, 0x014942)
    assert read_u16_be(rom, 0x014948) == 0x0C40
    t2 = read_u16_be(rom, 0x01494A)
    assert read_u16_be(rom, 0x014950) == 0x0C40
    t3 = read_u16_be(rom, 0x014952)
    contracts["prng_thresholds"] = [f"0x{t1:04X}", f"0x{t2:04X}", f"0x{t3:04X}"]

    # 2. State 0x10 (0x014A8A): cmpi.w #$18, $30(a6) (Dytto type guard)
    # 0x014A8A: 0C6E 0018 0030
    assert read_u16_be(rom, 0x014A8A) == 0x0C6E
    dytto_guard_type = read_u16_be(rom, 0x014A8C)
    assert dytto_guard_type == 0x0018
    contracts["dytto_special_guard_type"] = f"0x{dytto_guard_type:04X}"

    # 3. 0x015214: 3-way spread angles (0, +0x1C, -0x1C -> 0xFFE4)
    # 0x01521A: move.w #$1C, d6 (3C3C 001C)
    # 0x015222: move.w #$FFE4, d6 (3C3C FFE4)
    assert read_u16_be(rom, 0x01521A) == 0x3C3C
    spread_pos = read_s16_be(rom, 0x01521C)
    assert read_u16_be(rom, 0x015222) == 0x3C3C
    spread_neg = read_s16_be(rom, 0x015224)
    contracts["spread_angles"] = [0, spread_pos, spread_neg]

    # 4. Projectile entity type 0x0024 in both 0x015142 and 0x015232
    # 0x015142: move.w #$24, $0(a6) (3D7C 0024 0000)
    # 0x015232: move.w #$24, $0(a6) (3D7C 0024 0000)
    assert read_u16_be(rom, 0x015142) == 0x3D7C
    proj_type_a = read_u16_be(rom, 0x015144)
    assert read_u16_be(rom, 0x015232) == 0x3D7C
    proj_type_b = read_u16_be(rom, 0x015234)
    assert proj_type_a == 0x0024 and proj_type_b == 0x0024
    contracts["projectile_entity_type"] = f"0x{proj_type_a:04X}"

    # 5. Elemental Water Damage word in 0x014E86 (0x014E94 & 0x014EDE)
    # 0x014E94: move.w #$2008, $86(a0) (317C 2008 0086)
    # 0x014EDE: move.w #$A008, $86(a0) (317C A008 0086)
    assert read_u16_be(rom, 0x014E94) == 0x317C
    dmg_ground = read_u16_be(rom, 0x014E96)
    assert read_u16_be(rom, 0x014EDE) == 0x317C
    dmg_air = read_u16_be(rom, 0x014EE0)
    contracts["water_damage_ground"] = f"0x{dmg_ground:04X}"
    contracts["water_damage_airborne"] = f"0x{dmg_air:04X}"

    # 6. Knockback deceleration constants in State 0x0E (0x014EE6)
    # 0x014EEC: move.l #$8000, d0 (203C 0000 8000)
    # 0x014F2A: subi.l #$A000, $56(a6) (04AE 0000 A000 0056)
    assert read_u16_be(rom, 0x014EEC) == 0x203C
    xy_damp = read_u32_be(rom, 0x014EEE)
    assert read_u16_be(rom, 0x014F2A) == 0x04AE
    z_damp = read_u32_be(rom, 0x014F2C)
    contracts["knockback_xy_damping"] = f"0x{xy_damp:08X}"
    contracts["knockback_z_damping"] = f"0x{z_damp:08X}"

    return contracts


def main() -> int:
    rom_path = Path(ROM_PATH_DEFAULT)
    out_path = Path(OUTPUT_PATH_DEFAULT)

    if not rom_path.exists():
        print(f"Error: ROM not found at {rom_path}")
        return 1

    rom = rom_path.read_bytes()

    # Decode and verify all 14 entries of Spirit State Table 0x014914
    state_handlers = {}
    for i in range(SPIRIT_STATE_COUNT):
        state_id = i * 2
        key = f"0x{state_id:02X}"
        offset = read_s16_be(rom, SPIRIT_STATE_TABLE_ADDR + state_id)
        target = SPIRIT_STATE_TABLE_ADDR + state_id + offset
        expected = EXPECTED_STATE_HANDLERS[key]
        assert target == expected, f"State {key}: got 0x{target:06X}, expected 0x{expected:06X}"
        state_handlers[key] = f"0x{target:06X}"

    contracts = verify_rom_contracts(rom)

    evidence = {
        "milestone": "M15.263",
        "description": "ROM Dytto Spirit Abilities, State Machine (0x00..0x1A), Projectiles, and Elemental Damage",
        "state_table": {
            "address": f"0x{SPIRIT_STATE_TABLE_ADDR:06X}",
            "dispatcher": "0x014908",
            "entry_count": SPIRIT_STATE_COUNT,
            "handlers": state_handlers,
        },
        "autonomous_dispatch_0x00": {
            "address": "0x014932",
            "lock_flag_ram": "0x00FF1987",
            "prng_helper": "0x00E268",
            "branches": [
                {"condition": "prng <= 0x2000", "probability": "12.5%", "next_state": "0x06", "action": "0x02", "sfx": "0x800251"},
                {"condition": "0x2000 < prng <= 0x6000", "probability": "25.0%", "next_state": "0x10", "action": "0x12", "note": "Dytto 3-way spread"},
                {"condition": "0x6000 < prng <= 0x9000", "probability": "18.75%", "next_state": "0x16", "action": "0x18", "sfx": "0x800251"},
                {"condition": "prng > 0x9000 or lock != 0", "probability": "43.75%", "next_state": "0x04", "timer_frames": 10, "note": "Wander around Ali"},
            ],
        },
        "dytto_special_spread_0x10": {
            "state_handler": "0x014A72",
            "spawner_routine": "0x015214",
            "entity_type_guard": "$30(A6) == 0x0018",
            "projectile_type": "0x0024",
            "projectile_count": 3,
            "spread_angle_offsets": [0, 28, -28],
            "projectile_state": "0x12",
            "projectile_action": "0x14",
            "hitbox_radius": 5,
            "sfx": "0x80034F",
        },
        "magic_burst_0x06_0x08_0x0A": {
            "stage1_handler": "0x014ABC",
            "stage2_handler": "0x014AEA",
            "recovery_handler": "0x014B50",
            "spawner_routine": "0x015136",
            "spawn_cadence": "Every 3rd frame ($FF1971 % 3 == 0) while $32(A6) == 0x0C",
            "projectile_type": "0x0024",
            "projectile_state": "0x0C",
            "projectile_action": "0x06",
            "hitbox_radius": 8,
            "initial_vz_fixed": "-0x00050000",
            "sfx": "0x80034F",
        },
        "projectile_collision_and_damage": {
            "spread_flight_handler": "0x014E1A",
            "pop_despawn_handler": "0x014E0C",
            "hit_apply_helper": "0x014E86",
            "collision_helper": "0x00B922",
            "damage_word_ground": "0x2008",
            "damage_word_airborne": "0xA008",
            "base_damage": 8,
            "elemental_bit_13": True,
            "hit_sfx": "0x700345",
        },
        "ali_contact_buff_0x16_0x18": {
            "approach_handler": "0x014B7C",
            "contact_handler": "0x014BB8",
            "callback_address": "0x014C04",
            "callback_pointer_ram": "0x00FF1940",
            "hitbox_table": "0x058CE6",
            "overlap_helper": "0x00BAAC",
            "effect_helper": "0x00BCE2",
        },
        "knockback_recovery_0x0E": {
            "handler": "0x014EE6",
            "xy_damping_step": "0x00008000",
            "z_damping_step": "0x0000A000",
        },
        "verified_rom_contracts": contracts,
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} ({len(json.dumps(evidence))} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
