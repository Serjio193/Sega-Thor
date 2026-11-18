#!/usr/bin/env python3
"""
extract_rom_boss_charge_engine.py - M15.456 ROM Extractor
Extracts Boss Type $3A Combat State Machine (Part 2), Velocity Damping,
3D Parabolic Trajectory Physics, Floor Collision 50% Recoil Bounce &
Spawner Notification Engine from 0x00EE94..0x00F054 (448 bytes).
"""

import json
import os
import sys
import capstone

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_boss_charge_engine_m15_456.json"

START_ADDR = 0x00EE94
END_ADDR = 0x00F054
TOTAL_BYTES = END_ADDR - START_ADDR

SUBROUTINES = [
    {
        "name": "Boss3A_State4_Update",
        "start": 0x00EE94,
        "end": 0x00EF1E,
        "size": 0x00EF1E - 0x00EE94,
        "description": "State 4 completion checker, subtype $A6 audio dispatcher, screen shake and state 6/8 transition"
    },
    {
        "name": "Boss3A_State2_Update",
        "start": 0x00EF1E,
        "end": 0x00EF38,
        "size": 0x00EF38 - 0x00EF1E,
        "description": "State 2 completion checker, transition to State 4"
    },
    {
        "name": "Boss3A_State0_CountdownTimer",
        "start": 0x00EF38,
        "end": 0x00EF5E,
        "size": 0x00EF5E - 0x00EF38,
        "description": "State 0 countdown timer at +$02(a6), transition to State 2 on zero"
    },
    {
        "name": "Boss3A_VelocityDampingIntegration",
        "start": 0x00EF5E,
        "end": 0x00EFA2,
        "size": 0x00EFA2 - 0x00EF5E,
        "description": "0.75x velocity damping per tick and subpixel coordinate integration"
    },
    {
        "name": "Boss3A_TrajectoryFloorBounceRecoil",
        "start": 0x00EFA2,
        "end": 0x00F038,
        "size": 0x00F038 - 0x00EFA2,
        "description": "3D trajectory physics, -0xC000 gravity, floor collision 50% bounce recoil & viewport culling"
    },
    {
        "name": "Boss3A_DespawnAndNotifySpawner",
        "start": 0x00F038,
        "end": 0x00F054,
        "size": 0x00F054 - 0x00F038,
        "description": "Spawner notification (+0x06(a0) = 0x10) and entity slot deallocation"
    }
]

def extract():
    if not os.path.isfile(ROM_PATH):
        print(f"Error: ROM file not found at {ROM_PATH}")
        sys.exit(1)

    with open(ROM_PATH, "rb") as f:
        rom_data = f.read()

    chunk = rom_data[START_ADDR:END_ADDR]
    if len(chunk) != TOTAL_BYTES:
        print(f"Error: Expected {TOTAL_BYTES} bytes, got {len(chunk)}")
        sys.exit(1)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    instructions = []
    for ins in md.disasm(chunk, START_ADDR):
        instructions.append({
            "address": f"0x{ins.address:06X}",
            "mnemonic": ins.mnemonic,
            "op_str": ins.op_str,
            "bytes": ins.bytes.hex()
        })

    subroutine_map = {}
    for sub in SUBROUTINES:
        sub_chunk = rom_data[sub["start"]:sub["end"]]
        sub_insns = []
        for ins in md.disasm(sub_chunk, sub["start"]):
            sub_insns.append({
                "address": f"0x{ins.address:06X}",
                "mnemonic": ins.mnemonic,
                "op_str": ins.op_str,
                "bytes": ins.bytes.hex()
            })
        subroutine_map[sub["name"]] = {
            "meta": sub,
            "instruction_count": len(sub_insns),
            "instructions": sub_insns
        }

    evidence = {
        "milestone": "M15.456",
        "title": "ROM Boss Type $3A Combat State Engine (Part 2), Damping & Bounce Physics",
        "rom_range": {
            "start": f"0x{START_ADDR:06X}",
            "end": f"0x{END_ADDR:06X}",
            "size_bytes": TOTAL_BYTES,
            "total_instructions": len(instructions)
        },
        "constants": {
            "gravity_acceleration": "-0x0000C000",
            "velocity_damping_factor": 0.75,
            "bounce_restitution": 0.5,
            "bounce_threshold": "0x00004000",
            "culling_margin_pixels": 48,
            "sfx_roar": "0x00B00225",
            "sfx_quake": "0x00600382",
            "sfx_swing": "0x00B00224",
            "sfx_bounce": "0x00400376",
            "screen_shake_frames": 4,
            "spawner_notify_msg": "0x0010"
        },
        "subroutines": subroutine_map,
        "all_instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({TOTAL_BYTES} bytes) to {OUTPUT_JSON}")

if __name__ == "__main__":
    extract()
