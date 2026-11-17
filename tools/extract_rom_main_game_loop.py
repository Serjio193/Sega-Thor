#!/usr/bin/env python3
"""M15.360 extractor: ROM master gameplay frame loop & menu dispatcher (0x008AA6..0x008C58)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_main_game_loop_m15_360.json")

PIPELINE_CALLS = [
    (0x008B1E, "0x00A4C8", "hud_conditional_update"),
    (0x008B22, "0x008BAC", "menu_and_soft_reset_check"),
    (0x008B26, "0x008B8E", "queued_item_or_spirit_trigger"),
    (0x008B2A, "0x00F788", "input_and_trigger_tick"),
    (0x008B2E, "0x00557A", "status_and_hud_tick"),
    (0x008B32, "0x00DEEC", "entity_ai_physics_tick"),
    (0x008B36, "0x00E0BA", "fx_particle_tick"),
    (0x008B3A, "0x00E0F4", "secondary_fx_tick"),
    (0x008B3E, "0x008E22", "camera_scroll_tick"),
    (0x008B42, "0x008E90", "palette_vdp_sync_tick"),
    (0x008B46, "0x008DD4", "audio_music_tick"),
    (0x008B52, "0x00C998", "room_pre_vint_hook"),
    (0x008B60, "0x00C980", "room_post_vint_hook"),
    (0x008B86, "0x00A196", "master_ysort_sat_render"),
]


def _u16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">H", data, addr)[0]


def _i16(data: bytes, addr: int) -> int:
    return struct.unpack_from(">h", data, addr)[0]


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x008AC0) == 0x00FF0D76
    assert _u32(rom_bytes, 0x008AE4) == 0x00FF1654
    assert _u32(rom_bytes, 0x008B90) == 0x00FF1864
    assert _u32(rom_bytes, 0x008C06) == 0x00FF1660

    verified_calls = []
    for call_addr, expected_target, name in PIPELINE_CALLS:
        assert _u16(rom_bytes, call_addr) == 0x6100
        rel = _i16(rom_bytes, call_addr + 2)
        target = call_addr + 2 + rel
        assert f"0x{target:06X}" == expected_target
        verified_calls.append({
            "call_site": f"0x{call_addr:06X}",
            "target": expected_target,
            "stage": name,
        })

    return {
        "milestone": "M15.360",
        "rom_range": "0x008AA6..0x008C58",
        "frame_loop": {
            "entry_addr": "0x008AA6",
            "play_timer_addr": "0xFF0D76",
            "frame_tick_addr": "0xFF1654",
            "slowdown_flag_addr": "0xFF17B8",
            "pipeline_calls": verified_calls,
        },
        "queued_trigger_008b8e": {
            "flag_addr": "0xFF1864",
            "bit7_target": "0x007CC6",
            "nonzero_low_target": "0x00530C",
        },
        "menu_dispatcher_008bac": {
            "abc_hold_mask": "0x70",
            "abc_flag_addr": "0xFF19A7",
            "menu_mode_addr": "0xFF185E",
            "menu_entry_target": "0x003B3E",
            "modes": [
                {"button": "Start", "mode": 0},
                {"button": "EdgeBit2", "mode": 3},
                {"button": "EdgeBit1", "mode": 1, "requires_ali_state_le_2_and_zero_1a6e": True},
                {"button": "EdgeBit0", "mode": 2, "requires_ali_state_le_2_and_zero_1a6e": True},
            ],
        },
    }


def main() -> None:
    rom_bytes = ROM_PATH.read_bytes()
    evidence = extract_evidence(rom_bytes)
    OUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUT_PATH.write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {OUT_PATH}")


if __name__ == "__main__":
    main()
