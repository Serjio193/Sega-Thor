#!/usr/bin/env python3
"""
Extract ROM Shade Spirit Grapple Retraction, Entity/Item Reel-In Sync,
Dynamic Tether Stretch & Tip Probe Offset Table (0x01341A..0x01366A, 592 bytes).

Disassembles exact M68K instructions across 0x01341A..0x013652 (568 bytes),
extracts the inline 24-byte ShadeGrappleTipOffsetTable at 0x013652..0x01366A
(3 tiers x 4 facings x 2 signed bytes), verifies key constants, and emits
evidence JSON for Milestone M15.495.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_shade_retract_tether_m15_495.json"

RANGE_START = 0x01341A
CODE_END = 0x013652
TABLE_START = 0x013652
TABLE_END = 0x01366A
RANGE_END = 0x01366A
RANGE_SIZE = RANGE_END - RANGE_START  # 592 bytes (568B code + 24B table)

EXPECTED_TIP_OFFSETS = [
    [(0, -24), (32, 0), (0, 24), (-32, 0)],   # Tier 0: +$32 <= $17
    [(0, -14), (16, 0), (0, 10), (-16, 0)],   # Tier 1: $18 <= +$32 <= $1A
    [(0, -2), (8, 0), (0, 0), (-8, 0)],       # Tier 2: +$32 > $1A
]

ROUTINES = [
    {
        "name": "AI_Shade_State0A_GrappleRetractAndComplete",
        "start": "0x01341A",
        "end": "0x0134DE",
        "size_bytes": 0x0134DE - 0x01341A,
        "sub_entry_points": {
            "state_0a_entry": "0x01341A",
            "retract_complete": "0x01348E",
        },
        "description": (
            "Clears Ali velocity +$72/+$76(a5), decrements +$02(a6) (<=0 -> 0x01348E). "
            "When +$AA==0, checks 0x00E398 Manhattan distance d5 <= $10 (16px -> 0x01348E). "
            "Accelerates +$4E/+$52 by (0x60, 0x60) via 0x00E2D4: if +$AA==0 adds to Shade "
            "+$72/+$76; if 0<+$AA<$FF2954 subtracts from +$AE/+$B2 and syncs Ali via "
            "0x01352A; if +$AA>=$FF2954 adds to Shade +$72/+$76 and drags item via "
            "0x013550. At 0x01348E: runs 0x009D00 on entity or snaps item +$08/+$0C to Ali, "
            "clears $FF1982, unlocks +$38(a5) bit 3 and $FF17B8 bit 2, frees tether slot "
            "+$A6 if +$00==$FFFF, and branches to 0x01378A."
        ),
    },
    {
        "name": "AI_Shade_GrappleTetherAnchorAndDragHelpers",
        "start": "0x0134DE",
        "end": "0x013576",
        "size_bytes": 0x013576 - 0x0134DE,
        "sub_entry_points": {
            "tether_anchor_lock": "0x0134DE",
            "sync_ali_to_target": "0x01352A",
            "drag_item_to_tip": "0x013550",
        },
        "description": (
            "0x0134DE returns if +$AA==0 or jumps to 0x013550 if +$AA>=$FF2954; else "
            "tests +$17(a5) bit 0 (horizontal vs vertical Ali facing) to lock +$AE/+$B2 "
            "along the grapple axis and align both Shade and Ali perpendicular coords onto "
            "the target. 0x01352A (or -> 0x013550 if >=$FF2954) keeps Ali at target + "
            "(+$AE, +$B2). 0x013550 skips if +$32<$15 (21), else calls 0x013622 and places "
            "item at (shade_x + dx, shade_y + dy - shade_z)."
        ),
    },
    {
        "name": "AI_Shade_UpdateTetherStretchEntity",
        "start": "0x013576",
        "end": "0x013608",
        "size_bytes": 0x013608 - 0x013576,
        "sub_entry_points": {
            "tether_stretch_callback": "0x013576",
        },
        "description": (
            "Tether sub-entity callback: despawns (+$00=0) if parent +$A6 type != "
            "$001C; skips render (returns without despawning) if parent +$36 bit 1 is "
            "set (low-SP blink). Computes Chebyshev max(|ali_x - shade_x|, "
            "|ali_y - (shade_y - shade_z)|) using $FF19E8 & $FF1AA4; if > $14 (20px), "
            "computes idx = min((dist - 20) >> 3, 11), sets +$32(a6) = $60 + idx*3, "
            "copies Shade (x, y, z_fixed), calls 0x00B022, and clears 32-bit +$0C(a6)."
        ),
    },
    {
        "name": "AI_Shade_ProbeItemsAtGrappleTip",
        "start": "0x013608",
        "end": "0x01366A",
        "size_bytes": 0x01366A - 0x013608,
        "code_end": "0x013652",
        "code_size_bytes": 0x013652 - 0x013608,
        "table_start": "0x013652",
        "table_end": "0x01366A",
        "table_size_bytes": 0x01366A - 0x013652,
        "sub_entry_points": {
            "probe_items_at_tip": "0x013608",
            "lookup_tip_offset": "0x013622",
            "shade_grapple_tip_offset_table": "0x013652",
        },
        "description": (
            "0x013608 calls 0x013622, expands [dx-4..dx+4, dy-4..dy+4, z=0], and jumps "
            "to 0x00B9EC item pool scanner. 0x013622 selects tier 0 (+$32 <= $17), "
            "tier 1 (+$32 <= $1A), or tier 2 (+$32 > $1A) from inline 24-byte "
            "ShadeGrappleTipOffsetTable at 0x013652..0x01366A and sign-extends (dx, dy) "
            "for facing +$16(a6) in 0..3."
        ),
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key opcodes, constants, and tip offset table in 0x01341A..0x01366A."""
    assert u16_be(rom, 0x013434) == 0x0010      # cmpi.w #$10, d5
    assert u16_be(rom, 0x01343E) == 0x0060      # move.w #$60, d1
    assert u16_be(rom, 0x013442) == 0x0060      # move.w #$60, d2
    assert u32_be(rom, 0x01346A) == 0x00FF2954  # cmpi.l #$ff2954, $aa(a6)
    assert u32_be(rom, 0x0134B8) == 0x00FF1982  # sf.b $ff1982.l
    assert u32_be(rom, 0x0134C6) == 0x00FF17B8  # bclr.b #$2, $ff17b8.l
    assert u16_be(rom, 0x0134D0) == 0xFFFF      # cmpi.w #$ffff, $0(a0)
    assert u16_be(rom, 0x013556) == 0x0015      # cmpi.w #$15, $32(a6)
    assert u16_be(rom, 0x01357C) == 0x001C      # cmpi.w #$1c, $0(a0)
    assert u32_be(rom, 0x013598) == 0x00FF19E8  # lea.l $ff19e8.l, a0
    assert u32_be(rom, 0x01359E) == 0x00FF1AA4  # lea.l $ff1aa4.l, a1
    assert u16_be(rom, 0x0135CA) == 0x0014      # subi.w #$14, d3
    assert u16_be(rom, 0x0135D2) == 0x000C      # cmpi.w #$c, d3
    assert u16_be(rom, 0x0135D8) == 0x000B      # move.w #$b, d3
    assert u16_be(rom, 0x0135E2) == 0x0060      # addi.w #$60, d3
    assert u32_be(rom, 0x0135FA) == 0x0000B022  # jsr $b022.l
    assert u16_be(rom, 0x013612) == 0x0004      # move.w #$4, d5
    assert u16_be(rom, 0x013632) == 0x0017      # cmpi.w #$17, d1
    assert u16_be(rom, 0x01363C) == 0x001A      # cmpi.w #$1a, d1

    parsed = []
    for tier in range(3):
        row = []
        for facing in range(4):
            dx, dy = struct.unpack_from(">2b", rom, TABLE_START + tier * 8 + facing * 2)
            row.append((dx, dy))
        parsed.append(row)
    assert parsed == EXPECTED_TIP_OFFSETS, f"Unexpected tip offsets: {parsed}"


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    assert len(rom[RANGE_START:RANGE_END]) == RANGE_SIZE
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    pos = RANGE_START
    for insn in md.disasm(rom[RANGE_START:CODE_END], RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size,
        })
        pos = insn.address + insn.size
    assert pos == CODE_END, f"Disassembly ended at 0x{pos:06X} != 0x{CODE_END:06X}"
    assert pos + 24 == RANGE_END

    tip_tiers = []
    tier_ranges = ["<= 0x17", "0x18..0x1A", "> 0x1A"]
    for tier in range(3):
        facings = []
        for facing in range(4):
            addr = TABLE_START + tier * 8 + facing * 2
            dx, dy = struct.unpack_from(">2b", rom, addr)
            facings.append({
                "facing": facing,
                "address": f"0x{addr:06X}",
                "bytes": rom[addr:addr + 2].hex(),
                "dx": dx,
                "dy": dy,
            })
        tip_tiers.append({
            "tier": tier,
            "sprite_frame_32_condition": tier_ranges[tier],
            "address": f"0x{TABLE_START + tier * 8:06X}",
            "facings": facings,
        })

    routines_meta = []
    for r in ROUTINES:
        r_start = int(r["start"], 16)
        r_end = int(r["end"], 16)
        r_insns = [ins for ins in instructions if r_start <= int(ins["address"], 16) < r_end]
        meta = {
            "name": r["name"],
            "start": r["start"],
            "end": r["end"],
            "size_bytes": r["size_bytes"],
            "instruction_count": len(r_insns),
            "sub_entry_points": r["sub_entry_points"],
            "description": r["description"],
        }
        if "table_start" in r:
            meta["code_end"] = r["code_end"]
            meta["code_size_bytes"] = r["code_size_bytes"]
            meta["table_start"] = r["table_start"]
            meta["table_end"] = r["table_end"]
            meta["table_size_bytes"] = r["table_size_bytes"]
        routines_meta.append(meta)

    evidence = {
        "milestone": "M15.495",
        "title": "ROM Shade Spirit Grapple Retraction, Entity/Item Reel-In Sync, Dynamic Tether Stretch & Tip Probe Offset Table",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "code_end": f"0x{CODE_END:06X}",
            "code_size_bytes": CODE_END - RANGE_START,
            "table_size_bytes": TABLE_END - TABLE_START,
            "size_bytes": RANGE_SIZE,
        },
        "embedded_data_tables": [
            {
                "name": "ShadeGrappleTipOffsetTable",
                "address": f"0x{TABLE_START:06X}",
                "end": f"0x{TABLE_END:06X}",
                "size_bytes": TABLE_END - TABLE_START,
                "layout": "3_tiers_x_4_facings_x_2_signed_bytes",
                "tiers": tip_tiers,
            }
        ],
        "constants": {
            "retract_early_manhattan_max_px": 16,
            "retract_accel_step": "0x0060",
            "item_pool_boundary_addr": "0x00FF2954",
            "tether_slot_free_sentinel": "0xFFFF",
            "item_drag_min_sprite_frame": "0x0015",
            "shade_entity_type": "0x001C",
            "tether_chebyshev_min_exclusive_px": 20,
            "tether_stretch_shift": 3,
            "tether_stretch_max_index": 11,
            "tether_sprite_base": "0x0060",
            "tether_sprite_stride": 3,
            "tip_probe_half_extent_px": 4,
            "tip_tier0_max_frame": "0x0017",
            "tip_tier1_max_frame": "0x001A",
        },
        "ram_addresses": {
            "player_lock_ff1982": "0x00FF1982",
            "engine_flags_ff17b8": "0x00FF17B8",
            "ali_entity_ff19e8": "0x00FF19E8",
            "shade_entity_ff1aa4": "0x00FF1AA4",
            "item_pool_start_ff2954": "0x00FF2954",
        },
        "external_targets": {
            "entity_terrain_refresh": "0x009D00",
            "tether_sprite_render": "0x00B022",
            "item_pool_aabb_probe": "0x00B9EC",
            "compute_8dir_velocity": "0x00E2D4",
            "ali_delta_facing_distance": "0x00E398",
            "shade_return_to_idle": "0x01378A",
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
    OUTPUT_PATH.write_text(formatted_json, encoding="utf-8")
    print(f"Extracted {len(instructions)} instructions + 24B table ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
