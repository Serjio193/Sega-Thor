#!/usr/bin/env python3
"""
extract_rom_hit_spark_generator.py — ROM Combat Hit Spark Particle Generator
& 3D Overlap Centerpoint Calculator (0x00C012..0x00C0BE, 172 bytes)
"""

import hashlib
import json
import os
import struct

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
START_ADDR = 0x00C012
END_ADDR = 0x00C0BE
SIZE = END_ADDR - START_ADDR  # 172 bytes

BUFFER_ADDR = 0x00FF19C8
SLOT_COUNT = 4
SLOT_SIZE = 8
LIFETIME_FRAMES = 5
PARTICLE_TYPE = 2

def compute_spark_coords(hitbox: dict, target: dict) -> tuple[int, int, int]:
    """
    Computes (spark_x, spark_y_projected, mid_z)
    mid_z = clamped overlap midpoint of Z
    mid_x = clamped overlap midpoint of X
    mid_y = clamped overlap midpoint of Y
    spark_x = mid_x
    spark_y = mid_y - mid_z
    """
    # Z span
    tz_min = target["z"] + target["elev"]
    tz_max = tz_min + target["height"]
    z0 = max(hitbox["z_min"], tz_min)
    z1 = min(hitbox["z_max"], tz_max)
    mid_z = z0 + ((z1 - z0) // 2)

    # X span
    tx_min = target["x"] - target["rad_x"]
    tx_max = target["x"] + target["rad_x"]
    x0 = max(hitbox["x_min"], tx_min)
    x1 = min(hitbox["x_max"], tx_max)
    mid_x = x0 + ((x1 - x0) // 2)

    # Y span
    ty_min = target["y"] + target["elev"] - target["rad_y"]
    ty_max = target["y"] + target["elev"] + target["rad_y"]
    y0 = max(hitbox["y_min"], ty_min)
    y1 = min(hitbox["y_max"], ty_max)
    mid_y = y0 + ((y1 - y0) // 2)

    spark_x = mid_x
    spark_y = mid_y - mid_z
    return spark_x, spark_y, mid_z

def extract():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    routine_bytes = rom[START_ADDR:END_ADDR]
    assert len(routine_bytes) == SIZE, f"Expected {SIZE} bytes, got {len(routine_bytes)}"

    # Check key opcodes
    assert struct.unpack_from(">H", rom, 0x00C012)[0] == 0x45F9, "lea.l $ff19c8.l, a2"
    assert struct.unpack_from(">I", rom, 0x00C014)[0] == BUFFER_ADDR
    assert struct.unpack_from(">H", rom, 0x00C036)[0] == 0x4E75, "rts (buffer full)"
    assert struct.unpack_from(">H", rom, 0x00C03C)[0] == 0x34BC, "move.w #2, (a2)"
    assert struct.unpack_from(">H", rom, 0x00C040)[0] == 0x357C, "move.w #5, $6(a2)"
    assert struct.unpack_from(">H", rom, 0x00C0B2)[0] == 0x9449, "sub.w a1, d2"
    assert struct.unpack_from(">H", rom, 0x00C0BC)[0] == 0x4E75, "rts (exit)"

    data = {
        "routine": "Combat Hit Spark Particle Generator & 3D Overlap Centerpoint Calculator",
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "size_bytes": SIZE,
        "buffer_address": f"0x{BUFFER_ADDR:08X}",
        "slot_count": SLOT_COUNT,
        "slot_size_bytes": SLOT_SIZE,
        "particle_type": PARTICLE_TYPE,
        "lifetime_frames": LIFETIME_FRAMES,
        "sha256": hashlib.sha256(routine_bytes).hexdigest(),
    }

    out_path = "data/thor-evidence/rom_hit_spark_generator_m15_424.json"
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    print(f"Extracted {SIZE} bytes, saved evidence to {out_path}")

if __name__ == "__main__":
    extract()
