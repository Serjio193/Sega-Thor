"""Extract World Map Screen & Overworld/Dungeon Pin Coordinate Tables (0x004950..0x004C27).

Milestone: M15.287.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x004950..0x004A00: World Map entry, 3 compressed GFX chunks (0x15BAC2->0x9500, 0x15C238->0xA180, 0x15CA9C->0xAE00)
  - 0x004B4E..0x004BAC: Ali map pin coordinate math ($FF0E40 dungeon vs $FF0E41 area + (X>>6, Y>>6) + (160, 120))
  - 0x004BAE..0x004BE7: 29-entry overworld area (s8 dx, s8 dy) table
  - 0x004BE8..0x004C27: 16+ dungeon map pin (s8 dx, s8 dy) table (plus $FF0E40 == 0x0F01 -> +6)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_world_map_m15_287.json")

OVERWORLD_TABLE_ADDR = 0x004BAE
OVERWORLD_COUNT = 29  # 0x004BAE..0x004BE7
DUNGEON_TABLE_ADDR = 0x004BE8
DUNGEON_COUNT = 16


def extract_world_map(rom_path: Path) -> dict:
    """Extract World Map graphics chunks and overworld/dungeon pin tables."""
    rom = rom_path.read_bytes()

    overworld_areas = []
    for area_id in range(OVERWORLD_COUNT):
        dx, dy = struct.unpack_from(">bb", rom, OVERWORLD_TABLE_ADDR + area_id * 2)
        overworld_areas.append({
            "area_id": area_id,
            "area_hex": f"0x{area_id:02X}",
            "dx": dx,
            "dy": dy,
        })

    dungeon_pins = []
    for d_id in range(DUNGEON_COUNT):
        dx, dy = struct.unpack_from(">bb", rom, DUNGEON_TABLE_ADDR + d_id * 2)
        dungeon_pins.append({
            "dungeon_id": d_id,
            "dungeon_hex": f"0x{d_id:02X}",
            "dx": dx,
            "dy": dy,
            "screen_x": 0xA0 + dx,
            "screen_y": 0x78 + dy,
        })

    return {
        "milestone": "M15.287",
        "rom_path": str(rom_path),
        "world_map": {
            "entry_addr": "0x004950",
            "coord_calc_addr": "0x004B4E",
            "close_sfx": "0xA00347",
            "screen_center_x": 0xA0,
            "screen_center_y": 0x78,
            "overworld_shift_bits": 6,
            "special_dungeon_0f01_offset": 6,
            "gfx_chunks": [
                {"rom_addr": "0x15BAC2", "vram_addr": "0x9500", "dma_words": 0x640},
                {"rom_addr": "0x15C238", "vram_addr": "0xA180", "dma_words": 0x640},
                {"rom_addr": "0x15CA9C", "vram_addr": "0xAE00", "dma_words": 0x640},
            ],
        },
        "overworld_areas": overworld_areas,
        "dungeon_pins": dungeon_pins,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_world_map(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['overworld_areas'])} overworld areas and {len(data['dungeon_pins'])} dungeon pins to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
