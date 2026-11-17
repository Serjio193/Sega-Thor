"""Extract Full-Screen Wrap-Around VDP Plane Blitter (0x00D800..0x00D94E).

Milestone: M15.311.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x00D820: Viewport blit size 41x29 tiles (dbra 0x28 x 0x1C)
  - 0x00D830..0x00D846: Full-plane 64x32 blit (dbra 0x3F x 0x1F) when $27 bit 1 set or (tiles_h==32 && tiles_w<=64)
  - 0x00D8C4..0x00D946: Dual wrap-around: source modulo (tiles_w, tiles_h) & VDP nametable modulo (64, 32)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_vdp_plane_blitter_m15_311.json")


def select_blit_dimensions(tiles_w: int, tiles_h: int, force_full_bit1: bool) -> tuple[int, int]:
    """Select (cols, rows) matching ROM 0x00D820..0x00D846."""
    if force_full_bit1 or (tiles_h == 0x20 and tiles_w <= 0x40):
        return (64, 32)
    return (41, 29)


def extract_vdp_plane_blitter(rom_path: Path) -> dict:
    """Extract VDP plane blitter dimensions and wrap constants."""
    rom = rom_path.read_bytes()
    vp_packed = struct.unpack_from(">I", rom, 0x00D822)[0]
    full_packed = struct.unpack_from(">I", rom, 0x00D848)[0]

    return {
        "milestone": "M15.311",
        "rom_path": str(rom_path),
        "vdp_plane_blitter": {
            "entry_addr": "0x00D800",
            "viewport_cols": ((vp_packed >> 16) & 0xFFFF) + 1,
            "viewport_rows": (vp_packed & 0xFFFF) + 1,
            "full_plane_cols": ((full_packed >> 16) & 0xFFFF) + 1,
            "full_plane_rows": (full_packed & 0xFFFF) + 1,
            "vdp_nametable_cols": 64,
            "vdp_nametable_rows": 32,
            "vdp_row_bytes": 0x80,
            "vdp_plane_bytes": 0x1000,
        },
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_vdp_plane_blitter(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote VDP plane blitter spec (vp={data['vdp_plane_blitter']['viewport_cols']}x{data['vdp_plane_blitter']['viewport_rows']}) to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
