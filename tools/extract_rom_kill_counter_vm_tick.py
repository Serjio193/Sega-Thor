#!/usr/bin/env python3
"""Extract and verify ROM pickup overflow reclaimer, kill-counter & VM wait-tick (0x00F64C..0x00F7CE)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_kill_counter_vm_tick_m15_349.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x00F64E) == 0x00FF2ABC  # 8-slot pickup/fx pool
    assert u16(rom, 0x00F654) == 0x0694      # Base VRAM tile 0x0694
    assert u16(rom, 0x00F684) == 0x0009      # 9-tile stride per slot
    assert u32(rom, 0x00F74A) == 0x00FF0DBB  # Guaranteed-drop flag
    assert u16(rom, 0x00F75A) == 0x1101      # Guaranteed drop item (0x1101)
    assert u32(rom, 0x00F760) == 0x00FF19E8  # Ali attacker check
    assert u32(rom, 0x00F778) == 0x00FF1A7C  # Kill counter RAM (u16 saturating)
    assert u32(rom, 0x00F798) == 0x00FF17B6  # Event VM wait timer
    assert u32(rom, 0x00F7B6) == 0x00FF17B2  # Event VM PC

    return {
        "milestone": "M15.349",
        "title": "ROM Pickup Overflow Reclaimer, Kill-Counter & Event VM Wait-Tick",
        "routines": {
            "reclaim_pickup_slot": {"start": "0x00F64C", "end": "0x00F6E0"},
            "update_kill_counter_and_bonus_drop": {"start": "0x00F740", "end": "0x00F786"},
            "step_event_vm_tick": {"start": "0x00F788", "end": "0x00F7CE"},
        },
        "constants": {
            "pickup_pool_ram": "0x00FF2ABC",
            "pickup_pool_slots": 8,
            "pickup_vram_base_tile": "0x0694",
            "pickup_vram_tile_stride": 9,
            "guaranteed_drop_item": "0x1101",
            "kill_counter_ram": "0x00FF1A7C",
            "vm_wait_timer_ram": "0x00FF17B6",
            "vm_pc_ram": "0x00FF17B2",
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
