#!/usr/bin/env python3
"""
Extract ROM Multi-Part Boss/Golem (0x9A) Phase Damage Handler, Plane A Tilemap
Damage Patches, Tentacle/Arm Sub-State Machine (0..4) & Spore Homing (0x015758..0x0159E2, 650 bytes).

Disassembles code segments cleanly with Capstone, unpacks 12-byte tilemap patch records,
validates opcodes/constants, and generates evidence JSON for Milestone M15.510.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_golem_boss_damage_arm_m15_510.json"

RANGE_START = 0x015758
RANGE_END = 0x0159E2
RANGE_SIZE = RANGE_END - RANGE_START  # 650 bytes

CODE_SEGMENTS = [
    ("AI_GolemBoss_BodyHitDamage", 0x015758, 0x0157FC),
    ("AI_GolemBoss_Phase0Check", 0x015808, 0x015850),
    ("AI_GolemBoss_TentacleArmAndSporeHoming", 0x01585C, 0x0159E2),
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key ROM addresses, offsets and table words."""
    assert u16_be(rom, 0x015758) == 0x302E      # move.w $86(a6), d0
    assert u16_be(rom, 0x01575A) == 0x0086
    assert u32_be(rom, 0x0157C8) == 0x00FF1CD8  # Golem main body struct
    assert u32_be(rom, 0x0157F0) == 0x00FF1716  # Plane A buffer context
    assert u32_be(rom, 0x01581C) == 0x00FF1CD8  # Golem main body struct
    assert u32_be(rom, 0x015844) == 0x00FF1716  # Plane A buffer context
    assert u32_be(rom, 0x01588A) == 0x00FF19F0  # Ali target X
    assert u32_be(rom, 0x015890) == 0x00FF19F4  # Ali target Y
    assert u32_be(rom, 0x0158BE) == 0x00FF19E8  # Ali entity pointer
    assert u32_be(rom, 0x0158EE) == 0xFFFB0000  # Knockback vx (-5.0 px)
    assert u32_be(rom, 0x0158FA) == 0x00020000  # Knockback vz (+2.0 px)
    assert u32_be(rom, 0x015914) == 0x00800381  # Tentacle whip SFX ID
    assert u32_be(rom, 0x015942) == 0x00FF1CE0  # Tentacle home X
    assert u32_be(rom, 0x015948) == 0x00FF1CE4  # Tentacle home Y


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    # 1. Unpack 12-byte Tilemap Patch Record 1 at 0x0157FC..0x015808 (Phase 1 fracture)
    p1_addr = 0x0157FC
    p1_src_left = u16_be(rom, p1_addr + 0)
    p1_src_top = u16_be(rom, p1_addr + 2)
    p1_src_right = u16_be(rom, p1_addr + 4)
    p1_src_bottom = u16_be(rom, p1_addr + 6)
    p1_dst_x = u16_be(rom, p1_addr + 8)
    p1_dst_y = u16_be(rom, p1_addr + 10)
    p1_record = {
        "address": f"0x{p1_addr:06X}",
        "size_bytes": 12,
        "phase": 1,
        "source_rect": [p1_src_left, p1_src_top, p1_src_right, p1_src_bottom],
        "dest_coord": [p1_dst_x, p1_dst_y],
        "width_tiles": p1_src_right - p1_src_left,
        "height_tiles": p1_src_bottom - p1_src_top,
        "raw_hex": rom[p1_addr : p1_addr + 12].hex(),
    }

    # 2. Unpack 12-byte Tilemap Patch Record 2 at 0x015850..0x01585C (Phase 0 fracture)
    p0_addr = 0x015850
    p0_src_left = u16_be(rom, p0_addr + 0)
    p0_src_top = u16_be(rom, p0_addr + 2)
    p0_src_right = u16_be(rom, p0_addr + 4)
    p0_src_bottom = u16_be(rom, p0_addr + 6)
    p0_dst_x = u16_be(rom, p0_addr + 8)
    p0_dst_y = u16_be(rom, p0_addr + 10)
    p0_record = {
        "address": f"0x{p0_addr:06X}",
        "size_bytes": 12,
        "phase": 0,
        "source_rect": [p0_src_left, p0_src_top, p0_src_right, p0_src_bottom],
        "dest_coord": [p0_dst_x, p0_dst_y],
        "width_tiles": p0_src_right - p0_src_left,
        "height_tiles": p0_src_bottom - p0_src_top,
        "raw_hex": rom[p0_addr : p0_addr + 12].hex(),
    }

    # 3. Disassemble each code segment
    disassembled_segments = []
    total_instructions = 0
    all_instructions = []

    for name, seg_start, seg_end in CODE_SEGMENTS:
        seg_bytes = rom[seg_start:seg_end]
        seg_insns = []
        for insn in md.disasm(seg_bytes, seg_start):
            insn_dict = {
                "address": f"0x{insn.address:06X}",
                "mnemonic": insn.mnemonic,
                "op_str": insn.op_str,
                "bytes": insn.bytes.hex(),
                "size": insn.size,
            }
            seg_insns.append(insn_dict)
            all_instructions.append(insn_dict)

        disassembled_segments.append({
            "name": name,
            "start": f"0x{seg_start:06X}",
            "end": f"0x{seg_end:06X}",
            "size_bytes": seg_end - seg_start,
            "instruction_count": len(seg_insns),
            "instructions": seg_insns,
        })
        total_instructions += len(seg_insns)

    routines = [
        {
            "name": "AI_GolemBoss_BodyHitDamage",
            "start": "0x015758",
            "end": "0x0157FC",
            "size_bytes": 0x0157FC - 0x015758,
            "description": (
                "Golem Body Hit Damage & Phase 1 Fracture. Checks pending damage +$86(a6); "
                "bypasses to tentacle arm 0x01585C if 0 or if already in hitstun state 4. "
                "Subtracts 12-bit masked damage from HP +$8A; triggers death routine 0x00E96C (d0=7) "
                "on HP=0. Otherwise enters State 4 (anim 6, hitstun timer 8). If phase +$8E == 1 and "
                "HP <= max_hp / 4, increments +$8E to 2, emits debris particle 3 at (x-136, y-64, z), "
                "and applies Plane A tilemap fracture patch 1 via 0x00DB68."
            ),
        },
        {
            "name": "TilemapPatch_Phase1",
            "start": "0x0157FC",
            "end": "0x015808",
            "size_bytes": 12,
            "description": "Embedded 12-byte Plane A tilemap patch record for Phase 1 fracture: rect [6, 32, 11, 37] -> [33, 12].",
        },
        {
            "name": "AI_GolemBoss_Phase0Check",
            "start": "0x015808",
            "end": "0x015850",
            "size_bytes": 0x015850 - 0x015808,
            "description": (
                "Phase 0 fracture check. If +$8E == 0 and HP <= max_hp / 2, increments +$8E to 1, "
                "sets bit 1 in $FF2A3E, emits debris particle 3 at (x-128, y+8, z), "
                "and applies Plane A tilemap fracture patch 0 via 0x00DB68."
            ),
        },
        {
            "name": "TilemapPatch_Phase0",
            "start": "0x015850",
            "end": "0x01585C",
            "size_bytes": 12,
            "description": "Embedded 12-byte Plane A tilemap patch record for Phase 0 fracture: rect [0, 32, 5, 37] -> [32, 17].",
        },
        {
            "name": "AI_GolemBoss_TentacleArm_Dispatcher",
            "start": "0x01585C",
            "end": "0x015882",
            "size_bytes": 0x015882 - 0x01585C,
            "description": "Tentacle arm sub-state dispatcher: dispatches on +$04 to States 0..4 (0=idle, 1=extend, 2=retract, 3=cooldown, 4=hitstun).",
        },
        {
            "name": "AI_GolemBoss_TentacleArm_State01_Extend",
            "start": "0x015882",
            "end": "0x015932",
            "size_bytes": 0x015932 - 0x015882,
            "description": (
                "State 1 Extend / Attack. Decrements timer +$2; transitions to State 2 on expiration. "
                "Steers toward Ali ($FF19F0/$FF19F4) via 0x00E45E at 2x speed. When distance d4 <= 4, "
                "checks Ali guard (+$38:3, +$82); if vulnerable, inflicts 10 damage (+bit 15 if airborne), "
                "knockback vx=-$50000, vz=+$20000, hitstun timer 14, plays SFX $800381, and enters State 3 cooldown (8f)."
            ),
        },
        {
            "name": "AI_GolemBoss_TentacleArm_State03_Cooldown",
            "start": "0x015932",
            "end": "0x015940",
            "size_bytes": 0x015940 - 0x015932,
            "description": "State 3 Post-attack cooldown. Decrements timer +$2; transitions to State 2 (retract) upon expiration.",
        },
        {
            "name": "AI_GolemBoss_TentacleArm_State02_Retract",
            "start": "0x015940",
            "end": "0x01596C",
            "size_bytes": 0x01596C - 0x015940,
            "description": (
                "State 2 Retract toward home ($FF1CE0/$FF1CE4) via 0x00E45E at 2x speed. "
                "When distance d4 <= 4, resets arm to State 0 (idle)."
            ),
        },
        {
            "name": "AI_GolemBoss_TentacleArm_State04_Hitstun",
            "start": "0x01596C",
            "end": "0x0159B4",
            "size_bytes": 0x0159B4 - 0x01596C,
            "description": (
                "State 4 Hitstun retract. Decrements hitstun timer +$83. While active, sets flashing bit +$36:6 "
                "and retracts toward home at 1x speed (clamping when d4 <= 2). On expiration, clears +$36:6 "
                "and +$39:3, transitioning to State 2 (retract)."
            ),
        },
        {
            "name": "AI_GolemBoss_SporeHomingReturn",
            "start": "0x0159B4",
            "end": "0x0159E2",
            "size_bytes": 0x0159E2 - 0x0159B4,
            "description": (
                "Spore homing return sub-mode (+A2 == 3). Reads parent entity pointer +$A6(a6); "
                "steers toward parent pos via 0x00E45E at 4x speed. Despawns spore (+0=0) when distance d4 <= 10."
            ),
        },
    ]

    evidence = {
        "metadata": {
            "milestone": "M15.510",
            "rom_file": "Beyond Oasis (USA).md",
            "range_start": f"0x{RANGE_START:06X}",
            "range_end": f"0x{RANGE_END:06X}",
            "range_size_bytes": RANGE_SIZE,
            "total_instructions": total_instructions,
            "code_segment_count": len(CODE_SEGMENTS),
            "tilemap_patch_record_count": 2,
        },
        "tilemap_patch_records": [p1_record, p0_record],
        "code_segments": disassembled_segments,
        "routines": routines,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(json.dumps(evidence, indent=2))
    print(f"Wrote evidence JSON to {OUTPUT_PATH}")
    print(f"Total instructions extracted: {total_instructions}")
    print(f"Tilemap patch records unpacked: {len(evidence['tilemap_patch_records'])}")


if __name__ == "__main__":
    extract()
