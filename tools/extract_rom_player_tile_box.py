#!/usr/bin/env python3
"""Extract and verify ROM player tile-box containment & overlap checks (0x00C17A..0x00C220)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_player_tile_box_m15_323.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00C184) == 0x00070007  # +7px inclusive tile max bias
    assert u32(rom, 0x00C192) == 0x00FF19E8  # Ali entity base
    assert u16(rom, 0x00C198) == 0x0046      # Half-extent offset 0x46(a0)
    assert u16(rom, 0x00C19C) == 0x0008      # X center offset 0x08(a0)
    assert u16(rom, 0x00C1AE) == 0x000C      # Y center offset 0x0C(a0)
    assert u32(rom, 0x00C1D8) == 0x00070007  # +7px inclusive tile max bias
    assert u32(rom, 0x00C1E6) == 0x00FF19E8  # Ali entity base

    return {
        "milestone": "M15.323",
        "title": "ROM Player Tile-Box Containment & Overlap Checks",
        "routines": {
            "player_inside_tile_box": {"start": "0x00C17A", "end": "0x00C1CC"},
            "player_overlaps_tile_box": {"start": "0x00C1CE", "end": "0x00C220"},
        },
        "constants": {
            "tile_shift": 3,
            "tile_max_bias_packed": "0x00070007",
            "player_entity_ram": "0x00FF19E8",
            "entity_x_offset": "0x08",
            "entity_y_offset": "0x0C",
            "entity_radius_offset": "0x46",
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
