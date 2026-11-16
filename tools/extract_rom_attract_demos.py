"""Extract Attract/Demo Stream Table (0x008A94, 9 demos at 0x1698FE) & Playback Setup.

Milestone: M15.289.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x0089EC..0x008A93: Attract/Demo mode entry ($FF1996 != 0), cycles $FF0BFC 0..8
  - 0x008A94..0x008AA5: 9-entry u16 offset table relative to 0x1698FE
  - 0x008A26: Decompresses demo stream via 0x003820 into $FFBFAC
  - 0x008A30..0x008A48: Delta-decodes 8191 bytes at $FFDFB2
  - 0x008A50..0x008A5C: Reads header (room_id, player_x, player_y) and stores stream ptr $FF1998
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_attract_demos_m15_289.json")

DEMO_TABLE_ADDR = 0x008A94
DEMO_BASE_ADDR = 0x1698FE
DEMO_COUNT = 9


def extract_attract_demos(rom_path: Path) -> dict:
    """Extract all 9 attract demo stream offsets and playback setup constants."""
    rom = rom_path.read_bytes()
    demos = []
    for slot in range(DEMO_COUNT):
        entry_addr = DEMO_TABLE_ADDR + slot * 2
        rel_off = struct.unpack_from(">H", rom, entry_addr)[0]
        rom_addr = DEMO_BASE_ADDR + rel_off
        demos.append({
            "slot": slot,
            "table_entry_addr": f"0x{entry_addr:06X}",
            "rel_offset": rel_off,
            "rom_addr": f"0x{rom_addr:06X}",
        })

    return {
        "milestone": "M15.289",
        "rom_path": str(rom_path),
        "attract_setup": {
            "entry_addr": "0x0089B2",
            "demo_branch_addr": "0x0089FA",
            "table_addr": f"0x{DEMO_TABLE_ADDR:06X}",
            "stream_base_addr": f"0x{DEMO_BASE_ADDR:06X}",
            "demo_count": DEMO_COUNT,
            "decompressor_addr": "0x003820",
            "decomp_ram_dest": "0xFFBFAC",
            "delta_buffer_addr": "0xFFDFB2",
            "delta_decode_bytes": 0x1FFF,
            "ram_demo_slot": "0xFF0BFC",
            "ram_demo_active": "0xFF1996",
            "ram_demo_stream_ptr": "0xFF1998",
            "gameplay_entry_addr": "0x00CA24",
        },
        "demos": demos,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_attract_demos(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['demos'])} attract demo streams to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
