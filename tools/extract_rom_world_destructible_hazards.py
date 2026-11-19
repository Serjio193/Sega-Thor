#!/usr/bin/env python3
"""
Extract ROM Interactive World Objects: Timed Switch, Destructible Block, Periodic
Pop-Up Hazard & Periodic Toggle/Vent Hazard (0x016EBC..0x0171C4, 776 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.520.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_world_destructible_hazards_m15_520.json"
)

RANGE_START = 0x016EBC
RANGE_END = 0x0171C4
RANGE_SIZE = RANGE_END - RANGE_START  # 776 bytes


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
            "name": "AI_WorldObject_TimedSwitch",
            "start": "0x016EBC",
            "end": "0x016F10",
            "size_bytes": 0x016F10 - 0x016EBC,
            "description": (
                "Timed world object / switch helper. One-shot init via +$A6 sets z = +$8C, height +$4A = 0x10 "
                "(if +$9E == 0) or sets layer bit +$3C:0 (if +$9E == 1). When triggered (+66 != 0), counts 6 frames; "
                "upon expiration despawns (+0 = 0) and sets persistent room bit via 0x018030."
            )
        },
        {
            "name": "AI_WorldObject_Destructible",
            "start": "0x016F10",
            "end": "0x016F7C",
            "size_bytes": 0x016F7C - 0x016F10,
            "description": (
                "Destructible block / object. One-shot init via +$A6 sets z = +$8C, max and current HP from +$A0. "
                "Active processing manages 6-frame invulnerability cooldown +$82. On weapon hit (+86 != 0): sets cooldown "
                "+$82 = 6, subtracts 12-bit masked damage from HP +$8A. On 0 HP: sets persistent bit via 0x018030, "
                "despawns (+0 = 0), and if +$9E != 0 offsets z by +$18 and triggers shatter effect 0x00E96C mode 0."
            )
        },
        {
            "name": "AI_WorldObject_PeriodicPopUpHazard",
            "start": "0x016F7C",
            "end": "0x0170E0",
            "size_bytes": 0x0170E0 - 0x016F7C,
            "description": (
                "Periodic pop-up spike/hazard trap. State +$66 (0..2): 0 = init timer +$2 = +$A0 -> State 1. "
                "1 = hidden countdown: on expiration checks persistent bit 0x018030; if clear, pops up -> State 2 "
                "(timer +$2 = +$9E, anim 2, layer +$3C:0 set, flying +$38:2 clear, height +$4A = 0x30, plays SFX #$6004DF "
                "and #$500260). 2 = active hazard: AABB check 0x00B922 [-8..+8, -2..+2] z < 47 inflicts 10 HP damage, "
                "directional knockback $400/$3C0 via 0x00E2D4, upward impulse vz = $20000, reaction mode +$83 = 8, "
                "and plays SFX #$80035B. When timer expires or persistent bit set, retracts back to State 1."
            )
        },
        {
            "name": "AI_WorldObject_PeriodicToggleVentHazard",
            "start": "0x0170E0",
            "end": "0x0171C4",
            "size_bytes": 0x0171C4 - 0x0170E0,
            "description": (
                "Periodic toggle steam/flame vent hazard. State +$66 (0..2): 0 = init timer +$2 = +$A0, z = +$8C -> State 1. "
                "1 = inactive wait: updates anim 0x00BC8E, clears +$2A on finish, counts timer +$2, checks persistent bit; "
                "on expiration opens vent -> State 2 (timer +$2 = +$9E, anim 2, layer +$3C:1 set, hazard depth +$49 = 0x38, "
                "plays SFX #$80034F via 0x00E7E8). 2 = active venting: checks persistent bit, loops anim 4 on finish, "
                "counts timer +$2; upon expiration closes vent -> State 1 (timer +$2 = +$A0, closing anim 6, clears layer +$3C:1 "
                "and hazard depth +$49)."
            )
        }
    ]

    evidence = {
        "milestone": "M15.520",
        "title": "ROM Interactive World Objects: Timed Switch, Destructible Block, Periodic Pop-Up Hazard & Periodic Toggle/Vent Hazard",
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
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
