#!/usr/bin/env python3
"""Extract and verify ROM flying/projectile dual-axis (±X, ±Y) tilemap raycaster & point collision dispatcher (0x009708..0x0098EC)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_flying_raycaster_m15_403.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x009708: suba.l a3, a3
    assert u16(rom, 0x009708) == 0x97CB

    # 0x00971A: +X flying direction code 0x11
    assert u16(rom, 0x00971A) == 0x1D7C
    assert u16(rom, 0x00971A + 2) == 0x0011
    assert u16(rom, 0x00971A + 4) == 0x0040

    # 0x009780: -X flying direction code 0x13
    assert u16(rom, 0x009780) == 0x1D7C
    assert u16(rom, 0x009780 + 2) == 0x0013
    assert u16(rom, 0x009780 + 4) == 0x0040

    # 0x0097DE: write final X pos
    assert u16(rom, 0x0097DE) == 0x3D41
    assert u16(rom, 0x0097DE + 2) == 0x0008
    assert u16(rom, 0x0097E2) == 0x42AE          # clr.l $72(a6)
    assert u16(rom, 0x0097E2 + 2) == 0x0072

    # 0x0097F6: +Y flying direction code 0x12
    assert u16(rom, 0x0097F6) == 0x1D7C
    assert u16(rom, 0x0097F6 + 2) == 0x0012
    assert u16(rom, 0x0097F6 + 4) == 0x0041

    # 0x00985C: -Y flying direction code 0x10
    assert u16(rom, 0x00985C) == 0x1D7C
    assert u16(rom, 0x00985C + 2) == 0x0010
    assert u16(rom, 0x00985C + 4) == 0x0041

    # 0x0098BA: write final Y pos
    assert u16(rom, 0x0098BA) == 0x3D42
    assert u16(rom, 0x0098BA + 2) == 0x000C
    assert u16(rom, 0x0098BE) == 0x42AE          # clr.l $76(a6)
    assert u16(rom, 0x0098BE + 2) == 0x0076

    # 0x0098E4: store surface height
    assert u16(rom, 0x0098E4) == 0x3D44
    assert u16(rom, 0x0098E4 + 2) == 0x0014

    # 0x0098E8: bra.w $9382 (to entity return dispatcher)
    assert u16(rom, 0x0098E8) == 0x6000
    disp = s16(rom, 0x0098E8 + 2)
    assert (0x0098E8 + 2 + disp) & 0xFFFFFF == 0x009382

    return {
        "milestone": "M15.403",
        "title": "ROM Flying/Projectile Dual-Axis (±X, ±Y) Tilemap Raycaster & Point Collision Dispatcher",
        "routines": {
            "flying_horizontal_raycast": {
                "start": "0x009708",
                "end": "0x0097E6",
                "bytes": 222,
                "pos_x_dir": "0x11",
                "neg_x_dir": "0x13",
                "boundary_clamp_pos": "0x009D4E",
                "solid_entity_sweep_pos": "0x009DD6",
                "boundary_clamp_neg": "0x009D70",
                "solid_entity_sweep_neg": "0x009EBA",
                "point_wall_probe": "0x0098EC",
                "wall_hit_flag_bit": 6,
                "collision_flags_offset": "0x40",
            },
            "flying_vertical_raycast": {
                "start": "0x0097E6",
                "end": "0x0098C2",
                "bytes": 220,
                "pos_y_dir": "0x12",
                "neg_y_dir": "0x10",
                "boundary_clamp_pos": "0x009D92",
                "solid_entity_sweep_pos": "0x009FA0",
                "boundary_clamp_neg": "0x009DB4",
                "solid_entity_sweep_neg": "0x00A09A",
                "point_wall_probe": "0x0098EC",
                "wall_hit_flag_bit": 6,
                "collision_flags_offset": "0x41",
            },
            "flying_surface_sample_and_exit": {
                "start": "0x0098C2",
                "end": "0x0098EC",
                "bytes": 42,
                "surface_height_probe": "0x009BF2",
                "surface_height_store": "0x009AD6",
                "exit_target": "0x009382",
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
