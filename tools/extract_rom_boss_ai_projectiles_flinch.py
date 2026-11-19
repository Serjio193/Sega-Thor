#!/usr/bin/env python3
"""
Extract ROM Boss AI Projectile States $0C/$14/$12, Hit Callbacks & State $0E
Hit-Reaction Deceleration (0x014D1A..0x014F60, 582 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.505.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_boss_ai_projectiles_flinch_m15_505.json"
)

RANGE_START = 0x014D1A
RANGE_END = 0x014F60
RANGE_SIZE = RANGE_END - RANGE_START  # 582 bytes


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
            "name": "AI_Boss_State0C_FeatherSporeProjectile",
            "start": "0x014D1A",
            "end": "0x014DA0",
            "size_bytes": 0x014DA0 - 0x014D1A,
            "description": (
                "State $0C Boss feather/spore projectile. Counts down timer +$2(a6); on expiration "
                "runs 0x00B922 AABB vs Ali with bounds [-16..+16, -16..+16, -8..+16], calling hit "
                "callback 0x014DA0 on contact. Integrates position into +$72/+$76, accelerates vertical "
                "velocity +$56 by +$6000 into +$10. Evaluates life timer +$30(a6) against thresholds "
                "<=4, <=16, <=28 to scale size byte +$3D(a6) to 8, 16, 24, setting blink flag +$36:3 "
                "at end of life. Despawns slot +$00=0 when animation completes (+0x37:1 set)."
            )
        },
        {
            "name": "AI_Boss_HitCallback_FeatherSpore",
            "start": "0x014DA0",
            "end": "0x014E0C",
            "size_bytes": 0x014E0C - 0x014DA0,
            "description": (
                "Hit callback for feather/spore projectile against Ali (a0). Bypasses if Ali is "
                "invulnerable (+0x38:3). Assigns attacker pointer +$7E(a0)=a6, damage $200C (ground) "
                "or $A00C (airborne), hitstun 6 frames in +$83, knockback mode 2 in +$84, dispatches "
                "SFX #$700345 via 0x00E7FC. Copies XY velocities, sets Z velocity $28000 if airborne, "
                "asserts Ali hurt state $FF198E=$0E and invulnerability +$38:3, sets projectile cooldown 10."
            )
        },
        {
            "name": "AI_Boss_State14_ProjectileFadeout",
            "start": "0x014E0C",
            "end": "0x014E1A",
            "size_bytes": 0x014E1A - 0x014E0C,
            "description": (
                "State $14 projectile/flame impact fadeout. Tests animation completion flag bit 1 of "
                "+$37(a6); upon completion, clears entity slot +$00(a6)=0 to despawn."
            )
        },
        {
            "name": "AI_Boss_State12_FireballProjectile",
            "start": "0x014E1A",
            "end": "0x014E86",
            "size_bytes": 0x014E86 - 0x014E1A,
            "description": (
                "State $12 Boss fireball projectile. Transitions to State $14 impact fadeout (+0x2A=$16, "
                "+0x04=$14) if incoming damage +$86 is received or wall collision (+0x40|+0x41 bit 6) occurs. "
                "Runs 0x00B922 AABB vs Ali with bounds [-6..+6, -6..+6, -4..+6], invoking hit callback "
                "0x014E86 before entering fadeout. Integrates XYZ position, clamping Z >= 0 upon floor contact."
            )
        },
        {
            "name": "AI_Boss_HitCallback_Fireball",
            "start": "0x014E86",
            "end": "0x014EE6",
            "size_bytes": 0x014EE6 - 0x014E86,
            "description": (
                "Hit callback for fireball projectile against Ali (a0). Assigns attacker pointer +$7E(a0)=a6, "
                "damage $2008 (ground) or $A008 (airborne), hitstun 8 frames in +$83, computes knockback facing "
                "via 0x00E338 into +$84, plays SFX #$700345 via 0x00E7FC. Copies XY velocities and sets airborne "
                "vertical launch impulse $28000."
            )
        },
        {
            "name": "AI_Boss_State0E_HitReactionDecel",
            "start": "0x014EE6",
            "end": "0x014F60",
            "size_bytes": 0x014F60 - 0x014EE6,
            "description": (
                "State $0E Boss hit-reaction deceleration and counter-attack switch. Asserts flinch flag +$36:6. "
                "Applies friction step $8000 per frame to XY velocities toward zero, integrates position into "
                "+$72/+$76. Decelerates vertical velocity +$56 by -$A000 until <= 0. Once airborne arc ends "
                "and residual XY velocity is stopped ((d1|d2) & $FFFFF000 == 0), clears flinch flag +$36:6 "
                "and evaluates hit counter +$B6: every 4th hit ((+$B6 & 3) == 0) counter-attacks via 0x0149FE "
                "(State 8), otherwise transitions via 0x014A22 (State $10). Maintains invulnerability +$38:3 while moving."
            )
        }
    ]

    evidence = {
        "milestone": "M15.505",
        "title": (
            "ROM Boss AI Projectile States $0C/$14/$12, Hit Callbacks & State $0E "
            "Hit-Reaction Deceleration"
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
