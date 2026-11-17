#!/usr/bin/env python3
"""
extract_rom_ali_input_seq_durability.py — M15.391: ROM Ali Input Sequence Pattern Matcher, Shockwave Spawner, Weapon Durability & Break SFX
Disassembles and validates Beyond Oasis (USA) ROM routines at 0x0083F8..0x0085E2:
  - 0x0083F8..0x00846A: 3-stage command motion pattern matcher (Press -> Release -> Re-press across 12 frames in $FF1668)
  - 0x00846C..0x0084FA: Ground slam shockwave projectile spawner (entity type 0x16 at $FF1AA4, table 0x0084FC, velocity 0xE2D4)
  - 0x0084FC..0x008504: 4-word shockwave orientation offset table [0xF800, 0x0B00, 0x0817, 0xF500]
  - 0x008504..0x00852E: Atomic VDP DMA ring enqueue ($FF1892, 0x9D2FE, 0x9F00, 0x390 with interrupt masking SR 0x0700)
  - 0x008530..0x0085C2: Weapon durability decrement & break handler (durability == 0xFF infinite; on 0 breaks weapon, clears slot, reverts to Dagger ID 1 / type 0x0A, 90f toast, SFX 0xC00269)
  - 0x0085C4..0x0085E2: Level-up EXP bonus drop trigger ($FF0E3C >= $FF1A70 sets $FF0DBB = 0xFF)
"""

import json
import os
import struct
import sys

ROM_PATH = "local-roms/Beyond Oasis (USA).md"
OUTPUT_JSON = "data/thor-evidence/rom_ali_input_seq_durability_m15_391.json"


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

    # 0x0083F8: Command motion sequence matcher
    assert u16(rom, 0x0083F8) == 0x7A00  # moveq #0, d5
    assert u16(rom, 0x0083FA) == 0x41F9  # lea $FF1668.l, a0
    assert u32(rom, 0x0083FC) == 0x00FF1668
    assert u16(rom, 0x008412) == 0x0C44  # cmpi.w #$b, d4
    assert u16(rom, 0x008414) == 0x000B
    assert u16(rom, 0x008466) == 0x003C  # ori.b #1, ccr
    assert u16(rom, 0x008468) == 0x0001
    assert u16(rom, 0x00846A) == 0x4E75  # rts

    # 0x00846C: Shockwave projectile spawner
    assert u16(rom, 0x008476) == 0x4DF9  # lea $FF1AA4.l, a6
    assert u32(rom, 0x008478) == 0x00FF1AA4
    assert u16(rom, 0x00847C) == 0x3D7C  # move.w #$16, 0(a6)
    assert u16(rom, 0x00847E) == 0x0016
    assert u16(rom, 0x008492) == 0x3D7C  # move.w #$13, $10(a6)
    assert u16(rom, 0x008494) == 0x0013
    assert u16(rom, 0x00849E) == 0x3D7C  # move.w #$4f8, $18(a6)
    assert u16(rom, 0x0084A0) == 0x04F8
    assert u16(rom, 0x0084D2) == 0x323C  # move.w #$800, d1
    assert u16(rom, 0x0084D4) == 0x0800
    assert u16(rom, 0x0084E0) == 0x4EB9  # jsr $E2D4.l
    assert u32(rom, 0x0084E2) == 0x0000E2D4

    # 0x0084FC: 4-word shockwave table
    table_offsets = [u16(rom, 0x0084FC + i * 2) for i in range(4)]
    assert table_offsets == [0xF800, 0x0B00, 0x0817, 0xF500]

    # 0x008504: VDP DMA atomic enqueue
    assert u16(rom, 0x008508) == 0x007C  # ori.w #$700, sr
    assert u16(rom, 0x00850A) == 0x0700
    assert u16(rom, 0x00850C) == 0x2079  # movea.l $FF1892.l, a0
    assert u32(rom, 0x00850E) == 0x00FF1892
    assert u32(rom, 0x008514) == 0x0009D2FE  # move.l #$9d2fe, (a0)+
    assert u16(rom, 0x00851A) == 0x9F00  # move.w #$9f00, (a0)+
    assert u16(rom, 0x00851E) == 0x0390  # move.w #$390, (a0)+
    assert u16(rom, 0x008526) == 0x027C  # andi.w #$f9ff, sr
    assert u16(rom, 0x008528) == 0xF9FF

    # 0x008530: Weapon durability decrement & break
    assert u16(rom, 0x008536) == 0x1039  # move.b $FF0DB0.l, d0
    assert u32(rom, 0x008538) == 0x00FF0DB0
    assert u16(rom, 0x008548) == 0x0C01  # cmpi.b #2, d1
    assert u16(rom, 0x008550) == 0x0C01  # cmpi.b #$c, d1
    assert u16(rom, 0x008560) == 0x0C01  # cmpi.b #$ff, d1 (unbreakable)
    assert u16(rom, 0x008562) == 0x00FF
    assert u16(rom, 0x008568) == 0x5301  # subq.b #1, d1
    assert u16(rom, 0x008572) == 0x4270  # clr.w (a0, d0.w)
    assert u16(rom, 0x008576) == 0x51F9  # sf.b $FF0DB0.l
    assert u16(rom, 0x00857C) == 0x13FC  # move.b #1, $FF0DB1.l
    assert u16(rom, 0x008584) == 0x303C  # move.w #$a, d0
    assert u16(rom, 0x008588) == 0x33C0  # move.w d0, $FF0D7E.l
    assert u16(rom, 0x0085A4) == 0x33FC  # move.w #$5a, $FF1856.l (90 frames)
    assert u16(rom, 0x0085A6) == 0x005A
    assert u32(rom, 0x0085B2) == 0x203C00C0  # move.l #$c00269, d0 (weapon break SFX)
    assert u16(rom, 0x0085B6) == 0x0269

    # 0x0085C4: EXP level-up bonus check
    assert u16(rom, 0x0085D4) == 0x4279  # clr.w $FF0E3C.l
    assert u32(rom, 0x0085D6) == 0x00FF0E3C
    assert u16(rom, 0x0085DA) == 0x50F9  # st.b $FF0DBB.l
    assert u32(rom, 0x0085DC) == 0x00FF0DBB
    assert u16(rom, 0x0085E0) == 0x4E75  # rts

    evidence = {
        "milestone": "M15.391",
        "description": "ROM Ali Input Sequence Pattern Matcher, Shockwave Spawner, Weapon Durability & Break SFX",
        "address_range": ["0x0083F8", "0x0085E2"],
        "byte_count": 0x0085E2 - 0x0083F8,
        "shockwave_offsets": [f"0x{w:04X}" for w in table_offsets],
        "routines": [
            {
                "name": "ali_motion_sequence_matcher",
                "start": "0x0083F8",
                "end": "0x00846A",
                "purpose": "Stateful 3-stage command motion recognizer: scans up to 12 frames in $FF1668 for Press -> Release -> Re-press pattern"
            },
            {
                "name": "ali_shockwave_projectile_spawner",
                "start": "0x00846C",
                "end": "0x0084FA",
                "purpose": "Spawns shockwave projectile entity 0x16 at $FF1AA4 with elevation 19, orientation offsets 0x0084FC, and speed 0x800 via 0xE2D4"
            },
            {
                "name": "ali_vdp_dma_enqueue",
                "start": "0x008504",
                "end": "0x00852E",
                "purpose": "Enqueues VDP DMA parameters (0x9D2FE, 0x9F00, 0x390) with interrupt safety mask SR 0x0700"
            },
            {
                "name": "ali_weapon_durability_break",
                "start": "0x008530",
                "end": "0x0085C2",
                "purpose": "Decrements weapon durability; ignores 0xFF; on 0 clears slot, reverts to Dagger ID 1 / type 0x0A, 90f toast, and plays weapon break SFX 0xC00269"
            },
            {
                "name": "ali_exp_bonus_check",
                "start": "0x0085C4",
                "end": "0x0085E2",
                "purpose": "Checks EXP threshold $FF0E3C >= $FF1A70; clears EXP and sets bonus item drop flag $FF0DBB = 0xFF"
            }
        ]
    }

    os.makedirs(os.path.dirname(OUTPUT_JSON), exist_ok=True)
    with open(OUTPUT_JSON, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Evidence written to {OUTPUT_JSON} ({len(evidence['routines'])} routines)")


if __name__ == "__main__":
    extract_evidence()
