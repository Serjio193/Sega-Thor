#!/usr/bin/env python3
"""Extract and verify ROM rapid-mash & triple-tap input ring detectors (0x008910..0x0089A8)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_mash_tripletap_m15_347.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x008912) == 0x00FF1668  # 64-frame input ring buffer
    assert u32(rom, 0x008918) == 0x00FF16E8  # Ring read head
    assert u16(rom, 0x00891E) == 0x000C      # 13-frame mash window (dbra 0x0C)
    assert u16(rom, 0x00893A) == 0x0003      # >= 3 button presses
    assert u32(rom, 0x008952) == 0x00FF1668  # 64-frame input ring buffer
    assert u16(rom, 0x00896A) == 0x0018      # Max 25-frame tap window (dbra 0x18)
    assert u16(rom, 0x008982) == 0x0003      # 3 matching facing frames

    return {
        "milestone": "M15.347",
        "title": "ROM Rapid-Mash & Triple-Tap Input Ring Detectors",
        "routines": {
            "detect_rapid_mash": {"start": "0x008910", "end": "0x00894E"},
            "detect_triple_tap": {"start": "0x008950", "end": "0x0089A8"},
        },
        "constants": {
            "input_ring_ram": "0x00FF1668",
            "ring_head_ram": "0x00FF16E8",
            "ring_count_ram": "0x00FF16EA",
            "ring_byte_mask": "0x7E",
            "mash_window_frames": 13,
            "mash_min_presses": 3,
            "mash_button_bit": 4,
            "tap_max_window_frames": 25,
            "tap_required_matches": 3,
        },
    }


def main() -> None:
    rom = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
