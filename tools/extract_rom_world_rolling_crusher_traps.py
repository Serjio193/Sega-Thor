#!/usr/bin/env python3
"""
Extract ROM World Rolling/Sliding Crusher & Ceiling Crusher Traps
(0x0173D6..0x01778C, 950 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.522.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_world_rolling_crusher_traps_m15_522.json"
)

RANGE_START = 0x0173D6
RANGE_END = 0x01778C
RANGE_SIZE = RANGE_END - RANGE_START  # 950 bytes


def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    instructions = []
    for insn in md.disasm(slice_data, RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex()
        })

    routines = [
        {
            "name": "AI_WorldObject_RollingSlidingBoulder_Crusher",
            "start": "0x0173D6",
            "end": "0x017528",
            "size_bytes": 0x017528 - 0x0173D6,
            "instruction_count": 84,
            "description": (
                "Rolling / sliding boulder or crusher hazard. Dispatches on mode +$9E (0,1,2,3,4,5). "
                "Mode 0 (Idle): increments ticker +$2E while velocity deltas +$72|+$76 == 0. "
                "Mode 5 (Spawn Init): sets +$37 = 0x40, clears flying bit +$38:7, sets +$38:2, anim +$2A = 2, "
                "mode +$9E = 1, clears facing +$16, sets velocities +$4E = +$A0, +$52 = +$8C. "
                "Mode 1 (Rolling / Sliding Motion): copies velocities to deltas +$72/+$76, tests wall collision "
                "(+$40|+$41)&0x40 -> on wall collision triggers impact (anim 4, mode 2, screen shake $FF17C4 = 4, "
                "SFX #$800346). Else runs AABB collision test 0x00B922 [-16..+16, -16..+16, 0..32] vs player: "
                "inflicts 20 HP damage ($8014), upward impulse vz = $10000, transfers velocities +$4E/+$52, "
                "computes facing via 0x00E338, plays SFX #$800346 (#$900237 for Ali), transitions to wall impact. "
                "Mode 2 (Shatter / Despawn): when animation finished +$37:1 != 0, despawns entity (+0 = 0)."
            )
        },
        {
            "name": "AI_WorldObject_CeilingCrusher_Respawning",
            "start": "0x017528",
            "end": "0x017634",
            "size_bytes": 0x017634 - 0x017528,
            "instruction_count": 70,
            "description": (
                "Persistent-bit-triggered respawning ceiling crusher. Mode +$9E == 3, sub-state +$A6 (0..3). "
                "Sub-state 0 (Raised Ceiling Wait): sets anim +$2A = 6, z = +$8C, sets flag +$36:3. Polls persistent room "
                "bit via 0x018030: when set, advances +$A6 = 1 and sets drop timer +$2 = +$A0. "
                "Sub-state 1 (Countdown to Drop): decrements +$2. When 0: advances +$A6 = 2, clears vz +$56, clears +$36:3, "
                "sets +$36:2, clears shadow scale +$3D = 0, snaps position x = $FF19F0, y = $FF19F4 + 1 to target player, "
                "plays drop SFX #$800350. "
                "Sub-state 2 (Falling & Ground Impact): computes dynamic shadow scale +$3D = (3 - min(z >> 6, 3)) << 3, "
                "applies gravity vz -= $C000, integrates z += vz. On ground impact (z < 0): advances +$A6 = 3, clamps z = 0, "
                "sets flying bit +$38:2, anim +$2A = 4, triggers screen shake $FF17C4 = 4, plays impact SFX #$800346. "
                "Runs AABB collision test [-16..+16, -16..+16, 0..32] vs player calling heavy crush damage 0x017722. "
                "Sub-state 3 (Reset / Retract): on anim finished +$37:1 != 0, clears +$A6 = 0, clears +$38:2, and loops "
                "back to Sub-state 0 to raise back up."
            )
        },
        {
            "name": "AI_WorldObject_CeilingCrusher_OneShot",
            "start": "0x017634",
            "end": "0x017722",
            "size_bytes": 0x017722 - 0x017634,
            "instruction_count": 64,
            "description": (
                "Persistent-bit-triggered one-shot ceiling crusher. Mode +$9E == 4, sub-state +$A6 (0..3). "
                "Identical to respawning crusher with two critical distinctions: "
                "1) Sub-state 1 drop initiation drops at its fixed placement without snapping x,y to player coordinates. "
                "2) Sub-state 3 on impact animation finished despawns entity (+0 = 0) permanently instead of resetting."
            )
        },
        {
            "name": "Object_HeavyCrush_DamageCallback",
            "start": "0x017722",
            "end": "0x01778C",
            "size_bytes": 0x01778C - 0x017722,
            "instruction_count": 24,
            "description": (
                "Heavy crush damage collision callback routine. Verifies target vulnerability (checks invulnerability +$38:3, "
                "special immunity +$3A:0, and cooldown +$82). Sets attacker reference +$7E = a6, deals massive 40 HP crush "
                "damage (+$86 = $8028), clears defense cooldowns/flags (+$82 = 0, +$83 = 0, +$9D = 0), applies upward launch "
                "vz = $8000, z += 1, zeroes target horizontal velocity (+$4E = 0, +$52 = 0), sets knockback facing +$84 "
                "from target orientation +$16, and plays SFX #$900237 for Ali (+0 == 2) or #$800346 for other entities."
            )
        }
    ]

    evidence = {
        "milestone": "M15.522",
        "title": "ROM World Rolling/Sliding Crusher & Ceiling Crusher Traps",
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
