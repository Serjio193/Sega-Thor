#!/usr/bin/env python3
"""
Extracts ROM evidence for M15.416: Static Entity & Shadowless Fast SAT Sprite Compositors
ROM Range: 0x00AD76..0x00B15E (1000 bytes).
"""

import hashlib
import json
import os
import struct

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_JSON = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_static_fast_sat_compositor_m15_416.json")

START_ADDR = 0x00AD76
END_ADDR = 0x00B15E
BYTE_COUNT = END_ADDR - START_ADDR  # 1000 bytes

def u8(data, off):
    return data[off]

def u16(data, off):
    return struct.unpack_from(">H", data, off)[0]

def s16(data, off):
    return struct.unpack_from(">h", data, off)[0]

def u32(data, off):
    return struct.unpack_from(">I", data, off)[0]

def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    assert len(rom) >= END_ADDR, "ROM too small"

    block = rom[START_ADDR:END_ADDR]
    sha256 = hashlib.sha256(block).hexdigest()

    # Verify Static Entity Compositor Entry Point (0x00AD76)
    # 0x00AD76: btst.b #1, $36(a6) -> 0x082E, 0x0001, 0x0036
    assert u16(rom, 0x00AD76) == 0x082E
    assert u16(rom, 0x00AD78) == 0x0001
    assert u16(rom, 0x00AD7A) == 0x0036
    # 0x00AD7C: bne.w $b010 -> 0x6600, 0x0292
    assert u16(rom, 0x00AD7C) == 0x6600
    assert u16(rom, 0x00AD7E) == 0x0292
    # 0x00AD80: movem.l d7, -(a7) -> 0x48E7, 0x0100
    assert u16(rom, 0x00AD80) == 0x48E7
    assert u16(rom, 0x00AD82) == 0x0100

    # 0x00B00C: movem.l (a7)+, d7 -> 0x4CDF, 0x0080
    assert u16(rom, 0x00B00C) == 0x4CDF
    assert u16(rom, 0x00B00E) == 0x0080
    # 0x00B010: rts -> 0x4E75
    assert u16(rom, 0x00B010) == 0x4E75

    # Verify Dynamic Tails (0x00B012..0x00B020)
    assert u16(rom, 0x00B012) == 0x6100  # bsr.w $b34c
    assert u16(rom, 0x00B016) == 0x6000  # bra.w $aee6
    assert u16(rom, 0x00B01A) == 0x6100  # bsr.w $b3b2
    assert u16(rom, 0x00B01E) == 0x6000  # bra.w $af9c

    # Verify Shadowless Fast Sprite Compositor (0x00B022..0x00B15E)
    # 0x00B022: moveq #0, d0 -> 0x7000
    assert u16(rom, 0x00B022) == 0x7000
    # 0x00B024: move.w $16(a6), d0 -> 0x302E, 0x0016
    assert u16(rom, 0x00B024) == 0x302E
    assert u16(rom, 0x00B026) == 0x0016
    # 0x00B14A: suba.l #$ff13cc, a1 -> 0x93FC, 0x00FF, 0x13CC
    assert u16(rom, 0x00B14A) == 0x93FC
    assert u32(rom, 0x00B14C) == 0x00FF13CC
    # 0x00B15C: rts -> 0x4E75
    assert u16(rom, 0x00B15C) == 0x4E75

    evidence = {
        "subsystem": "ROM Static Entity & Shadowless Fast SAT Sprite Compositors",
        "milestone": "M15.416",
        "rom_range": {
            "start": f"0x{START_ADDR:06X}",
            "end": f"0x{END_ADDR:06X}",
            "size_bytes": BYTE_COUNT,
            "sha256": sha256
        },
        "routines": {
            "static_sat_compositor": {
                "start": "0x00AD76",
                "end": "0x00B022",
                "size_bytes": 0x00B022 - 0x00AD76,
                "exit_rts": "0x00B010",
                "features": [
                    "Early visibility exit: btst.b #1, $36(a6)",
                    "Status code 3 check with flag 0x8000001",
                    "Direct cache update in +0x34(a6) without DMA quota check",
                    "Secondary animation lookup +0x5E(a6) with -$8 offset",
                    "Flash palette counter setup (+0x3E = 0x200 or 0x300)",
                    "Camera-relative coordinate transform (+0x80) with plane A/B selection",
                    "Screen coordinate bypass mode (bit 4 of $36 set)",
                    "Subpixel camera jitter application (+0x17 bit 0)",
                    "Sprite piece blitting with screen viewport clipping [0x60, 0x160] Y and [0x60, 0x1C0] X",
                    "Max 80 sprites link limit enforcement",
                    "Dynamic shadow blitter referencing table 0x00ACFE",
                    "SAT buffer offset and link count writeback"
                ]
            },
            "shadowless_fast_sat_compositor": {
                "start": "0x00B022",
                "end": "0x00B15E",
                "size_bytes": 0x00B15E - 0x00B022,
                "exit_rts": "0x00B15C",
                "features": [
                    "Zero register overhead (no register saving)",
                    "No DMA queueing or tile dimension accumulation",
                    "No shadow sprite generation",
                    "Lightweight screen viewport clipping [0x60, 0x160] Y and [0x60, 0x1C0] X",
                    "Fast SAT attribute write and buffer offset writeback"
                ]
            }
        },
        "dispatch_mechanics": {
            "mode_select_bit": "bit 0 of +0x36(a6)",
            "rule": "If bit 0 clear (0) -> static compositor (0x00AD76); if bit 0 set (1) -> dynamic DMA compositor (0x00A9A0)"
        }
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted M15.416 evidence: {BYTE_COUNT} bytes, SHA256: {sha256}")

if __name__ == "__main__":
    extract()
