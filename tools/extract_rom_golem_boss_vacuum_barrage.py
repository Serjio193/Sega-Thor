#!/usr/bin/env python3
"""
Extract ROM Multi-Part Boss/Golem (0x9A) Vacuum Pull, Boulder Barrage Spawner,
Body Knockback & Camera-Relative SFX Helpers (0x0154E8..0x015758, 624 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.509.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_golem_boss_vacuum_barrage_m15_509.json"
)

RANGE_START = 0x0154E8
RANGE_END = 0x015758
RANGE_SIZE = RANGE_END - RANGE_START  # 624 bytes


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
            "name": "AI_GolemBoss_State04_VacuumPull",
            "start": "0x0154E8",
            "end": "0x01552E",
            "size_bytes": 0x01552E - 0x0154E8,
            "description": (
                "State $04 Vacuum Pull & Spore Emitter. Decrements timer +$2(a6); on expiration "
                "transitions to reset helper 0x01563C. Every 4th frame (d0 & 3 == 0) calls spore/rock "
                "spawner 0x01569E. Computes directional pull vector toward boss via 0x00E45E, scaled "
                "2x on ground or 4x when airborne (+0x10 != 0), accumulating directly into Ali pos."
            )
        },
        {
            "name": "AI_GolemBoss_State02_BoulderBarrage",
            "start": "0x01552E",
            "end": "0x01563C",
            "size_bytes": 0x01563C - 0x01552E,
            "description": (
                "State $02 Boulder/Debris Barrage Spawner. Decrements interval timer +$2; upon expiration "
                "decrements burst counter +$A6 until 0 (resetting to State 0 via 0x01563C). Sets next "
                "interval (PRNG & $F) + 2 frames. Allocates Pool 2 slot via $DA2A + $E66E at (x-$30, y, z=10). "
                "Every 8th boulder (+$A6 & 7 == 0) aims directly at Ali via 0x010684 with speed $C0000 and "
                "vz=$80000; otherwise calculates PRNG scatter velocities. Sets anim $0C, sub-mode +$A2=2, "
                "and plays SFX $70035C via camera-relative audio helper 0x01571A."
            )
        },
        {
            "name": "AI_GolemBoss_ResetToState0",
            "start": "0x01563C",
            "end": "0x015648",
            "size_bytes": 0x015648 - 0x01563C,
            "description": (
                "Resets Golem Boss to State 0 with a delay timer of 0x28 (40 frames)."
            )
        },
        {
            "name": "AI_GolemBoss_BodyContactKnockback",
            "start": "0x015648",
            "end": "0x01569E",
            "size_bytes": 0x01569E - 0x015648,
            "description": (
                "Main body contact collision callback. Checks Ali invulnerability (+0x38:3) and guard (+0x82); "
                "if vulnerable, inflicts 10 damage ($800A), knockback mode 3, horizontal knockback -$100000 "
                "(-16.0 px), vertical impulse $30000 (+3.0 px), and SFX $800208 via 0x60004. If Ali is invulnerable "
                "or guarding, applies continuous leftward push delta of -$40000 (-4.0 px)."
            )
        },
        {
            "name": "AI_GolemBoss_SpawnSporeRock",
            "start": "0x01569E",
            "end": "0x01571A",
            "size_bytes": 0x01571A - 0x01569E,
            "description": (
                "Spawns falling spore/rock debris entity in Pool 2 via $DA2A + $E66E. Spawns with PRNG "
                "offsets: x = boss_x - 176 + (prng1 >> 4), y = boss_y + (prng2 >> 2) + (prng2 >> 3), z = 30. "
                "Sets flying flag +$38:2, anim $10, State 0, sub-mode +$A2=3, links parent back-pointer, "
                "and plays SFX $80034F via camera-relative helper 0x01571A."
            )
        },
        {
            "name": "AI_GolemBoss_PlayCameraRelativeSFX",
            "start": "0x01571A",
            "end": "0x01573C",
            "size_bytes": 0x01573C - 0x01571A,
            "description": (
                "Positional sound effect dispatcher. Reads camera plane anchor coordinates from $FF17AE, "
                "offsets X by -$30 (48 px), and calls positional audio engine 0x00E7A6."
            )
        },
        {
            "name": "AI_GolemBoss_CheckSubMode4",
            "start": "0x01573C",
            "end": "0x015758",
            "size_bytes": 0x015758 - 0x01573C,
            "description": (
                "Checks previous entity slot -$BC(a6) in Pool 2 for Type $34 and anim timer +$30 == 8. "
                "On match, triggers screen shake / particle effect 0x00E96C with mode d0 = 3."
            )
        }
    ]

    evidence = {
        "milestone": "M15.509",
        "title": (
            "ROM Multi-Part Boss/Golem (0x9A) Vacuum Pull, Boulder Barrage Spawner, "
            "Body Knockback & Camera-Relative SFX Helpers"
        ),
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "routines": routines,
        "instruction_count": len(instructions),
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w") as f:
        json.dump(evidence, f, indent=2)

    print(
        f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) "
        f"across {len(routines)} routines to {OUTPUT_PATH}"
    )


if __name__ == "__main__":
    extract()
