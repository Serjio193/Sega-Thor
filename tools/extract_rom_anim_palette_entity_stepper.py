#!/usr/bin/env python3
"""Extract and verify ROM animated palette CRAM sequencer & 3-group entity master stepper (0x008E22..0x008F22)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_anim_palette_entity_stepper_m15_397.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x008E22: anim_palette_sequencer
    assert u32(rom, 0x008E22 + 2) == 0x00FF164F  # tst.b $FF164F
    assert u32(rom, 0x008E2C + 2) == 0x00FF17A2  # movea.l $FF17A2, a0
    assert u32(rom, 0x008E32 + 2) == 0x00FF134C  # lea $FF134C, a1
    assert u32(rom, 0x008E86 + 4) == 0x00FF164D  # bset.b #0, $FF164D (CRAM sync)

    # 0x008E90: entity_master_stepper
    assert u32(rom, 0x008E90 + 2) == 0x00FF2954  # Group 1: 4 slots x 90B
    assert u16(rom, 0x008E96) == 0x7E03          # moveq #3, d7
    assert u32(rom, 0x008E98 + 2) == 0x00008EAA  # return vector
    assert u32(rom, 0x008E98 + 6) == 0x00FF193C  # move.l #$8EAA, $FF193C

    assert u32(rom, 0x008EB2 + 2) == 0x00FF19E8  # Group 2: 21 slots x 188B
    assert u16(rom, 0x008EB8) == 0x7E14          # moveq #$14, d7
    assert u32(rom, 0x008EBA + 2) == 0x00008ECC  # return vector
    assert u32(rom, 0x008EBA + 6) == 0x00FF193C  # move.l #$8ECC, $FF193C

    assert u32(rom, 0x008ED4 + 2) == 0x00FF2D8C  # Group 3: 6 slots x 90B
    assert u16(rom, 0x008EDA) == 0x7E05          # moveq #5, d7
    assert u32(rom, 0x008EDC + 2) == 0x00008EEE  # return vector
    assert u32(rom, 0x008EDC + 6) == 0x00FF193C  # move.l #$8EEE, $FF193C

    # 0x008EF8: entity_state_reset
    assert u16(rom, 0x008EF8) == 0x3D40          # move.w d0, $2A(a6)
    assert u16(rom, 0x008EFA) == 0x002A
    assert u16(rom, 0x008EFC) == 0x3D7C          # move.w #$FFFF, $2C(a6)
    assert u16(rom, 0x008EFE) == 0xFFFF
    assert u16(rom, 0x008F00) == 0x002C
    assert u16(rom, 0x008F02) == 0x022E          # andi.b #$F9, $37(a6)
    assert u16(rom, 0x008F04) == 0xFFF9
    assert u16(rom, 0x008F06) == 0x0037

    # 0x008F12: secondary_entity_dispatch
    assert u16(rom, 0x008F12) == 0x08EE          # bset.b #4, $37(a6)
    assert u16(rom, 0x008F12 + 2) == 0x0004

    groups = [
        {"base": "0x00FF2954", "slots": 4, "stride": 90, "handler": "0x008F12"},
        {"base": "0x00FF19E8", "slots": 21, "stride": 188, "handler": "0x008F22"},
        {"base": "0x00FF2D8C", "slots": 6, "stride": 90, "handler": "0x008F12"},
    ]

    return {
        "milestone": "M15.397",
        "title": "ROM Animated Palette CRAM Sequencer & 3-Group Entity Master Stepper",
        "routines": {
            "anim_palette_sequencer": {
                "start": "0x008E22",
                "end": "0x008E8E",
                "bytes": 108,
                "active_flag_ram": "0x00FF164F",
                "table_ptr_ram": "0x00FF17A2",
                "cram_buffer_ram": "0x00FF134C",
                "cram_sync_flag_ram": "0x00FF164D",
                "cram_sync_bit": 0,
            },
            "entity_master_stepper": {
                "start": "0x008E90",
                "end": "0x008EF6",
                "bytes": 102,
                "groups": groups,
                "recovery_vector_ram": "0x00FF193C",
            },
            "entity_state_reset": {
                "start": "0x008EF8",
                "end": "0x008F10",
                "bytes": 24,
                "state_offset": "0x2A",
                "timer_offset": "0x2C",
                "timer_init_val": "0xFFFF",
                "flags_37_mask": "0xF9",
            },
            "secondary_entity_dispatch": {
                "start": "0x008F12",
                "end": "0x008F22",
                "bytes": 16,
                "active_bit": 4,
                "script_engine_addr": "0x0039CA",
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
