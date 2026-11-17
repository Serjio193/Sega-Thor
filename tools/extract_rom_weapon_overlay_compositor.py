#!/usr/bin/env python3
"""
Extracts ROM evidence for M15.417: Entity Secondary Weapon & Auxiliary Overlay
SAT Compositor Subsystem
ROM Range: 0x00B15E..0x00B34C (494 bytes).
"""

import hashlib
import json
import os
import struct

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_JSON = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_weapon_overlay_compositor_m15_417.json")

START_ADDR = 0x00B15E
END_ADDR = 0x00B34C
BYTE_COUNT = END_ADDR - START_ADDR  # 494 bytes

def u16(data, off):
    return struct.unpack_from(">H", data, off)[0]

def u32(data, off):
    return struct.unpack_from(">I", data, off)[0]

def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    assert len(rom) >= END_ADDR, "ROM too small"

    block = rom[START_ADDR:END_ADDR]
    sha256 = hashlib.sha256(block).hexdigest()

    # Verify entry point: btst.b #9, d5 -> 0x0805, 0x0009
    assert u16(rom, 0x00B15E) == 0x0805
    assert u16(rom, 0x00B160) == 0x0009
    # Branch to non-DMA path: bne.w $b28e -> 0x6600, 0x012A
    assert u16(rom, 0x00B162) == 0x6600
    assert u16(rom, 0x00B164) == 0x012A

    # DMA path entry: movem.l d0-d1/d3/a4-a5, -(a7) -> 0x48E7, 0xD00C
    assert u16(rom, 0x00B166) == 0x48E7
    assert u16(rom, 0x00B168) == 0xD00C

    # Descriptor table reference at 0x00B1AA: lea $3f2fa.l, a3 -> 0x47F9, 0x0003, 0xF2FA
    assert u16(rom, 0x00B1AA) == 0x47F9
    assert u32(rom, 0x00B1AC) == 0x0003F2FA

    # DMA path exit: movem.l (a7)+, d0-d1/d3/a4-a5; rts
    assert u16(rom, 0x00B288) == 0x4CDF
    assert u16(rom, 0x00B28A) == 0x300B
    assert u16(rom, 0x00B28C) == 0x4E75

    # Non-DMA path entry: 0x00B28E
    assert u16(rom, 0x00B28E) == 0x48E7  # movem.l

    # Non-DMA path exit: 0x00B34A: rts
    assert u16(rom, 0x00B34A) == 0x4E75

    evidence = {
        "subsystem": "ROM Entity Secondary Weapon & Auxiliary Overlay SAT Compositor",
        "milestone": "M15.417",
        "rom_range": {
            "start": f"0x{START_ADDR:06X}",
            "end": f"0x{END_ADDR:06X}",
            "size_bytes": BYTE_COUNT,
            "sha256": sha256
        },
        "paths": {
            "dma_weapon_overlay": {
                "start": "0x00B15E",
                "exit": "0x00B28C",
                "descriptor_table": "0x0003F2FA",
                "aux_vram_tile_offset": "+0x5A(a6)",
                "cache_offset": "+0x5C(a6)",
                "tile_count_table": "0x00AD56"
            },
            "nondma_weapon_overlay": {
                "start": "0x00B28E",
                "exit": "0x00B34A",
                "descriptor_table": "0x0003F2FA",
                "aux_vram_tile_offset": "+0x5A(a6)"
            }
        },
        "clipping_limits": {
            "viewport_y": ["0x60", "0x160"],
            "viewport_x": ["0x60", "0x1C0"],
            "max_sprites": 80
        }
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted M15.417 evidence: {BYTE_COUNT} bytes, SHA256: {sha256}")

if __name__ == "__main__":
    extract()
