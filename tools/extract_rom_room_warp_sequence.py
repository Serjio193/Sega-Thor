#!/usr/bin/env python3
"""Extract and verify ROM room update hooks & warp transition sequence (0x00C980..0x00CB0A)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_room_warp_sequence_m15_318.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # Room update hooks
    assert u32(rom, 0x00C984) == 0x00FF17B8
    assert u32(rom, 0x00C98E) == 0x00FF16EC
    assert u32(rom, 0x00C99C) == 0x00FF17B8
    assert u32(rom, 0x00C9A6) == 0x00FF16EC

    # 6-flag warp lock guard (0x00C9B2..0x00C9EA)
    lock_addrs = [
        u32(rom, 0x00C9B4),  # 0x00FF17B9
        u32(rom, 0x00C9BC),  # 0x00FF1987
        u32(rom, 0x00C9C6),  # 0x00FF1985
        u32(rom, 0x00C9D0),  # 0x00FF17B8 (bit 2)
        u32(rom, 0x00C9D8),  # 0x00FF1983
        u32(rom, 0x00C9E0),  # 0x00FF1982
    ]
    assert lock_addrs == [0x00FF17B9, 0x00FF1987, 0x00FF1985, 0x00FF17B8, 0x00FF1983, 0x00FF1982]

    # Half-screen camera centering offsets (0x00A0, 0x0070)
    assert u16(rom, 0x00C9FC) == 0x00A0
    assert u16(rom, 0x00CA04) == 0x0070
    assert u16(rom, 0x00CA28) == 0x00A0
    assert u16(rom, 0x00CA30) == 0x0070

    # Room 0x0004 -> 0x0611 redirect (0x00CA3E..0x00CA46)
    assert u16(rom, 0x00CA40) == 0x0004
    assert u16(rom, 0x00CA46) == 0x0611

    # Checkpoint call 0x0017DA & fade speed 4
    assert u32(rom, 0x00CA60) == 0x000017DA
    assert u32(rom, 0x00CA94) == 0x00060004
    assert u16(rom, 0x00CAFA) == 0x0004

    return {
        "milestone": "M15.318",
        "title": "ROM Room Update Hooks & Warp Transition Sequence",
        "routines": {
            "room_update_early": {"start": "0x00C980", "end": "0x00C996", "header_rel_offset": 0},
            "room_update_late": {"start": "0x00C998", "end": "0x00C9B0", "header_rel_offset": 2},
            "room_warp_guard": {"start": "0x00C9B2", "end": "0x00C9EA"},
            "warp_door_fade": {"start": "0x00C9EC", "end": "0x00CA0E"},
            "warp_instant_cut": {"start": "0x00CA10", "end": "0x00CB0A"},
        },
        "warp_lock_flags": [f"0x{a:08X}" for a in lock_addrs],
        "constants": {
            "camera_half_w": 160,
            "camera_half_h": 112,
            "redirect_src_room": "0x0004",
            "redirect_dst_room": "0x0611",
            "checkpoint_flag_offset": "0x19",
            "checkpoint_flag_bit": 6,
            "checkpoint_routine": "0x0017DA",
            "sound_stop_cmd": 2,
            "sound_cmd_entry": "0x00060004",
            "fade_speed": 4,
            "spawn_block_end_ram": "0x00FF178E",
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
