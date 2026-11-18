#!/usr/bin/env python3
"""
Extract ROM Dytto Spirit Idle Follow, Auto-Rescue Heal Trigger,
Circular Orbit & Dismissal Physics Subsystem (0x0120D4..0x0123E2, 782 bytes).

Disassembles exact M68K instructions across 0x0120D4..0x0123E2, verifies key
constants and RAM symbols, and generates evidence JSON for Milestone M15.488.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_dytto_follow_dismissal_m15_488.json"

RANGE_START = 0x0120D4
RANGE_END = 0x0123E2
RANGE_SIZE = RANGE_END - RANGE_START  # 782 bytes

ROUTINES = [
    {
        "name": "AI_Dytto_ApproachStatesAndBuzzer",
        "start": "0x0120D4",
        "end": "0x0121AA",
        "size_bytes": 0x0121AA - 0x0120D4,
        "entrypoints": {
            "State02_Materialize": "0x0120D4",
            "State06_SummonFinish": "0x012104",
            "State1A_AnimSettle": "0x01212E",
            "State04_HomingApproach": "0x012150",
            "HomingVelocityHelper": "0x01215E",
            "InsufficientSpBuzzer": "0x01219A",
        },
        "description": (
            "States $02 (materialize: half-speed homing until +$37 bit 1 -> State $04, "
            "+$2A=8), $06 (summon finish: half-speed homing until +$37 bit 1 -> State $00), "
            "$1A (anim settle: on +$37 bit 1 clears +$2A then enters State $00), $04 "
            "(full-speed approach calling 0x0123B4 & 0x00E398; when d5 <= $20 enters "
            "State $06 +$2A=$16, else calls 0x00E45E with z-delta y+(ali_z-self_z) "
            "doubled into +$72/+$76), and 0x01219A (plays #$00A00253 via $060004, "
            "branches to 0x0121D2)."
        ),
    },
    {
        "name": "AI_Dytto_State00_IdleCommandAndFollowLoop",
        "start": "0x0121AA",
        "end": "0x012314",
        "size_bytes": 0x012314 - 0x0121AA,
        "entrypoints": {
            "State00_Entry": "0x0121AA",
            "AutoRescueCheck": "0x0121D2",
            "IdleFollowAndOrbit": "0x01220E",
        },
        "description": (
            "Checks dismissal flag $FF19A7 (-> 0x01231C), dispatches $FF19A2 command "
            "(1->0x011D8A, 2->0x011BCC, 3->0x011FA6), evaluates auto-rescue heal (+$AA "
            "cooldown $1E, $FF0DB6 >= 3, +$8A(a5) < +$88(a5)>>3, 20 SP via 0x011B28 "
            "-> 0x011BE0), tracks elevation (0x0123B4), and runs either idle circular "
            "orbit (+$A0=120, +$A1=40 delay, +$9E angle +/-6 by +$9F bit 1 via $05D706 "
            "with cos>>2 X and isometric (sin>>3)+(sin>>4) Y) or 18px ($12) 4-way "
            "behind-Ali follow via 0x00E45E (d4==0 arms orbit, 1<=d4<=2 halves vel "
            "and copies Ali facing, d4>2 full follow +$2A=2)."
        ),
    },
    {
        "name": "AI_Dytto_DismissalAndState1C_1E",
        "start": "0x012314",
        "end": "0x0123AE",
        "size_bytes": 0x0123AE - 0x012314,
        "entrypoints": {
            "DismissalGuard": "0x012314",
            "StartDismissal": "0x01231C",
            "State1C_DismissalFall": "0x012342",
            "State1E_DismissalDespawn": "0x012394",
        },
        "description": (
            "0x012314 guards +$04==0 (else 0x011B8A); 0x01231C plays SFX #$A0039E "
            "(0x00E7FC), sets facing 2, State $1C, anim $26 (0x008EF8), clears +$56. "
            "State $1C (0x012342) clears +$36 bit 1, waits +$30 >= $14, integrates "
            "gravity -$C000 on +$56/+$10 until < 0 -> clamps +$10=0, enters State $1E "
            "(+$2A=$24, sprite #$00141374, priority $07A8, +$36&=~1, +$36|=8, facing 0). "
            "State $1E (0x012394) clears +$36 bit 1, sets bit 3, despawns +$00=0 on +$37 bit 1."
        ),
    },
    {
        "name": "AI_Dytto_TrackTargetElevation",
        "start": "0x0123AE",
        "end": "0x0123E2",
        "size_bytes": 0x0123E2 - 0x0123AE,
        "entrypoints": {
            "Offset8px": "0x0123AE",
            "Offset48px": "0x0123B4",
        },
        "description": (
            "Two entrypoints (d4=$08 at 0x0123AE, d4=$30 / 48px at 0x0123B4) adjusting "
            "+$10(a6) by +/-1 (clamped >= 0) toward target height +$14(a5)+d4 using "
            "effective surface +$14(a6) (falling back to +$14(a5) if +$14(a6) < 0)."
        ),
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key opcodes, constants, and RAM addresses in 0x0120D4..0x0123E2."""
    assert u16_be(rom, 0x01215A) == 0x0020      # cmpi.w #$20, d5
    assert u32_be(rom, 0x01219C) == 0x00A00253  # buzzer SFX #$A00253
    assert u32_be(rom, 0x0121AC) == 0x00FF19A7  # tst.b $ff19a7.l
    assert u32_be(rom, 0x0121B6) == 0x00FF19A2  # move.b $ff19a2.l, d0
    assert u16_be(rom, 0x0121E4) == 0x0003      # cmpi.b #$3, $ff0db6.l
    assert u32_be(rom, 0x0121E6) == 0x00FF0DB6  # jewel rank $FF0DB6
    assert u16_be(rom, 0x012206) == 0x001E      # move.b #$1e, $aa(a6)
    assert u32_be(rom, 0x012264) == 0x0005D706  # sin/cos table $5D706
    assert u16_be(rom, 0x01228A) == 0x7012      # moveq #$12, d0 (18px)
    assert u16_be(rom, 0x0122EC) == 0x0078      # orbit timer 120 ($78)
    assert u16_be(rom, 0x0122F2) == 0x0028      # orbit delay 40 ($28)
    assert u32_be(rom, 0x01231E) == 0x00A0039E  # dismissal SFX #$A0039E
    assert u32_be(rom, 0x012338) == 0x00008EF8  # jsr $8ef8.l
    assert u32_be(rom, 0x012356) == 0x0000C000  # gravity step $C000
    assert u32_be(rom, 0x012376) == 0x00141374  # sprite table #$141374
    assert u16_be(rom, 0x01237E) == 0x07A8      # priority $07A8
    assert u16_be(rom, 0x0123B0) == 0x0008      # elevation offset 8
    assert u16_be(rom, 0x0123B6) == 0x0030      # elevation offset 48


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
            "entrypoints": r["entrypoints"],
            "description": r["description"],
        })

    evidence = {
        "milestone": "M15.488",
        "title": "ROM Dytto Spirit Idle Follow, Auto-Rescue Heal Trigger, Circular Orbit & Dismissal Physics Subsystem",
        "range": {"start": f"0x{RANGE_START:06X}", "end": f"0x{RANGE_END:06X}", "size_bytes": RANGE_SIZE},
        "constants": {
            "state04_approach_threshold_px": 32,
            "insufficient_sp_buzzer_sfx": "0x00A00253",
            "auto_rescue_cooldown_frames": 30,
            "auto_rescue_min_jewel_rank": 3,
            "auto_rescue_sp_cost": 20,
            "orbit_duration_frames": 120,
            "orbit_delay_frames": 40,
            "orbit_angle_step": 6,
            "sin_cos_table_rom": "0x05D706",
            "follow_offset_px": 18,
            "follow_slow_d4_max": 2,
            "dismissal_sfx": "0x00A0039E",
            "dismissal_anim_id": "0x0026",
            "dismissal_fall_start_frame": 20,
            "dismissal_gravity_fixed": "0x0000C000",
            "dismissal_land_state": "0x001E",
            "dismissal_land_anim": "0x0024",
            "dismissal_sprite_ptr": "0x00141374",
            "dismissal_priority": "0x07A8",
            "elevation_offset_low": 8,
            "elevation_offset_default": 48,
        },
        "ram_and_external_targets": {
            "dismiss_flag_ff19a7": "0x00FF19A7",
            "command_code_ff19a2": "0x00FF19A2",
            "jewel_rank_ff0db6": "0x00FF0DB6",
            "try_consume_sp": "0x011B28",
            "start_heal_silently": "0x011BE0",
            "start_magic_bubble": "0x011D8A",
            "start_healing_drop": "0x011BCC",
            "start_water_tornado": "0x011FA6",
            "master_blink_dispatch": "0x011B8A",
            "homing_velocity_helper": "0x00E45E",
            "manhattan_dist_ali": "0x00E398",
            "prng_helper": "0x00E268",
            "set_entity_anim": "0x008EF8",
        },
        "routines": routines_meta,
        "instruction_count": len(instructions),
        "instructions": instructions,
    }

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    placeholder = "__INSTRUCTIONS_PLACEHOLDER__"
    raw_json = json.dumps({**evidence, "instructions": placeholder}, indent=2)
    ins_lines = ",\n".join(f"    {json.dumps(item)}" for item in instructions)
    formatted_json = raw_json.replace(f'"{placeholder}"', f"[\n{ins_lines}\n  ]") + "\n"
    assert json.loads(formatted_json) == evidence
    OUTPUT_PATH.write_text(formatted_json, encoding="utf-8")
    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
