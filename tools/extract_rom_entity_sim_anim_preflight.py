#!/usr/bin/env python3
"""Extract and verify ROM universal entity simulation machine, animation frame stepper & terrain pre-flight (0x008F22..0x009084)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_entity_sim_anim_preflight_m15_398.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x008F22: timers and freeze flag check
    assert u16(rom, 0x008F22) == 0x362E          # move.w $9C(a6), d3
    assert u16(rom, 0x008F24) == 0x009C
    assert u32(rom, 0x008F4C + 4) == 0x00FF17B8  # btst.b #1, $FF17B8 (freeze)

    # 0x008F6C: slow motion check & velocity halving
    assert u32(rom, 0x008F84 + 4) == 0x00FF17B8  # btst.b #0, $FF17B8 (slow motion)
    assert u16(rom, 0x008F94) == 0xE280          # asr.l #1, d0 (+0x72)
    assert u16(rom, 0x008F9E) == 0xE280          # asr.l #1, d0 (+0x76)

    # 0x008FA4: animation frame stepper
    assert u16(rom, 0x008FAC) == 0x302E          # move.w $2A(a6), d0 (state)
    assert u16(rom, 0x008FAC + 2) == 0x002A
    assert u16(rom, 0x008FB4) == 0xB06E          # cmp.w $2C(a6), d0
    assert u16(rom, 0x008FB4 + 2) == 0x002C
    assert u16(rom, 0x008FC0) == 0x3D7C          # move.w #$FFFC, $30(a6)
    assert u16(rom, 0x008FC0 + 2) == 0xFFFC
    assert u16(rom, 0x008FCC) == 0x206E          # movea.l $26(a6), a0 (anim table)
    assert u16(rom, 0x008FCC + 2) == 0x0026
    assert u16(rom, 0x00900C) == 0x3D41          # move.w d1, $32(a6) (sprite ID)
    assert u16(rom, 0x00900C + 2) == 0x0032

    # 0x009040: collision preflight & layer select
    assert u32(rom, 0x009052 + 2) == 0x00FF1766  # lea $FF1766, a0 (Layer 1)
    assert u32(rom, 0x009058 + 4) == 0x00FF16F0  # btst.b #7, $FF16F0
    assert u32(rom, 0x009062 + 2) == 0x00FF1770  # lea $FF1770, a0 (Layer 2)

    # 0x009068: flying & velocity checks
    assert u16(rom, 0x009068) == 0x082E          # btst.b #1, $38(a6) (flying)
    assert u16(rom, 0x009084) == 0x1D7C          # move.b #$11, $40(a6) (+X move code)
    assert u16(rom, 0x009084 + 2) == 0x0011

    return {
        "milestone": "M15.398",
        "title": "ROM Universal Entity Simulation Machine, Animation Frame Stepper & Terrain Pre-flight",
        "routines": {
            "entity_sim_anim_preflight": {
                "start": "0x008F22",
                "end": "0x008F84",
                "bytes": 98,
                "freeze_flag_ram": "0x00FF17B8",
                "freeze_bit": 1,
                "slow_motion_ram": "0x00FF17B8",
                "slow_motion_bit": 0,
            },
            "anim_frame_stepper": {
                "start": "0x008F84",
                "end": "0x009040",
                "bytes": 188,
                "state_offset": "0x2A",
                "prev_state_offset": "0x2C",
                "anim_frame_offset": "0x30",
                "keyframe_timer_offset": "0x2E",
                "sprite_asset_offset": "0x32",
                "anim_table_ptr_offset": "0x26",
                "flags_37_offset": "0x37",
            },
            "terrain_collision_preflight": {
                "start": "0x009040",
                "end": "0x009084",
                "bytes": 68,
                "collision_dir_offset": "0x40",
                "layer_1_ram": "0x00FF1766",
                "layer_2_ram": "0x00FF1770",
                "layer_select_ram": "0x00FF16F0",
                "layer_select_bit": 7,
                "intangible_bit": 4,
                "flying_bit": 1,
                "right_move_code": "0x11",
                "branch_neg_x": "0x009128",
                "branch_y_sweep": "0x0091F6",
                "branch_flying": "0x009708",
            },
        },
    }


def main() -> None:
    rom = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
