#!/usr/bin/env python3
"""Extract and verify ROM Ali rapid-mash, triple-tap detectors & demo stream loader (0x008910..0x008AA6)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_rapid_tripletap_demo_m15_394.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x008910: rapid_mash_detector
    assert u32(rom, 0x008912) == 0x00FF1668  # Joypad ring buffer
    assert u32(rom, 0x008918) == 0x00FF16E8  # Ring write pointer
    assert u16(rom, 0x00891E) == 0x000C      # 13-frame loop (dbra 12)
    assert u16(rom, 0x00893A) == 0x0003      # cmpi.w #3, d1
    assert u32(rom, 0x008946) == 0x00FF16EA  # clr.w $FF16EA

    # 0x008950: triple_tap_detector
    assert u32(rom, 0x008958) == 0x00FF16E8  # Ring write pointer
    assert u32(rom, 0x00895E) == 0x00FF16EA  # Available ring frames
    assert u16(rom, 0x00896A) == 0x0018      # Max 25 frames (cmpi.w #$18)
    assert u16(rom, 0x008982) == 0x0003      # 3 matching taps required
    assert u32(rom, 0x0089A8) == 0x00FF16EA  # clr.w $FF16EA on match

    # 0x0089B2: demo_stream_initializer
    assert u32(rom, 0x0089B6) == 0x00FF164D  # bset #2, $FF164D
    assert u32(rom, 0x0089E2) == 0x00FF1930  # move.l a7, $FF1930
    assert u32(rom, 0x008A06) == 0x00FF0BFC  # move.b $FF0BFC, d0
    assert u32(rom, 0x008A1E) == 0x001698FE  # lea $1698FE, a0
    assert u32(rom, 0x008A28) == 0x00FFBFAC  # lea $FFBFAC, a1 (decompress dest)
    assert u16(rom, 0x008A2E) == 0xADF2      # bsr.w $3820 (0x8A2E - 0x520E = 0x3820)
    assert u32(rom, 0x008A32) == 0x00FFDFB2  # lea $FFDFB2, a1 (delta buffer)
    assert u16(rom, 0x008A38) == 0x1FFE      # move.w #$1FFE, d0 (delta loop count)

    # 0x008A94: 9 demo asset offsets
    offsets = list(struct.unpack_from(">9H", rom, 0x008A94))
    expected_offsets = [0x0000, 0x042C, 0x0BC0, 0x1202, 0x144C, 0x16C8, 0x18E0, 0x1EE8, 0x201E]
    assert offsets == expected_offsets

    abs_addrs = [0x1698FE + off for off in offsets]

    return {
        "milestone": "M15.394",
        "title": "ROM Ali Rapid-Mash (0x8910), Triple-Tap (0x8950) Motion Detectors & Demo Stream Loader (0x008910..0x008AA6)",
        "routines": {
            "rapid_mash_detector": {
                "start": "0x008910",
                "end": "0x00894E",
                "bytes": 64,
                "scan_window_frames": 13,
                "button_bit": 4,
                "required_presses": 3,
                "ring_buf_ram": "0x00FF1668",
                "ring_wr_ptr_ram": "0x00FF16E8",
                "ring_count_ram": "0x00FF16EA",
            },
            "triple_tap_detector": {
                "start": "0x008950",
                "end": "0x0089B0",
                "bytes": 98,
                "max_scan_frames": 25,
                "required_taps": 3,
                "facing_pattern_table": "0x0087F4",
                "ring_buf_ram": "0x00FF1668",
                "ring_wr_ptr_ram": "0x00FF16E8",
                "ring_count_ram": "0x00FF16EA",
            },
            "demo_stream_initializer": {
                "start": "0x0089B2",
                "end": "0x008A92",
                "bytes": 226,
                "vblank_flag_ram": "0x00FF164D",
                "stack_checkpoint_ram": "0x00FF1930",
                "demo_flag_ram": "0x00FF1996",
                "demo_index_ram": "0x00FF0BFC",
                "demo_max_index": 8,
                "demo_bank_base_rom": "0x1698FE",
                "decompress_routine": "0x00003820",
                "decompress_buffer_ram": "0x00FFBFAC",
                "delta_buffer_ram": "0x00FFDFB2",
                "delta_count_words": 8190,
                "gameplay_coordinator": "0x00CA24",
            },
            "demo_asset_offsets_table": {
                "start": "0x008A94",
                "end": "0x008AA6",
                "bytes": 18,
                "entry_count": 9,
                "offsets": [f"0x{off:04X}" for off in offsets],
                "absolute_rom_addrs": [f"0x{addr:06X}" for addr in abs_addrs],
            },
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
