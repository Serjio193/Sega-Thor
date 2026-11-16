"""Extract Best 5 Players / Clear Records Screen & SRAM Record Clear (0x03E034..0x03E60D).

Milestone: M15.284.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x001E98: Clear SRAM record magic bytes ($2025CD, $2026E1) with $A130F1 write enable
  - 0x001ECE: Read SRAM clear records ($2026E1, 6-byte magic check)
  - 0x03E034..0x03E097: Clear Records UI strings ("BEST 5 PLAYERS", "TIME", "RANK", "KILLS", "JEWELS", "1st".."5th")
  - 0x03E25E: Plane A header & rank-row layout renderer
  - 0x03E4FA..0x03E60D: Clear Records screen entry, SRAM load (0x001DDC), BGM 0x0121, Start -> soft reset 0x00020E
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_clear_records_screen_m15_284.json")


def read_cstr(rom: bytes, addr: int, max_len: int = 64) -> str:
    """Read NUL-terminated ASCII string from ROM."""
    chunk = rom[addr:addr + max_len]
    nul = chunk.find(0)
    if nul != -1:
        chunk = chunk[:nul]
    return chunk.decode("ascii", errors="replace")


def extract_clear_records_screen(rom_path: Path) -> dict:
    """Extract Best 5 Players UI strings, palette, and SRAM record addresses."""
    rom = rom_path.read_bytes()
    string_addrs = [
        ("title", 0x03E034),
        ("col_time", 0x03E043),
        ("col_rank", 0x03E048),
        ("col_kills", 0x03E04D),
        ("col_jewels", 0x03E053),
        ("rank_1st", 0x03E05A),
        ("rank_2nd", 0x03E05E),
        ("rank_3rd", 0x03E062),
        ("rank_4th", 0x03E066),
        ("rank_5th", 0x03E06A),
        ("dash_time", 0x03E06E),
        ("dash_rank", 0x03E079),
        ("dash_kills", 0x03E081),
        ("dash_jewels", 0x03E08B),
    ]
    labels = {key: read_cstr(rom, addr) for key, addr in string_addrs}
    bgm_id = struct.unpack_from(">H", rom, 0x03E5B0)[0]
    exit_sfx = struct.unpack_from(">H", rom, 0x03E5E4)[0]

    return {
        "milestone": "M15.284",
        "rom_path": str(rom_path),
        "screen": {
            "entry_addr": "0x03E4FA",
            "layout_renderer_addr": "0x03E25E",
            "sram_loader_addr": "0x001DDC",
            "sram_clear_addr": "0x001E98",
            "sram_magic_addrs": ["0x2025CD", "0x2026E1"],
            "sram_enable_reg": "0xA130F1",
            "bgm_id": f"0x{bgm_id:04X}",
            "exit_sfx_id": f"0x{exit_sfx:04X}",
            "soft_reset_target": "0x00020E",
            "record_slots": 5,
            "categories": 4,
        },
        "labels": labels,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_clear_records_screen(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['labels'])} clear-records labels to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
