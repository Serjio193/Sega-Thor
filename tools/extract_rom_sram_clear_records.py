"""Extract SRAM Clear Records & Best Time Persistence (0x001DDC..0x001F20).

Milestone: M15.288.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x0004BA: "Rank00" (6-byte magic header for $2025CD clear-records block)
  - 0x0004C0: "Race00" (6-byte magic header for $2026E1 best-time block)
  - 0x001DDC: Load 130-byte (0x82) clear-records block + u16_be sum checksum from $2025CD
  - 0x001E46: Save 130-byte clear-records block + u16_be sum checksum to $2025CD
  - 0x001E98: Clear both magic headers ($2025CD, $2026E1)
  - 0x001ECE: Read 32-bit best time from $2026E1 ("Race00" magic)
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_sram_clear_records_m15_288.json")


def extract_sram_clear_records(rom_path: Path) -> dict:
    """Extract SRAM clear-records and best-time persistence layout."""
    rom = rom_path.read_bytes()
    rank_magic = rom[0x0004BA:0x0004C0].decode("ascii", errors="replace")
    race_magic = rom[0x0004C0:0x0004C6].decode("ascii", errors="replace")

    return {
        "milestone": "M15.288",
        "rom_path": str(rom_path),
        "sram_control": {
            "enable_reg": "0xA130F1",
            "mode_write": "0x01",
            "mode_lock": "0x02",
            "mode_read": "0x03",
            "stride_bytes": 2,
        },
        "rank_block": {
            "sram_base_addr": "0x2025CD",
            "magic_rom_addr": "0x0004BA",
            "magic_ascii": rank_magic,
            "magic_len": 6,
            "payload_bytes": 130,
            "checksum_type": "u16_be_byte_sum",
            "load_addr": "0x001DDC",
            "save_addr": "0x001E46",
            "clear_addr": "0x001E98",
        },
        "race_block": {
            "sram_base_addr": "0x2026E1",
            "magic_rom_addr": "0x0004C0",
            "magic_ascii": race_magic,
            "magic_len": 6,
            "payload_type": "u32_be_best_time",
            "load_addr": "0x001ECE",
            "clear_addr": "0x001E98",
        },
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_sram_clear_records(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote SRAM clear-records spec ({data['rank_block']['magic_ascii']}, {data['race_block']['magic_ascii']}) to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
