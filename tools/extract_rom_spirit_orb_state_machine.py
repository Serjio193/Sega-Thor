#!/usr/bin/env python3
"""
Extract ROM Spirit Orb State 0 Scan, Catalyst Transmutation,
Trail Particle Emitter & Tilemap Sampler (0x01171C..0x011A4C, 816 bytes).

Disassembles exact M68K instructions across 0x01171C..0x011A3A, extracts the
18-byte embedded SpiritOrbTrailTileTable at 0x011A3A..0x011A4C, and generates
structured evidence JSON for Milestone M15.484.
"""

import json
import os
import capstone

ROM_PATH = os.path.join(os.path.dirname(__file__), "..", "local-roms", "Beyond Oasis (USA).md")
OUTPUT_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_spirit_orb_state_machine_m15_484.json"
)

RANGE_START = 0x01171C
RANGE_END = 0x011A4C
RANGE_SIZE = RANGE_END - RANGE_START  # 816 bytes

CODE_END = 0x011A3A
TABLE_START = 0x011A3A
TABLE_END = 0x011A4C
EXPECTED_TABLE = [16, 8, 16, 9, 16, 10, 16, 11, 12, 13, 12, 11, 14, 15, 14, 17, 18, 19]


def build_routines():
    return [
        {
            "name": "AI_SpiritOrb_State0_ElementalMediumAndCatalystScan",
            "start": "0x01171C",
            "end": "0x011810",
            "size_bytes": 244,
            "instruction_count": 69,
            "description": (
                "State 0 handler: checks cancel/force-summon flag $FF1990; decays 32-bit Z-elevation "
                "+$10(a6) by $18000 clamped >= 0; if airborne checks +$40|+$41 bit 6 (and bit 3 -> "
                "despawn at 0x0119C8); probes self bit 1 of +$39(a6) -> spirit 0, water/medium "
                "$FF17C2 <= 3 with +$6F bit 5, and shadow/medium $FF17C3 <= 3 with +$6F bit 6; "
                "otherwise runs entity AABB scan 0x00B922 (d0.b=$FC mask skipping slots 0..1, "
                "box x[-12,+12], y[-10,+10], z[0,+24]), handling Torch $0064 (+$02(a0)=1), Crystal "
                "$00B4 (+$A6(a0)=$FF), +$39(a0) bit 3 -> spirit 1, and +$3C(a0) bits 0..3 -> "
                "spirits 0..3."
            ),
        },
        {
            "name": "AI_SpiritOrb_TransitionToState1Or2",
            "start": "0x011810",
            "end": "0x011866",
            "size_bytes": 86,
            "instruction_count": 20,
            "description": (
                "Entry 0x011810 stores d0 into $FF19A6; entry 0x011818 checks unlocked spirit bit "
                "in $FF0D7A and inhibit bit 0 of $FF16F1, falling back to State 2 (0x011832) if "
                "locked/inhibited else State 1. Clears velocities +$4E/+$52, writes +$04=state, "
                "+$2A=2*state, adds 12 ($0C) to +$10.w, clears timer +$02, sets bit 3 on +$36, "
                "plays SFX #$8004D2 via 0x00E7FC, and tail-jumps to 0x0115C8."
            ),
        },
        {
            "name": "AI_SpiritOrb_ScanItemCatalysts",
            "start": "0x011866",
            "end": "0x011938",
            "size_bytes": 210,
            "instruction_count": 57,
            "description": (
                "Scans FX/item pool via 0x00B9EC (box x[-4,+4], y[-4,+4], z=0) for Type $0008 items. "
                "If $FF16F1 bit 0 is set, rebounds to State 2 (0x011832). When corresponding bit "
                "0..3 of $FF0D7A is unlocked, matches direct catalysts ($12..$15 -> spirits 0..3) "
                "or transmutes subtypes $1A->$21 (spirit 0), $1B->$05 with +$52(a0)=$14 (spirit 1), "
                "$1C->$11 (spirit 2), $1D->$2A and $27->$26 (spirit 3), setting +$22(a0)=$1E and "
                "clearing bit 1 of +$36(a0) before transitioning via 0x011810."
            ),
        },
        {
            "name": "AI_SpiritOrb_IntegrateAndUpdateTrail",
            "start": "0x011938",
            "end": "0x0119CE",
            "size_bytes": 150,
            "instruction_count": 46,
            "description": (
                "Calls 0x00E36E, integrates +$4E/+$52 into +$72/+$76, checks viewport culling via "
                "0x00E5AE (margin $10; off-screen despawns via 0x0119C8), decrements trail cooldown "
                "+$AC(a6) to allocate one of 6 trail slots +$A6..+$AB(a6) every 3 frames (value 1 "
                "with alternating bit 7 via +$AD(a6) ^= 1, cooldown reload 2), and loops over all "
                "6 slots calling 0x0119EC to emit $FF19A9.. vertex data with header 0x80|(count-1) "
                "at $FF19A8, +$06=$16, +$62=$FF19A8 (or +$06=0 when empty)."
            ),
        },
        {
            "name": "Tilemap_SampleLowNibbleAttribute",
            "start": "0x0119CE",
            "end": "0x0119EC",
            "size_bytes": 30,
            "instruction_count": 11,
            "description": (
                "Computes byte offset d4 = ((y >> 3) << +$08(a0)) + (x >> 3) using arithmetic "
                "shifts on (d1.w, d2.w), reads byte at (a1, d4.w) where a1 = +$00(a0), and "
                "returns the low nibble (d3 & $0F)."
            ),
        },
        {
            "name": "AI_SpiritOrb_EmitTrailParticleVertex",
            "start": "0x0119EC",
            "end": "0x011A4C",
            "size_bytes": 96,
            "instruction_count": 31,
            "code_end": "0x011A3A",
            "code_size_bytes": 78,
            "table_start": "0x011A3A",
            "table_end": "0x011A4C",
            "table_size_bytes": 18,
            "description": (
                "Emits 3-byte (tile_id, dx, dy) trail particle vertex rotated by 4-way facing "
                "+$66(a6) with -12 Y sprite anchor offset, reading tile_id from embedded 18-byte "
                "SpiritOrbTrailTileTable at 0x011A3A..0x011A4C, lateral jitter +1 (bit 7 clear) or "
                "-2 (bit 7 set), and radial distance 3*age+12. When age >= 18 ($12), writes $FF to "
                "(a0) and decrements d4 so caller's addq.b #1 wraps slot $FF -> $00."
            ),
        },
    ]


def build_semantics():
    return {
        "constants": {
            "z_decay_step_fixed": "0x00018000",
            "summon_z_boost_words": 12,
            "summon_sfx_id": "0x008004D2",
            "viewport_cull_margin_px": 16,
            "trail_slot_count": 6,
            "trail_spawn_cooldown_reload": 2,
            "trail_max_age_exclusive": 18,
            "trail_sprite_y_offset": -12,
            "trail_radial_step_px": 3,
            "trail_vertex_anim_id": "0x0016",
            "entity_type_torch": "0x0064",
            "entity_type_crystal": "0x00B4",
            "item_pool_type": "0x0008",
            "item_transmute_timer": "0x001E",
            "item_spirit1_param_52": "0x14",
        },
        "aabb_scans": {
            "entity_scan": {
                "helper": "0x00B922",
                "slot_mask_d0_byte": "0xFC",
                "box": {"x_min": -12, "y_min": -10, "x_max": 12, "y_max": 10, "z_min": 0, "z_max": 24},
            },
            "item_scan": {
                "helper": "0x00B9EC",
                "box": {"x_min": -4, "y_min": -4, "x_max": 4, "y_max": 4, "z_min": 0},
            },
        },
        "direct_item_catalysts": [
            {"spirit_id": 0, "subtype": "0x12"},
            {"spirit_id": 1, "subtype": "0x13"},
            {"spirit_id": 2, "subtype": "0x14"},
            {"spirit_id": 3, "subtype": "0x15"},
        ],
        "item_transmutations": [
            {"spirit_id": 0, "from_subtype": "0x1A", "to_subtype": "0x21", "extra_field_52": None},
            {"spirit_id": 1, "from_subtype": "0x1B", "to_subtype": "0x05", "extra_field_52": "0x14"},
            {"spirit_id": 2, "from_subtype": "0x1C", "to_subtype": "0x11", "extra_field_52": None},
            {"spirit_id": 3, "from_subtype": "0x1D", "to_subtype": "0x2A", "extra_field_52": None},
            {"spirit_id": 3, "from_subtype": "0x27", "to_subtype": "0x26", "extra_field_52": None},
        ],
        "ram_symbols": {
            "0x00FF0D7A": "Spirit unlock bitmask (bits 0..3)",
            "0x00FF16F1": "Environment spirit inhibit flags (bit 0 = inhibit summon)",
            "0x00FF17C2": "Elemental medium probe 1 (valid <= 3 when +$6F bit 5 set)",
            "0x00FF17C3": "Elemental medium probe 2 (valid <= 3 when +$6F bit 6 set)",
            "0x00FF1990": "Orb cancel / forced summon trigger flag",
            "0x00FF19A6": "Selected spirit index (0..3)",
            "0x00FF19A8": "Orb vertex list header (0x80 | (count - 1))",
            "0x00FF19A9": "Orb vertex list payload (3 bytes per trail particle)",
        },
        "helper_calls": {
            "0x00B922": "EntityPool_ScanAABB",
            "0x00B9EC": "FXItemPool_ScanAABB",
            "0x00E36E": "Entity_UpdateSupportShadowFlag",
            "0x00E5AE": "Viewport_CheckEntityBounds",
            "0x00E7FC": "Audio_PlaySFX",
            "0x0115C8": "AI_SpiritOrb_Main",
            "0x0119C8": "AI_SpiritOrb_Despawn",
        },
    }


def extract():
    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    disasm_end = RANGE_START
    for insn in md.disasm(rom[RANGE_START:CODE_END], RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size,
        })
        disasm_end = insn.address + insn.size

    assert disasm_end == CODE_END, f"Disassembly ended at 0x{disasm_end:06X} != 0x{CODE_END:06X}"

    table_values = list(rom[TABLE_START:TABLE_END])
    assert table_values == EXPECTED_TABLE, f"Table mismatch: {table_values} != {EXPECTED_TABLE}"

    evidence = {
        "milestone": "M15.484",
        "title": "ROM Spirit Orb State 0 Scan, Catalyst Transmutation, Trail Particle Emitter & Tilemap Sampler",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE,
        },
        "table": {
            "name": "SpiritOrbTrailTileTable",
            "address": f"0x{TABLE_START:06X}",
            "end": f"0x{TABLE_END:06X}",
            "size_bytes": len(table_values),
            "values": table_values,
        },
        "routines": build_routines(),
        "semantics": build_semantics(),
        "instruction_count": len(instructions),
        "instructions": instructions,
    }

    os.makedirs(os.path.dirname(OUTPUT_PATH), exist_ok=True)
    with open(OUTPUT_PATH, "w", encoding="utf-8", newline="\n") as f:
        json.dump(evidence, f, indent=2)
        f.write("\n")

    print(
        f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes, "
        f"{len(table_values)}-byte table) to {OUTPUT_PATH}"
    )


if __name__ == "__main__":
    extract()
