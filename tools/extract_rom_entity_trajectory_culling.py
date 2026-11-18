#!/usr/bin/env python3
"""
extract_rom_entity_trajectory_culling.py — Extract ROM Entity Trajectory Gravity
Integration, Viewport AABB Culling & 188-Byte Slot Cloner
(0x00E502..0x00E6BA, 376 bytes).
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_trajectory_culling_m15_450.json")

START_ADDR = 0x00E502
END_ADDR = 0x00E6BA


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
        "routine": "rom_entity_trajectory_culling",
        "description": (
            "ROM Entity Trajectory Gravity Integration, Viewport AABB Culling "
            "& 188-Byte Slot Cloner"
        ),
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "byte_count": END_ADDR - START_ADDR,
        "instruction_count": len(instructions),
        "subroutines": {
            "entity_integrate_trajectory_gravity_fall": {
                "start": "0x00E502",
                "end": "0x00E5AE",
                "callers_count": 22,
                "notes": (
                    "Integrates fixed-point gravity (subi.l #$c000, $56(a6)), "
                    "updates elevation +$10(a6). If negative, handles ground landing: "
                    "zeroes elevation and velocity, sets bit 2 of +$39, sets recovery "
                    "timer +$9A to 15 frames, optionally triggers landing shockwave ($800270). "
                    "If airborne, updates displacements (+$72, +$76) and tests collision."
                )
            },
            "entity_viewport_culling_test_screen": {
                "start": "0x00E5AE",
                "end": "0x00E60E",
                "callers_count": 35,
                "notes": (
                    "Tests entity screen pos (x, y - elev) against camera viewport "
                    "[320 x 224] plus margin d7. Returns CCR carry clear if visible, "
                    "carry set if culled."
                )
            },
            "entity_viewport_culling_test_shadow": {
                "start": "0x00E610",
                "end": "0x00E66C",
                "callers_count": 18,
                "notes": (
                    "Tests entity ground shadow pos (x, y) against camera viewport "
                    "[320 x 224] plus margin d7 without elevation subtraction."
                )
            },
            "entity_slot_copy_188b": {
                "start": "0x00E66E",
                "end": "0x00E680",
                "callers_count": 12,
                "notes": (
                    "Copies 94 words (188 bytes / $BC) from (a4)+ to (a6)+ "
                    "via dbra d0, copying full entity record."
                )
            },
            "entity_invulnerability_cooldown_tick": {
                "start": "0x00E682",
                "end": "0x00E6BA",
                "callers_count": 16,
                "notes": (
                    "Clears invulnerability bit 6 of +$36(a6). Checks flash timer +$9D; "
                    "if < 20, sets bit 6 to blink. Decrements hitstun cooldown +$82; "
                    "if still active, aborts caller execution via addq.l #4, a7."
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
