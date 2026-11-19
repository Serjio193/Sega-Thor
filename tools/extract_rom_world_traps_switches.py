#!/usr/bin/env python3
"""
Extract ROM Interactive World Objects: Moving Bridge/Dust Streamer, Falling
Stalactite Ceiling Trap, Floor Pressure Switch & Hit-Toggle Crystal Switch
(0x016A64..0x016CBE, 602 bytes).

Disassembles exact M68K instructions, structures metadata, and generates
evidence JSON for Milestone M15.518.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_world_traps_switches_m15_518.json"
)

RANGE_START = 0x016A64
RANGE_END = 0x016CBE
RANGE_SIZE = RANGE_END - RANGE_START  # 602 bytes


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
            "name": "Object_MovingBridge_DustStreamer",
            "start": "0x016A64",
            "end": "0x016ADE",
            "size_bytes": 0x016ADE - 0x016A64,
            "description": (
                "Moving bridge particle / dust streamer. Mode +$9E: 0 = X-axis motion with "
                "(PRNG << 1) Z-height jitter, steps +$8 by +$4E, despawns (+0=0) when crossing +$A0 target bound; "
                "1 = Y-axis motion with (PRNG << 2) Z-height jitter, steps +$C by +$52, despawns when crossing +$A0."
            )
        },
        {
            "name": "Object_FallingStalactite_Trap",
            "start": "0x016ADE",
            "end": "0x016BD0",
            "size_bytes": 0x016BD0 - 0x016ADE,
            "description": (
                "Falling stalactite ceiling trap. Sub-state +$9E: 0 = ceiling initialization (z=0xF0=240), "
                "PRNG wait timer (d0 & $3F) + 4 (or immediate drop timer=1, vz=-$18 if +$A0!=0), sets +$36:1 -> State 1; "
                "1 = viewport culling 0x00E5AE(4) + timer countdown -> random anim 0 or 4 via $FF1971:0 -> State 2; "
                "2 = falling ballistics with gravity -$C000, AABB collision check 0x00B852 [-1..+1, -1..+1] vs Ali "
                "(shatters on non-flying target +$38:2 == 0), floor collision (z < 0) clamps z=1, advances anim +2 -> State 3; "
                "3 = shatter animation: on anim finished +$37:1, respawns to State 0 if +$A0==0, else despawns (+0=0)."
            )
        },
        {
            "name": "Object_FloorPressureSwitch",
            "start": "0x016BD0",
            "end": "0x016C5C",
            "size_bytes": 0x016C5C - 0x016BD0,
            "description": (
                "Floor pressure switch. Sets height z = $FFF0 (-16). Mode +$9E: 0 = momentary switch, "
                "1 = one-shot latched switch. Performs AABB check vs Ali 0x00B852 [-6..+10, +6..+22]. "
                "Momentary: sets/clears persistent bit via 0x018030 and updates anim 2 (pressed) / 0 (released). "
                "One-shot latch: checks persistent bit; if unpressed, detects player contact, sets bit, plays latch "
                "SFX #$900229 via 0x00E7FC, and permanently locks anim 2."
            )
        },
        {
            "name": "Object_HitToggleCrystalSwitch",
            "start": "0x016C5C",
            "end": "0x016CBE",
            "size_bytes": 0x016CBE - 0x016C5C,
            "description": (
                "Hit-toggle crystal switch. Retrieves persistent bit address/index via 0x018030. "
                "Manages 10-frame invulnerability cooldown timer +$2 (clearing +$82 on expiration). On weapon hit "
                "(+$86 != 0): clears damage and flinch, sets invulnerability guard +$82=$FF, resets cooldown +$2=10, "
                "toggles persistent bit via bchg.b, plays chime SFX #$900229, and updates anim: 2 if bit set, 0 if clear."
            )
        }
    ]

    evidence = {
        "milestone": "M15.518",
        "title": "ROM Interactive World Objects: Moving Bridge/Dust Streamer, Falling Stalactite Ceiling Trap, Floor Pressure Switch & Hit-Toggle Crystal Switch",
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
