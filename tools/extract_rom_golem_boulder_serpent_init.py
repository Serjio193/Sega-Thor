#!/usr/bin/env python3
"""
Extract ROM Golem Boulder/Debris Projectile Ballistics & Composite Serpent/
Multi-Segment Boss Constructor (0x0159E2..0x015C4A, 616 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.511.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_golem_boulder_serpent_init_m15_511.json"
)

RANGE_START = 0x0159E2
RANGE_END = 0x015C4A
RANGE_SIZE = RANGE_END - RANGE_START  # 616 bytes


def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    for insn in md.disasm(slice_data, RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    routines = [
        {
            "name": "AI_GolemBoulder_Tick",
            "start": "0x0159E2",
            "end": "0x015A8C",
            "size_bytes": 0x015A8C - 0x0159E2,
            "description": (
                "Boulder/debris projectile flight physics, collision, and shatter dispatcher. "
                "If state +$4 != 0, despawns on anim finish flag +$37:1. If state == 0, integrates "
                "horizontal and vertical velocities into subpixel position (+$72/+$76), applies gravity "
                "-$C000 to +$56 into Z position +$10. If Z < 0, clamps Z=0 and enters shatter State 2 "
                "(anim $E). In flight, executes AABB check vs Ali via 0x00B922 ([-8..+8, -8..+8]) "
                "calling Hit Callback 0x015A8C on contact. Checks terrain collision: bit 6 wall collision "
                "with condition ($41 & $13 == $12) performs Z-bounce (+10 -= +52); otherwise shatters. "
                "Bit 7 entity collision shatters on contact with Ali ($FF19E8)."
            )
        },
        {
            "name": "AI_GolemBoulder_HitCallback",
            "start": "0x015A8C",
            "end": "0x015AE2",
            "size_bytes": 0x015AE2 - 0x015A8C,
            "description": (
                "Boulder collision hit callback. Enters State 2 (anim $0E). Checks Ali invulnerability "
                "(+$38:3) and guard (+$82). If vulnerable, stores attacker pointer, clears guard and +$9D, "
                "inflicts 6 HP damage ($8006), knockback mode 3, horizontal knockback -$40000 (-4.0 px), "
                "vertical impulse $30000 (+3.0 px), and triggers impact SFX $800207 via 0x00060004."
            )
        },
        {
            "name": "AI_CompositeSerpent_SubModeRouter",
            "start": "0x015AE2",
            "end": "0x015AFE",
            "size_bytes": 0x015AFE - 0x015AE2,
            "description": (
                "Composite Serpent/Multi-segment boss sub-mode dispatcher. Reads +$66(a6): "
                "0 -> 0x016294, 1 -> 0x015AFE (9-slot composite constructor), 2 -> 0x015CCA, "
                "3 -> 0x015CB6."
            )
        },
        {
            "name": "AI_CompositeSerpent_InitCompositeSegments",
            "start": "0x015AFE",
            "end": "0x015C4A",
            "size_bytes": 0x015C4A - 0x015AFE,
            "description": (
                "Initializes 9 sequential child entity slots in Pool 2 with stride $BC (188 bytes) "
                "via CopyParentEntityAttributes (0x00E66E). Configures 4 body segments (anim $3A, facings 3/5), "
                "1 head segment (anim $3C, sub-mode 2, size 16, bounds 8x6x16, extent 4, dx=-32, dy=+40, "
                "HP 20, max HP 20, tile $200), and 4 tail/orb segments (anim $2C/$2E/$30/$30, tile $200). "
                "Restores root entity pointer, resets sub-mode/facing, sets State $18, anim 4, HP 50, "
                "max HP 50, clears +$B0, and jumps to script/entity engine 0x02ED20."
            )
        }
    ]

    composite_segment_config = [
        {"slot_index": 1, "stride_offset": 0x0BC, "anim_id": 0x3A, "facing_delta": 2, "type": "body_1"},
        {"slot_index": 2, "stride_offset": 0x178, "anim_id": 0x3A, "facing_delta": 0, "type": "body_2"},
        {"slot_index": 3, "stride_offset": 0x234, "anim_id": 0x3A, "facing_delta": 2, "type": "body_3"},
        {"slot_index": 4, "stride_offset": 0x2F0, "anim_id": 0x3A, "facing_delta": 0, "type": "body_4"},
        {
            "slot_index": 5,
            "stride_offset": 0x3AC,
            "anim_id": 0x3C,
            "sub_mode": 2,
            "size_3d": 0x10,
            "bounds_w": 8,
            "bounds_d": 6,
            "bounds_h": 16,
            "bounds_extent": 4,
            "dx": -0x20,
            "dy": 0x28,
            "hp": 20,
            "max_hp": 20,
            "vram_tile": 0x0200,
            "type": "head"
        },
        {"slot_index": 6, "stride_offset": 0x468, "anim_id": 0x2C, "size_3d": 8, "vram_tile": 0x0200, "type": "tail_orb_1"},
        {"slot_index": 7, "stride_offset": 0x524, "anim_id": 0x2E, "size_3d": 0, "vram_tile": 0x0200, "type": "tail_orb_2"},
        {"slot_index": 8, "stride_offset": 0x5E0, "anim_id": 0x30, "size_3d": 0, "vram_tile": 0x0200, "type": "tail_orb_3"},
        {"slot_index": 9, "stride_offset": 0x69C, "anim_id": 0x30, "size_3d": 0, "vram_tile": 0x0200, "type": "tail_orb_4"}
    ]

    root_final_config = {
        "sub_mode": 0,
        "facing": 0,
        "state": 0x18,
        "anim_id": 4,
        "hp": 50,
        "max_hp": 50,
        "flag_b0": 0,
        "exit_jmp": "0x02ED20"
    }

    evidence = {
        "milestone": "M15.511",
        "title": "ROM Golem Boulder/Debris Projectile Ballistics & Composite Serpent/Multi-Segment Boss Constructor",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "routines": routines,
        "instruction_count": len(instructions),
        "composite_segment_config": composite_segment_config,
        "root_final_config": root_final_config,
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
