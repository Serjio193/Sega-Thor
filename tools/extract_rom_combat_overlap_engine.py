#!/usr/bin/env python3
"""
Extracts ROM evidence for M15.420: Entity 3D AABB Combat Overlap &
Multi-Pool Collision Iterator Engine
ROM Range: 0x00B79A..0x00BAAC (786 bytes).
"""

import hashlib
import json
import os
import struct

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_JSON = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_combat_overlap_engine_m15_420.json")

START_ADDR = 0x00B79A
END_ADDR = 0x00BAAC
BYTE_COUNT = END_ADDR - START_ADDR  # 786 bytes

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

    # Verify Directional Hitbox entry (0x00B79A)
    # 0x00B79A: moveq #-1, d0 -> 0x70FF
    assert u16(rom, 0x00B79A) == 0x70FF
    # 0x00B79C: move.w $42(a6), d1 -> 0x322E, 0x0042
    assert u16(rom, 0x00B79C) == 0x322E
    assert u16(rom, 0x00B79E) == 0x0042
    # 0x00B7D6: andi.b #$c, d0 -> 0x0200, 0x000C
    assert u16(rom, 0x00B7D6) == 0x0200
    assert u16(rom, 0x00B7D8) == 0x000C
    # 0x00B7FE: addi.b #1, $82(a0) -> 0x0628, 0x0001, 0x0082
    assert u16(rom, 0x00B7FE) == 0x0628
    assert u16(rom, 0x00B800) == 0x0001
    assert u16(rom, 0x00B802) == 0x0082
    # Exit RTS: 0x00B850 -> 0x4E75
    assert u16(rom, 0x00B850) == 0x4E75

    # Verify Group 2 AABB Scanner entry (0x00B856)
    # 0x00B872: lea $ff19e8.l, a0 -> 0x41F9, 0x00FF, 0x19E8
    assert u16(rom, 0x00B872) == 0x41F9
    assert u32(rom, 0x00B874) == 0x00FF19E8
    # 21 slots: moveq #$14, d7 -> 0x7E14
    assert u16(rom, 0x00B878) == 0x7E14
    # Context save buffer at 0x00B900: lea $ff196a.l, a0
    assert u16(rom, 0x00B900) == 0x41F9
    assert u32(rom, 0x00B902) == 0x00FF196A
    # Hit exit RTS: 0x00B910 -> 0x4E75
    assert u16(rom, 0x00B910) == 0x4E75

    # Verify Group 4 Scanner entry (0x00B9EC)
    # 0x00BA08: lea $ff2abc.l, a0 -> 0x41F9, 0x00FF, 0x2ABC
    assert u16(rom, 0x00BA08) == 0x41F9
    assert u32(rom, 0x00BA0A) == 0x00FF2ABC
    # 8 slots: moveq #7, d7 -> 0x7E07
    assert u16(rom, 0x00BA0E) == 0x7E07
    # Exit RTS: 0x00BA9A -> 0x4E75
    assert u16(rom, 0x00BA9A) == 0x4E75

    evidence = {
        "subsystem": "ROM Entity 3D AABB Combat Overlap & Multi-Pool Collision Iterator Engine",
        "milestone": "M15.420",
        "rom_range": {
            "start": f"0x{START_ADDR:06X}",
            "end": f"0x{END_ADDR:06X}",
            "size_bytes": BYTE_COUNT,
            "sha256": sha256
        },
        "routines": {
            "directional_hitbox_scanner": {
                "start": "0x00B79A",
                "exit": "0x00B850",
                "facing_modes": {
                    "0_down": "clear d2 (forward Y)",
                    "1_up": "clear d3 (backward Y)",
                    "2_right": "clear d4 (rightward X)",
                    "3_left": "clear d1 (leftward X)"
                },
                "target_filters": {
                    "invulnerable_mask": "0x0C on +0x38(a0)",
                    "status_bit": "bit 0 on +0x3A(a0)",
                    "hit_cooldown": "+0x82(a0)",
                    "self_damage_check": "+0x7E(a6) vs a0",
                    "hit_register": "addi.b #1, +0x82(a0)"
                }
            },
            "group2_aabb_scanner": {
                "start": "0x00B856",
                "hit_exit": "0x00B910",
                "resume_find_next": "0x00B912",
                "pool_address": "0x00FF19E8",
                "slot_count": 21,
                "stride": 188,
                "context_buffer": "0x00FF194A..0x00FF196A"
            },
            "group2_3d_interval_scanner": {
                "start": "0x00B922",
                "hit_exit": "0x00B9DC",
                "resume_find_next": "0x00B9DE",
                "pool_address": "0x00FF19E8",
                "z_interval": "[d5, d6]"
            },
            "group4_aabb_scanner": {
                "start": "0x00B9EC",
                "hit_exit": "0x00BA9A",
                "resume_find_next": "0x00BA9C",
                "pool_address": "0x00FF2ABC",
                "slot_count": 8,
                "stride": 90
            }
        }
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted M15.420 evidence: {BYTE_COUNT} bytes, SHA256: {sha256}")

if __name__ == "__main__":
    extract()
