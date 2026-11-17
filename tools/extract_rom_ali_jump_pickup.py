#!/usr/bin/env python3
"""Extract and verify ROM Ali jump trigger, pickup probe & 4-dir interaction box (0x00822C..0x008350)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_ali_jump_pickup_m15_351.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def s8(val: int) -> int:
    return val - 256 if val >= 128 else val


def extract_evidence(rom: bytes) -> dict:
    assert u32(rom, 0x008250) == 0x00070000  # Running jump Z vel (+7.0)
    assert u16(rom, 0x00825E) == 0x0016      # Running jump state 0x16 (22)
    assert u32(rom, 0x008282) == 0x00070000  # Standing jump Z vel (+7.0)
    assert u16(rom, 0x008290) == 0x0010      # Standing jump state 0x10 (16)
    assert u16(rom, 0x0082AE) == 0x7206      # moveq #6, d1 (6px pickup probe)
    assert u16(rom, 0x0082C2) == 0x0008      # Pickup entity type 0x08
    assert u32(rom, 0x0082D8) == 0x00FF1976  # Collected item (id, qty) RAM

    # 4-dir interact probe boxes (d1=x1, d2=y1, d3=x2, d4=y2)
    boxes = {
        "facing_0_up": [s8(rom[0x00831B]), s8(rom[0x00831D]), s8(rom[0x00831F]), s8(rom[0x008321])],
        "facing_1_right": [s8(rom[0x008311]), s8(rom[0x008313]), s8(rom[0x008315]), s8(rom[0x008317])],
        "facing_2_down": [s8(rom[0x008325]), s8(rom[0x008327]), s8(rom[0x008329]), s8(rom[0x00832B])],
        "facing_3_left": [s8(rom[0x008307]), s8(rom[0x008309]), s8(rom[0x00830B]), s8(rom[0x00830D])],
    }

    return {
        "milestone": "M15.351",
        "title": "ROM Ali Jump Trigger, Pickup Probe & 4-Dir Interaction Box",
        "routines": {
            "ali_jump_trigger": {"start": "0x00822C", "end": "0x0082AC"},
            "ali_pickup_probe": {"start": "0x0082AE", "end": "0x0082F6"},
            "ali_interact_probe": {"start": "0x0082F8", "end": "0x008350"},
        },
        "interact_boxes": boxes,
        "constants": {
            "jump_z_vel_16_16": "0x00070000",
            "running_jump_state": 22,
            "standing_jump_state": 16,
            "running_jump_boost": 8,
            "pickup_probe_radius_px": 6,
            "pickup_entity_type": 8,
            "collected_item_ram": "0x00FF1976",
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
