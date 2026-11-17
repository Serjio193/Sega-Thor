"""Extract Save/Load File Select Menu, 2x3 Grid Cursor & Copy/Erase Flow (0x0004C6..0x000D30).

Milestone: M15.290.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x0004C6..0x0006F5: File Select entry, 2x3 grid cursor $FF10A8 (0..3 slots, 4 Copy, 5 Erase)
  - 0x000682..0x0006B0: Persists last-selected slot ($FF10A8) to SRAM $2025C9 ($A130F1 write=1, lock=2)
  - 0x0006E2..0x0006F5: Occupied bitmask $FF10AA -> New Game (0x001300) vs Continue (0x00161C)
  - 0x0006F6..0x0007A1: 2D grid cursor navigation (max_row d2, excluded_slot d3, SFX 0x6003A6)
  - 0x000B04: Copy save flow (aborts if $FF10AA == 0, excludes source slot via d3)
  - 0x000C46: Erase save flow (aborts if $FF10AA == 0, error SFX 0xA00253 on empty slot)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_file_select_menu_m15_290.json")


def extract_file_select_menu(rom_path: Path) -> dict:
    """Extract File Select grid cursor, SFX IDs, and SRAM last-slot addresses."""
    rom = rom_path.read_bytes()
    confirm_sfx = struct.unpack_from(">I", rom, 0x000630)[0]
    move_sfx = struct.unpack_from(">I", rom, 0x000794)[0]
    error_sfx = struct.unpack_from(">I", rom, 0x000C86)[0]
    last_slot_sram = struct.unpack_from(">I", rom, 0x000696)[0]

    return {
        "milestone": "M15.290",
        "rom_path": str(rom_path),
        "file_select": {
            "entry_addr": "0x0004C6",
            "grid_nav_addr": "0x0006F6",
            "copy_flow_addr": "0x000B04",
            "erase_flow_addr": "0x000C46",
            "new_game_addr": "0x001300",
            "continue_game_addr": "0x00161C",
            "cancel_mode": "0x04",
            "ram_cursor_ff10a8": "0xFF10A8",
            "ram_occupied_mask_ff10aa": "0xFF10AA",
            "ram_copy_src_ff19a8": "0xFF19A8",
            "sram_last_slot_addr": f"0x{last_slot_sram:06X}",
            "save_slot_count": 4,
            "copy_cursor_index": 4,
            "erase_cursor_index": 5,
            "confirm_sfx": f"0x{confirm_sfx:08X}",
            "cursor_move_sfx": f"0x{move_sfx:08X}",
            "invalid_erase_sfx": f"0x{error_sfx:08X}",
        },
        "grid_options": [
            {"index": 0, "role": "save_slot_0"},
            {"index": 1, "role": "save_slot_1"},
            {"index": 2, "role": "save_slot_2"},
            {"index": 3, "role": "save_slot_3"},
            {"index": 4, "role": "copy_slot", "handler_addr": "0x000B04"},
            {"index": 5, "role": "erase_slot", "handler_addr": "0x000C46"},
        ],
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_file_select_menu(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['grid_options'])} File Select grid options to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
