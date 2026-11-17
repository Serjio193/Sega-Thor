#!/usr/bin/env python3
"""
extract_rom_ali_item_drop_button_dispatch.py — M15.387: ROM Ali Item Drop Spawner, 8-Slot Allocation Probe, Input Ring Sampler & Master Button Dispatcher
Disassembles and validates Beyond Oasis (USA) ROM routines at 0x007C3C..0x007E46:
  - 0x007C3C..0x007CC4: Item drop entity spawner (entity type 8, 60-frame countdown 0x3C, coordinate clone, state restoration & 0x62CC transition)
  - 0x007CC6..0x007D22: Item drop rejection audio gate (state 0x3C, anim 0x44, SFX 0xC00255)
  - 0x007D24..0x007D88: 8-slot entity allocation probe ($FF2ABC stride 0x5A, classification table 0x05CE16, distance check 0xE5AE)
  - 0x007D8A..0x007DC6: Multi-frame input ring buffer sampler ($FF1668, head $FF16E8, count $FF16EA, wrap 0x7E)
  - 0x007DC8..0x007E46: Master player button dispatcher (Jump bit 6 -> 0x7E46, Attack bit 4 -> 0x7F82, Latched attack -> 0x7FB6, Spirit bit 5 -> 0x822C)
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_ali_item_drop_button_dispatch_m15_387.json"


def u16(buf, offset):
    return struct.unpack_from(">H", buf, offset)[0]


def u32(buf, offset):
    return struct.unpack_from(">I", buf, offset)[0]


def extract_evidence():
    if not os.path.exists(ROM_PATH):
        print(f"Error: ROM not found at {ROM_PATH}")
        sys.exit(1)

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    # 0x007C3C: Item drop entity spawn
    assert u16(rom, 0x007C3C) == 0x082E  # btst.b #1, $37(a6)
    assert u16(rom, 0x007C48) == 0x2A4E  # movea.l a6, a5
    assert u16(rom, 0x007C58) == 0x3D7C  # move.w #8, 0(a6)
    assert u16(rom, 0x007C5A) == 0x0008
    assert u16(rom, 0x007C68) == 0x1039  # move.b $FF1976.l, d0
    assert u32(rom, 0x007C6A) == 0x00FF1976
    assert u16(rom, 0x007C82) == 0x3D7C  # move.w #$3c, $22(a6) (60-frame timer)
    assert u16(rom, 0x007C84) == 0x003C
    assert u16(rom, 0x007C90) == 0x2D6D  # move.l 8(a5), 8(a6) (X clone)
    assert u16(rom, 0x007C96) == 0x2D6D  # move.l $c(a5), $c(a6) (Y clone)
    assert u16(rom, 0x007CAA) == 0x2C4D  # movea.l a5, a6
    assert u16(rom, 0x007CAC) == 0x0239  # andi.b #$f9, $FF17B8.l
    assert u16(rom, 0x007CBC) == 0x3D7C  # move.w #$ffff, $5c(a6)
    assert u16(rom, 0x007CBE) == 0xFFFF

    # 0x007CC6: Drop reject buzzer
    assert u16(rom, 0x007D04) == 0x3D7C  # move.w #$44, $2a(a6)
    assert u16(rom, 0x007D0A) == 0x3D7C  # move.w #$3c, $4(a6)
    assert u32(rom, 0x007D10) == 0x203C00C0  # move.l #$c00255, d0
    assert u16(rom, 0x007D14) == 0x0255

    # 0x007D24: Allocation probe
    assert u16(rom, 0x007D24) == 0x43F9  # lea $5CE16.l, a1
    assert u32(rom, 0x007D26) == 0x0005CE16
    assert u16(rom, 0x007D2A) == 0x4DF9  # lea $FF2ABC.l, a6
    assert u32(rom, 0x007D2C) == 0x00FF2ABC
    assert u16(rom, 0x007D30) == 0x303C  # move.w #$694, d0
    assert u16(rom, 0x007D32) == 0x0694
    assert u16(rom, 0x007D34) == 0x7A07  # moveq #7, d5 (8 slots)
    assert u16(rom, 0x007D4C) == 0x4DEE  # lea $5a(a6), a6 (stride 90 bytes)
    assert u16(rom, 0x007D4E) == 0x005A

    # 0x007D8A: Multi-frame ring buffer sampler
    assert u16(rom, 0x007D8C) == 0x4A39  # tst.b $FF1983.l
    assert u32(rom, 0x007D8E) == 0x00FF1983
    assert u16(rom, 0x007D96) == 0x41F9  # lea $FF1668.l, a0
    assert u32(rom, 0x007D98) == 0x00FF1668
    assert u16(rom, 0x007D9C) == 0x3A39  # move.w $FF16E8.l, d5 (head)
    assert u16(rom, 0x007DA2) == 0x3839  # move.w $FF16EA.l, d4 (count)
    assert u16(rom, 0x007DBC) == 0x0245  # andi.w #$7e, d5 (wrap 126)
    assert u16(rom, 0x007DBE) == 0x007E

    # 0x007DC8: Master player button dispatcher
    assert u16(rom, 0x007DE6) == 0x50F9  # st.b $FF1986.l
    assert u32(rom, 0x007DE8) == 0x00FF1986
    assert u16(rom, 0x007E04) == 0x0C44  # cmpi.w #4, d4 (max 5 frames)
    assert u16(rom, 0x007E06) == 0x0004
    assert u16(rom, 0x007E20) == 0x0800  # btst.b #6, d0 (Button C: Jump)
    assert u16(rom, 0x007E22) == 0x0006
    assert u16(rom, 0x007E28) == 0x0800  # btst.b #4, d0 (Button B: Attack)
    assert u16(rom, 0x007E2A) == 0x0004
    assert u16(rom, 0x007E30) == 0x0839  # btst.b #4, $FF165E.l (Latched attack)
    assert u32(rom, 0x007E34) == 0x00FF165E
    assert u16(rom, 0x007E3C) == 0x0800  # btst.b #5, d0 (Button A: Spirit)
    assert u16(rom, 0x007E3E) == 0x0005
    assert u16(rom, 0x007E44) == 0x4E75  # rts

    evidence = {
        "milestone": "M15.387",
        "description": "ROM Ali Item Drop Spawner, 8-Slot Allocation Probe, Input Ring Sampler & Master Button Dispatcher",
        "address_range": ["0x007C3C", "0x007E46"],
        "byte_count": 0x007E46 - 0x007C3C,
        "routines": [
            {
                "name": "ali_item_drop_spawn",
                "start": "0x007C3C",
                "end": "0x007CC4",
                "purpose": "Allocates world entity type 8 for item drop with 60-frame timer, clones player coordinates, and returns to idle/walk 0x62CC"
            },
            {
                "name": "ali_item_drop_reject",
                "start": "0x007CC6",
                "end": "0x007D22",
                "purpose": "Plays full bag / drop rejection buzzer SFX 0xC00255 with state 0x3C and anim 0x44"
            },
            {
                "name": "ali_item_slot_probe",
                "start": "0x007D24",
                "end": "0x007D88",
                "purpose": "Scans 8 entity slots at $FF2ABC (stride 90 bytes) against table 0x05CE16 to verify drop eligibility"
            },
            {
                "name": "ali_input_ring_sample",
                "start": "0x007D8A",
                "end": "0x007DC6",
                "purpose": "Samples and ORs composite button masks across d0 frames from input ring buffer at $FF1668 with wrap 0x7E"
            },
            {
                "name": "ali_master_button_dispatcher",
                "start": "0x007DC8",
                "end": "0x007E46",
                "purpose": "Evaluates 5 recent input frames and dispatches Jump (bit 6), Attack (bit 4), Latched Attack ($FF165E), and Spirit (bit 5)"
            }
        ]
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Evidence written to {OUTPUT_JSON} ({len(evidence['routines'])} routines)")


if __name__ == "__main__":
    extract_evidence()
