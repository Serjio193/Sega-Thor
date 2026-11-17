#!/usr/bin/env python3
"""
Extracts ROM evidence for M15.419: Entity Special Dynamic FX Halo, Swirl &
SAT Piece Compositor Subsystem (Part 2: Bit 2 Halo, Bit 1 Swirl, FX Blitter & Raw Blitter)
ROM Range: 0x00B56E..0x00B79A (556 bytes).
"""

import hashlib
import json
import os
import struct

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_JSON = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_dynamic_fx_renderer_p2_m15_419.json")

START_ADDR = 0x00B56E
END_ADDR = 0x00B79A
BYTE_COUNT = END_ADDR - START_ADDR  # 556 bytes

def u8(data, off):
    return data[off]

def u16(data, off):
    return struct.unpack_from(">H", data, off)[0]

def s8(data, off):
    return struct.unpack_from(">b", data, off)[0]

def u32(data, off):
    return struct.unpack_from(">I", data, off)[0]

def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    assert len(rom) >= END_ADDR, "ROM too small"

    block = rom[START_ADDR:END_ADDR]
    sha256 = hashlib.sha256(block).hexdigest()

    # Verify Bit 2 Halo Emitter (0x00B56E..0x00B5CE)
    # 0x00B56E: lea $9a(a6), a0 -> 0x41EE, 0x009A
    assert u16(rom, 0x00B56E) == 0x41EE
    assert u16(rom, 0x00B570) == 0x009A
    # Exit RTS: 0x00B5C6 -> 0x4E75
    assert u16(rom, 0x00B5C6) == 0x4E75
    # Bit clear at 0x00B5C8: bclr.b #2, $39(a6) -> 0x08AE, 0x0002, 0x0039
    assert u16(rom, 0x00B5C8) == 0x08AE
    assert u16(rom, 0x00B5CA) == 0x0002
    assert u16(rom, 0x00B5CC) == 0x0039
    assert u16(rom, 0x00B5CE) == 0x4E75

    # Extract Bit 2 Halo Table at 0x00B5D0 (21 entries x 3 bytes = 63 bytes)
    halo_table = []
    for i in range(21):
        off = 0x00B5D0 + i * 3
        halo_table.append({
            "step": i,
            "frame_flag": u8(rom, off),
            "dx": s8(rom, off + 1),
            "dy": s8(rom, off + 2)
        })

    # Verify Bit 1 Ground Energy Swirl Emitter (0x00B610..0x00B65E)
    # 0x00B610: tst.l $10(a6) -> 0x4AAE, 0x0010
    assert u16(rom, 0x00B610) == 0x4AAE
    assert u16(rom, 0x00B612) == 0x0010
    # Exit RTS: 0x00B65E -> 0x4E75
    assert u16(rom, 0x00B65E) == 0x4E75

    # Extract 9-byte Swirl Frame Sequence Table at 0x00B660
    swirl_frames = [u8(rom, 0x00B660 + i) for i in range(9)]

    # Verify Universal FX Sprite Piece Blitter (0x00B6A6..0x00B72E)
    # 0x00B6AA: movea.l $3f326.l, a0 -> 0x2079, 0x0003, 0xF326
    assert u16(rom, 0x00B6AA) == 0x2079
    assert u32(rom, 0x00B6AC) == 0x0003F326
    # Base VRAM tile 0x055E at 0x00B6B8: ori.w #$55e, d3 -> 0x0043, 0x055E
    assert u16(rom, 0x00B6B8) == 0x0043
    assert u16(rom, 0x00B6BA) == 0x055E
    # Exit RTS: 0x00B72E -> 0x4E75
    assert u16(rom, 0x00B72E) == 0x4E75

    # Verify Static Raw Sprite Piece Blitter (0x00B730..0x00B798)
    # 0x00B734: addi.w #$80, d1 -> 0x0641, 0x0080
    assert u16(rom, 0x00B734) == 0x0641
    assert u16(rom, 0x00B736) == 0x0080
    # 0x00B738: addi.w #$80, d4 -> 0x0644, 0x0080
    assert u16(rom, 0x00B738) == 0x0644
    assert u16(rom, 0x00B73A) == 0x0080
    # Exit RTS: 0x00B798 -> 0x4E75
    assert u16(rom, 0x00B798) == 0x4E75

    evidence = {
        "subsystem": "ROM Entity Special Dynamic FX Halo, Swirl & SAT Piece Compositor Subsystem",
        "milestone": "M15.419",
        "rom_range": {
            "start": f"0x{START_ADDR:06X}",
            "end": f"0x{END_ADDR:06X}",
            "size_bytes": BYTE_COUNT,
            "sha256": sha256
        },
        "embedded_tables": {
            "halo_table": {
                "address": "0x00B5D0",
                "entries": halo_table
            },
            "swirl_sequence": {
                "address": "0x00B660",
                "frames": swirl_frames
            }
        },
        "fx_blitter": {
            "entry": "0x00B6A6",
            "frame_table": "0x0003F326",
            "vram_tile_base": "0x055E",
            "viewport_clipping": {
                "y": ["0x60", "0x160"],
                "x": ["0x60", "0x1C0"]
            },
            "max_sprites": 80
        },
        "raw_piece_blitter": {
            "entry": "0x00B730",
            "screen_bias": "0x0080",
            "viewport_clipping": {
                "y": ["0x60", "0x160"],
                "x": ["0x60", "0x1C0"]
            }
        }
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted M15.419 evidence: {BYTE_COUNT} bytes, SHA256: {sha256}")

if __name__ == "__main__":
    extract()
