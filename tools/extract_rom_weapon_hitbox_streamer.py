#!/usr/bin/env python3
"""
Extracts ROM evidence for M15.421: Weapon Attack Frame 3D Hitbox Streamer &
Dual-Pool Scanner Engine
ROM Range: 0x00BAAC..0x00BC8E (482 bytes).
"""

import hashlib
import json
import os
import struct

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_JSON = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_weapon_hitbox_streamer_m15_421.json")

START_ADDR = 0x00BAAC
END_ADDR = 0x00BC8E
BYTE_COUNT = END_ADDR - START_ADDR  # 482 bytes

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

    # Verify All-Slots Entry Point (0x00BAAC)
    # 0x00BAAC: lea $ff19e8.l, a0 -> 0x41F9, 0x00FF, 0x19E8
    assert u16(rom, 0x00BAAC) == 0x41F9
    assert u32(rom, 0x00BAAE) == 0x00FF19E8

    # Verify Enemy-Only Entry Point (0x00BAB8)
    # 0x00BAB8: lea $ff1cd8.l, a0 -> 0x41F9, 0x00FF, 0x1CD8
    assert u16(rom, 0x00BAB8) == 0x41F9
    assert u32(rom, 0x00BABA) == 0x00FF1CD8
    # 17 slots (0..16): moveq #16, d7 -> 0x7E10
    assert u16(rom, 0x00BABE) == 0x7E10

    # Verify 12-byte stream indexing
    # 0x00BAE0: movem.w (a1)+, d0/a2 -> 0x4C99, 0x0401
    assert u16(rom, 0x00BAE0) == 0x4C99
    assert u16(rom, 0x00BAE2) == 0x0401
    # 0x00BAE4: movem.w (a1)+, d1-d4 -> 0x4C99, 0x001E
    assert u16(rom, 0x00BAE4) == 0x4C99
    assert u16(rom, 0x00BAE6) == 0x001E

    # Verify Hitbox Mirroring opcodes (0x00BAF0..0x00BAF4)
    # neg.w d1 -> 0x4441; neg.w d3 -> 0x4443; exg d1, d3 -> 0xC343
    assert u16(rom, 0x00BAF0) == 0x4441
    assert u16(rom, 0x00BAF2) == 0x4443
    assert u16(rom, 0x00BAF4) == 0xC343

    # Primary exit: 0x00BBA6: rts -> 0x4E75
    assert u16(rom, 0x00BBA6) == 0x4E75

    # Secondary streamer entry: 0x00BBA8: lea $ff1cd8.l, a0
    assert u16(rom, 0x00BBA8) == 0x41F9
    assert u32(rom, 0x00BBAA) == 0x00FF1CD8

    # Secondary exit: 0x00BC8C: rts -> 0x4E75
    assert u16(rom, 0x00BC8C) == 0x4E75

    evidence = {
        "subsystem": "ROM Weapon Attack Frame 3D Hitbox Streamer & Dual-Pool Scanner Engine",
        "milestone": "M15.421",
        "rom_range": {
            "start": f"0x{START_ADDR:06X}",
            "end": f"0x{END_ADDR:06X}",
            "size_bytes": BYTE_COUNT,
            "sha256": sha256
        },
        "stream_format": {
            "entry_size_bytes": 12,
            "word_fields": ["z_min", "z_max", "x_min", "y_min", "x_max", "y_max"],
            "mirror_rule": "When facing left (status == 3), neg(x_min), neg(x_max), exg(x_min, x_max)"
        },
        "pools": {
            "all_slots_group2": {
                "entry": "0x00BAAC",
                "base_address": "0x00FF19E8",
                "slot_count": 21,
                "stride": 188
            },
            "enemy_only_slots": {
                "entry": "0x00BAB8",
                "base_address": "0x00FF1CD8",
                "slot_count": 17,
                "stride": 188
            }
        },
        "target_filters": {
            "active_check": "type > 0",
            "self_check": "target != self",
            "invulnerable_bit": "bit 3 on +0x38(a0)",
            "status_protect": "bit 0 on +0x3A(a0)"
        }
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted M15.421 evidence: {BYTE_COUNT} bytes, SHA256: {sha256}")

if __name__ == "__main__":
    extract()
