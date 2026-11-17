#!/usr/bin/env python3
"""M15.362 extractor: ROM linked-list CRAM palette animator & 3-pool AI dispatcher (0x008E22..0x008F10)."""

from __future__ import annotations

import json
import struct
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUT_PATH = Path("data/thor-evidence/rom_palette_anim_pool_dispatch_m15_362.json")


def _u32(data: bytes, addr: int) -> int:
    return struct.unpack_from(">I", data, addr)[0]


def extract_evidence(rom_bytes: bytes) -> dict:
    assert _u32(rom_bytes, 0x008E24) == 0x00FF164F
    assert _u32(rom_bytes, 0x008E2E) == 0x00FF17A2
    assert _u32(rom_bytes, 0x008E34) == 0x00FF134C
    assert _u32(rom_bytes, 0x008E92) == 0x00FF2954
    assert _u32(rom_bytes, 0x008EB4) == 0x00FF19E8
    assert _u32(rom_bytes, 0x008ED6) == 0x00FF2D8C

    return {
        "milestone": "M15.362",
        "rom_range": "0x008E22..0x008F10",
        "palette_animator_008e22": {
            "enable_flag_addr": "0xFF164F",
            "list_head_ptr_addr": "0xFF17A2",
            "cram_shadow_addr": "0xFF134C",
            "vint_cram_dirty_addr": "0xFF164D",
            "vint_cram_dirty_bit": 0,
            "node_layout": {
                "next_delta_i16": "0x00",
                "cram_offset_u16": "0x02",
                "cursor_u16": "0x04",
                "delay_timer_u16": "0x06",
                "keyframes_start": "0x08",
                "keyframe_stride_bytes": 4,
            },
        },
        "ai_pool_dispatcher_008e90": {
            "return_continuation_addr": "0xFF193C",
            "pools": [
                {"base_addr": "0xFF2954", "slot_count": 4, "stride_bytes": 0x5A, "handler": "0x008F12"},
                {"base_addr": "0xFF19E8", "slot_count": 21, "stride_bytes": 0xBC, "handler": "0x008F22"},
                {"base_addr": "0xFF2D8C", "slot_count": 6, "stride_bytes": 0x5A, "handler": "0x008F12"},
            ],
            "set_anim_state_helper": "0x008EF8",
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
