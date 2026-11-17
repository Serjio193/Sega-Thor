"""Extract Room Tilemap & Collision Layer Descriptor Builder (0x00D500..0x00D7BF).

Milestone: M15.309.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x00D50A..0x00D53C: Wait async fade ($FF164D bit 4), DMA 0x2000 words tileset to VRAM 0x0000
  - 0x00D542..0x00D559: Decompress room data via 0x003820 to $FF3FA8 (4B header + layers)
  - 0x00D560..0x00D620: Build Plane A ($FF1716, 0xC000, $FF1766 col) & Plane B ($FF173E, 0xE000, $FF1770 col)
  - 0x00D7B0..0x00D7BF: Compute byte-stride log2 shift d1 = ctz(tiles_w * 2)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_room_layer_loader_m15_309.json")


def compute_row_shift_and_layout(header4: bytes) -> dict:
    """Decode 4-byte decompressed room header into Plane A/B & collision offsets."""
    b0, b1, b2, b3 = header4[0], header4[1], header4[2], header4[3]
    a_w = (b0 & 0x7F) << 5
    a_h = b1 << 5
    a_shift = (a_w * 2).bit_length() - 1 if a_w > 0 else 0
    a_tilemap_bytes = a_h << a_shift
    a_has_col = (b0 & 0x80) != 0
    a_col_bytes = (a_tilemap_bytes >> 1) if a_has_col else 0

    b_w = (b2 & 0x7F) << 5
    b_h = b3 << 5
    b_shift = (b_w * 2).bit_length() - 1 if b_w > 0 else 0
    b_tilemap_bytes = b_h << b_shift

    return {
        "plane_a": {
            "tiles_w": a_w,
            "tiles_h": a_h,
            "row_shift": a_shift,
            "tilemap_offset": 4,
            "tilemap_bytes": a_tilemap_bytes,
            "has_collision": a_has_col,
            "collision_offset": 4 + a_tilemap_bytes,
            "collision_bytes": a_col_bytes,
        },
        "plane_b": {
            "tiles_w": b_w,
            "tiles_h": b_h,
            "row_shift": b_shift,
            "tilemap_offset": 4 + a_tilemap_bytes + a_col_bytes,
            "tilemap_bytes": b_tilemap_bytes,
        },
    }


def extract_room_layer_loader(rom_path: Path) -> dict:
    """Extract room tileset DMA size, VDP plane bases, and descriptor RAM addresses."""
    rom = rom_path.read_bytes()
    return {
        "milestone": "M15.309",
        "rom_path": str(rom_path),
        "room_layer_loader": {
            "entry_addr": "0x00D500",
            "row_shift_helper_addr": "0x00D7B0",
            "tileset_dma_words": struct.unpack_from(">H", rom, 0x00D520)[0],
            "decomp_ram_dest": "0xFF3FA8",
            "plane_a_desc_addr": "0xFF1716",
            "plane_a_col_desc_addr": "0xFF1766",
            "plane_a_vram": f"0x{struct.unpack_from('>H', rom, 0x00D56E)[0]:04X}",
            "plane_b_desc_addr": "0xFF173E",
            "plane_b_col_desc_addr": "0xFF1770",
            "plane_b_vram": f"0x{struct.unpack_from('>H', rom, 0x00D5D2)[0]:04X}",
        },
        "sample_2x1_room": compute_row_shift_and_layout(bytes([0x82, 0x01, 0x01, 0x01])),
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_room_layer_loader(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote room layer loader spec (tileset={data['room_layer_loader']['tileset_dma_words']} words) to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
