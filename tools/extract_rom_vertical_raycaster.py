#!/usr/bin/env python3
"""Extract and verify ROM vertical (±Y) tilemap raycaster, collision sweep & entity return dispatcher (0x0091F6..0x00938E)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_vertical_raycaster_m15_400.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x0091F6: clr.l $72(a6)
    assert u16(rom, 0x0091F6) == 0x42AE
    assert u16(rom, 0x0091F6 + 2) == 0x0072

    # 0x00920C: +Y raycaster (code 0x12)
    assert u16(rom, 0x00920C) == 0x1D7C          # move.b #$12, $41(a6)
    assert u16(rom, 0x00920C + 2) == 0x0012
    assert u16(rom, 0x00920C + 4) == 0x0041
    assert u16(rom, 0x009214) == 0x3D40          # move.w d0, $E(a6)
    assert u16(rom, 0x00921E) == 0x3D40          # move.w d0, $76(a6)

    # 0x0092B0: -Y raycaster (code 0x10)
    assert u16(rom, 0x0092B0) == 0x1D7C          # move.b #$10, $41(a6)
    assert u16(rom, 0x0092B0 + 2) == 0x0010
    assert u16(rom, 0x0092B0 + 4) == 0x0041
    assert u16(rom, 0x0092B8) == 0x3D40          # move.w d0, $E(a6)
    assert u16(rom, 0x0092C2) == 0x3D40          # move.w d0, $76(a6)

    # 0x00934E: write final Y pos
    assert u16(rom, 0x00934E) == 0x3D42          # move.w d2, $C(a6)
    assert u16(rom, 0x00934E + 2) == 0x000C
    assert u16(rom, 0x00937E) == 0x42AE          # clr.l $76(a6)
    assert u16(rom, 0x00937E + 2) == 0x0076

    # 0x009382: restore d7 and jump to return vector in $FF193C
    assert u16(rom, 0x009382) == 0x4C9F          # movem.l (a7)+, d7
    assert u16(rom, 0x009382 + 2) == 0x0080
    assert u16(rom, 0x009386) == 0x2079          # movea.l $FF193C.l, a0
    assert u32(rom, 0x009386 + 2) == 0x00FF193C
    assert u16(rom, 0x00938C) == 0x4ED0          # jmp (a0)

    # 16-byte terrain surface type LUT at 0x0096E8
    lut = list(rom[0x0096E8:0x0096E8 + 16])
    expected_lut = [0xFF, 0x00, 0x02, 0x01, 0x04, 0xFF, 0x03, 0xFF, 0x06, 0x07, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0xFF]
    assert lut == expected_lut

    return {
        "milestone": "M15.400",
        "title": "ROM Vertical (±Y) Tilemap Raycaster, Collision Sweep & Entity Return Dispatcher",
        "routines": {
            "pos_y_raycaster": {
                "start": "0x00920C",
                "end": "0x0092B0",
                "bytes": 164,
                "direction_code": "0x12",
                "boundary_clamp": "0x009D92",
                "solid_entity_sweep": "0x009FA0",
                "tile_solid_probe": "0x0095AA",
                "surface_height_probe": "0x009BF2",
                "surface_height_store": "0x009AD6",
                "terrain_lut_addr": "0x0096E8",
            },
            "neg_y_raycaster": {
                "start": "0x0092B0",
                "end": "0x00934E",
                "bytes": 158,
                "direction_code": "0x10",
                "boundary_clamp": "0x009DB4",
                "solid_entity_sweep": "0x00A09A",
                "tile_solid_probe": "0x0094D2",
                "surface_height_probe": "0x009BF2",
                "surface_height_store": "0x009AD6",
                "terrain_lut_addr": "0x0096E8",
            },
            "final_y_write_and_cleanup": {
                "start": "0x00934E",
                "end": "0x009382",
                "bytes": 52,
                "y_pos_offset": "0x0C",
                "y_vel_offset": "0x76",
            },
            "entity_return_dispatcher": {
                "start": "0x009382",
                "end": "0x00938E",
                "bytes": 12,
                "return_vector_ptr": "0x00FF193C",
                "instruction": "jmp (a0)",
            },
            "terrain_surface_lut": {
                "start": "0x0096E8",
                "end": "0x0096F8",
                "bytes": 16,
                "table": [f"0x{b:02X}" for b in lut],
            },
        },
    }


def main() -> None:
    rom = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
