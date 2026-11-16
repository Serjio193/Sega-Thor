#!/usr/bin/env python3
"""
extract_rom_entity_master_tables.py — Milestone M15.264

Extracts the universal ROM Entity Master Tables and Damage Pipeline for
Standalone engine scaling across all rooms and enemy types:
1. Entity Update Dispatch Table (0x00DF54..0x00E0B8):
   - Covers Type 0x0016..0x00C6 (89 even-numbered entity types).
   - Entry address = 0x00DF54 - 0x2C + type_id * 2 (32-bit big-endian handler PC).
2. 32-Byte Entity Descriptor Table (0x03F2FA, parsed by 0x008D06..0x008D88):
   - Covers Type 0x0002..0x00C6 (99 descriptors).
   - Base address = 0x03F2FA - 0x20 + (type_id << 4).
   - Extracts hitbox/Z dimensions, render priority, mass, flags, 4 animation
     table pointers (+0x0C..+0x18), and AI/script pointer (+0x1C -> $5E(A6)).
3. Universal Damage & Elemental Bit-13 Pipeline (0x017E30..0x017E9E):
   - Reads incoming damage word from $86(A6), tests bit 13 (elemental flag,
     sets $39(A6) bit 3 and plays SFX 0x8004D9), masks magnitude with 0x0FFF,
     subtracts entity defense $92(A6) (clamped >= 1), updates HP $8A(A6), and
     triggers death State 2 via 0x008EF8 when HP reaches 0.
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_entity_master_tables_m15_264.json"

UPDATE_DISPATCH_TABLE_ADDR = 0x00DF54
DESCRIPTOR_TABLE_ADDR = 0x03F2FA
FIRST_DISPATCH_TYPE = 0x0016
LAST_DISPATCH_TYPE = 0x00C6
FIRST_DESCRIPTOR_TYPE = 0x0002


def read_u8(rom: bytes, addr: int) -> int:
    return rom[addr]


def read_u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def read_u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def decode_descriptor(rom: bytes, type_id: int) -> dict:
    """Decode the 32-byte entity descriptor at 0x03F2FA - 0x20 + (type_id << 4)."""
    addr = DESCRIPTOR_TABLE_ADDR - 0x20 + (type_id << 4)
    return {
        "descriptor_addr": f"0x{addr:06X}",
        "z_radius_4a": read_u16_be(rom, addr + 0x00),
        "xy_radius_46": read_u8(rom, addr + 0x02),
        "dim_x_48": read_u8(rom, addr + 0x03),
        "dim_y_49": read_u8(rom, addr + 0x04),
        "render_prio_3d": read_u8(rom, addr + 0x05),
        "mass_4c": read_u8(rom, addr + 0x06),
        "flags_3a": f"0x{read_u8(rom, addr + 0x07):02X}",
        "flags_36": f"0x{read_u8(rom, addr + 0x08):02X}",
        "flags_37": f"0x{read_u8(rom, addr + 0x09):02X}",
        "flags_38": f"0x{read_u8(rom, addr + 0x0A):02X}",
        "flags_3c": f"0x{read_u8(rom, addr + 0x0B):02X}",
        "anim_ptr_1a": f"0x{read_u32_be(rom, addr + 0x0C):08X}",
        "anim_ptr_1e": f"0x{read_u32_be(rom, addr + 0x10):08X}",
        "anim_ptr_26": f"0x{read_u32_be(rom, addr + 0x14):08X}",
        "anim_ptr_22": f"0x{read_u32_be(rom, addr + 0x18):08X}",
        "ai_ptr_5e": f"0x{read_u32_be(rom, addr + 0x1C):08X}",
    }


def verify_damage_pipeline(rom: bytes) -> dict:
    """Verify M68k opcodes of the universal damage & elemental routine (0x017E30)."""
    # 0x017E4A: btst.b #$d, d0 -> 0800 000D
    assert read_u16_be(rom, 0x017E4A) == 0x0800
    assert read_u16_be(rom, 0x017E4C) == 0x000D
    # 0x017E58: move.l #$8004d9, d0 -> 203C 0080 04D9
    assert read_u16_be(rom, 0x017E58) == 0x203C
    sfx = read_u32_be(rom, 0x017E5A)
    assert sfx == 0x008004D9
    # 0x017E66: andi.w #$fff, d0 -> 0240 0FFF
    assert read_u16_be(rom, 0x017E66) == 0x0240
    dmg_mask = read_u16_be(rom, 0x017E68)
    assert dmg_mask == 0x0FFF
    # 0x017E6A: sub.w $92(a6), d0 -> 906E 0092
    assert read_u16_be(rom, 0x017E6A) == 0x906E
    assert read_u16_be(rom, 0x017E6C) == 0x0092

    return {
        "routine_addr": "0x017E30",
        "incoming_damage_field": "$86(A6)",
        "elemental_bit_test": 13,
        "elemental_status_field": "$39(A6) bit 3",
        "elemental_sfx": f"0x{sfx:06X}",
        "damage_magnitude_mask": f"0x{dmg_mask:04X}",
        "defense_field": "$92(A6)",
        "min_damage_clamp": 1,
        "hp_field": "$8A(A6)",
        "death_state": 2,
        "death_helper": "0x008EF8",
    }


def main() -> int:
    rom_path = Path(ROM_PATH_DEFAULT)
    out_path = Path(OUTPUT_PATH_DEFAULT)
    if not rom_path.exists():
        print(f"Error: ROM not found at {rom_path}")
        return 1

    rom = rom_path.read_bytes()

    # 1. Extract all 89 update handlers from 0x00DF54..0x00E0B8
    update_handlers = {}
    for tid in range(FIRST_DISPATCH_TYPE, LAST_DISPATCH_TYPE + 2, 2):
        entry = UPDATE_DISPATCH_TABLE_ADDR - 0x2C + tid * 2
        ptr = read_u32_be(rom, entry)
        assert 0x000100 <= ptr < 0x080000, f"Invalid pointer at 0x{tid:04X}: 0x{ptr:08X}"
        update_handlers[f"0x{tid:04X}"] = f"0x{ptr:06X}"

    # Verify sentinel RTS (0x4E75) immediately follows Type 0x00C6 at 0x00E0B8
    assert read_u16_be(rom, 0x00E0B8) == 0x4E75

    # 2. Extract all 99 descriptors (0x0002..0x00C6) from 0x03F2FA
    descriptors = {}
    for tid in range(FIRST_DESCRIPTOR_TYPE, LAST_DISPATCH_TYPE + 2, 2):
        key = f"0x{tid:04X}"
        desc = decode_descriptor(rom, tid)
        if key in update_handlers:
            desc["update_handler"] = update_handlers[key]
        descriptors[key] = desc

    damage_pipeline = verify_damage_pipeline(rom)

    evidence = {
        "milestone": "M15.264",
        "description": "ROM Entity Master Update Dispatch Table (0x00DF54), 32-Byte Descriptors (0x03F2FA), and Damage/Elemental Pipeline (0x017E30)",
        "update_dispatch_table": {
            "address": f"0x{UPDATE_DISPATCH_TABLE_ADDR:06X}",
            "end_sentinel_rts": "0x00E0B8",
            "first_type": f"0x{FIRST_DISPATCH_TYPE:04X}",
            "last_type": f"0x{LAST_DISPATCH_TYPE:04X}",
            "count": len(update_handlers),
            "handlers": update_handlers,
        },
        "descriptor_table": {
            "address": f"0x{DESCRIPTOR_TABLE_ADDR:06X}",
            "allocator": "0x008D06",
            "stride_bytes": 32,
            "count": len(descriptors),
            "entities": descriptors,
        },
        "damage_pipeline": damage_pipeline,
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} ({len(descriptors)} descriptors, {len(update_handlers)} handlers)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
