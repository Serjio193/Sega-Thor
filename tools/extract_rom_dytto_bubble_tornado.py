#!/usr/bin/env python3
"""
Extract ROM Dytto Spirit Magic Bubble Projectile, Flame/Enemy Stun AABB Scanner
& Water Tornado Vortex Subsystem (0x011D8A..0x0120D4, 842 bytes).

Disassembles exact M68K instructions, verifies key constants/offsets, and
generates evidence JSON for Milestone M15.487.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_dytto_bubble_tornado_m15_487.json"

RANGE_START = 0x011D8A
RANGE_END = 0x0120D4
RANGE_SIZE = RANGE_END - RANGE_START  # 842 bytes

ROUTINES = [
    {
        "name": "AI_Dytto_TryStartMagicBubble",
        "start": "0x011D8A",
        "end": "0x011DB6",
        "size_bytes": 0x011DB6 - 0x011D8A,
        "description": (
            "Consumes 5 SP via 0x011B28 (branching to 0x01219A on Carry set), reserves "
            "a Pool 1 slot via 0x00DA62 into +$A4(a6) (branching to 0x01219A on Carry set), "
            "copies $FF19A4 to +$16(a6), and transitions Dytto to State $0E (+$2A = 6)."
        ),
    },
    {
        "name": "AI_Dytto_State0E_CastAndLaunchMagicBubble",
        "start": "0x011DB6",
        "end": "0x011ED2",
        "size_bytes": 0x011ED2 - 0x011DB6,
        "description": (
            "Lowers +$10(a6) by 2 while +$30(a6) < 0x18 (falling back to 0x0123B4 if < 2 "
            "or +$30 >= 0x18). At frame +$30 == 0x18, converts reserved slot $FF19E8 + +$A4(a4) "
            "(if still Type $002E) into Type $0018 via 0x008D06, sprite #$00141374, priority "
            "(parent_18 & $8000) | $07A8, +$3D=8, +$36=0, +$38=$0E, 8x8x16 hitbox, +$A2=$FF, "
            "State $10 (+$2A=$1A), +$10 += $0A, launch offset $1800 (+$20000 X if facing Down=2) "
            "and velocity ($0400, $03C0) via 0x00E2D4, probes 0x009D00/0x009AD6 -> +$14(a6)=d4, "
            "and plays SFX #$800354. On +$37 bit 1 returns parent to State 0 (0x0121AA)."
        ),
    },
    {
        "name": "AI_Dytto_State10_16_MagicBubbleFlightAndImpact",
        "start": "0x011ED2",
        "end": "0x011FA6",
        "size_bytes": 0x011FA6 - 0x011ED2,
        "description": (
            "State $10 (0x011ED2..0x011F98) calls 0x00E36E, pops on wall contact ((+$40|+$41) "
            "bit 6 -> State $16), scans 0x00B922 (d0=$F0, box [-9..9, -9..9, -1..17]) for Flame "
            "$004C or Type $0098 (+$9E==1) -> sets +$66(a0)=$FF + SFX #$8004D4 -> State $16, or "
            "enemy checks (+$3A bit 1 -> pop, +$38 bit 3 -> fly/cull, else +$9D=($FF0DB6<<3)+80 "
            "and +$9C=($FF0DB6<<2)+20 + SFX #$8004D4 -> State $16 +$2A=$24). Otherwise culls via "
            "0x00E5AE (margin $18 -> clear $0(a6)) and integrates +$4E/+$52 into +$72/+$76. "
            "State $16 (0x011F98..0x011FA6) clears +$0(a6) when +$37 bit 1 is set."
        ),
    },
    {
        "name": "AI_Dytto_WaterTornadoLifecycle",
        "start": "0x011FA6",
        "end": "0x0120D4",
        "size_bytes": 0x0120D4 - 0x011FA6,
        "description": (
            "Command 3 entry (0x011FA6..0x011FF6) clears $FF199E, lowers +$10 by $28 (clamping "
            "32-bit +$10 to 0 on borrow), enters State $08 (+$2A=$0A), plays SFX #$80034E, sets "
            "angle +$4E=((5-$FF19FE)&3)<<6, step +$50=4, radial step +$51=20, radius +$52=$20000. "
            "State $08 (0x011FF6..0x01202A) orbits via 0x0123E2 until +$37 bit 1 -> State $0A "
            "(+$2A=$0C, +$3D=$10, +$02=($FF0DB6<<3)+120). State $0A (0x01202A..0x012086) drains "
            "+$8E(a6) by 8, attacks via 0x00BCA4, orbits via 0x0123E2, plays #$700340 when "
            "(+$02&7)==0, and enters State $0C (+$2A=$18, +$3D=0, +$10+=$28, +$02=$1E) when SP "
            "or timer hits 0. State $0C (0x012086..0x0120D4) attacks via 0x00BCA4, decays +$52 by "
            "$5000 (>=0), orbits via 0x0123E2, toggles +$3D ($10/0) with -/+$28 on +$10 every "
            "frame, and exits to State 0 (0x0121AA) when +$02 reaches 0."
        ),
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key opcodes, constants, and target addresses across 0x011D8A..0x0120D4."""
    assert u16_be(rom, 0x011D8A) == 0x7005      # moveq #$5, d0
    assert u32_be(rom, 0x011DA2) == 0x00FF19A4  # move.w $ff19a4.l, $16(a6)
    assert u16_be(rom, 0x011DB8) == 0x0018      # cmpi.w #$18, $30(a6)
    assert u32_be(rom, 0x011E12) == 0x00141374  # move.l #$141374, $1a(a6)
    assert u16_be(rom, 0x011E22) == 0x07A8      # ori.w #$7a8, d0
    assert u16_be(rom, 0x011E7E) == 0x1800      # move.w #$1800, d1
    assert u32_be(rom, 0x011E90) == 0x00020000  # addi.l #$20000, d1
    assert u16_be(rom, 0x011E9E) == 0x0400      # move.w #$400, d1
    assert u16_be(rom, 0x011EA2) == 0x03C0      # move.w #$3c0, d2
    assert u32_be(rom, 0x011EC6) == 0x00800354  # move.l #$800354, d0
    assert u16_be(rom, 0x011EFE) == 0x004C      # cmpi.w #$4c, $0(a0)
    assert u16_be(rom, 0x011F06) == 0x0098      # cmpi.w #$98, $0(a0)
    assert u32_be(rom, 0x011F4C) == 0x008004D4  # move.l #$8004d4, d0
    assert u16_be(rom, 0x011FAE) == 0x0028      # subi.w #$28, $10(a6)
    assert u32_be(rom, 0x011FC6) == 0x0080034E  # move.l #$80034e, d0
    assert u32_be(rom, 0x011FEE) == 0x00020000  # move.l #$20000, $52(a6)
    assert u16_be(rom, 0x012022) == 0x0078      # addi.w #$78, d0
    assert u32_be(rom, 0x012052) == 0x00700340  # move.l #$700340, d0
    assert u32_be(rom, 0x012090) == 0x00005000  # subi.l #$5000, $52(a6)


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE, f"Slice length mismatch: {len(slice_data)} != {RANGE_SIZE}"
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    disasm_bytes = 0
    for insn in md.disasm(slice_data, RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size,
        })
        disasm_bytes += insn.size

    assert disasm_bytes == RANGE_SIZE, f"Disassembled {disasm_bytes} != {RANGE_SIZE}"

    routines_meta = []
    for r in ROUTINES:
        r_start = int(r["start"], 16)
        r_end = int(r["end"], 16)
        r_insns = [
            ins for ins in instructions
            if r_start <= int(ins["address"], 16) < r_end
        ]
        routines_meta.append({
            "name": r["name"],
            "start": r["start"],
            "end": r["end"],
            "size_bytes": r["size_bytes"],
            "instruction_count": len(r_insns),
            "description": r["description"],
        })

    evidence = {
        "milestone": "M15.487",
        "title": "ROM Dytto Spirit Magic Bubble Projectile, Flame/Enemy Stun AABB Scanner & Water Tornado Vortex Subsystem",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE,
        },
        "constants": {
            "bubble_sp_cost": 5,
            "bubble_cast_launch_frame": 24,
            "bubble_reservation_type": "0x002E",
            "bubble_entity_type": "0x0018",
            "bubble_sprite_ptr": "0x00141374",
            "bubble_launch_offset": "0x1800",
            "bubble_down_x_nudge": "0x00020000",
            "bubble_velocity": {"vx": "0x0400", "vy": "0x03C0"},
            "bubble_scan_box": {"d0": "0xF0", "x": [-9, 9], "y": [-9, 9], "z": [-1, 17]},
            "bubble_cull_margin": 24,
            "tornado_z_drop": 40,
            "tornado_orbit_step": 4,
            "tornado_radial_step": 20,
            "tornado_orbit_radius": "0x00020000",
            "tornado_radius_decay": "0x00005000",
            "tornado_base_duration": 120,
            "tornado_winddown_duration": 30,
            "tornado_extra_drain_step": 8,
            "sfx": {
                "bubble_launch": "0x00800354",
                "bubble_impact": "0x008004D4",
                "tornado_start": "0x0080034E",
                "tornado_loop": "0x00700340",
            },
        },
        "ram_addresses": {
            "spirit_level_array": "0x00FF0DB6",
            "spirit_hold_counter": "0x00FF199E",
            "spirit_latched_facing": "0x00FF19A4",
            "player_entity_base": "0x00FF19E8",
            "player_facing_word": "0x00FF19FE",
        },
        "external_targets": {
            "entity_type_init": "0x008D06",
            "latch_ground_elevation": "0x009AD6",
            "terrain_ground_probe": "0x009D00",
            "scan_3d_interval": "0x00B922",
            "entity_attack_scan": "0x00BCA4",
            "alloc_pool1_slot": "0x00DA62",
            "compute_8dir_velocity": "0x00E2D4",
            "entity_collision_helper": "0x00E36E",
            "viewport_cull_check": "0x00E5AE",
            "play_sfx": "0x00E7FC",
            "try_consume_sp": "0x011B28",
            "command_fail_sfx": "0x01219A",
            "dytto_state0_dispatch": "0x0121AA",
            "dytto_hover_track": "0x0123B4",
            "dytto_orbit_step": "0x0123E2",
        },
        "routines": routines_meta,
        "instruction_count": len(instructions),
        "instructions": instructions,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    placeholder = "__INSTRUCTIONS_PLACEHOLDER__"
    raw_json = json.dumps({**evidence, "instructions": placeholder}, indent=2)
    insn_lines = ",\n".join(f"    {json.dumps(item)}" for item in instructions)
    formatted_json = raw_json.replace(f'"{placeholder}"', f"[\n{insn_lines}\n  ]") + "\n"
    assert json.loads(formatted_json) == evidence
    OUTPUT_PATH.write_text(formatted_json, encoding="utf-8")
    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
