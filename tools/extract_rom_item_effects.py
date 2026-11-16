#!/usr/bin/env python3
"""
extract_rom_item_effects.py — Milestone M15.266

Extracts ROM semantic knowledge for the Consumable Item & Spirit Summon
Usage Engine (`0x00482A`, `0x005236..0x005472`):
1. Item Eligibility Check (`0x005236`):
   - Rejects usage in Area `0x10` (`$FF0E40`).
   - Spirit Rings (`0x1A..0x1D`): checks `$FF16F1:0 == 0`, `$FF1AA4 == 0`,
     `$FF1A76 != 0` (SP > 0), and `btst (id - 0x1A), $FF0D7A`.
   - HP Food (`0x1E..0x21`): checks `$FF1A70 != $FF1A72` (Max HP != HP).
   - SP Potions (`0x22..0x25`): checks `$FF1A74 != $FF1A76` (Max SP != SP).
   - Dual Elixirs (`0x26..0x2A`): checks HP < MaxHP or SP < MaxSP.
2. Item Effect Jump Table (`0x00532C`, 17 entries for IDs `0x1A..0x2A`):
   - Spirit Rings (`0x1A..0x1D`): restore SP, set `$FF19A6 = 0..3`, call `0x007E9E`.
   - HP/SP/Dual Items (`0x1E..0x2A`): exact 25%/50%/75%/100% MaxHP/MaxSP formulas
     with clamping and SFX `0xA00234`.
3. Key Item / Non-Discardable Attribute Table (`0x05CE16` bit 5 at `0x0048C8`).
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_item_effects_m15_266.json"

ITEM_JUMP_TABLE_ADDR = 0x00532C
ITEM_COUNT = 17  # IDs 0x1A..0x2A

EXPECTED_HANDLERS = {
    "0x1A": 0x005378,
    "0x1B": 0x00537C,
    "0x1C": 0x005380,
    "0x1D": 0x005384,
    "0x1E": 0x0053A2,
    "0x1F": 0x0053AE,
    "0x20": 0x0053BA,
    "0x21": 0x0053CC,
    "0x22": 0x0053FA,
    "0x23": 0x005406,
    "0x24": 0x005412,
    "0x25": 0x005424,
    "0x26": 0x005452,
    "0x27": 0x00545A,
    "0x28": 0x005462,
    "0x29": 0x00546A,
    "0x2A": 0x00546A,
}


def read_u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def read_s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def read_u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_contracts(rom: bytes) -> dict:
    """Verify M68k opcodes for eligibility, summon, and heal SFX."""
    # 0x00523A: cmpi.b #$10, $ff0e40.l -> 0C39 0010 00FF 0E40
    assert read_u16_be(rom, 0x00523A) == 0x0C39
    assert read_u16_be(rom, 0x00523C) == 0x0010
    assert read_u32_be(rom, 0x00523E) == 0x00FF0E40

    # 0x0053EC: move.l #$a00234, d0 -> 203C 00A0 0234
    assert read_u16_be(rom, 0x0053EC) == 0x203C
    heal_sfx = read_u32_be(rom, 0x0053EE)
    assert heal_sfx == 0x00A00234

    # 0x004862: move.l #$a00256, d0 -> 203C 00A0 0256 (consume SFX)
    assert read_u16_be(rom, 0x004862) == 0x203C
    consume_sfx = read_u32_be(rom, 0x004864)
    assert consume_sfx == 0x00A00256

    return {
        "blocked_area_id": "0x10",
        "heal_sfx": f"0x{heal_sfx:06X}",
        "consume_sfx": f"0x{consume_sfx:06X}",
        "attribute_table_addr": "0x05CE16",
        "selected_slot_ram": "0x00FF0E51",
        "queued_item_ram": "0x00FF1864",
    }


def main() -> int:
    rom_path = Path(ROM_PATH_DEFAULT)
    out_path = Path(OUTPUT_PATH_DEFAULT)
    if not rom_path.exists():
        print(f"Error: ROM not found at {rom_path}")
        return 1

    rom = rom_path.read_bytes()

    jump_targets = {}
    for i in range(ITEM_COUNT):
        item_id = 0x1A + i
        key = f"0x{item_id:02X}"
        off = read_s16_be(rom, ITEM_JUMP_TABLE_ADDR + i * 2)
        tgt = ITEM_JUMP_TABLE_ADDR + i * 2 + off
        assert tgt == EXPECTED_HANDLERS[key], f"Item {key}: got 0x{tgt:06X}"
        jump_targets[key] = f"0x{tgt:06X}"

    contracts = verify_rom_contracts(rom)

    evidence = {
        "milestone": "M15.266",
        "description": "ROM Consumable Item Effects (0x1A..0x2A), Eligibility Rules (0x005236), and Spirit Summon Dispatch (0x00530C)",
        "eligibility_routine": "0x005236",
        "effect_dispatcher_routine": "0x00530C",
        "jump_table_addr": f"0x{ITEM_JUMP_TABLE_ADDR:06X}",
        "jump_targets": jump_targets,
        "item_effects": {
            "0x1A": {"category": "spirit_ring", "spirit_index": 0, "spirit": "Dytto", "handler": "0x005378"},
            "0x1B": {"category": "spirit_ring", "spirit_index": 1, "spirit": "Efreet", "handler": "0x00537C"},
            "0x1C": {"category": "spirit_ring", "spirit_index": 2, "spirit": "Shade", "handler": "0x005380"},
            "0x1D": {"category": "spirit_ring", "spirit_index": 3, "spirit": "Bow", "handler": "0x005384"},
            "0x1E": {"category": "hp_heal", "hp_pct": 25, "sp_pct": 0, "handler": "0x0053A2"},
            "0x1F": {"category": "hp_heal", "hp_pct": 50, "sp_pct": 0, "handler": "0x0053AE"},
            "0x20": {"category": "hp_heal", "hp_pct": 75, "sp_pct": 0, "handler": "0x0053BA"},
            "0x21": {"category": "hp_heal", "hp_pct": 100, "sp_pct": 0, "handler": "0x0053CC"},
            "0x22": {"category": "sp_heal", "hp_pct": 0, "sp_pct": 25, "handler": "0x0053FA"},
            "0x23": {"category": "sp_heal", "hp_pct": 0, "sp_pct": 50, "handler": "0x005406"},
            "0x24": {"category": "sp_heal", "hp_pct": 0, "sp_pct": 75, "handler": "0x005412"},
            "0x25": {"category": "sp_heal", "hp_pct": 0, "sp_pct": 100, "handler": "0x005424"},
            "0x26": {"category": "dual_heal", "hp_pct": 25, "sp_pct": 25, "handler": "0x005452"},
            "0x27": {"category": "dual_heal", "hp_pct": 50, "sp_pct": 50, "handler": "0x00545A"},
            "0x28": {"category": "dual_heal", "hp_pct": 75, "sp_pct": 75, "handler": "0x005462"},
            "0x29": {"category": "dual_heal", "hp_pct": 100, "sp_pct": 100, "handler": "0x00546A"},
            "0x2A": {"category": "dual_heal", "hp_pct": 100, "sp_pct": 100, "handler": "0x00546A"},
        },
        "verified_rom_contracts": contracts,
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} ({len(jump_targets)} items)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
