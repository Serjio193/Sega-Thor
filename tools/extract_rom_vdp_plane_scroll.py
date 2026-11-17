#!/usr/bin/env python3
"""Extract and verify ROM VDP incremental row/column plane scroller (0x0023CC..0x002630)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_vdp_plane_scroll_m15_313.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # Column streamer (0x0023CC..0x002510)
    assert u16(rom, 0x0023DE) == 0x38BC  # move.w #$8f80, (a4)
    auto_inc_col = u16(rom, 0x0023E0)
    assert auto_inc_col == 0x8F80

    assert u16(rom, 0x002420) == 0x0647  # addi.w #$28, d7
    col_ahead_tiles = u16(rom, 0x002422)
    assert col_ahead_tiles == 0x0028  # 40 tiles

    assert u16(rom, 0x002450) == 0x0641  # addi.w #$50, d1
    col_ahead_bytes = u16(rom, 0x002452)
    assert col_ahead_bytes == 0x0050  # 80 bytes

    assert u16(rom, 0x0024C8) == 0x3E3C  # move.w #$1c, d7
    col_dbra_count = u16(rom, 0x0024CA)
    assert col_dbra_count == 0x001C  # 29 rows (dbra 0x1C..0)

    assert u16(rom, 0x0024E8) == 0x363C  # move.w #$3e8, d3
    sentinel_height = u16(rom, 0x0024EA)
    assert sentinel_height == 0x03E8  # 1000

    assert u16(rom, 0x0024F2) == 0x0C44  # cmpi.w #$20, d4
    vdp_rows = u16(rom, 0x0024F4)
    assert vdp_rows == 0x0020  # 32 rows

    assert u16(rom, 0x00250C) == 0x38BC  # move.w #$8f02, (a4)
    auto_inc_restore = u16(rom, 0x00250E)
    assert auto_inc_restore == 0x8F02

    assert u16(rom, 0x002510) == 0x4E75  # rts

    # Row streamer (0x002512..0x002630)
    assert u16(rom, 0x002558) == 0x0647  # addi.w #$1c, d7
    row_ahead_tiles = u16(rom, 0x00255A)
    assert row_ahead_tiles == 0x001C  # 28 tiles

    assert u16(rom, 0x002588) == 0x0641  # addi.w #$e00, d1
    row_ahead_bytes = u16(rom, 0x00258A)
    assert row_ahead_bytes == 0x0E00  # 28 * 128 bytes

    assert u16(rom, 0x0025FC) == 0x3E3C  # move.w #$28, d7
    row_dbra_count = u16(rom, 0x0025FE)
    assert row_dbra_count == 0x0028  # 41 cols (dbra 0x28..0)

    assert u16(rom, 0x002616) == 0x0C44  # cmpi.w #$40, d4
    vdp_cols = u16(rom, 0x002618)
    assert vdp_cols == 0x0040  # 64 cols

    assert u16(rom, 0x002630) == 0x4E75  # rts

    # VDP ports & nametable masks
    assert u32(rom, 0x0023D6) == 0x00C00000
    assert u32(rom, 0x00251C) == 0x00C00000

    return {
        "milestone": "M15.313",
        "title": "ROM Incremental Row/Col VDP Plane Scroller",
        "routines": {
            "stream_column": {
                "start": "0x0023CC",
                "end": "0x002510",
                "vdp_auto_inc_setup": f"0x{auto_inc_col:04X}",
                "vdp_auto_inc_restore": f"0x{auto_inc_restore:04X}",
                "forward_ahead_tiles": col_ahead_tiles,
                "forward_ahead_bytes": f"0x{col_ahead_bytes:04X}",
                "dbra_count": col_dbra_count,
                "tiles_per_column": col_dbra_count + 1,
                "wrap_sentinel_height": sentinel_height,
                "vdp_plane_rows": vdp_rows,
            },
            "stream_row": {
                "start": "0x002512",
                "end": "0x002630",
                "forward_ahead_tiles": row_ahead_tiles,
                "forward_ahead_bytes": f"0x{row_ahead_bytes:04X}",
                "dbra_count": row_dbra_count,
                "tiles_per_row": row_dbra_count + 1,
                "vdp_plane_cols": vdp_cols,
            },
        },
        "plane_state_offsets": {
            "src_col": "0x08",
            "src_row": "0x0A",
            "vram_offset": "0x0C",
            "tilemap_ptr": "0x16",
            "vram_base": "0x1A",
            "pending_dx_tiles": "0x1C",
            "pending_dy_tiles": "0x1E",
            "map_width_tiles": "0x20",
            "map_height_tiles": "0x22",
            "row_shift": "0x24",
        },
        "vdp_nametable": {
            "data_port": "0x00C00000",
            "ctrl_port": "0x00C00004",
            "col_byte_mask": "0x007E",
            "row_byte_mask": "0x0F80",
            "row_stride_bytes": "0x0080",
            "plane_size_bytes": "0x1000",
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
