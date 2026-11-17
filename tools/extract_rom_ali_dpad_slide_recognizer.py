#!/usr/bin/env python3
"""
extract_rom_ali_dpad_slide_recognizer.py — M15.392: ROM Ali 8-Direction D-Pad Velocity Resolver & Slide Motion Recognizer (0x86B4)
Disassembles and validates Beyond Oasis (USA) ROM routines at 0x0085E2..0x008778:
  - 0x0085E2..0x0086B2: 16-entry D-pad jump table 0x0085FA resolver (computes 8-way directional velocity & facing: Up/Down 0x30000, Left/Right 0x36000, Diagonals 0x2A000/0x25800)
  - 0x0086B4..0x008766: Slide attack motion recognizer 0x86B4 (30-frame window scan in $FF1668, rotational sequence tracking via table 0x008768, clear $FF16EA on match)
  - 0x008768..0x008778: 16-byte directional encoding lookup table
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_ali_dpad_slide_recognizer_m15_392.json"


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

    # 0x0085E2: D-pad resolver entry
    assert u16(rom, 0x0085E2) == 0x362E  # move.w $16(a6), d3
    assert u16(rom, 0x0085E4) == 0x0016
    assert u16(rom, 0x0085E6) == 0x1039  # move.b $FF165E.l, d0
    assert u32(rom, 0x0085E8) == 0x00FF165E
    assert u16(rom, 0x0085EC) == 0x0240  # andi.w #$f, d0
    assert u16(rom, 0x0085EE) == 0x000F
    assert u16(rom, 0x0085F8) == 0x4ED0  # jmp (a0)

    # 0x00861A: Directional velocity targets
    assert u16(rom, 0x00861A) == 0x7200  # moveq #0, d1 (neutral)
    assert u16(rom, 0x00861C) == 0x7400  # moveq #0, d2
    assert u16(rom, 0x008624) == 0x7601  # moveq #1, d3 (Right)
    assert u32(rom, 0x008628) == 0x00036000  # d1 = 0x36000
    assert u16(rom, 0x008634) == 0x7603  # moveq #3, d3 (Left)
    assert u32(rom, 0x008638) == 0xFFFCA000  # d1 = -0x36000
    assert u16(rom, 0x008644) == 0x7602  # moveq #2, d3 (Down)
    assert u32(rom, 0x00864A) == 0x00030000  # d2 = 0x30000
    assert u16(rom, 0x008654) == 0x7600  # moveq #0, d3 (Up)
    assert u32(rom, 0x00865A) == 0xFFFD0000  # d2 = -0x30000

    # Diagonals
    assert u32(rom, 0x008668) == 0x0002A000  # d1 = 0x2A000
    assert u32(rom, 0x00866E) == 0x00025800  # d2 = 0x25800 (Down-Right)
    assert u32(rom, 0x00867C) == 0x0002A000  # d1 = 0x2A000
    assert u32(rom, 0x008682) == 0xFFFDA800  # d2 = -0x25800 (Up-Right)

    # 0x0086B4: Slide motion recognizer
    assert u16(rom, 0x0086B4) == 0x41F9  # lea $FF1668.l, a0
    assert u32(rom, 0x0086B6) == 0x00FF1668
    assert u16(rom, 0x0086CC) == 0x0C47  # cmpi.w #$1d, d7 (30 frames max)
    assert u16(rom, 0x0086CE) == 0x001D
    assert u16(rom, 0x00875C) == 0x4279  # clr.w $FF16EA.l
    assert u32(rom, 0x00875E) == 0x00FF16EA
    assert u16(rom, 0x008762) == 0x003C  # ori.b #1, ccr (match!)
    assert u16(rom, 0x008764) == 0x0001
    assert u16(rom, 0x008766) == 0x4E75  # rts

    # 0x008768: 16-byte directional encoding LUT
    lut_bytes = list(rom[0x008768:0x008778])
    assert lut_bytes == [0xFF, 0, 4, 0xFF, 6, 7, 5, 0xFF, 2, 1, 3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]

    evidence = {
        "milestone": "M15.392",
        "description": "ROM Ali 8-Direction D-Pad Velocity Resolver & Slide Motion Recognizer (0x86B4)",
        "address_range": ["0x0085E2", "0x008778"],
        "byte_count": 0x008778 - 0x0085E2,
        "direction_lut": lut_bytes,
        "routines": [
            {
                "name": "ali_dpad_8way_velocity_resolver",
                "start": "0x0085E2",
                "end": "0x0086B2",
                "purpose": "Decodes 16-entry jump table 0x0085FA to calculate 8-way directional velocities (Up/Down 0x30000, Left/Right 0x36000, Diagonals 0x2A000/0x25800) and facing"
            },
            {
                "name": "ali_motion_slide_recognizer_0x86b4",
                "start": "0x0086B4",
                "end": "0x008766",
                "purpose": "Official Slide motion recognizer: scans up to 30 frames in $FF1668 for 4-step rotational sequence using LUT 0x008768; clears ring count on match"
            }
        ]
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Evidence written to {OUTPUT_JSON} ({len(evidence['routines'])} routines)")


if __name__ == "__main__":
    extract_evidence()
