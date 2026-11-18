#!/usr/bin/env python3
"""
extract_rom_boss_attachment_engine.py — Extract ROM Entity Type $36 Boss Attachment,
Hierarchical Parent-Child Transform & 3D Combat Physics Subsystem
(0x00EA76..0x00ED04, 654 bytes).
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_boss_attachment_engine_m15_454.json")

START_ADDR = 0x00EA76
END_ADDR = 0x00ED04


def extract_instructions_and_evidence():
    if not os.path.exists(ROM_PATH):
        raise FileNotFoundError(f"ROM not found at {ROM_PATH}")

    with open(ROM_PATH, "rb") as f:
        rom_data = f.read()

    chunk = rom_data[START_ADDR:END_ADDR]
    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    for insn in md.disasm(chunk, START_ADDR):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    evidence = {
        "routine": "rom_boss_attachment_engine",
        "description": (
            "ROM Entity Type $36 Boss Attachment, Hierarchical Parent-Child Transform "
            "& 3D Combat Physics Subsystem"
        ),
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "byte_count": END_ADDR - START_ADDR,
        "instruction_count": len(instructions),
        "subroutines": {
            "attachment_update_parent_child_transforms": {
                "start": "0x00EA76",
                "end": "0x00EAC0",
                "callers_count": 8,
                "notes": (
                    "Checks parent tracking flag +$A8(a6). If latch timer +$B8 > 1, "
                    "decrements it, reads parent pointer +$B4, and adds parent pos (+$8, +$C, +$10) "
                    "to relative offsets (+$AE, +$B0, +$B2) to lock child position."
                )
            },
            "attachment_combat_overlap_and_knockback": {
                "start": "0x00EAC0",
                "end": "0x00EBC4",
                "callers_count": 4,
                "notes": (
                    "Performs 3D AABB overlap query via 0x00B922, applies damage +$A8 to target "
                    "+$86(a0), attacker link +$7E(a0), reaction mode 0x0A, projects knockback "
                    "with vertical launch #$20000, and sets boss HUD health tracking."
                )
            },
            "attachment_projectile_trajectory_and_cull": {
                "start": "0x00EBC4",
                "end": "0x00EC0C",
                "callers_count": 6,
                "notes": (
                    "Integrates horizontal displacement, gravity (subi.l #$6000, $56(a6)), "
                    "clamps elevation to 0, tests viewport culling with margin 10 via 0x00E5AE, "
                    "despawning when offscreen."
                )
            },
            "attachment_latch_to_parent": {
                "start": "0x00EC0C",
                "end": "0x00EC5A",
                "callers_count": 6,
                "notes": (
                    "Calculates relative offsets (+$AE, +$B0, +$B2) = child_pos - parent_pos, "
                    "stores parent pointer +$B4, sets latch timer +$B8 = 8, and provides "
                    "slot despawn helper at 0x00EC54 (clr.w $0(a6))."
                )
            },
            "attachment_secondary_branch_physics": {
                "start": "0x00EC5A",
                "end": "0x00ED04",
                "callers_count": 4,
                "notes": (
                    "Detached secondary part physics, terrain collision, and viewport culling."
                )
            }
        },
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({END_ADDR - START_ADDR} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract_instructions_and_evidence()
