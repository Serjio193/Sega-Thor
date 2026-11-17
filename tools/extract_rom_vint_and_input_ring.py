"""Extract V-Int DMA Queue, 64-Frame Input History Ring & Timers (0x001F76..0x002218).

Milestone: M15.296.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x001F76: V-Int entry (vector 0x78)
  - 0x002112: VDP DMA queue flush ($FF164E bit 1, base $FF10B4, tail $FF1892, 8B entries via 0x0027FC)
  - 0x00217C: Controller poll ($A10003 -> $FF165C..$FF1661 via 0x002992)
  - 0x00218C: Soft-reset button combo ($FF165E.L == 0xE0000202 -> 0x00020E)
  - 0x0021B2: 64-entry input history ring ($FF1668, idx $FF16E8 step -2 & 0x7E, count $FF16EA <= 0x3F)
  - 0x0021EC: Frame counter ($FF1656 += 1) and countdowns ($FF1658, $FF1654)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_vint_and_input_ring_m15_296.json")


def extract_vint_and_input_ring(rom_path: Path) -> dict:
    """Extract V-Int vector, DMA queue base, soft-reset combo, and input ring specs."""
    rom = rom_path.read_bytes()
    vint_vec = struct.unpack_from(">I", rom, 0x000078)[0]
    dma_base = struct.unpack_from(">I", rom, 0x002138)[0]
    reset_combo = struct.unpack_from(">I", rom, 0x00218E)[0]
    ring_mask = struct.unpack_from(">H", rom, 0x0021C8)[0]
    ring_max_count = struct.unpack_from(">H", rom, 0x0021DC)[0]

    return {
        "milestone": "M15.296",
        "rom_path": str(rom_path),
        "vint": {
            "vector_addr": f"0x{vint_vec:06X}",
            "dma_queue_flag_addr": "0xFF164E",
            "dma_queue_base_addr": f"0x{dma_base:06X}",
            "dma_queue_tail_ptr": "0xFF1892",
            "dma_entry_bytes": 8,
            "dma_emitter_addr": "0x0027FC",
            "soft_reset_combo_ff165e": f"0x{reset_combo:08X}",
            "soft_reset_target": "0x00020E",
        },
        "input_ring": {
            "controller_poll_addr": "0x002992",
            "ring_base_addr": "0xFF1668",
            "ring_index_addr": "0xFF16E8",
            "ring_count_addr": "0xFF16EA",
            "ring_byte_mask": f"0x{ring_mask:04X}",
            "ring_entries": (ring_mask + 2) // 2,
            "ring_max_count": ring_max_count,
            "frame_counter_addr": "0xFF1656",
            "countdown_timers": ["0xFF1658", "0xFF1654"],
        },
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_vint_and_input_ring(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote V-Int & {data['input_ring']['ring_entries']}-entry input ring spec to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
