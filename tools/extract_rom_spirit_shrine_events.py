#!/usr/bin/env python3
"""
extract_rom_spirit_shrine_events.py — Milestone M15.261

Extracts ROM semantic knowledge for:
1. Spirit Awakening Dispatcher at 0x018CAA
2. All 5 Spirit / Shrine Awakening Handlers:
   - Dytto (Water Shrine, Room 0x0106, 0x018CDE)
   - Efreet (Fire Shrine, Room 0x0206, 0x018D4E)
   - Shade (Shadow Shrine, Room 0x0805, 0x018DAA)
   - Bow (Plant Shrine, Room 0x0A10, 0x018E12)
   - Agito (Seal Chamber, Room 0x090B, 0x018E7A)
3. Altar Entity 0x0020 Spawn Gating & Global Flags ($FF0DBC)
4. Active Spirit Selection ($FF0D7A) and Area Clear Flags ($FF0E1D)
"""

import json
import os
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_spirit_shrine_events_m15_261.json"

MASTER_SPAWN_TABLE_ADDR = 0x04371E
DISPATCHER_ADDR = 0x018CAA
DIALOGUE_TABLE_ADDR = 0x051514

SHRINE_DEFINITIONS = [
    {
        "spirit_name": "Dytto",
        "element": "Water",
        "room_id": 0x0106,
        "handler_addr": 0x018CDE,
        "handler_end": 0x018D4E,
        "spawn_helper_addr": 0x014668,
        "dialogue_msg_id": 0,
        "global_flag_idx": 3,
        "global_flag_addr": 0x00FF0DBC,
        "global_flag_bit": 3,
        "active_spirit_bit": 0,
        "area_flag_addr": 0x00FF0E1D,
        "area_flag_bit": 2,
        "altar_entity_type": 0x0020,
        "altar_vram_tile": 0x0200,
        "prop_slot_ptr": 0x00FF1CD8,
        "elevation_delta_px": 16,
    },
    {
        "spirit_name": "Efreet",
        "element": "Fire",
        "room_id": 0x0206,
        "handler_addr": 0x018D4E,
        "handler_end": 0x018DAA,
        "spawn_helper_addr": 0x0146D6,
        "dialogue_msg_id": 9,
        "global_flag_idx": 27,
        "global_flag_addr": 0x00FF0DBF,
        "global_flag_bit": 3,
        "active_spirit_bit": 1,
        "area_flag_addr": None,
        "area_flag_bit": None,
        "altar_entity_type": 0x0020,
        "altar_vram_tile": 0x2200,
        "prop_slot_ptr": 0x00FF1F0C,
        "elevation_delta_px": 8,
    },
    {
        "spirit_name": "Shade",
        "element": "Shadow",
        "room_id": 0x0805,
        "handler_addr": 0x018DAA,
        "handler_end": 0x018E12,
        "spawn_helper_addr": 0x014742,
        "dialogue_msg_id": 12,
        "global_flag_idx": 63,
        "global_flag_addr": 0x00FF0DC3,
        "global_flag_bit": 7,
        "active_spirit_bit": 2,
        "area_flag_addr": None,
        "area_flag_bit": None,
        "altar_entity_type": 0x0020,
        "altar_vram_tile": 0x2200,
        "prop_slot_ptr": 0x00FF1CD8,
        "elevation_delta_px": 16,
    },
    {
        "spirit_name": "Bow",
        "element": "Plant",
        "room_id": 0x0A10,
        "handler_addr": 0x018E12,
        "handler_end": 0x018E7A,
        "spawn_helper_addr": 0x014798,
        "dialogue_msg_id": 13,
        "global_flag_idx": 20,
        "global_flag_addr": 0x00FF0DBE,
        "global_flag_bit": 4,
        "active_spirit_bit": 3,
        "area_flag_addr": None,
        "area_flag_bit": None,
        "altar_entity_type": 0x0020,
        "altar_vram_tile": 0x2200,
        "prop_slot_ptr": 0x00FF1CD8,
        "elevation_delta_px": 0,
    },
    {
        "spirit_name": "Agito",
        "element": "Dark/Seal",
        "room_id": 0x090B,
        "handler_addr": 0x018E7A,
        "handler_end": 0x018EBC,
        "spawn_helper_addr": 0x00F53C,
        "dialogue_msg_id": 56,
        "global_flag_idx": 54,
        "global_flag_addr": 0x00FF0DC2,
        "global_flag_bit": 6,
        "active_spirit_bit": None,
        "area_flag_addr": None,
        "area_flag_bit": None,
        "altar_entity_type": 0x0020,
        "altar_vram_tile": 0x2200,
        "prop_slot_ptr": 0x00FF1CD8,
        "elevation_delta_px": 0,
    },
]


def extract_dialogue_text(rom: bytes, msg_id: int) -> str:
    entry_addr = DIALOGUE_TABLE_ADDR + msg_id * 2
    offset = struct.unpack_from(">h", rom, entry_addr)[0]
    text_addr = entry_addr + offset
    chars = []
    i = text_addr
    while i < len(rom):
        b = rom[i]
        if b == 0x00:
            break
        elif b == 0x01:
            chars.append("\n")
        elif b == 0x07:
            chars.append("\n--- PAGE ---\n")
        elif 32 <= b < 127:
            chars.append(chr(b))
        i += 1
    return "".join(chars)


def extract_shrine_spawn_record(rom: bytes, room_id: int, flag_idx: int) -> dict:
    area = room_id >> 8
    room_num = room_id & 0xFF
    area_table_ptr = struct.unpack_from(">I", rom, MASTER_SPAWN_TABLE_ADDR + area * 4)[0]
    room_entry = area_table_ptr + room_num * 2
    disp = struct.unpack_from(">h", rom, room_entry)[0]
    script_addr = room_entry + disp

    for i in range(script_addr, script_addr + 300):
        if rom[i] == 0x15:
            bank = rom[i + 1]
            etype = struct.unpack_from(">H", rom, i + 2)[0]
            flag = struct.unpack_from(">H", rom, i + 4)[0]
            if etype == 0x0020 and flag == flag_idx:
                x = struct.unpack_from(">h", rom, i + 10)[0]
                y = struct.unpack_from(">h", rom, i + 12)[0]
                z = struct.unpack_from(">h", rom, i + 14)[0]
                vram = struct.unpack_from(">H", rom, i + 16)[0]
                return {
                    "script_addr": f"0x{script_addr:06X}",
                    "opcode_offset": f"0x{i:06X}",
                    "bank": bank,
                    "entity_type": f"0x{etype:04X}",
                    "flag_index": flag,
                    "position": {"x": x, "y": y, "z": z},
                    "vram_tile_base": f"0x{vram:04X}",
                }
    return {}


def main() -> int:
    rom_path = Path(ROM_PATH_DEFAULT)
    out_path = Path(OUTPUT_PATH_DEFAULT)

    if not rom_path.exists():
        print(f"Error: ROM file not found at {rom_path}")
        return 1

    rom = rom_path.read_bytes()
    print(f"Loaded ROM: {len(rom)} bytes")

    evidence = {
        "milestone": "M15.261",
        "description": "ROM Spirit Awakening Dispatcher, Shrine Events, and Global Flag 3 Extraction",
        "dispatcher": {
            "address": f"0x{DISPATCHER_ADDR:06X}",
            "caller_trigger_address": "0x008392",
            "caller_condition": "entity_type == 0x0020 (Altar / Prop)",
            "room_id_ram_address": "0x00FF0E40",
            "branches": [
                {"room_id": f"0x{s['room_id']:04X}", "target": f"0x{s['handler_addr']:06X}", "spirit": s["spirit_name"]}
                for s in SHRINE_DEFINITIONS
            ],
        },
        "shrines": [],
        "architectural_notes": {
            "m16_50_correction": (
                "Standalone M16.50 previously mislabeled ROM 0x018CAA / Global Flag 3 as a 'Princess Rescue' cutscene. "
                "Static ROM analysis definitively proves that 0x018CAA is the Spirit Awakening Dispatcher, "
                "and Global Flag 3 ($FF0DBC, bit 3) is the Dytto Water Spirit acquisition flag. "
                "Altar trigger entity 0x0020 in Room 0x0106 gates spawning on Global Flag 3."
            ),
            "state_transition_sequence": [
                "1. Entity 0x0020 proximity / button interaction triggers 0x008352..0x008392.",
                "2. Dispatcher 0x018CAA reads room ID from $FF0E40 and jumps to shrine handler.",
                "3. Handler freezes trigger entity by setting state 0x3F in $32(A6).",
                "4. JSR 0x00C2EC starts authentic dialogue (Msg 0 for Dytto).",
                "5. Coordinates and orientation read from prop slot ($FF1CD8 or $FF1F0C).",
                "6. Prop entity deactivated by clearing $00(prop).",
                "7. BSR spawn helper creates spirit entity attached to player base ($FF1AA4).",
                "8. Spirit entity state set to 0x1A (APPEAR), action/anim to 0x22.",
                "9. Area flag set if applicable (e.g. $FF0E1D bit 2 for Water Shrine).",
                "10. Global flag set ($FF0DBC bit 3 for Dytto).",
                "11. Active spirit flag set ($FF0D7A bit 0 for Dytto).",
                "12. Returns with carry set (ORI.B #1, CCR; RTS).",
            ],
        },
    }

    for s in SHRINE_DEFINITIONS:
        dialogue = extract_dialogue_text(rom, s["dialogue_msg_id"])
        spawn_rec = extract_shrine_spawn_record(rom, s["room_id"], s["global_flag_idx"])
        shrine_entry = {
            "spirit_name": s["spirit_name"],
            "element": s["element"],
            "room_id": f"0x{s['room_id']:04X}",
            "handler_address": f"0x{s['handler_addr']:06X}",
            "handler_end_address": f"0x{s['handler_end']:06X}",
            "spawn_helper_address": f"0x{s['spawn_helper_addr']:06X}",
            "dialogue_message_id": s["dialogue_msg_id"],
            "dialogue_text": dialogue,
            "global_flag": {
                "index": s["global_flag_idx"],
                "ram_address": f"0x{s['global_flag_addr']:08X}",
                "bit": s["global_flag_bit"],
            },
            "active_spirit_bit": s["active_spirit_bit"],
            "area_flag": {
                "ram_address": f"0x{s['area_flag_addr']:08X}" if s["area_flag_addr"] else None,
                "bit": s["area_flag_bit"],
            },
            "altar_spawn_record": spawn_rec,
        }
        evidence["shrines"].append(shrine_entry)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} ({len(json.dumps(evidence))} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
