#!/usr/bin/env python3
"""
Extract ROM Serpent Boss Phase 1 States 0x00..0x18 (Hover, Bounds-Clamped Glide,
Dive Leap, Ground Slam Tremor & 4-Shot Fan Barrage) (0x0164E2..0x016764, 642 bytes).

Disassembles exact M68K instructions, unpacks embedded angle table, structures
metadata, and generates evidence JSON for Milestone M15.516.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_serpent_boss_phase1_states_m15_516.json"
)

RANGE_START = 0x0164E2
RANGE_END = 0x016764
RANGE_SIZE = RANGE_END - RANGE_START  # 642 bytes

TABLE_START = 0x016734
TABLE_END = 0x016738


def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    code1 = rom[RANGE_START:TABLE_START]
    table_bytes = list(rom[TABLE_START:TABLE_END])
    code2 = rom[TABLE_END:RANGE_END]

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    for insn in md.disasm(code1, RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    for insn in md.disasm(code2, TABLE_END):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size
        })

    routines = [
        {
            "name": "AI_SerpentBoss_State00_HoverWait",
            "start": "0x0164E2",
            "end": "0x0164F2",
            "size_bytes": 0x0164F2 - 0x0164E2,
            "description": (
                "State $00 Idle/Hover Wait. Calls 0x016764 (d5=0); decrements countdown timer +$2; "
                "when timer <= 0, branches to pattern action 0x016862."
            )
        },
        {
            "name": "AI_SerpentBoss_State16_DeathGravityFall",
            "start": "0x0164F2",
            "end": "0x01651C",
            "size_bytes": 0x01651C - 0x0164F2,
            "description": (
                "State $16 Death Gravity Fall. Applies gravity -$C000 to +$56 into height +$10; "
                "on ground impact (Z <= 0), clamps Z=0, calls cleanup 0x00F740 and effect 0x00E96C mode 12."
            )
        },
        {
            "name": "AI_SerpentBoss_State18_SettleWait",
            "start": "0x01651C",
            "end": "0x01652E",
            "size_bytes": 0x01652E - 0x01651C,
            "description": (
                "State $18 Settle Wait. Calls 0x016764 (d5=0); when event flag +$F3:1 is set, "
                "transitions to pattern sequencer 0x01685A."
            )
        },
        {
            "name": "AI_SerpentBoss_State02_ArenaGlide",
            "start": "0x01652E",
            "end": "0x01656A",
            "size_bytes": 0x01656A - 0x01652E,
            "description": (
                "State $02 Arena Glide & Footstep SFX. Calls 0x016764 (d5=-20), applies X-velocity "
                "(+$A2 << 16) clamped to [0x60..0x1A0] via 0x01656A. Increments step timer +$2; every "
                "8th frame plays SFX #$70035E via 0x00E7FC. On +$F3:1 -> transitions to 0x01685A."
            )
        },
        {
            "name": "AI_SerpentBoss_Helper_ClampXPosition",
            "start": "0x01656A",
            "end": "0x01658A",
            "size_bytes": 0x01658A - 0x01656A,
            "description": (
                "X-velocity clamping helper. Clamps arena X movement to bounds [0x60..0x1A0] (96..416 px). "
                "If moving right (d1 >= 0) and pos > 0x1A0, sets d1=0. If moving left (d1 < 0) and pos < 0x60, "
                "sets d1=0. Accumulates into +$72(a6)."
            )
        },
        {
            "name": "AI_SerpentBoss_States04_06_08_0A_WindupArmWait",
            "start": "0x01658A",
            "end": "0x0165E2",
            "size_bytes": 0x0165E2 - 0x01658A,
            "description": (
                "Attack Windup & Arm Wait states. State $04: on +$F3:1 -> State 6, timer +$2=20, "
                "anim 0x016A46($2A). State $06: timer +$2 countdown -> State 8, anim 0x016A46($E). "
                "State $08: on +$F3:1 -> 0x01685A. State $0A: waits for child arm slot $FF2084 +$4 == 0 -> 0x016862."
            )
        },
        {
            "name": "AI_SerpentBoss_States0C_0E_10_DiveLeapGroundSlam",
            "start": "0x0165E2",
            "end": "0x0166D2",
            "size_bytes": 0x0166D2 - 0x0165E2,
            "description": (
                "Dive Leap toward Ali & Ground Slam. State $0C: on +$F3:1, computes trajectory vector "
                "(ali_pos - boss_pos) / 42 (divs #$2A) into +$4E/+$52, sets vertical launch velocity +$56=$100000, "
                "enters State $0E, anim 0x016A46($26). State $0E: while +$EC > 0, applies gravity -$C000; "
                "while Z > 0x30, integrates velocities (Y + clamped X); on landing (Z <= 0x30), clamps Z=0x30, "
                "clears +$56=0, enters State $10, anim 0x016A46($22). State $10: when +$EC == 4, triggers screen "
                "shake $FF17C4=0x0C, plays impact SFX #$90021C and rumble SFX #$80035D. On +$F3:1 -> 0x01685A."
            )
        },
        {
            "name": "AI_SerpentBoss_States12_14_FanBarrage",
            "start": "0x0166D2",
            "end": "0x016764",
            "size_bytes": 0x016764 - 0x0166D2,
            "description": (
                "Fan Barrage Spawner & Exit. State $12: on +$F3:1 -> State $14, +$2A=2, anim 0x016A46($2A). "
                "State $14: at anim frame +$30 == 8, checks burst counter +$A0: if > 0, increments +$2E, decrements "
                "+$A0, and on even count spawns projectile via 0x0167BE using 4-byte angle table at 0x016734 "
                "([0xB5, 0xCB, 0x9F, 0xE1]) at speed $30000; if +$A0 == 0, spawns aimed/center projectile (angle $C0, "
                "speed $40000). On anim finish +$37:1 -> resets to State 8, +$2A=0, anim 0x016A46($E)."
            )
        }
    ]

    angle_fan_table = {
        "address": f"0x{TABLE_START:06X}",
        "size_bytes": 4,
        "values": table_bytes,
        "hex_values": [f"0x{b:02X}" for b in table_bytes],
        "description": "4-byte projectile fan spread angles [0xB5, 0xCB, 0x9F, 0xE1] (181, 203, 159, 225 steps)."
    }

    evidence = {
        "milestone": "M15.516",
        "title": "ROM Serpent Boss Phase 1 States 0x00..0x18 (Hover, Bounds-Clamped Glide, Dive Leap, Ground Slam Tremor & 4-Shot Fan Barrage)",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE
        },
        "routines": routines,
        "instruction_count": len(instructions),
        "angle_fan_table": angle_fan_table,
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes, 4-byte table) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
