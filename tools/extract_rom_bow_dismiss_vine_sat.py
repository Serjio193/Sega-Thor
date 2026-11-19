#!/usr/bin/env python3
"""
Extract ROM Bow Spirit (0x1E) Dismissal, States $10/$02, Neck/Vine Chain
Interpolator, SAT Sub-Entity Renderer & Leash Velocity Clamper
(0x013F22..0x014242, 800 bytes).

Disassembles all 206 M68K instructions across 0x013F22..0x014242, verifies
constants/SAT bounds/RAM targets, and emits evidence JSON for Milestone M15.500.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_bow_dismiss_vine_sat_m15_500.json"

RANGE_START = 0x013F22
RANGE_END = 0x014242
RANGE_SIZE = RANGE_END - RANGE_START  # 800 bytes

ROUTINES = [
    {
        "name": "AI_Bow_Dismissal",
        "start": "0x013F22",
        "end": "0x013F5A",
        "size_bytes": 0x013F5A - 0x013F22,
        "description": (
            "Guards +$04(a6)==0 (else branches to 0x0138B8 low-SP blink check). Plays "
            "SFX #$00A0039E via 0x00E7FC, clears child slot +$00(+$A6(a6))=0, enters "
            "State $10 (+$2A=$1E), clears +$16=0, sets priority +$18=$07A8, and clears "
            "+$36 bit 0."
        ),
    },
    {
        "name": "AI_Bow_State10_DescendDespawn",
        "start": "0x013F5A",
        "end": "0x013F6E",
        "size_bytes": 0x013F6E - 0x013F5A,
        "description": (
            "Clears +$36(a6) bit 1 and waits for +$37(a6) bit 1; when set clears "
            "+$00(a6)=0 to despawn Bow."
        ),
    },
    {
        "name": "AI_Bow_State02_ReturnAndSettle",
        "start": "0x013F6E",
        "end": "0x013FDC",
        "size_bytes": 0x013FDC - 0x013F6E,
        "description": (
            "Waits for +$37(a6) bit 1, settles Bow into State 0 (+$2A=0, +$9E=0, "
            "+$16=1, clears +$38 bit 2), initializes $FF1C1C vine SAT sub-entity "
            "(+$00=$FFFF, +$22=$00014084, +$18=(parent_18 & $8000)|$07A8, anchor "
            "+$4E/+$52/+$56 from parent +$08/+$0C/+$10, cross-links +$A6), and calls "
            "0x013FFC."
        ),
    },
    {
        "name": "AI_Bow_InterpolateVineChainNodes",
        "start": "0x013FDC",
        "end": "0x014084",
        "size_bytes": 0x014084 - 0x013FDC,
        "sub_entry_points": {
            "update_facing_from_anchor": "0x013FDC",
            "interpolate_4_vine_nodes": "0x013FFC",
            "step_and_quantize_node": "0x01406A",
        },
        "description": (
            "0x013FDC computes (head - anchor) into 0x00E338 to face Bow away from "
            "anchor (+$16). 0x013FFC updates $FF1C1C sort Y = min(head_y-2, anchor_y) "
            "and steps 4 nodes (1/5..4/5) from anchor (+$4E/+$52/+$56) to head "
            "(+$08/+$0C/+$10) using 12.4 fixed-point divs #5 with per-step asr.w #4 -> "
            "store -> asl.w #4 quantization into $FF19A8..$FF19BF (4x 6-byte XYZ)."
        ),
    },
    {
        "name": "AI_Bow_RenderVineSubEntitySAT",
        "start": "0x014084",
        "end": "0x014200",
        "size_bytes": 0x014200 - 0x014084,
        "sub_entry_points": {
            "render_callback_entry": "0x014084",
            "project_vine_node": "0x014126",
            "project_root_anchor": "0x01415C",
            "project_spirit_shadow": "0x014192",
            "clip_and_emit_sat_entry": "0x0141CA",
        },
        "description": (
            "Checks parent +$00==$001E (else frees self +$0=0); skips on parent "
            "+$36 bit 1 blink. Emits up to 6 SAT entries to $FF13CC+$FF188C (clip "
            "d5<80, X in [$68,$1C0), Y in [$70,$160)): 4 vine nodes walking backward "
            "$FF19BA->$FF19A8 (tile +$18, size 0x0000, +$7C/+$7C-z), Bow ground shadow "
            "at $FF1AA4 (tile $0549, size $0400, +$78/+$7D), and root anchor at $FF1C1C "
            "(tile +$18+9, size $0E00, +$71/+$74-z), committing $FF188C/$FF188A."
        ),
    },
    {
        "name": "AI_Bow_StepAndClampVelocityByLeash",
        "start": "0x014200",
        "end": "0x014242",
        "size_bytes": 0x014242 - 0x014200,
        "description": (
            "Saves +$08/+$0C(a6) on stack, adds +$72/+$76(a6) into +$08/+$0C, calls "
            "leash clamp helper 0x014242, computes clamped delta (new_pos - old_pos) "
            "into +$72/+$76(a6), and restores original +$08/+$0C(a6)."
        ),
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key opcodes, constants, SAT clip bounds, and RAM targets."""
    assert u32_be(rom, 0x013F2C) == 0x00A0039E  # move.l #$a0039e, d0
    assert u16_be(rom, 0x013F44) == 0x001E      # move.w #$1e, $2a(a6)
    assert u16_be(rom, 0x013F4E) == 0x07A8      # move.w #$7a8, $18(a6)
    assert u32_be(rom, 0x013F98) == 0x00FF1C1C  # lea.l $ff1c1c.l, a6
    assert u32_be(rom, 0x013FA4) == 0x00014084  # move.l #$14084, $22(a6)
    assert u16_be(rom, 0x013FB4) == 0x07A8      # ori.w #$7a8, d0
    assert u16_be(rom, 0x014042) == 0x0005      # divs.w #$5, d4
    assert u32_be(rom, 0x014054) == 0x00FF19A8  # lea.l $ff19a8.l, a3
    assert u16_be(rom, 0x01408A) == 0x001E      # cmpi.w #$1e, $0(a0)
    assert u32_be(rom, 0x0140A8) == 0x00FF19BA  # lea.l $ff19ba.l, a3
    assert u32_be(rom, 0x0140B4) == 0x00FF13CC  # lea.l $ff13cc.l, a5
    assert u16_be(rom, 0x0140EC) == 0x0549      # move.w #$549, d3
    assert u16_be(rom, 0x0140F0) == 0x0400      # move.w #$400, d4
    assert u16_be(rom, 0x014100) == 0x0009      # addi.w #$9, d3
    assert u16_be(rom, 0x014104) == 0x0E00      # move.w #$e00, d4
    assert u32_be(rom, 0x014194) == 0x00FF1AA4  # lea.l $ff1aa4.l, a0
    assert u16_be(rom, 0x0141CC) == 0x0050      # cmpi.w #$50, d5
    assert u16_be(rom, 0x0141D2) == 0x0068      # cmpi.w #$68, d1
    assert u16_be(rom, 0x0141DA) == 0x01C0      # cmpi.w #$1c0, d1
    assert u16_be(rom, 0x0141E2) == 0x0070      # cmpi.w #$70, d2
    assert u16_be(rom, 0x0141EA) == 0x0160      # cmpi.w #$160, d2


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    pos = RANGE_START
    for insn in md.disasm(slice_data, RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size,
        })
        pos = insn.address + insn.size
    assert pos == RANGE_END, f"Disassembly ended at 0x{pos:06X} != 0x{RANGE_END:06X}"

    routines_meta = []
    for r in ROUTINES:
        r_start = int(r["start"], 16)
        r_end = int(r["end"], 16)
        r_insns = [ins for ins in instructions if r_start <= int(ins["address"], 16) < r_end]
        entry = {
            "name": r["name"],
            "start": r["start"],
            "end": r["end"],
            "size_bytes": r["size_bytes"],
            "instruction_count": len(r_insns),
            "description": r["description"],
        }
        if "sub_entry_points" in r:
            entry["sub_entry_points"] = r["sub_entry_points"]
        routines_meta.append(entry)

    evidence = {
        "milestone": "M15.500",
        "title": "ROM Bow Spirit (0x1E) Dismissal, States $10/$02, Neck/Vine Chain Interpolator, SAT Sub-Entity Renderer & Leash Velocity Clamper",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE,
        },
        "constants": {
            "bow_entity_type": "0x001E",
            "dismissal_sfx": "0x00A0039E",
            "dismissal_anim_2a": "0x001E",
            "vine_subentity_sentinel_00": "0xFFFF",
            "vine_render_callback": "0x00014084",
            "vine_priority_base": "0x07A8",
            "vine_chain_node_count": 4,
            "vine_chain_divisor": 5,
            "shadow_tile_attr": "0x0549",
            "shadow_sat_size": "0x0400",
            "root_tile_offset": 9,
            "root_sat_size": "0x0E00",
            "sat_max_sprites": 80,
            "sat_clip_x": [0x68, 0x1C0],
            "sat_clip_y": [0x70, 0x160],
            "screen_offsets": {
                "vine_node": [0x7C, 0x7C],
                "root_anchor": [0x71, 0x74],
                "spirit_shadow": [0x78, 0x7D],
            },
        },
        "ram_addresses": {
            "sat_buffer_base": "0x00FF13CC",
            "plane_select_flags": "0x00FF16F0",
            "camera_plane_a": "0x00FF1716",
            "camera_plane_b": "0x00FF173E",
            "sat_sprite_count": "0x00FF188A",
            "sat_byte_offset": "0x00FF188C",
            "vine_nodes_start": "0x00FF19A8",
            "vine_node3_tail": "0x00FF19BA",
            "spirit_entity_slot": "0x00FF1AA4",
            "vine_subentity_slot": "0x00FF1C1C",
        },
        "external_targets": {
            "compute_cardinal_facing": "0x00E338",
            "audio_play_sfx": "0x00E7FC",
            "bow_low_sp_blink_check": "0x0138B8",
            "bow_leash_range_clamp": "0x014242",
        },
        "routines": routines_meta,
        "instruction_count": len(instructions),
        "instructions": instructions,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    ins_placeholder = "__INSTRUCTIONS_PLACEHOLDER__"
    raw_json = json.dumps({**evidence, "instructions": ins_placeholder}, indent=2)
    ins_lines = ",\n".join(f"    {json.dumps(item)}" for item in instructions)
    formatted_json = raw_json.replace(f'"{ins_placeholder}"', f"[\n{ins_lines}\n  ]") + "\n"
    assert json.loads(formatted_json) == evidence
    OUTPUT_PATH.write_text(formatted_json, encoding="utf-8", newline="\n")
    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
