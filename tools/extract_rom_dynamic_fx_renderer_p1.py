#!/usr/bin/env python3
"""
Extracts ROM evidence for M15.418: Entity Special Dynamic FX, Aura, Wave & Attachment
SAT Renderer Subsystem (Part 1: Dispatcher, Bit 0 Aura, Bit 3 Wave Ring, Bit 4 Ripple)
ROM Range: 0x00B34C..0x00B56E (546 bytes).
"""

import hashlib
import json
import os
import struct

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_JSON = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_dynamic_fx_renderer_p1_m15_418.json")

START_ADDR = 0x00B34C
END_ADDR = 0x00B56E
BYTE_COUNT = END_ADDR - START_ADDR  # 546 bytes

def u8(data, off):
    return data[off]

def u16(data, off):
    return struct.unpack_from(">H", data, off)[0]

def s8(data, off):
    return struct.unpack_from(">b", data, off)[0]

def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    assert len(rom) >= END_ADDR, "ROM too small"

    block = rom[START_ADDR:END_ADDR]
    sha256 = hashlib.sha256(block).hexdigest()

    # Verify Master Dynamic FX Dispatcher entry (0x00B34C)
    # 0x00B34C: tst.b $ff1927.l -> 0x4A39, 0x00FF, 0x1927
    assert u16(rom, 0x00B34C) == 0x4A39
    assert u16(rom, 0x00B34E) == 0x00FF
    assert u16(rom, 0x00B350) == 0x1927
    # 0x00B356: move.b $39(a6), d5 -> 0x1A2E, 0x0039
    assert u16(rom, 0x00B356) == 0x1A2E
    assert u16(rom, 0x00B358) == 0x0039
    # Dispatcher exit RTS: 0x00B382 -> 0x4E75
    assert u16(rom, 0x00B382) == 0x4E75

    # Verify Bit 0 Aura Emitter (0x00B3D2..0x00B42A)
    # 0x00B3D2: lea $9b(a6), a0 -> 0x41EE, 0x009B
    assert u16(rom, 0x00B3D2) == 0x41EE
    assert u16(rom, 0x00B3D4) == 0x009B
    # Exit RTS: 0x00B42A -> 0x4E75
    assert u16(rom, 0x00B42A) == 0x4E75

    # Extract 16-entry Bit 0 Offset Table at 0x00B42C (3 bytes per entry = 48 bytes)
    bit0_table = []
    for i in range(16):
        off = 0x00B42C + i * 3
        bit0_table.append({
            "step": i,
            "flag": u8(rom, off),
            "dx": s8(rom, off + 1),
            "dy": s8(rom, off + 2)
        })

    # Verify Bit 3 Wave Ring Emitter (0x00B45C..0x00B4C6)
    # 0x00B45C: lea $98(a6), a0 -> 0x41EE, 0x0098
    assert u16(rom, 0x00B45C) == 0x41EE
    assert u16(rom, 0x00B45E) == 0x0098
    # Exit RTS: 0x00B4C6 -> 0x4E75
    assert u16(rom, 0x00B4C6) == 0x4E75

    # Extract Bit 3 Tables: Small at 0x00B4C8 (8 entries x 3B = 24B), Large at 0x00B4E0 (10 entries x 3B = 30B)
    bit3_small_table = []
    for i in range(8):
        off = 0x00B4C8 + i * 3
        bit3_small_table.append({
            "step": i,
            "b0": u8(rom, off),
            "b1": u8(rom, off + 1),
            "b2": s8(rom, off + 2)
        })

    bit3_large_table = []
    for i in range(10):
        off = 0x00B4E0 + i * 3
        bit3_large_table.append({
            "step": i,
            "b0": u8(rom, off),
            "b1": u8(rom, off + 1),
            "b2": s8(rom, off + 2)
        })

    # Verify Bit 4 Ground Ripple Emitter (0x00B4FE..0x00B554)
    # 0x00B4FE: lea $98(a6), a0 -> 0x41EE, 0x0098
    assert u16(rom, 0x00B4FE) == 0x41EE
    assert u16(rom, 0x00B500) == 0x0098
    # Exit RTS: 0x00B54C -> 0x4E75
    assert u16(rom, 0x00B54C) == 0x4E75
    # Bit 4 clear exit RTS: 0x00B554 -> 0x4E75
    assert u16(rom, 0x00B554) == 0x4E75

    # Extract Bit 4 Offset Table at 0x00B556 (8 entries x 3B = 24B)
    bit4_table = []
    for i in range(8):
        off = 0x00B556 + i * 3
        bit4_table.append({
            "step": i,
            "b0": u8(rom, off),
            "b1": u8(rom, off + 1),
            "b2": u8(rom, off + 2)
        })

    evidence = {
        "subsystem": "ROM Entity Special Dynamic FX, Aura, Wave & Attachment SAT Renderer (Part 1)",
        "milestone": "M15.418",
        "rom_range": {
            "start": f"0x{START_ADDR:06X}",
            "end": f"0x{END_ADDR:06X}",
            "size_bytes": BYTE_COUNT,
            "sha256": sha256
        },
        "embedded_tables": {
            "bit0_aura_table": {
                "address": "0x00B42C",
                "entries": bit0_table
            },
            "bit3_small_wave_table": {
                "address": "0x00B4C8",
                "entries": bit3_small_table
            },
            "bit3_large_wave_table": {
                "address": "0x00B4E0",
                "entries": bit3_large_table
            },
            "bit4_ripple_table": {
                "address": "0x00B556",
                "entries": bit4_table
            }
        },
        "mode_flags_39": {
            "bit0": "Aura / spark emitter (0x00B3D2)",
            "bit1": "Energy swirl emitter (0x00B610)",
            "bit2": "Pulsing halo emitter (0x00B56E)",
            "bit3": "Shadow / wave ring emitter (0x00B45C)",
            "bit4": "Ground ripple / flame attachment emitter (0x00B4FE)"
        }
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted M15.418 evidence: {BYTE_COUNT} bytes, SHA256: {sha256}")

if __name__ == "__main__":
    extract()
