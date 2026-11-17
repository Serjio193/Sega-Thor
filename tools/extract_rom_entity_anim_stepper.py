#!/usr/bin/env python3
"""M15.363 extractor: ROM entity animation keyframe stepper & hitstop/slow-mo damper (0x008F12..0x00904A)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_entity_anim_stepper_m15_363.json")


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x008F50) == 0x00FF17B8
    assert _u16(rom_bytes, 0x008FC2) == 0xFFFC
    assert _u16(rom_bytes, 0x008FF4) == 0x5840  # addq.w #4, d0
    assert _u16(rom_bytes, 0x009024) == 0xE24B  # lsr.w #1, d3

    return {
        "milestone": "M15.363",
        "rom_range": "0x008F12..0x00904A",
        "hitstop_and_slowmo": {
            "global_flags_addr": "0xFF17B8",
            "slowmo_bit": 0,
            "timestop_bit": 1,
            "freeze_timer_word_offset": "0x9C",
            "freeze_byte_offset": "0x9D",
            "vx_offset": "0x72",
            "vy_offset": "0x76",
        },
        "animation_stepper": {
            "anim_table_ptr_offset": "0x26",
            "anim_id_offset": "0x2A",
            "cached_anim_id_offset": "0x2C",
            "delay_offset": "0x2E",
            "cursor_offset": "0x30",
            "frame_id_offset": "0x32",
            "flags_offset": "0x37",
            "end_pulse_bit": 1,
            "end_of_seq_bit": 2,
            "skip_anim_bit": 0,
            "keyframe_stride_bytes": 4,
            "initial_cursor": -4,
            "duration_end_bit": 15,
            "duration_shift_right": 1,
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
