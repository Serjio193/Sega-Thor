#!/usr/bin/env python3
"""M15.369 extractor: ROM Ali ledge climb, pit fall/platform carry & push trigger (0x005A30..0x005BA0)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_ledge_pit_push_m15_369.json")


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u16(rom_bytes, 0x005A3A) == 0x002A
    assert _u16(rom_bytes, 0x005A42) == 0x002E
    assert _u16(rom_bytes, 0x005A52) == 0x0028
    assert _u16(rom_bytes, 0x005A5A) == 0x002C
    assert _u16(rom_bytes, 0x005AB2) == 0x003E
    assert _u32(rom_bytes, 0x005AEE) == 0x00B00213
    assert _u32(rom_bytes, 0x005AFC) == 0x00B00240
    assert _u16(rom_bytes, 0x005B92) == 0x0030

    return {
        "milestone": "M15.369",
        "rom_range": "0x005A30..0x005BA0",
        "ledge_climb": {
            "x_pos_state": "0x2A",
            "x_neg_state": "0x2E",
            "y_neg_state": "0x28",
            "y_pos_state": "0x2C",
            "anim_id": "0x34",
            "counter_reset_addr": "0xFF1970",
        },
        "pit_fall_and_platform": {
            "tile_mask": "0x90",
            "tile_match": "0x80",
            "pit_state": "0x3E",
            "pit_anim_id": "0x38",
            "pit_flag_addr": "0xFF1995",
            "fall_sfx_normal": "0x00B00213",
            "fall_sfx_alt": "0x00B00240",
        },
        "push_block_trigger": {
            "contact_ptr_offset": "0x68",
            "pool_min_addr": "0xFF19E8",
            "pool_max_addr": "0xFF2954",
            "pushable_flag_offset": "0x38",
            "pushable_flag_bit": 7,
            "push_state": "0x30",
            "push_anim_id": "0x36",
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
