#!/usr/bin/env python3
"""
extract_rom_ali_somersault_dash_leap.py — M15.393: ROM Ali Somersault Flip (0x8778) & Compound Dash Leap (0x8814) Motion Recognizers
Disassembles and validates Beyond Oasis (USA) ROM routines at 0x008778..0x008910:
  - 0x008778..0x0087F2: Somersault flip motion recognizer 0x8778 (forward-back-forward 3-step sequence matching table 0x0087F4)
  - 0x0087F4..0x008814: 32-byte 4-direction motion pattern table (facing 0: [1,2,1], 1: [8,4,8], 2: [2,1,2], 3: [4,8,4])
  - 0x008814..0x00890E: Compound Dash Leap motion recognizer 0x8814 (matches forward-back-forward lead-in then chains to 360-degree rotational slide sequence; clears $FF16EA on match)
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_ali_somersault_dash_leap_m15_393.json"


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

    # 0x008778: Somersault flip motion recognizer
    assert u16(rom, 0x008778) == 0x41F9  # lea $FF1668.l, a0
    assert u32(rom, 0x00877A) == 0x00FF1668
    assert u16(rom, 0x008790) == 0x302E  # move.w $16(a6), d0
    assert u16(rom, 0x008792) == 0x0016
    assert u16(rom, 0x0087E8) == 0x4279  # clr.w $FF16EA.l
    assert u32(rom, 0x0087EA) == 0x00FF16EA
    assert u16(rom, 0x0087EE) == 0x003C  # ori.b #1, ccr
    assert u16(rom, 0x0087F0) == 0x0001
    assert u16(rom, 0x0087F2) == 0x4E75  # rts

    # 0x0087F4: 32-byte 4-direction motion pattern table
    table_bytes = list(rom[0x0087F4:0x008814])
    expected = [
        1, 2, 1, 0, 15, 2, 15, 0,   # Facing 0 (Up: Up->Down->Up)
        8, 4, 8, 0, 15, 4, 15, 0,   # Facing 1 (Right: Right->Left->Right)
        2, 1, 2, 0, 15, 1, 15, 0,   # Facing 2 (Down: Down->Up->Down)
        4, 8, 4, 0, 15, 8, 15, 0,   # Facing 3 (Left: Left->Right->Left)
    ]
    assert table_bytes == expected

    # 0x008814: Compound Dash Leap motion recognizer
    assert u16(rom, 0x008814) == 0x41F9  # lea $FF1668.l, a0
    assert u32(rom, 0x008816) == 0x00FF1668
    assert u16(rom, 0x008904) == 0x4279  # clr.w $FF16EA.l
    assert u32(rom, 0x008906) == 0x00FF16EA
    assert u16(rom, 0x00890A) == 0x003C  # ori.b #1, ccr
    assert u16(rom, 0x00890C) == 0x0001
    assert u16(rom, 0x00890E) == 0x4E75  # rts

    evidence = {
        "milestone": "M15.393",
        "description": "ROM Ali Somersault Flip (0x8778) & Compound Dash Leap (0x8814) Motion Recognizers",
        "address_range": ["0x008778", "0x008910"],
        "byte_count": 0x008910 - 0x008778,
        "somersault_table": {
            "facing_0_up": expected[0:8],
            "facing_1_right": expected[8:16],
            "facing_2_down": expected[16:24],
            "facing_3_left": expected[24:32],
        },
        "routines": [
            {
                "name": "ali_motion_somersault_0x8778",
                "start": "0x008778",
                "end": "0x0087F2",
                "purpose": "Recognizes Somersault flip forward-back-forward motion sequence using table 0x0087F4"
            },
            {
                "name": "ali_motion_dash_leap_0x8814",
                "start": "0x008814",
                "end": "0x00890E",
                "purpose": "Recognizes compound Dash Leap: matches 3-step lead-in from table 0x0087F4 then chains into 360-degree rotational slide sequence"
            }
        ]
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Evidence written to {OUTPUT_JSON} ({len(evidence['routines'])} routines)")


if __name__ == "__main__":
    extract_evidence()
