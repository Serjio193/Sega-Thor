#!/usr/bin/env python3
"""
extract_rom_ali_shoreline_water_stepper.py — M15.381: ROM Ali Shoreline Stepper, Water 3/4 Scaler & Landing Gate
Disassembles and validates Beyond Oasis (USA) ROM routines at 0x007074..0x007262:
  - 0x007074..0x0070F6: Shoreline exit airborne stepper (0xC000 gravity, landing splash SFX 0x600343)
  - 0x0070F8..0x007110: Ripple callback 0x70F8
  - 0x007112..0x007196: Water jump landing & water surface audio gate (SFX 0x500341)
  - 0x007198..0x0071D8: Water 75% velocity scaler (v >> 1) + (v >> 2) & opposing input cancellation
  - 0x0071DA..0x007262: Aerial jump slash descent, shadow update & landing recovery (timer 0x0C)
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_ali_shoreline_water_stepper_m15_381.json"


def u16(buf, offset):
    return struct.unpack_from(">H", buf, offset)[0]


def u32(buf, offset):
    return struct.unpack_from(">I", buf, offset)[0]


def extract_evidence():
    if not os.path.exists(ROM_PATH):
        print(f"Error: ROM not found at {ROM_PATH}")
        sys.exit(1)

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    # 0x007074: Shoreline exit airborne stepper
    assert u16(rom, 0x007074) == 0x3D7C  # move.w #0, $4a(a6)
    assert u16(rom, 0x007080) == 0x0480  # subi.l #$c000, d0
    assert u32(rom, 0x007082) == 0x0000C000
    assert u16(rom, 0x0070BA) == 0x42AE  # clr.l $10(a6)
    assert u16(rom, 0x0070BE) == 0x42AE  # clr.l $56(a6)
    assert u32(rom, 0x0070C2) == 0x203C0060  # move.l #$600343, d0
    assert u16(rom, 0x0070C6) == 0x0343

    # 0x007112: Water jump landing audio gate
    assert u16(rom, 0x007124) == 0x0C6E  # cmpi.w #4, $30(a6)
    assert u16(rom, 0x00712C) == 0x08EE  # bset.b #0, $67(a6)
    assert u16(rom, 0x007134) == 0x08AE  # bclr.b #0, $67(a6)
    assert u32(rom, 0x00713C) == 0x203C0050  # move.l #$500341, d0
    assert u16(rom, 0x007140) == 0x0341
    assert u16(rom, 0x00718E) == 0x3D7C  # move.w #$14, $30(a6)

    # 0x007198: 75% velocity scaler (v >> 1) + (v >> 2)
    assert u16(rom, 0x007198) == 0xE281  # asr.l #1, d1
    assert u16(rom, 0x00719A) == 0x2001  # move.l d1, d0
    assert u16(rom, 0x00719C) == 0xE281  # asr.l #1, d1
    assert u16(rom, 0x00719E) == 0xD280  # add.l d0, d1
    assert u16(rom, 0x0071A0) == 0xE282  # asr.l #1, d2
    assert u16(rom, 0x0071A2) == 0x2002  # move.l d2, d0
    assert u16(rom, 0x0071A4) == 0xE282  # asr.l #1, d2
    assert u16(rom, 0x0071A6) == 0xD480  # add.l d0, d2

    # 0x0071DA: Jump slash aerial descent
    assert u16(rom, 0x0071EE) == 0x0480  # subi.l #$c000, d0
    assert u16(rom, 0x007248) == 0x3D7C  # move.w #$c, $30(a6)
    assert u16(rom, 0x007260) == 0x4E75  # rts

    evidence = {
        "milestone": "M15.381",
        "description": "ROM Ali Shoreline Stepper, Water 3/4 Scaler & Landing Gate",
        "address_range": ["0x007074", "0x007262"],
        "byte_count": 0x007262 - 0x007074,
        "routines": [
            {
                "name": "ali_shoreline_exit_airborne_stepper",
                "start": "0x007074",
                "end": "0x0070F6",
                "purpose": "Airborne gravity step for shoreline climb, ripple 0x70F8 callback, landing SFX 0x600343"
            },
            {
                "name": "ali_water_jump_landing_audio_gate",
                "start": "0x007112",
                "end": "0x007196",
                "purpose": "Water contact flag bit 0 of $67, splash SFX 0x500341 on timer > 4, landing timer 0x14"
            },
            {
                "name": "ali_water_velocity_3_4_scaler_and_clamp",
                "start": "0x007198",
                "end": "0x0071D8",
                "purpose": "Exact 75% velocity scaling (d >> 1) + (d >> 2) and opposing inertia input cancellation"
            },
            {
                "name": "ali_jump_slash_descent_shadow",
                "start": "0x0071DA",
                "end": "0x007262",
                "purpose": "Aerial descent, shadow update 0x9D00, landing timer 0x0C and anim recovery 0x62CC"
            }
        ]
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Evidence written to {OUTPUT_JSON} ({len(evidence['routines'])} routines)")


if __name__ == "__main__":
    extract_evidence()
