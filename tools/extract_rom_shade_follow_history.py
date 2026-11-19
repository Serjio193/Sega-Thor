#!/usr/bin/env python3
"""
Extract ROM Shade Spirit Idle Command Dispatcher, Hitstun Recovery,
Ascend Dismissal, Approach Homing & 2-Slot Ali Pose History Ring Buffer
(0x01366A..0x01387C, 530 bytes).

Disassembles exact M68K instructions, verifies constants/descriptors/RAM buffers,
and emits structured evidence JSON for Milestone M15.496.
"""

from __future__ import annotations

import json
from pathlib import Path
import struct

import capstone

ROOT_DIR = Path(__file__).resolve().parent.parent
ROM_PATH = ROOT_DIR / "local-roms" / "Beyond Oasis (USA).md"
OUTPUT_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_shade_follow_history_m15_496.json"

RANGE_START = 0x01366A
RANGE_END = 0x01387C
RANGE_SIZE = RANGE_END - RANGE_START  # 530 bytes

SHADE_BODY_DESC_ADDR = 0x03F4A6
ALI_DOPPELGANGER_DESC_ADDR = 0x03F306

ROUTINES = [
    {
        "name": "AI_Shade_State00_IdleCommandAndHistoryFollow",
        "start": "0x01366A",
        "end": "0x0136AC",
        "size_bytes": 0x0136AC - 0x01366A,
        "description": (
            "Propagates +$36(a5) bit 1 -> +$36(a6) bit 1 when set, checks dismissal flag "
            "$FF19A7 (-> 0x0136D4), dispatches $FF19A2 command codes (1->0x01301E, "
            "2->0x01319E, 3->0x013134), asserts $FF1989=$FF, and calls 0x0137F0."
        ),
    },
    {
        "name": "AI_Shade_State04_HitstunBlinkRecovery",
        "start": "0x0136AC",
        "end": "0x0136CC",
        "size_bytes": 0x0136CC - 0x0136AC,
        "description": (
            "Decrements +$83(a6); while non-zero sets +$36(a6) bit 6 (blink). When "
            "+$83(a6) hits 0, clears +$36(a6) bit 6 and +$39(a6) bit 3, and exits "
            "via 0x01378A into State 0 doppelganger mimic mode."
        ),
    },
    {
        "name": "AI_Shade_DismissalAndState10_AscendDespawn",
        "start": "0x0136CC",
        "end": "0x013716",
        "size_bytes": 0x013716 - 0x0136CC,
        "sub_entry_points": {
            "sp_drain_dismissal_guard": "0x0136CC",
            "start_dismissal": "0x0136D4",
            "state_10_ascend_despawn": "0x0136F8",
        },
        "description": (
            "0x0136CC guards +$04(a6)==0 (else 0x012F6A). 0x0136D4 plays SFX #$00A0039E "
            "via 0x00E7FC, calls 0x013764 ($03F4A6), clears +$16(a6)=0, sets State $10, "
            "and triggers anim $14 via 0x008EF8. State $10 (0x0136F8) clears +$36 bit 1 "
            "and ascends +$10(a6) by +8/frame until > $100 (256), then clears +$00(a6)=0."
        ),
    },
    {
        "name": "AI_Shade_State02_SummonApproachHoming",
        "start": "0x013716",
        "end": "0x013764",
        "size_bytes": 0x013764 - 0x013716,
        "description": (
            "Promotes +$2A(a6)=$14 once +$37(a6) bit 1 is set, calls 0x0123B8 (d4=1) "
            "for elevation tracking, computes Z-compensated Ali target Y (+ +$14(a5) - "
            "+$14(a6) when both >= 0) and calls 0x00E45E; if d4 <= 7 enters mimic via "
            "0x01378A, else shifts (d1, d2) left by 2 (4x) into +$72/+$76(a6)."
        ),
    },
    {
        "name": "AI_Shade_DescriptorSwitchAndPoseHistoryRing",
        "start": "0x013764",
        "end": "0x01387C",
        "size_bytes": 0x01387C - 0x013764,
        "sub_entry_points": {
            "switch_to_shade_body_desc": "0x013764",
            "enter_doppelganger_mimic_mode": "0x01378A",
            "step_pose_history_ring": "0x0137F0",
            "write_pose_snapshot": "0x01386E",
        },
        "description": (
            "0x013764 loads Shade body descriptor $03F4A6 (+$1A/+$1E/+$26), +$38=$0E, "
            "+$2C=$FFFF, +$34=$FFFF. 0x01378A sets State 0, loads doppelganger descriptor "
            "$03F306 (+$38=$0D), clears +$06/+$39/+$82/+$83/+$86, invalidates +$34=$FFFF, "
            "primes both 12-byte ring slots $FF19A8 and $FF19B4 via 0x01386E, and falls "
            "through to 0x0137F0. 0x0137F0 clears +$72/+$76, pops 6 words from $FF19B4 "
            "(x,y,z,ground_z,facing,anim) into Shade, locks +$2E=$03DE, clears +$39/+$6E/"
            "+$6F, copies 6 words $FF19A8->$FF19B4, and captures current Ali pose "
            "(x+vx,y+vy,z,ground_z,facing,anim) into $FF19A8 via 0x01386E."
        ),
    },
]


def u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def verify_rom_constants(rom: bytes) -> None:
    """Assert key opcodes, constants, descriptors, and RAM symbols in 0x01366A..0x01387C."""
    assert u32_be(rom, 0x01367A) == 0x00FF19A7  # tst.b $ff19a7.l
    assert u32_be(rom, 0x013684) == 0x00FF19A2  # move.b $ff19a2.l, d0
    assert u32_be(rom, 0x0136A2) == 0x00FF1989  # st.b $ff1989.l
    assert u32_be(rom, 0x0136D6) == 0x00A0039E  # move.l #$a0039e, d0
    assert u16_be(rom, 0x0136EE) == 0x0014      # move.w #$14, d0 (dismiss anim)
    assert u32_be(rom, 0x0136F2) == 0x00008EF8  # jsr $8ef8.l
    assert u16_be(rom, 0x013704) == 0x0100      # cmpi.w #$100, d0
    assert u16_be(rom, 0x013718) == 0x0014      # cmpi.w #$14, $2a(a6)
    assert u16_be(rom, 0x01372E) == 0x0001      # move.w #$1, d4
    assert u16_be(rom, 0x013752) == 0x0007      # cmpi.w #$7, d4
    assert u32_be(rom, 0x013766) == SHADE_BODY_DESC_ADDR
    assert u32_be(rom, 0x013792) == ALI_DOPPELGANGER_DESC_ADDR
    assert u32_be(rom, 0x0137E4) == 0x00FF19A8  # lea.l $ff19a8.l, a0
    assert u32_be(rom, 0x0137FA) == 0x00FF19B4  # lea.l $ff19b4.l, a0
    assert u16_be(rom, 0x013824) == 0x03DE      # move.w #$3de, $2e(a6)
    assert u16_be(rom, 0x013840) == 0x0005      # move.w #$5, d0 (6 words)


def extract() -> None:
    rom = ROM_PATH.read_bytes()
    slice_data = rom[RANGE_START:RANGE_END]
    assert len(slice_data) == RANGE_SIZE
    verify_rom_constants(rom)

    shade_desc = [f"0x{v:08X}" for v in struct.unpack_from(">III", rom, SHADE_BODY_DESC_ADDR)]
    ali_desc = [f"0x{v:08X}" for v in struct.unpack_from(">III", rom, ALI_DOPPELGANGER_DESC_ADDR)]

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
        entry = {
            "name": r["name"],
            "start": r["start"],
            "end": r["end"],
            "size_bytes": r["size_bytes"],
            "instruction_count": len(r_insns),
            "description": r["description"],
        }
        if "sub_entry_points" in r:
            entry["sub_entry_points"] = r["sub_entry_points"]
        routines_meta.append(entry)

    evidence = {
        "milestone": "M15.496",
        "title": "ROM Shade Spirit Idle Command Dispatcher, Hitstun Recovery, Ascend Dismissal, Approach Homing & 2-Slot Ali Pose History Ring Buffer",
        "range": {
            "start": f"0x{RANGE_START:06X}",
            "end": f"0x{RANGE_END:06X}",
            "size_bytes": RANGE_SIZE,
        },
        "constants": {
            "dismissal_sfx_id": "0x00A0039E",
            "dismissal_anim_id": "0x0014",
            "ascend_step_z": 8,
            "ascend_despawn_gt_threshold": 256,
            "approach_anim_2a": "0x0014",
            "approach_elevation_offset_d4": 1,
            "approach_arrival_threshold_d4": 7,
            "approach_velocity_shift_left": 2,
            "shade_body_descriptor_addr": f"0x{SHADE_BODY_DESC_ADDR:06X}",
            "shade_body_descriptor_ptrs": shade_desc,
            "shade_body_flags_38": "0x0E",
            "ali_doppelganger_descriptor_addr": f"0x{ALI_DOPPELGANGER_DESC_ADDR:06X}",
            "ali_doppelganger_descriptor_ptrs": ali_desc,
            "ali_doppelganger_flags_38": "0x0D",
            "pose_history_slot_words": 6,
            "pose_history_slot_bytes": 12,
            "pose_history_anim_timer_lock_2e": "0x03DE",
        },
        "ram_addresses": {
            "shade_mimic_active_flag": "0x00FF1989",
            "command_code": "0x00FF19A2",
            "dismiss_flag": "0x00FF19A7",
            "pose_history_slot0_current": "0x00FF19A8",
            "pose_history_slot1_delayed": "0x00FF19B4",
        },
        "external_targets": {
            "set_entity_anim": "0x008EF8",
            "homing_velocity_helper": "0x00E45E",
            "audio_play_sfx": "0x00E7FC",
            "track_target_elevation_d4": "0x0123B8",
            "shade_master_blink_guard": "0x012F6A",
            "shade_command1_handler": "0x01301E",
            "shade_command3_handler": "0x013134",
            "shade_command2_handler": "0x01319E",
        },
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
    print(f"Extracted {len(instructions)} instructions ({RANGE_SIZE} bytes) to {OUTPUT_PATH}")


if __name__ == "__main__":
    extract()
