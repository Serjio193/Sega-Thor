"""Extract Floor Elevation Probe, Terrain LUT & Room Boundary Clamp (0x009D00..0x009DD5).

Milestone: M15.304.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x0096E8..0x0096F7: 16-byte terrain nibble -> surface effect LUT ($70(a6))
  - 0x009D00..0x009D4D: Surface probe (selects $FF1766 vs $FF1770 by $FF16F0 bit 7,
                        stores $6F surface flags, $6E tile byte, $70 LUT, $14 floor Z)
  - 0x009D4E..0x009DD5: 4-side room boundary clamping (max_dim = tiles * 8 - 1 - radius,
                        sets $40/$41 |= 0x48 on clamp)
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_terrain_probe_and_clamp_m15_304.json")


def clamp_entity_to_room_bounds(
    x: int,
    y: int,
    dx: int,
    dy: int,
    radius: int,
    room_tiles_w: int,
    room_tiles_h: int,
) -> dict:
    """Apply exact ROM 0x009D4E..0x009DD4 4-side room boundary clamp."""
    col_x = 0
    col_y = 0

    if dx > 0:
        limit = (room_tiles_w * 8 - 1) - (x + radius)
        if dx > limit:
            dx = max(0, limit)
            if dx == 0:
                col_x |= 0x48
    elif dx < 0:
        limit = x - radius
        if limit < 0:
            dx = 0
            col_x |= 0x48
        elif -dx > limit:
            dx = -limit
            if dx == 0:
                col_x |= 0x48

    if dy > 0:
        limit = (room_tiles_h * 8 - 1) - (y + radius)
        if dy > limit:
            dy = max(0, limit)
            if dy == 0:
                col_y |= 0x48
    elif dy < 0:
        limit = y - radius
        if limit < 0:
            dy = 0
            col_y |= 0x48
        elif -dy > limit:
            dy = -limit
            if dy == 0:
                col_y |= 0x48

    return {"dx": dx, "dy": dy, "col_x_flags": col_x, "col_y_flags": col_y}


def extract_terrain_probe_and_clamp(rom_path: Path) -> dict:
    """Extract 16-byte terrain LUT and room boundary clamp constants."""
    rom = rom_path.read_bytes()
    lut = list(rom[0x0096E8:0x0096F8])
    return {
        "milestone": "M15.304",
        "rom_path": str(rom_path),
        "terrain_probe": {
            "entry_addr": "0x009D00",
            "primary_layer_desc": "0xFF1766",
            "alt_layer_desc": "0xFF1770",
            "alt_layer_flag": "0xFF16F0 bit 7",
            "lut_addr": "0x0096E8",
            "terrain_lut_16": lut,
        },
        "room_clamp": {
            "right_addr": "0x009D4E",
            "left_addr": "0x009D70",
            "bottom_addr": "0x009D92",
            "top_addr": "0x009DB4",
            "tile_pixel_shift": 3,
            "edge_wall_flag": "0x48",
        },
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_terrain_probe_and_clamp(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote 16-byte terrain LUT & room clamp spec to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
