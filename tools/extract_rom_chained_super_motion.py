#!/usr/bin/env python3
"""M15.358 extractor: ROM chained F-B-F + 360-spin super-motion recognizer (0x008814..0x00890E)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_chained_super_motion_m15_358.json")


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x008816) == 0x00FF1668
    assert _u32(rom_bytes, 0x00881C) == 0x00FF16E8
    assert _u32(rom_bytes, 0x008822) == 0x00FF16EA
    assert _u16(rom_bytes, 0x008874) == 0x007E
    assert _u16(rom_bytes, 0x0088F8) == 0x007E
    assert _u32(rom_bytes, 0x008906) == 0x00FF16EA

    return {
        "milestone": "M15.358",
        "rom_range": "0x008814..0x00890E",
        "ring_buffer": {
            "base_addr": "0xFF1668",
            "head_offset_addr": "0xFF16E8",
            "count_addr": "0xFF16EA",
            "ring_byte_mask": "0x7E",
            "uses_full_ring_count": True,
        },
        "phase_1_fbf": {
            "addr": "0x008814..0x00887C",
            "pattern_table_addr": "0x0087F4",
            "resets_ring_index_on_transition": False,
        },
        "phase_2_spin_360": {
            "addr": "0x00887E..0x00890E",
            "dir_lut_addr": "0x008768",
            "continues_remaining_count": True,
            "clears_count_on_match": True,
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
