#!/usr/bin/env python3
"""
extract_rom_interaction_dispatcher.py — Milestone M15.275

Extracts ROM semantic knowledge for the Universal Prop / NPC / Shrine /
Signpost Interaction Dispatcher (`0x008330..0x0083D2`) and Story NPC
Dialogue Handler (`0x018EBE..0x018F48`):
1. Proximity & Facing Gate (`0x008330..0x008350`):
   - `d5 = 0x10` (16 px) proximity via `0x00B852`, facing check via `0x00E338`.
2. Entity Type Dispatch (`0x008352..0x00839E`):
   - `0x0032` -> `0x018BBC` (Town NPCs / Village Elder)
   - `0x0020` -> `0x018CAA` (Spirit Awakening Dispatcher)
   - `0x0094`, `0x0096`, `0x00BE` -> `0x018EBE` (Story NPCs / Conditional Dialogue)
   - `0x00B0` -> `0x00839E` (Signpost / Tablet: SFX `0xA00264`, freeze `0x3F`, `0x00C2EC`)
3. Story NPC Post-Dialogue Hooks (`0x018F12..0x018F38`):
   - Messages `0x24` -> `0x018FAA`, `0x39` -> `0x019048`, `0x2F` -> `0x018FF0`, `0x30` -> `0x01903C`.
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_interaction_dispatcher_m15_275.json"


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

    # Verify JMP targets at 0x00838C, 0x008392, 0x008398 (4EF9 <u32>)
    assert read_u16_be(rom, 0x00838C) == 0x4EF9
    tgt_0032 = read_u32_be(rom, 0x00838E)
    assert read_u16_be(rom, 0x008392) == 0x4EF9
    tgt_0020 = read_u32_be(rom, 0x008394)
    assert read_u16_be(rom, 0x008398) == 0x4EF9
    tgt_story = read_u32_be(rom, 0x00839A)

    # Verify Signpost 0x00B0 SFX 0xA00264 at 0x00839E (203C 00A0 0264)
    assert read_u16_be(rom, 0x00839E) == 0x203C
    signpost_sfx = read_u32_be(rom, 0x0083A0)

    evidence = {
        "milestone": "M15.275",
        "description": "ROM Universal Interaction Dispatcher (0x008330) and Story NPC Handler (0x018EBE)",
        "dispatcher_addr": "0x008330",
        "proximity_radius_px": 16,
        "proximity_helper": "0x00B852",
        "facing_helper": "0x00E338",
        "routes": {
            "0x0032": {"target": f"0x{tgt_0032:06X}", "role": "Town NPCs / Village Elder"},
            "0x0020": {"target": f"0x{tgt_0020:06X}", "role": "Spirit Awakening Dispatcher"},
            "0x0094": {"target": f"0x{tgt_story:06X}", "role": "Story NPC Dialogue"},
            "0x0096": {"target": f"0x{tgt_story:06X}", "role": "Story NPC Dialogue"},
            "0x00BE": {"target": f"0x{tgt_story:06X}", "role": "Story NPC Dialogue"},
            "0x00B0": {"target": "0x00839E", "role": "Signpost / Tablet", "sfx": f"0x{signpost_sfx:06X}"},
        },
        "story_npc_post_dialogue_hooks": {
            "0x24": "0x018FAA",
            "0x2F": "0x018FF0",
            "0x30": "0x01903C",
            "0x39": "0x019048",
        },
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} ({len(evidence['routes'])} entity routes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
