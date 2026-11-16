#!/usr/bin/env python3
"""
extract_rom_bgm_track_table.py — Milestone M15.278

Extracts the ROM BGM Sequence Offset Table (`0x0638D4`) and Z80 Bus Handshake
used by the BGM Track Loader (`0x060B50..0x060BD0`):
1. Table Base `0x0638D4` (256 x 32-bit relative offsets, first sequence at +0x0400 = 0x063CD4):
   - `seq_rom_addr = 0x0638D4 + u32_be(0x0638D4 + track_id * 4)` (0 = unused).
   - Extracts all valid BGM tracks (`0x00..0xFF`).
2. Z80 Bus Handshake (`0x060B68..0x060BC4`):
   - Requests Z80 bus via `$A11100 = 0x0100`, writes `0x83` to Z80 RAM `$A00003`
     (stop/reset Z80 playback), releases bus (`$A11100 = 0x0000`), and polls
     `$A00003 == 0` for Z80 acknowledgement.
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_bgm_track_table_m15_278.json"

BGM_TABLE_BASE = 0x0638D4
BGM_LOADER_ADDR = 0x060B50


def read_u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def read_u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def main() -> int:
    rom_path = Path(ROM_PATH_DEFAULT)
    out_path = Path(OUTPUT_PATH_DEFAULT)
    if not rom_path.exists():
        print(f"Error: ROM not found at {rom_path}")
        return 1

    rom = rom_path.read_bytes()

    # Verify lea $638d4(pc), a0 at 0x060B50 (41FA <s16>)
    assert read_u16_be(rom, BGM_LOADER_ADDR) == 0x41FA
    disp = struct.unpack_from(">h", rom, BGM_LOADER_ADDR + 2)[0]
    assert BGM_LOADER_ADDR + 2 + disp == BGM_TABLE_BASE

    # Verify Z80 bus request $00A11100 and reset byte $83 -> $00A00003
    assert read_u32_be(rom, 0x060B6C) == 0x00A11100
    assert rom[0x060B7F] == 0x83
    assert read_u32_be(rom, 0x060B80) == 0x00A00003

    tracks = []
    for tid in range(256):
        rel = read_u32_be(rom, BGM_TABLE_BASE + tid * 4)
        if rel != 0 and BGM_TABLE_BASE + rel < len(rom):
            tracks.append({
                "track_id": f"0x{tid:02X}",
                "command_word": f"0x01{tid:02X}",
                "rel_offset": f"0x{rel:06X}",
                "rom_addr": f"0x{BGM_TABLE_BASE + rel:06X}",
            })

    evidence = {
        "milestone": "M15.278",
        "description": "ROM BGM Sequence Offset Table (0x0638D4) and Z80 Bus Handshake (0x060B50)",
        "loader_addr": f"0x{BGM_LOADER_ADDR:06X}",
        "table_base": f"0x{BGM_TABLE_BASE:06X}",
        "z80_handshake": {
            "bus_req_addr": "0x00A11100",
            "z80_cmd_addr": "0x00A00003",
            "z80_stop_byte": "0x83",
        },
        "valid_track_count": len(tracks),
        "tracks": tracks,
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} ({len(tracks)} BGM tracks)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
