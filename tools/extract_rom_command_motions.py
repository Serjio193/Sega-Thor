#!/usr/bin/env python3
"""M15.357 extractor: ROM 360-spin & F-B-F command-motion recognizers (0x0086B4..0x008813)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_command_motions_m15_357.json")


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x0086B6) == 0x00FF1668
    assert _u32(rom_bytes, 0x0086BC) == 0x00FF16E8
    assert _u32(rom_bytes, 0x0086C2) == 0x00FF16EA
    assert _u16(rom_bytes, 0x0086CE) == 0x001D
    assert _u32(rom_bytes, 0x00877A) == 0x00FF1668

    dir_lut = [
        rom_bytes[0x008768 + i] - 256 if rom_bytes[0x008768 + i] >= 128 else rom_bytes[0x008768 + i]
        for i in range(16)
    ]
    fbf_table = []
    for facing in range(4):
        base = 0x0087F4 + facing * 8
        fbf_table.append({
            "facing": facing,
            "sequence": list(rom_bytes[base : base + 4]),
            "masks": list(rom_bytes[base + 4 : base + 8]),
        })

    return {
        "milestone": "M15.357",
        "rom_range": "0x0086B4..0x008813",
        "ring_buffer": {
            "base_addr": "0xFF1668",
            "head_offset_addr": "0xFF16E8",
            "count_addr": "0xFF16EA",
            "ring_byte_mask": "0x7E",
            "entry_stride_bytes": 2,
        },
        "spin_360_recognizer": {
            "addr": "0x0086B4..0x008777",
            "max_samples": 30,
            "pad_to_dir8_lut": dir_lut,
        },
        "fbf_dash_recognizer": {
            "addr": "0x008778..0x008813",
            "facing_offset": "0x16",
            "patterns": fbf_table,
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
