#!/usr/bin/env python3
"""
extract_rom_enemy_ai_0030_0048.py — Milestone M15.269

Extracts ROM semantic knowledge for two key dungeon/progression enemies:
1. Entity 0x0048 (Charger / Trap-Runner, `0x01778C..0x0178E8`):
   - 4-substate AI (`$A6(A6)` = 0..3):
     * 0 (`0x0177A6`): Record home `(x, y)` in `$A8/$AA`, set `action = 0`.
     * 1 (`0x0177C6`): Test 16-byte directional tile AABB table `0x01783E` via
       `0x00C1CE`; on player trigger, compute velocity via `0x00E2D4`, set
       `action = 2`, timer `$9E(A6)`, SFX `0x800399`, advance to substate 2.
     * 2 (`0x01784E`): Charge/decelerate (`0x00BC8E` collision, invert accel at
       halfway `timer < $9E/2`), then flip facing (`bchg #1, $17`) -> substate 3.
     * 3 (`0x0178B2`): Return to home `(x, y)` via `0x00E45E`, flip facing -> substate 1.
2. Entity 0x0030 (Room 0x0105 Waterfall Gate Boss, `0x015AE2..0x015B9C`):
   - 4-substate AI (`$66(A6)` = 0..3), spawns 5 linked child slots (`+$BC` stride)
     via `0x00E66E` with `action 0x3A / 0x3C` and VRAM base `0x4200`.
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_enemy_ai_0030_0048_m15_269.json"


def read_u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def read_u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def main() -> int:
    rom_path = Path(ROM_PATH_DEFAULT)
    out_path = Path(OUTPUT_PATH_DEFAULT)
    if not rom_path.exists():
        print(f"Error: ROM not found at {rom_path}")
        return 1

    rom = rom_path.read_bytes()

    # Verify 0x0048 Charger AABB table at 0x01783E (4 facings x 4 signed bytes)
    aabb_bytes = rom[0x01783E:0x01784E]
    assert len(aabb_bytes) == 16
    aabb_boxes = []
    for facing in range(4):
        dx1, dy1, dx2, dy2 = struct.unpack_from(">bbbb", aabb_bytes, facing * 4)
        aabb_boxes.append({"facing": facing, "dx1": dx1, "dy1": dy1, "dx2": dx2, "dy2": dy2})

    # Verify 0x0048 SFX 0x800399 at 0x017830 (203C 0080 0399)
    assert read_u16_be(rom, 0x017830) == 0x203C
    charger_sfx = read_u32_be(rom, 0x017832)
    assert charger_sfx == 0x00800399

    # Verify 0x0030 Gate Boss VRAM base 0x4200 at 0x015B0C (3D7C 4200 0018)
    assert read_u16_be(rom, 0x015B0C) == 0x3D7C
    boss_vram = read_u16_be(rom, 0x015B0E)
    assert boss_vram == 0x4200

    evidence = {
        "milestone": "M15.269",
        "description": "ROM Enemy AI Contracts for Entity 0x0048 (Charger) and Entity 0x0030 (Room 0x0105 Gate Boss)",
        "entity_0048_charger": {
            "update_handler": "0x01778C",
            "substate_field": "$A6(A6)",
            "aabb_table_addr": "0x01783E",
            "aabb_boxes": aabb_boxes,
            "trigger_sfx": f"0x{charger_sfx:06X}",
            "helpers": {
                "aabb_check": "0x00C1CE",
                "velocity_init": "0x00E2D4",
                "collision_step": "0x00BC8E",
                "return_home_step": "0x00E45E",
            },
        },
        "entity_0030_gate_boss": {
            "update_handler": "0x015AE2",
            "substate_field": "$66(A6)",
            "vram_base": f"0x{boss_vram:04X}",
            "child_slots_spawned": 5,
            "child_slot_stride_bytes": 0xBC,
            "child_init_helper": "0x00E66E",
        },
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} ({len(aabb_boxes)} AABB facings)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
