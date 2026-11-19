#!/usr/bin/env python3
"""
Extract ROM Composite Serpent/Multi-Segment Boss States 0x00..0x16 (Grab, Slam,
Spit, Death Gravity Fall & 4-Segment Despawn Stagger) (0x015E0E..0x0160E2, 724 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.513.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_serpent_boss_states_m15_513.json"
)

RANGE_START = 0x015E0E
RANGE_END = 0x0160E2
RANGE_SIZE = RANGE_END - RANGE_START  # 724 bytes


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
            "name": "AI_CompositeSerpent_State00_AnchorSync",
            "start": "0x015E0E",
            "end": "0x015E3C",
            "size_bytes": 0x015E3C - 0x015E0E,
            "description": (
                "State $00 Anchor Offset Sync. Reads root entity at $FF1CD8, computes target offset "
                "(root.x - $20, root.y + $18), accumulates position deltas into +$72/+$76, and syncs "
                "Z height +$10 from root."
            )
        },
        {
            "name": "AI_CompositeSerpent_State08_Ascend",
            "start": "0x015E3C",
            "end": "0x015E56",
            "size_bytes": 0x015E56 - 0x015E3C,
            "description": (
                "State $08 Ascend. Decrements timer +$2; while non-zero, increments height +$10 by 3. "
                "On timer expiration (0), transitions to State $02 (hover/grab probe) with timer 17."
            )
        },
        {
            "name": "AI_CompositeSerpent_State02_HoverGrabProbe",
            "start": "0x015E56",
            "end": "0x015EEA",
            "size_bytes": 0x015EEA - 0x015E56,
            "description": (
                "State $02 Hover & Ali Grab Probe. Calls boundary check 0x016168; descends +$10 by 4. "
                "Evaluates Ali ($FF19E8): if grounded (+0x10 == 0) and vulnerable (+0x38:3 == 0) within "
                "AABB bounds (|x+4 - ali_x| <= 16 and |y+16 - ali_y| <= 16), executes grab: enters State $06 "
                "(anim $36, timer 17), negates trajectory multipliers +$A6/+$AA, locks Ali ($FF1982=1), "
                "sets grabbed flag +$36:1, and sets flying/invulnerability flags +$38 |= $0C. On miss, enters "
                "State $04 (anim $34, timer 17) and negates +$A6/+$AA."
            )
        },
        {
            "name": "AI_CompositeSerpent_State04_ReturnAscend",
            "start": "0x015EEA",
            "end": "0x015F04",
            "size_bytes": 0x015F04 - 0x015EEA,
            "description": (
                "State $04 Return Ascend. Calls 0x016168; increments height +$10 by 4 until boundary "
                "is reached, then resets to State $00 (anim $12)."
            )
        },
        {
            "name": "AI_CompositeSerpent_State06_SlamSpitSetup",
            "start": "0x015F04",
            "end": "0x015F34",
            "size_bytes": 0x015F34 - 0x015F04,
            "description": (
                "State $06 Grabbed Player Lift & Directional Dispatcher. Positions grabbed Ali via 0x016184, "
                "ascends +$10 by 4 via 0x016168 until top boundary. Evaluates arena X position $FF1CE0: "
                "if <= $100 branches to Rightward sequence (State $0A), if > $100 branches to Leftward "
                "sequence (State $14), setting timer 7."
            )
        },
        {
            "name": "AI_CompositeSerpent_States0A_0C_0E_RightwardSlam",
            "start": "0x015F34",
            "end": "0x015FE0",
            "size_bytes": 0x015FE0 - 0x015F34,
            "description": (
                "Rightward Slam & Spit Sequence (States $0A, $0C, $0E). State $0A: winds back left (+72 -= 3) "
                "for 7 frames -> State $0C. State $0C: lunges right (+72 += 16) for 5 frames -> State $0E, "
                "releases Ali, clears $FF1982, clears +$36:1 and +$38 bits, inflicts 20 HP damage ($8014), "
                "launches Ali rightward (vx=$200000, vz=-1, z-=32, x+=4, facing 1, knockback mode 1). "
                "State $0E: recovers left (+72 -= 4) for 13 frames -> State $00."
            )
        },
        {
            "name": "AI_CompositeSerpent_States14_16_LeftwardSlam",
            "start": "0x015FE0",
            "end": "0x01608C",
            "size_bytes": 0x01608C - 0x015FE0,
            "description": (
                "Leftward Slam & Spit Sequence (States $14, $16, recovery). State $14: winds back right (+72 += 3) "
                "for 7 frames -> State $16. State $16: lunges left (+72 -= 16) for 5 frames -> recovery, "
                "releases Ali, clears $FF1982, clears +$36:1 and +$38 bits, inflicts 20 HP damage ($8014), "
                "launches Ali leftward (vx=-$200000, vz=-1, z-=32, x-=4, facing 1, knockback mode 3). "
                "Recovery: recovers right (+72 += 4) for 13 frames -> State $00."
            )
        },
        {
            "name": "AI_CompositeSerpent_State10_DeathGravityFall",
            "start": "0x01608C",
            "end": "0x0160E2",
            "size_bytes": 0x0160E2 - 0x01608C,
            "description": (
                "State $10 Death Gravity Fall & 4-Segment Staggered Despawn. Applies gravity -$C000 to +$56 "
                "into Z height +$10 until ground impact (Z <= 0). Clamps Z=0, triggers effect 0x00E96C mode 2, "
                "and configures 4 child segments for staggered death sequence: Segment 1 (+$122=3, timer +$BE=4), "
                "Segment 2 (+$1DE=3, timer +$17A=8), Segment 3 (+$29A=3, timer +$236=12), and Segment 4 "
                "(+$356=3, timer +$2F2=16)."
            )
        }
    ]

    death_stagger_config = [
        {"segment_index": 1, "stride_offset": 0x0BC, "mode_offset": 0x122, "mode": 3, "timer_offset": 0x0BE, "timer": 4},
        {"segment_index": 2, "stride_offset": 0x178, "mode_offset": 0x1DE, "mode": 3, "timer_offset": 0x17A, "timer": 8},
        {"segment_index": 3, "stride_offset": 0x234, "mode_offset": 0x29A, "mode": 3, "timer_offset": 0x236, "timer": 12},
        {"segment_index": 4, "stride_offset": 0x2F0, "mode_offset": 0x356, "mode": 3, "timer_offset": 0x2F2, "timer": 16}
    ]

    evidence = {
        "milestone": "M15.513",
        "title": "ROM Composite Serpent/Multi-Segment Boss States 0x00..0x16 (Grab, Slam, Spit, Death Gravity Fall & 4-Segment Despawn Stagger)",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "routines": routines,
        "instruction_count": len(instructions),
        "death_stagger_config": death_stagger_config,
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
