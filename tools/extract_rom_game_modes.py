"""Extract Top-Level Game Mode State Machine & boot/mode transition table.

Milestone: M15.282.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x00034E: ROM checksum verification (red screen 0x000E on mismatch)
  - 0x0003E6: WRAM clear ($FF0BFE, 0x11D5 words)
  - 0x000412: Sound driver init (0x060008) & command 0 (0x060004)
  - 0x00042C..0x00045E: Top-level mode loop ($FF10AC next, $FF10AE current & 0x7C)
  - 0x00045E: 5-entry u32 mode handler pointer table (modes 0x00, 0x04, 0x08, 0x0C, 0x10)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_game_modes_m15_282.json")

MODE_TABLE_ADDR = 0x00045E
MODE_META = [
    (0x00, "sega_logo_and_region_check", "Reads $A10001 region, waits $FF10B0 timer or Start/A ($FF165F & 0xA0), transitions to 0x04"),
    (0x04, "title_and_file_select", "Loads font 0x16943C to VRAM 0xF020, runs title/save-select, transitions to 0x08/0x0C"),
    (0x08, "gameplay_new_or_continue", "Main gameplay loop entry (0x0089B2)"),
    (0x0C, "gameplay_alternate_entry", "Main gameplay loop entry (0x0089B2)"),
    (0x10, "ending_and_credits", "Fades to white (0x0EEE), clears Plane A/B, runs epilogue/credits (0x03A748)"),
]


def extract_game_modes(rom_path: Path) -> dict:
    """Extract top-level mode pointer table and $FF10AC transition sites."""
    rom = rom_path.read_bytes()
    modes = []
    for mode_id, name, desc in MODE_META:
        entry_addr = MODE_TABLE_ADDR + mode_id
        target = struct.unpack_from(">I", rom, entry_addr)[0]
        modes.append({
            "mode_id": mode_id,
            "mode_hex": f"0x{mode_id:02X}",
            "name": name,
            "table_entry_addr": f"0x{entry_addr:06X}",
            "handler_addr": f"0x{target:06X}",
            "description": desc,
        })

    pat = struct.pack(">I", 0x00FF10AC)
    refs = []
    idx = rom.find(pat)
    while idx != -1:
        refs.append(f"0x{idx:06X}")
        idx = rom.find(pat, idx + 2)

    return {
        "milestone": "M15.282",
        "rom_path": str(rom_path),
        "boot_and_loop": {
            "checksum_check_addr": "0x00034E",
            "wram_clear_addr": "0x0003E6",
            "wram_clear_base": "0xFF0BFE",
            "wram_clear_words": 0x11D5,
            "sound_init_addr": "0x000412",
            "mode_loop_addr": "0x00042C",
            "mode_table_addr": f"0x{MODE_TABLE_ADDR:06X}",
            "ram_next_mode": "0xFF10AC",
            "ram_current_mode": "0xFF10AE",
            "ram_mode_timer": "0xFF10B0",
        },
        "modes": modes,
        "next_mode_refs": refs,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_game_modes(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['modes'])} game modes and {len(data['next_mode_refs'])} $FF10AC refs to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
