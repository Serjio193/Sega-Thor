#!/usr/bin/env python3
"""
extract_rom_sram_and_inventory.py — Milestone M15.265

Extracts ROM semantic knowledge for the SRAM Save/Load System (0x001182..0x001360)
and Persistent WRAM Inventory/Progression Layout ($FF0BFE..$FF0E51):
1. SRAM Slot Table (0x001182..0x001298):
   - 4 Primary + 4 Backup odd-byte SRAM slots ($200009..$202111, stride 0x4B8)
   - SRAM lock/unlock register $A130F1 (0x03 unlock, 0x02 lock)
   - Valid slot bitmask $FF10AA and last-used slot $2025C9 -> $FF10A8 (<= 3)
2. SRAM Validation & Checksum (0x00129A..0x0012FE):
   - 6-byte magic signature at ROM 0x001754 (1A BC 00 02 2A 4E)
   - 596-byte (0x254) WRAM persistence block $FF0BFE..$FF0E51
   - 16-bit unsigned byte-sum checksum + automatic backup-to-primary repair (0x0012E8)
3. Persistent Inventory & Stats Layout (0x001300..0x00150E):
   - $FF0D7A active spirit flags, $FF0D82 7 weapon slots, $FF0D90 16 item slots
   - $FF0DBC global flags (32B), $FF0E1C area flags (16B), $FF0E2C room flags (16B)
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_sram_and_inventory_m15_265.json"

MAGIC_ADDR = 0x001754
MAGIC_BYTES_HEX = "1abc00022a4e"


def read_u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def read_u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_sram_and_inventory_contracts(rom: bytes) -> dict:
    """Verify M68k opcodes and constants directly against the canonical ROM."""
    # 1. Magic signature at 0x001754 (6 bytes)
    magic = rom[MAGIC_ADDR : MAGIC_ADDR + 6]
    assert magic.hex() == MAGIC_BYTES_HEX, f"Unexpected magic: {magic.hex()}"

    # 2. 0x0012B2: move.w #$129, d1 -> 298 word-pairs = 596 payload bytes
    # 0x0012B2: 323C 0129
    assert read_u16_be(rom, 0x0012B2) == 0x323C
    word_count_minus_1 = read_u16_be(rom, 0x0012B4)
    assert word_count_minus_1 == 0x0129
    payload_bytes = (word_count_minus_1 + 1) * 2  # 596 bytes

    # 3. 0x001300: lea $ff0bfe.l, a0 (41F9 00FF 0BFE)
    assert read_u16_be(rom, 0x001300) == 0x41F9
    wram_base = read_u32_be(rom, 0x001302)
    assert wram_base == 0x00FF0BFE

    # 4. 0x001188: lea $a130f1.l, a5 (4BF9 00A1 30F1)
    assert read_u16_be(rom, 0x001188) == 0x4BF9
    sram_ctrl = read_u32_be(rom, 0x00118A)
    assert sram_ctrl == 0x00A130F1

    # 5. Extract 4 primary & 4 backup SRAM slot addresses from 0x001194..0x00124C
    # lea <addr>.l, a0 -> 41F9 <u32>
    slot_addrs = [
        (read_u32_be(rom, 0x001196), read_u32_be(rom, 0x0011AC)),
        (read_u32_be(rom, 0x0011CA), read_u32_be(rom, 0x0011E0)),
        (read_u32_be(rom, 0x0011FE), read_u32_be(rom, 0x001214)),
        (read_u32_be(rom, 0x001232), read_u32_be(rom, 0x001248)),
    ]
    last_slot_sram = read_u32_be(rom, 0x001266)  # $2025C9

    # 6. Verify 7 weapon slot constants at 0x0014C0..0x0014D8
    # move.w #imm, (a0)+ -> 30FC <u16>
    weapon_words = []
    for i in range(7):
        pc = 0x0014C0 + i * 4
        assert read_u16_be(rom, pc) == 0x30FC
        weapon_words.append(f"0x{read_u16_be(rom, pc + 2):04X}")

    return {
        "magic_hex": magic.hex(),
        "payload_bytes": payload_bytes,
        "wram_base": f"0x{wram_base:08X}",
        "wram_end_inclusive": f"0x{wram_base + payload_bytes - 1:08X}",
        "sram_ctrl_reg": f"0x{sram_ctrl:08X}",
        "slots": [
            {"slot": i, "primary": f"0x{p:06X}", "backup": f"0x{b:06X}"}
            for i, (p, b) in enumerate(slot_addrs)
        ],
        "last_slot_sram": f"0x{last_slot_sram:06X}",
        "weapon_init_words": weapon_words,
    }


def main() -> int:
    rom_path = Path(ROM_PATH_DEFAULT)
    out_path = Path(OUTPUT_PATH_DEFAULT)
    if not rom_path.exists():
        print(f"Error: ROM not found at {rom_path}")
        return 1

    rom = rom_path.read_bytes()
    contracts = verify_sram_and_inventory_contracts(rom)

    evidence = {
        "milestone": "M15.265",
        "description": "ROM SRAM Save/Load System (0x001182..0x001360) and Persistent WRAM Inventory/Flags Layout",
        "sram_system": {
            "slot_scanner_routine": "0x001182",
            "slot_validator_routine": "0x00129A",
            "slot_repair_copy_routine": "0x0012E8",
            "new_game_clear_routine": "0x001300",
            "sram_ctrl_reg": contracts["sram_ctrl_reg"],
            "sram_unlock_val": "0x03",
            "sram_lock_val": "0x02",
            "valid_slots_bitmask_ram": "0x00FF10AA",
            "last_slot_ram": "0x00FF10A8",
            "magic_addr": f"0x{MAGIC_ADDR:06X}",
            "magic_hex": contracts["magic_hex"],
            "payload_bytes": contracts["payload_bytes"],
            "checksum_type": "16-bit unsigned sum of 596 payload bytes",
            "slots": contracts["slots"],
            "last_slot_sram": contracts["last_slot_sram"],
        },
        "persistent_wram_block": {
            "base_addr": contracts["wram_base"],
            "end_addr_inclusive": contracts["wram_end_inclusive"],
            "size_bytes": contracts["payload_bytes"],
            "key_fields": {
                "0x00FF0D7A": "Active/unlocked spirit bitmask (bits 0..3: Dytto, Efreet, Shade, Bow)",
                "0x00FF0D7E": "Power / Score word (copied to Player +$06)",
                "0x00FF0D80": "Equipment / status word (0x01FF on init)",
                "0x00FF0D82": "7 Weapon Slots (2 bytes each: (weapon_id << 8) | ammo_or_0xFE)",
                "0x00FF0D90": "16 Item/Consumable Slots (2 bytes each: item_id, count)",
                "0x00FF0DB6": "4 Spirit Level bytes (0x0F0F0F0F on full unlock)",
                "0x00FF0DBA": "Spirit slot flags (bits 0..3)",
                "0x00FF0DBC": "Global Story Flags (32 bytes)",
                "0x00FF0E1C": "Area Flags (16 bytes, cleared on area change)",
                "0x00FF0E2C": "Room Flags (16 bytes, cleared on room change)",
                "0x00FF0E40": "Current Room ID (0xMMRR)",
            },
        },
        "inventory_defaults_0x001450": {
            "weapon_slots_count": 7,
            "weapon_init_words": contracts["weapon_init_words"],
            "item_slots_count": 16,
            "item_ids_range": "0x1A..0x28 plus 0x2A",
        },
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} ({len(json.dumps(evidence))} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
