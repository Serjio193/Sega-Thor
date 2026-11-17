#!/usr/bin/env python3
"""
extract_rom_projectile_physics.py — Extract ROM Spark Effect Dispatcher,
Projectile Gravity Fall, Lifetime Blink & Despawn Subsystem
(0x00E0BA..0x00E25C, 418 bytes).
"""

import json
import os
import struct
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_projectile_physics_m15_447.json")

START_ADDR = 0x00E0BA
END_ADDR = 0x00E25C


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

    # Word offset table at 0x00E140:
    # State 0: 0x0010 (+16 -> 0x00E150)
    # State 2: 0x0050 (+80 -> 0x00E192)
    state0_off = struct.unpack_from(">h", rom_data, 0x00E140)[0]
    state2_off = struct.unpack_from(">h", rom_data, 0x00E142)[0]

    evidence = {
        "routine": "projectile_spark_physics_lifecycle_subsystem",
        "description": (
            "ROM Spark Effect Dispatcher, Projectile Gravity Fall, "
            "Lifetime Blink & Despawn Subsystem"
        ),
        "start_address": f"0x{START_ADDR:06X}",
        "end_address": f"0x{END_ADDR:06X}",
        "byte_count": END_ADDR - START_ADDR,
        "instruction_count": len(instructions),
        "subroutines": {
            "dispatch_spark_effects": {
                "start": "0x00E0BA",
                "end": "0x00E0F4",
                "notes": "Iterates 6 slots at $FF2D8C (stride $5A). Checks pause bit 1 of $FF17B8. Calls +$56(a6)."
            },
            "dispatch_projectile_entities": {
                "start": "0x00E0F4",
                "end": "0x00E140",
                "notes": "Decrements $FF1856 timer. Sweeps 8 slots at $FF2ABC (stride $5A). Dispatches via +$04(a6)."
            },
            "projectile_jump_table": {
                "address": "0x00E140",
                "state_0_target": f"0x{0x00E140 + state0_off:06X}",
                "state_2_target": f"0x{0x00E142 + state2_off:06X}"
            },
            "projectile_gravity_fall_and_landing": {
                "start": "0x00E150",
                "end": "0x00E192",
                "notes": "Applies gravity ($C000 subtracted from +$56). On landing: clears parent latch bit 1 on +$36(a4) and frees slot."
            },
            "projectile_gravity_lifetime_blink_despawn": {
                "start": "0x00E192",
                "end": "0x00E20A",
                "notes": "Applies gravity. Increments lifetime timer +$22(a6). Blinks bit 1 on +$36 between 240..300 frames. Despawns at > 300 frames."
            },
            "projectile_bounce_decay_and_extents": {
                "start": "0x00E20A",
                "end": "0x00E25C",
                "notes": "Decays +$9E timer. Updates elevation +$10 to 8 or 12. Sets extents +$48, +$49 to 24px."
            }
        },
        "constants": {
            "gravity_subtrahend": "0x0000C000",
            "spark_slots": 6,
            "spark_base": "$FF2D8C",
            "projectile_slots": 8,
            "projectile_base": "$FF2ABC",
            "slot_stride": 90,
            "blink_min_frames": 240,
            "blink_max_frames": 300,
            "despawn_frames": 300
        },
        "instructions": instructions
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8") as f:
        json.dump(evidence, f, indent=2)

    print(f"Extracted {len(instructions)} instructions ({END_ADDR - START_ADDR} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract_instructions_and_evidence()
