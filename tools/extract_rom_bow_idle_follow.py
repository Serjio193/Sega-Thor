#!/usr/bin/env python3
"""
Extract ROM Bow Spirit (0x1E) State $00 Idle, Command Dispatch,
Vulnerability/Wander & Leashed Follow AI (0x013CB4..0x013F22, 622 bytes).

Disassembles exact M68K instructions across 0x013CB4..0x013F22 (zero embedded
data tables), verifies opcodes/constants/jump targets, and emits evidence JSON
for Milestone M15.499.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_bow_idle_follow_m15_499.json"

RANGE_START = 0x013CB4
RANGE_END = 0x013F22
RANGE_SIZE = RANGE_END - RANGE_START  # 622 bytes

ROUTINES = [
    {
        "name": "AI_Bow_State00_DispatchAndSubstateRouter",
        "start": "0x013CB4",
        "end": "0x013DAA",
        "size_bytes": 0x013DAA - 0x013CB4,
        "sub_entry_points": {
            "state_00_entry": "0x013CB4",
            "command_reject_continuation": "0x013CDC",
            "substate_router": "0x013CEE",
            "idle_prng_selector": "0x013D0C",
        },
        "description": (
            "State $00 entry (from jump table 0x0138E6): branches to dismissal "
            "0x013F2A when $FF19A7!=0, dispatches $FF19A2 (1->0x013924 Lunge Bite, "
            "2->0x013996 Dive, 3->0x013A18 Bloom). Re-entry 0x013CDC (from reject "
            "0x013A3A) fast-paths +$9E==3 to 0x013E26; otherwise calls 0x013FDC (face "
            "$FF1C1C stem anchor) and 0x0143C8 (auto-pounce enemy scan <= $36px into "
            "+$9E=3). Routes +$9E: 0->idle PRNG selector, 1->0x013DE0, 2->0x013DF2, "
            "3->0x013E26, 4->0x013EBC. At 0x013D0C clears +$38 bit 2, updates stem "
            "chain 0x014242(d7=6), and rolls 0x00E268 byte: <=$C0 hop (0x013DAA); "
            "$C1..$C8 +$2A=2 +$9E=4 SFX #$70025C; $C9..$D0 +$2A=4 +$9E=4 SFX #$700221; "
            "$D1..$D8 +$2A=$0A +$9E=4; $D9..$E0 +$2A=$0E +$9E=4 SFX #$700234; >$E0 "
            "rest timer +$02=(prng&$0F)+$0C and +$9E=1."
        ),
    },
    {
        "name": "AI_Bow_Substates_01_02_HopAndRest",
        "start": "0x013DAA",
        "end": "0x013E26",
        "size_bytes": 0x013E26 - 0x013DAA,
        "sub_entry_points": {
            "substate_00_start_hop": "0x013DAA",
            "substate_01_rest_wait": "0x013DE0",
            "substate_02_hop_physics": "0x013DF2",
        },
        "description": (
            "0x013DAA launches PRNG hop: +$56 = (u16(prng)<<2)+$30000, +$4E/+$52 = "
            "(s16(prng)<<1), sets +$9E=2. Substate 1 (0x013DE0) updates stem chain "
            "0x014242(d7=6) and decrements +$02 (clearing +$9E=0 on 0). Substate 2 "
            "(0x013DF2) integrates +$4E/+$52 into +$72/+$76, subtracts gravity $C000 "
            "from +$56 into +$10 (clearing +$10=0 & +$9E=0 when <0), and applies "
            "6px/seg stem leash via 0x014200(d7=6)."
        ),
    },
    {
        "name": "AI_Bow_Substate03_AutoPounceAttack",
        "start": "0x013E26",
        "end": "0x013EBC",
        "size_bytes": 0x013EBC - 0x013E26,
        "sub_entry_points": {
            "substate_03_auto_pounce": "0x013E26",
            "substate_03_exit_to_hop": "0x013E96",
        },
        "description": (
            "Auto-pounce Substate 3 (0x013E26): decrements +$02 (-> 0x013E96 on 0). "
            "If +$2A!=8, probes Pool 2 via 0x00BBA8 with hitbox table $05887C and "
            "sets +$2A=8 on Carry set. When +$2A==8, writes +$90=(($FF0DB9+1)>>2)*2, "
            "calls 0x00BCA4(d0=0), clears +$90, and exits to 0x013E96 on +$37 bit 1. "
            "Otherwise integrates +$4E/+$52 into +$72/+$76 and +$56 into +$10 "
            "(clamped >=0) with 8px/seg leash 0x014200(d7=8). Exit 0x013E96 sets "
            "+$2A=0, +$9E=2, zeroes +$4E/+$52/+$56, clears +$38 bit 2, and calls "
            "0x014200(d7=6)."
        ),
    },
    {
        "name": "AI_Bow_Substate04_EmoteAndSettle",
        "start": "0x013EBC",
        "end": "0x013F22",
        "size_bytes": 0x013F22 - 0x013EBC,
        "sub_entry_points": {
            "substate_04_emote_and_settle": "0x013EBC",
        },
        "description": (
            "Substate 4 (0x013EBC): plays SFX #$700235 via 0x00E7FC when +$32==$3F "
            "and +$2E==0; on +$37 bit 1 sets +$9E=2 and +$2A=0. Integrates "
            "+$4E/+$52 -> +$72/+$76 and gravity -$C000 on +$56 -> +$10 (if <0 zeroes "
            "+$10/+$4E/+$52), then applies 6px/seg leash via 0x014200(d7=6)."
        ),
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key opcodes, constants, and jump targets across 0x013CB4..0x013F22."""
    assert u32_be(rom, 0x013CB6) == 0x00FF19A7  # tst.b $ff19a7.l
    assert u32_be(rom, 0x013CC0) == 0x00FF19A2  # move.b $ff19a2.l, d0
    assert u16_be(rom, 0x013CDE) == 0x0003      # cmpi.b #$3, $9e(a6)
    assert u16_be(rom, 0x013D12) == 0x7E06      # moveq #$6, d7
    assert u16_be(rom, 0x013D1E) == 0x00C0      # cmpi.b #$c0, d0
    assert u16_be(rom, 0x013D26) == 0x00C8      # cmpi.b #$c8, d0
    assert u16_be(rom, 0x013D2C) == 0x00D0      # cmpi.b #$d0, d0
    assert u16_be(rom, 0x013D32) == 0x00D8      # cmpi.b #$d8, d0
    assert u16_be(rom, 0x013D38) == 0x00E0      # cmpi.b #$e0, d0
    assert u16_be(rom, 0x013D42) == 0x000F      # andi.w #$f, d0
    assert u16_be(rom, 0x013D46) == 0x000C      # addi.w #$c, d0
    assert u32_be(rom, 0x013D62) == 0x0070025C  # emote SFX #$70025C
    assert u32_be(rom, 0x013D7A) == 0x00700221  # emote SFX #$700221
    assert u32_be(rom, 0x013DA0) == 0x00700234  # emote SFX #$700234
    assert u32_be(rom, 0x013DB4) == 0x00030000  # hop vz base $30000
    assert u32_be(rom, 0x013E08) == 0x0000C000  # gravity step $C000
    assert u32_be(rom, 0x013E36) == 0x0005887C  # hitbox table $05887C
    assert u32_be(rom, 0x013E3C) == 0x0000BBA8  # jsr $bba8.l
    assert u32_be(rom, 0x013E4E) == 0x00FF0DB9  # Bow rank $FF0DB9
    assert u32_be(rom, 0x013E60) == 0x0000BCA4  # jsr $bca4.l
    assert u16_be(rom, 0x013E8E) == 0x7E08      # moveq #$8, d7
    assert u16_be(rom, 0x013EBE) == 0x003F      # cmpi.w #$3f, $32(a6)
    assert u32_be(rom, 0x013ECE) == 0x00700235  # emote SFX #$700235
    assert u32_be(rom, 0x013F00) == 0x0000C000  # gravity step $C000


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE
    verify_rom_constants(rom)

    md = capstone.Cs(capstone.CS_ARCH_M68K, capstone.CS_MODE_BIG_ENDIAN)
    md.detail = True

    instructions = []
    pos = RANGE_START
    for insn in md.disasm(slice_data, RANGE_START):
        instructions.append({
            "address": f"0x{insn.address:06X}",
            "mnemonic": insn.mnemonic,
            "op_str": insn.op_str,
            "bytes": insn.bytes.hex(),
            "size": insn.size,
        })
        pos = insn.address + insn.size
    assert pos == RANGE_END, f"Disassembly ended at 0x{pos:06X} != 0x{RANGE_END:06X}"

    routines_meta = []
    for r in ROUTINES:
        r_start = int(r["start"], 16)
        r_end = int(r["end"], 16)
        r_insns = [ins for ins in instructions if r_start <= int(ins["address"], 16) < r_end]
        routines_meta.append({
            "name": r["name"],
            "start": r["start"],
            "end": r["end"],
            "size_bytes": r["size_bytes"],
            "instruction_count": len(r_insns),
            "sub_entry_points": r["sub_entry_points"],
            "description": r["description"],
        })

    evidence = {
        "milestone": "M15.499",
        "title": "ROM Bow Spirit (0x1E) State $00 Idle, Command Dispatch, Vulnerability/Wander & Leashed Follow AI",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE,
        },
        "embedded_data_tables": [],
        "constants": {
            "bow_entity_type": "0x001E",
            "idle_leash_margin_px": 6,
            "pounce_leash_margin_px": 8,
            "prng_hop_max_byte": "0xC0",
            "prng_emote_bands": {
                "anim_02_sfx_70025c": "0xC1..0xC8",
                "anim_04_sfx_700221": "0xC9..0xD0",
                "anim_0a_silent": "0xD1..0xD8",
                "anim_0e_sfx_700234": "0xD9..0xE0",
                "rest_wait_substate_01": "0xE1..0xFF",
            },
            "rest_timer_base_frames": 12,
            "rest_timer_rand_mask": "0x0F",
            "hop_vz_base_fixed": "0x00030000",
            "gravity_step": "0x0000C000",
            "pounce_bite_anim_id": "0x0008",
            "substate04_sound_sprite_id": "0x003F",
            "sfx": {
                "emote_70025c": "0x0070025C",
                "emote_700221": "0x00700221",
                "emote_700234": "0x00700234",
                "emote_700235": "0x00700235",
            },
        },
        "ram_addresses": {
            "bow_level_byte": "0x00FF0DB9",
            "command_code_ff19a2": "0x00FF19A2",
            "dismiss_flag_ff19a7": "0x00FF19A7",
        },
        "external_targets": {
            "hitbox_table_05887c": "0x05887C",
            "proximity_box_scan": "0x00BBA8",
            "entity_attack_scan": "0x00BCA4",
            "prng_helper": "0x00E268",
            "play_sfx": "0x00E7FC",
            "bow_cmd1_lunge_bite": "0x013924",
            "bow_cmd2_dive": "0x013996",
            "bow_cmd3_bloom": "0x013A18",
            "bow_start_dismissal": "0x013F2A",
            "bow_face_stem_anchor": "0x013FDC",
            "bow_leashed_integrate": "0x014200",
            "bow_stem_chain_update": "0x014242",
            "bow_auto_pounce_scan": "0x0143C8",
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
    OUTPUT_PATH.write_text(formatted_json, encoding="utf-8", newline="\n")
    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
