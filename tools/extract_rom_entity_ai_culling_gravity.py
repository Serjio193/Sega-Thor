#!/usr/bin/env python3
"""
Extract ROM Entity AI Viewport Culling, Parent Targeting, Cutscene Locks
& 3D Ballistic Gravity Arc Physics Subsystem (0x010EEA..0x01108C, 418 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.479.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_entity_ai_culling_gravity_m15_479.json")

RANGE_START = 0x010EEA
RANGE_END = 0x01108C
RANGE_SIZE = RANGE_END - RANGE_START  # 418 bytes

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

    evidence = {
        "milestone": "M15.479",
        "title": "ROM Entity AI Viewport Culling, Parent Targeting, Cutscene Locks & 3D Ballistic Gravity Arc Physics Subsystem",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "routines": [
            {
                "name": "AI_CheckEntityInViewportWithMargin",
                "start": "0x010EEA",
                "end": "0x010F48",
                "size_bytes": 94,
                "description": "Checks if entity coordinates are within plane viewport [plane_x - d7, plane_x + 320 + d7] x [plane_y - d7, plane_y + 224 + d7]. Returns Carry clear if inside, Carry set if outside/culled."
            },
            {
                "name": "AI_FaceTowardsParentEntity",
                "start": "0x010F48",
                "end": "0x010F84",
                "size_bytes": 60,
                "description": "Calculates dominant axis delta to parent entity a5, setting facing +$16(a6) towards parent (0=Up, 1=Right, 2=Down, 3=Left)."
            },
            {
                "name": "AI_JumpToSpiritSoundEffect",
                "start": "0x010F84",
                "end": "0x010F94",
                "size_bytes": 16,
                "description": "Preserves registers, targets active Spirit slot at $FF1AA4, and invokes sound dispatcher 0x00E710."
            },
            {
                "name": "AI_CheckCutscenePlayerLockFlags",
                "start": "0x010F94",
                "end": "0x010FC8",
                "size_bytes": 52,
                "description": "Tests player lock $FF19EC, cutscene lock $FF1982, fade lock $FF1985, and menu lock $FF1987. Returns Carry set if unlocked, Carry clear if locked."
            },
            {
                "name": "Physics_BallisticGravityArcStep_Simple",
                "start": "0x010FC8",
                "end": "0x01101E",
                "size_bytes": 86,
                "description": "Applies gravity deceleration -0xC000 to +$56, updates elevation +$10. Detects ground landing (< 0), clears velocities, sets landing timer $9A=15, returns d0=1 (landed) or d0=0 (airborne with horizontal advance)."
            },
            {
                "name": "Physics_BallisticGravityArcStep_CollisionChecked",
                "start": "0x01101E",
                "end": "0x01108C",
                "size_bytes": 110,
                "description": "Applies gravity -0xC000, updates elevation, checks wall collision and invokes surface check 0x00BCD8, handling landing with 15-frame recovery."
            }
        ],
        "instruction_count": len(instructions),
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")

if __name__ == "__main__":
    extract()
