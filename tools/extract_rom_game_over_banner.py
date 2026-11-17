#!/usr/bin/env python3
"""M15.373 extractor: ROM Game Over death sequence, banner decompressor & palette line (0x005F92..0x0060CA)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_game_over_banner_m15_373.json")


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x005FD6) == 0x00D00355  # game over chime
    assert _u32(rom_bytes, 0x006046) == 0x000541FE  # banner graphic ROM addr
    assert _u32(rom_bytes, 0x006056) == 0x40000003  # VDP DMA destination
    assert _u16(rom_bytes, 0x006062) == 0xE000  # plane nametable addr

    pal = [
        f"0x{_u16(rom_bytes, 0x0060CA + i * 2):04X}"
        for i in range(16)
    ]

    return {
        "milestone": "M15.373",
        "rom_range": "0x005F92..0x0060C8",
        "post_death_delay_frames": 40,  # 0x28
        "game_over_sfx": "0x00D00355",
        "banner_graphic": {
            "rom_addr": "0x0541FE",
            "ram_buffer": "0xFF3FAC",
            "vdp_command": "0x40000003",
            "nametable_addr": "0xE000",
        },
        "palette_line_3": {
            "source_rom_addr": "0x0060CA",
            "cram_shadow_addr": "0xFF13AC",
            "color_count": 16,
            "colors": pal,
        },
        "button_wait_mask": {
            "start_bit": 7,
            "c_bit": 6,
            "b_bit": 5,
            "a_bit": 4,
            "pad_addr": "0xFF165E",
            "target_continue_menu": "0x0060EA",
        },
    }


def main() -> None:
    rom_bytes = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom_bytes)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
