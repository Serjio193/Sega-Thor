#!/usr/bin/env python3
"""Extract and verify ROM entity sprite-DMA budget throttle & frame lookup (0x00A9A0..0x00AAA5)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_sprite_dma_throttle_m15_335.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00A9C0) == 0x08000001  # Facing 3 -> mirror flag + frame 1
    assert u32(rom, 0x00A9DC) == 0x00FF1652  # Remaining V-Int DMA word budget
    assert u32(rom, 0x00A9F4) == 0x00FF1651  # DMA-backlog throttle flag
    assert u32(rom, 0x00AA32) == 0x00FF1946  # Active metasprite frame ptr
    assert u16(rom, 0x00AA62) == 0x0200      # Fallback size (bit 7 clear)
    assert u16(rom, 0x00AA72) == 0x0300      # Fallback size (bit 7 set)

    return {
        "milestone": "M15.335",
        "title": "ROM Entity Sprite-DMA Budget Throttle & Frame Lookup",
        "routine": {"start": "0x00A9A0", "end": "0x00AAA5"},
        "constants": {
            "facing_mirror_val": "0x08000001",
            "dma_budget_ram": "0x00FF1652",
            "dma_backlog_flag_ram": "0x00FF1651",
            "active_frame_ptr_ram": "0x00FF1946",
            "fallback_size_normal": "0x0200",
            "fallback_size_large": "0x0300",
            "entity_facing_offset": "0x16",
            "entity_frame_offset": "0x32",
            "entity_cached_frame_offset": "0x34",
            "entity_dma_cost_offset": "0x4C",
            "entity_shadow_flags_offset": "0x36",
            "entity_shadow_id_offset": "0x3D",
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
