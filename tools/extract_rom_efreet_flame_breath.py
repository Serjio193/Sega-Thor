#!/usr/bin/env python3
"""
Extract ROM Efreet 7-Particle Flame Breath Ring Engine, Enemy Ignite Handler &
Composite Vertex Exporter (0x012C4E..0x012F1E, 720 bytes).

Disassembles exact M68K instructions across the 4 executable code segments
(0x012C4E..0x012C92, 0x012C96..0x012D80, 0x012DAE..0x012EA0, 0x012EA8..0x012F1E),
extracts the 3 inline data tables (0x012C92..0x012C96, 0x012D80..0x012DAE,
0x012EA0..0x012EA8), verifies key constants, and emits evidence JSON for
Milestone M15.492.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_efreet_flame_breath_m15_492.json"

RANGE_START = 0x012C4E
RANGE_END = 0x012F1E
RANGE_SIZE = RANGE_END - RANGE_START  # 720 bytes

SWEEP_TABLE_START = 0x012C92
SWEEP_TABLE_END = 0x012C96
ANIM_TABLE_START = 0x012D80
ANIM_TABLE_END = 0x012DAE
SPAWN_TABLE_START = 0x012EA0
SPAWN_TABLE_END = 0x012EA8

CODE_SEGMENTS = [
    (0x012C4E, 0x012C92),
    (0x012C96, 0x012D80),
    (0x012DAE, 0x012EA0),
    (0x012EA8, 0x012F1E),
]

EXPECTED_SWEEP_ANGLES = [0x20, 0xE0, 0xA0, 0x60]
EXPECTED_ANIM_ENTRIES = [
    (0x0063, 1),
    (0x0065, 1),
    (0x0067, 0),
    (0x0068, 0),
    (0x0069, 0),
    (0x0068, 0),
    (0x0067, 0),
    (0x0068, 0),
    (0x0069, 0),
    (0x006A, 0),
    (0x006B, 0),
]
EXPECTED_SPAWN_OFFSETS = [
    (0, -40),    # Facing 0 (Up): 0x00, 0xD8
    (32, -12),   # Facing 1 (Right): 0x20, 0xF4
    (0, 12),     # Facing 2 (Down): 0x00, 0x0C
    (-32, -12),  # Facing 3 (Left): 0xE0, 0xF4
]

ROUTINES = [
    {
        "name": "AI_Efreet_InitFlameBreathParticlesAndAngle",
        "start": "0x012C4E",
        "end": "0x012CDC",
        "size_bytes": 0x012CDC - 0x012C4E,
        "code_bytes": (0x012C92 - 0x012C4E) + (0x012CDC - 0x012C96),
        "table_bytes": SWEEP_TABLE_END - SWEEP_TABLE_START,
        "embedded_table": "0x012C92..0x012C96",
        "description": (
            "Marks all 7 particle slots at (a0)+$04 (stride $14) free via st.b $0(a1). "
            "Polls latched D-pad bits on $FF19A3 in priority Up(0)/Down(1)/Right(3)/Left(2) "
            "to latch fixed facing +$16, +$AA=0, and angle +$AB in {$40,$C0,$00,$80}. If no "
            "direction bit is set, indexes 0x012C92[+$16] ([0x20,0xE0,0xA0,0x60]), sets "
            "+$AA=$FF (sweep mode), and writes start angle to +$AB."
        ),
    },
    {
        "name": "AI_Efreet_UpdateFlameBreathParticles",
        "start": "0x012CDC",
        "end": "0x012DAE",
        "size_bytes": 0x012DAE - 0x012CDC,
        "code_bytes": 0x012D80 - 0x012CDC,
        "table_bytes": ANIM_TABLE_END - ANIM_TABLE_START,
        "embedded_table": "0x012D80..0x012DAE",
        "description": (
            "Iterates 7 particle slots at +$A6(a6)+$04 (stride $14), skipping +$00 != 0. "
            "Decrements +$12 timer; on < 0 increments +$13 index across 11-entry table "
            "0x012D80 (u16 tile, u16 duration) terminated by $FFFF (marking slot free with "
            "st.b $0(a1) and skipping tick). Integrates +$02 += +$0A, +$0E -= $4000, "
            "+$06 += +$0E, and scans 0x00B922 (d0=$F0, box [px-8..px+8, py-8..py+8, 4..20]) "
            "calling 0x012DAE if +$38(a0) bit 3 is clear and +$82(a0) == 0."
        ),
    },
    {
        "name": "AI_Efreet_IgniteEnemyOnFlameHit",
        "start": "0x012DAE",
        "end": "0x012DFA",
        "size_bytes": 0x012DFA - 0x012DAE,
        "code_bytes": 0x012DFA - 0x012DAE,
        "description": (
            "Applies flame breath impact to target a0: sets attacker link +$7E(a0)=a6, "
            "clears +$9D(a0)=0, plays SFX #$00700345 via 0x00E7FC, copies +$16(a6) to "
            "+$84(a0), projects (0x300, 0x300) velocity via 0x00E2D4 into +$4E/+$52(a0), "
            "sets upward velocity +$56(a0)=$00020000, and writes fire damage word "
            "+$86(a0) = $A004 + (($FF0DB7 + 1) >> 2)."
        ),
    },
    {
        "name": "AI_Efreet_SpawnFlameParticle",
        "start": "0x012DFA",
        "end": "0x012EA8",
        "size_bytes": 0x012EA8 - 0x012DFA,
        "code_bytes": 0x012EA0 - 0x012DFA,
        "table_bytes": SPAWN_TABLE_END - SPAWN_TABLE_START,
        "embedded_table": "0x012EA0..0x012EA8",
        "description": (
            "Finds first free particle slot (+$00(a1) != 0) or returns Carry set if all 7 "
            "slots are active. Initializes +$00=$0063, +$12=3, +$13=0; if +$AA != 0 advances "
            "+$AB by +5 (mod 256) before computing velocities (+cos*0x700, -sin*0x700 + "
            "$20000) via $05D706; initializes +$02/+$06 from 4x2-byte signed offset table "
            "0x012EA0[+$16] << 16 and returns Carry clear."
        ),
    },
    {
        "name": "AI_Efreet_ExportFlameVertexBuffer",
        "start": "0x012EA8",
        "end": "0x012F1E",
        "size_bytes": 0x012F1E - 0x012EA8,
        "code_bytes": 0x012F1E - 0x012EA8,
        "description": (
            "Scans all 7 particle slots, appending 3-byte tuples (+$01 tile, dx, +$07 dy) "
            "into $FF19A9.. (negating 8-bit dx = +$03 when +$16(a6) == 3 Left). If 0 active "
            "particles clears +$06(a6)=0; else writes header (count-1) | ($80 if +$16!=2) "
            "to $FF19A8, sets +$06(a6)=$1A, +$5E(a6)=0, +$5A(a6)=$07A8, +$62(a6)=$FF19A8."
        ),
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key opcodes, constants, and targets across 0x012C4E..0x012F1E."""
    assert u16_be(rom, 0x012C54) == 0x0006      # move.w #$6, d0 (7 slots)
    assert u16_be(rom, 0x012C5C) == 0x0014      # lea.l $14(a1), a1 (20-byte stride)
    assert u32_be(rom, 0x012C64) == 0x00FF19A3  # move.b $ff19a3.l, d0
    assert u32_be(rom, 0x012D2E) == 0x00004000  # subi.l #$4000, d1 (upward Y accel)
    assert u16_be(rom, 0x012D40) == 0x00F0      # move.b #$f0, d0 (skip slots 0..3)
    assert u16_be(rom, 0x012D5C) == 0x8BC6      # bsr.w $00B922
    assert u32_be(rom, 0x012DB8) == 0x00700345  # move.l #$700345, d0
    assert u16_be(rom, 0x012DCA) == 0x0300      # move.w #$300, d1
    assert u16_be(rom, 0x012DCE) == 0x0300      # move.w #$300, d2
    assert u32_be(rom, 0x012DDE) == 0x00020000  # move.l #$20000, $56(a0)
    assert u32_be(rom, 0x012DE8) == 0x00FF0DB7  # move.b $ff0db7.l, d0
    assert u16_be(rom, 0x012DF2) == 0xA004      # addi.w #$a004, d0
    assert u16_be(rom, 0x012E1E) == 0x0063      # move.w #$63, $0(a1)
    assert u16_be(rom, 0x012E42) == 0x0700      # move.w #$700, d6
    assert u32_be(rom, 0x012E54) == 0x0005D706  # lea.l $5d706.l, a0
    assert u32_be(rom, 0x012E70) == 0x00020000  # addi.l #$20000, d3
    assert u32_be(rom, 0x012EB2) == 0x00FF19A9  # lea.l $ff19a9.l, a0
    assert u32_be(rom, 0x012EFC) == 0x00FF19A8  # lea.l $ff19a8.l, a1
    assert u16_be(rom, 0x012F04) == 0x001A      # move.w #$1a, $6(a6)
    assert u16_be(rom, 0x012F0E) == 0x07A8      # move.w #$7a8, $5a(a6)


def disasm_range(md: capstone.Cs, rom: bytes, start: int, end: int) -> list[dict]:
    items = []
    pos = start
    for insn in md.disasm(rom[start:end], start):
        items.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size,
        })
        pos = insn.address + insn.size
    assert pos == end, f"Disassembly ended at 0x{pos:06X} != 0x{end:06X}"
    return items


def extract_tables(rom: bytes) -> list[dict]:
    sweep_vals = list(rom[SWEEP_TABLE_START:SWEEP_TABLE_END])
    assert sweep_vals == EXPECTED_SWEEP_ANGLES, f"Sweep table mismatch: {sweep_vals}"

    anim_entries = []
    for i in range(11):
        entry_addr = ANIM_TABLE_START + i * 4
        tile_id = u16_be(rom, entry_addr)
        duration = u16_be(rom, entry_addr + 2)
        anim_entries.append((tile_id, duration))
    assert anim_entries == EXPECTED_ANIM_ENTRIES, f"Anim table mismatch: {anim_entries}"
    terminator = u16_be(rom, ANIM_TABLE_END - 2)
    assert terminator == 0xFFFF, f"Anim terminator mismatch: 0x{terminator:04X}"

    spawn_offsets = [
        struct.unpack_from(">bb", rom, SPAWN_TABLE_START + i * 2)
        for i in range(4)
    ]
    assert spawn_offsets == EXPECTED_SPAWN_OFFSETS, f"Spawn offset mismatch: {spawn_offsets}"

    return [
        {
            "name": "EfreetFlameSweepStartAngleTable",
            "start": f"0x{SWEEP_TABLE_START:06X}",
            "end": f"0x{SWEEP_TABLE_END:06X}",
            "size_bytes": SWEEP_TABLE_END - SWEEP_TABLE_START,
            "raw_hex": rom[SWEEP_TABLE_START:SWEEP_TABLE_END].hex(),
            "entries": [
                {"facing": idx, "angle": f"0x{val:02X}"}
                for idx, val in enumerate(sweep_vals)
            ],
        },
        {
            "name": "EfreetFlameAnimTable",
            "start": f"0x{ANIM_TABLE_START:06X}",
            "end": f"0x{ANIM_TABLE_END:06X}",
            "size_bytes": ANIM_TABLE_END - ANIM_TABLE_START,
            "raw_hex": rom[ANIM_TABLE_START:ANIM_TABLE_END].hex(),
            "terminator": "0xFFFF",
            "entries": [
                {"index": idx, "tile_id": f"0x{tile:04X}", "duration": dur}
                for idx, (tile, dur) in enumerate(anim_entries)
            ],
        },
        {
            "name": "EfreetFlameSpawnOffsetTable",
            "start": f"0x{SPAWN_TABLE_START:06X}",
            "end": f"0x{SPAWN_TABLE_END:06X}",
            "size_bytes": SPAWN_TABLE_END - SPAWN_TABLE_START,
            "raw_hex": rom[SPAWN_TABLE_START:SPAWN_TABLE_END].hex(),
            "entries": [
                {"facing": idx, "dx": dx, "dy": dy}
                for idx, (dx, dy) in enumerate(spawn_offsets)
            ],
        },
    ]


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    assert len(rom[RANGE_START:RANGE_END]) == RANGE_SIZE
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    for seg_start, seg_end in CODE_SEGMENTS:
        instructions.extend(disasm_range(md, rom, seg_start, seg_end))

    tables = extract_tables(rom)
    total_code_bytes = sum(ins["size"] for ins in instructions)
    total_table_bytes = sum(t["size_bytes"] for t in tables)
    assert total_code_bytes + total_table_bytes == RANGE_SIZE

    routines_meta = []
    for r in ROUTINES:
        r_start = int(r["start"], 16)
        r_end = int(r["end"], 16)
        r_insns = [ins for ins in instructions if r_start <= int(ins["address"], 16) < r_end]
        entry = {
            "name": r["name"],
            "start": r["start"],
            "end": r["end"],
            "size_bytes": r["size_bytes"],
            "code_bytes": r["code_bytes"],
            "instruction_count": len(r_insns),
            "description": r["description"],
        }
        if "table_bytes" in r:
            entry["table_bytes"] = r["table_bytes"]
            entry["embedded_table"] = r["embedded_table"]
        routines_meta.append(entry)

    evidence = {
        "milestone": "M15.492",
        "title": "ROM Efreet 7-Particle Flame Breath Ring Engine, Enemy Ignite Handler & Composite Vertex Exporter",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE,
            "code_bytes": total_code_bytes,
            "table_bytes": total_table_bytes,
        },
        "constants": {
            "particle_slot_count": 7,
            "particle_slot_base_offset": "0x04",
            "particle_slot_stride_bytes": 20,
            "particle_free_sentinel": "0xFF",
            "particle_initial_tile": "0x0063",
            "particle_initial_duration": 3,
            "particle_sweep_angle_step": 5,
            "particle_launch_speed": "0x0700",
            "particle_initial_vy_bias": "0x00020000",
            "particle_y_accel_step": "-0x00004000",
            "particle_scan_mask_d0": "0xF0",
            "particle_scan_box": {"x": [-8, 8], "y": [-8, 8], "z": [4, 20]},
            "ignite_sfx_id": "0x00700345",
            "ignite_knockback_speed": {"vx": "0x0300", "vy": "0x0300"},
            "ignite_upward_vz": "0x00020000",
            "ignite_base_damage_word": "0xA004",
            "composite_anim_mode": "0x001A",
            "composite_priority": "0x07A8",
        },
        "ram_addresses": {
            "efreet_level_byte": "0x00FF0DB7",
            "latched_joypad_dir": "0x00FF19A3",
            "composite_vertex_header": "0x00FF19A8",
            "composite_vertex_payload": "0x00FF19A9",
        },
        "external_targets": {
            "entity_pool_scan_aabb": "0x00B922",
            "compute_8dir_velocity": "0x00E2D4",
            "audio_play_sfx": "0x00E7FC",
            "sin_cos_q15_table": "0x0005D706",
        },
        "embedded_data_tables": tables,
        "routines": routines_meta,
        "instruction_count": len(instructions),
        "instructions": instructions,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    ins_placeholder = "__INSTRUCTIONS_PLACEHOLDER__"
    raw_json = json.dumps({**evidence, "instructions": ins_placeholder}, indent=2)
    ins_lines = ",\n".join(f"    {json.dumps(item)}" for item in instructions)
    formatted_json = raw_json.replace(f'"{ins_placeholder}"', f"[\n{ins_lines}\n  ]") + "\n"
    assert json.loads(formatted_json) == evidence
    OUTPUT_PATH.write_text(formatted_json, encoding="utf-8", newline="\n")
    print(
        f"Extracted {len(instructions)} instructions ({total_code_bytes} code + "
        f"{total_table_bytes} table = {RANGE_SIZE} bytes) to {OUTPUT_PATH}"
    )


if __name__ == "__main__":
    extract()
