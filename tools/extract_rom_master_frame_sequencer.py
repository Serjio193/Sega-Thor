#!/usr/bin/env python3
"""Extract and verify ROM master frame sequencer, screen shake dispatcher & joypad menu hotkeys (0x008AA6..0x008C5A)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_master_frame_sequencer_m15_395.json")


def u16(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def u32(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">I", rom, addr)[0]


def extract_evidence(rom: bytes) -> dict:
    # 0x008AA6: master_frame_sequencer
    assert u32(rom, 0x008AA8) == 0x00FF1996  # tst.b $FF1996 (demo flag)
    assert u32(rom, 0x008AC0) == 0x00FF0D76  # addq.l #2, $FF0D76 (global frame counter)
    assert u32(rom, 0x008AC6) == 0x00FF1971  # addq.b #1, $FF1971 (anim sub-tick)
    assert u32(rom, 0x008ACC) == 0x00FF1654  # tst.w $FF1654
    assert u32(rom, 0x008AD6) == 0x00FF164D  # bset.b #3, $FF164D
    assert u32(rom, 0x008ADC) == 0x00FF199C  # addq.w #1, $FF199C
    assert u32(rom, 0x008AE4) == 0x00FF1654  # move.w #2, $FF1654
    assert u32(rom, 0x008AEC) == 0x00FF17B8  # btst.b #0, $FF17B8
    assert u32(rom, 0x008AF6) == 0x00FF1654  # move.w #3, $FF1654
    assert u32(rom, 0x008B0E) == 0x00FF1651  # tst.b $FF1651

    # 15 phase calls in the frame sequencer
    pipeline_calls = {
        "pause_menu": "0x00A4C8",
        "joypad_poll": "0x008BAC",
        "screen_shake_step": "0x008B8E",
        "event_vm_tick": "0x00F788",
        "dialogue_hud_step": "0x00557A",
        "spirit_mana_step": "0x00DEEC",
        "secondary_weapon_flight": "0x00E0BA",
        "hazard_blast_step": "0x00E0F4",
        "ali_machine_step": "0x008E22",
        "entities_list_step": "0x008E90",
        "entity_gc_reclaim": "0x008DD4",
        "camera_boundary_clamp": "0x00C998",
        "plane_scroll_sync": "0x00C980",
        "sat_compose": "0x00A196",
    }

    # 0x008B8E: screen_shake_dispatcher
    assert u32(rom, 0x008B90) == 0x00FF1864  # move.b $FF1864, d0
    assert u32(rom, 0x008B9C) == 0x00FF1864  # bclr.b #7, $FF1864

    # 0x008BAC: joypad_menu_hotkeys
    assert u32(rom, 0x008BAE) == 0x00FF165E  # move.b $FF165E, d0
    assert u32(rom, 0x008BBE) == 0x00FF19A7  # st.b $FF19A7
    assert u32(rom, 0x008BC4) == 0x00FF16EA  # clr.w $FF16EA
    assert u32(rom, 0x008BCA) == 0x00FF1987  # tst.b $FF1987
    assert u32(rom, 0x008BD6) == 0x00FF17B8  # btst.b #2, $FF17B8
    assert u32(rom, 0x008BE0) == 0x00FF1983  # tst.b $FF1983
    assert u32(rom, 0x008BEA) == 0x00FF1982  # tst.b $FF1982
    assert u32(rom, 0x008BF4) == 0x00FF185E  # sf.b $FF185E
    assert u32(rom, 0x008BFC) == 0x00FF165F  # btst.b #7, $FF165F (Start button)
    assert u32(rom, 0x008C06) == 0x00FF1660  # move.w $FF1660, d0 (extended buttons)

    return {
        "milestone": "M15.395",
        "title": "ROM Master Gameplay Frame Sequencer, Screen Shake Dispatcher & Joypad Menu Hotkeys",
        "routines": {
            "master_frame_sequencer": {
                "start": "0x008AA6",
                "end": "0x008B8C",
                "bytes": 230,
                "demo_flag_ram": "0x00FF1996",
                "demo_finished_ram": "0x00FF1997",
                "global_tick_ram": "0x00FF0D76",
                "anim_subtick_ram": "0x00FF1971",
                "vblank_sync_ram": "0x00FF1654",
                "pipeline_calls": pipeline_calls,
            },
            "screen_shake_dispatcher": {
                "start": "0x008B8E",
                "end": "0x008BAA",
                "bytes": 28,
                "trigger_ram": "0x00FF1864",
                "flash_handler": "0x00530C",
                "shake_handler": "0x007CC6",
            },
            "joypad_menu_hotkeys": {
                "start": "0x008BAC",
                "end": "0x008C58",
                "bytes": 172,
                "joypad_held_ram": "0x00FF165E",
                "joypad_edge_ram": "0x00FF165F",
                "joypad_extra_ram": "0x00FF1660",
                "menu_request_ram": "0x00FF185E",
                "menu_handler": "0x003B3E",
                "all_released_mask": "0x70",
                "menu_indices": {
                    "start_button": 0,
                    "item_ring": 1,
                    "weapon_ring": 2,
                    "map_status": 3,
                },
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
