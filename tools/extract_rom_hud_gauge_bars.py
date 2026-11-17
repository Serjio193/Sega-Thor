"""Extract HUD HP/SP/Enemy Gauge Bar VDP Renderer (0x00267C..0x0027A4).

Milestone: M15.300.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x00267C..0x0026B1: Ali HP gauge (VRAM cmd 0x70E00003 -> 0xF0E0, fill 0x4E2222E4)
  - 0x0026B2..0x0026D5: Ali SP gauge (VRAM cmd 0x72800003 -> 0xF280, fill 0x4E6666E4)
  - 0x0026D6..0x00271B: Enemy HP gauge ($FF184F timer, $FF1850 entity, VRAM cmd 0x74200003 -> 0xF420, fill 0xDE4444ED)
  - 0x00271C..0x0027A4: 48-row (0x30) gauge scaler (0->0, >=max->48, else (cur*47)/max+1) + differential VDP writer (erase mask 0x00EEEE00)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_hud_gauge_bars_m15_300.json")


def compute_gauge_fill_rows(cur: int, max_val: int) -> int:
    """Compute exact ROM 0x00271C gauge fill height in [0..48] rows."""
    if cur <= 0:
        return 0
    if cur >= max_val or max_val <= 0:
        return 48
    return ((cur * 47) // max_val) + 1


def extract_hud_gauge_bars(rom_path: Path) -> dict:
    """Extract HUD gauge VDP commands, 4bpp patterns, and scaling constants."""
    rom = rom_path.read_bytes()
    gauges = [
        {
            "name": "ali_hp",
            "vdp_cmd": f"0x{struct.unpack_from('>I', rom, 0x00269C)[0]:08X}",
            "fill_pattern_4bpp": f"0x{struct.unpack_from('>I', rom, 0x0026A2)[0]:08X}",
            "entity_base": "0xFF19E8",
            "stat_offset": "0x88",
        },
        {
            "name": "ali_sp",
            "vdp_cmd": f"0x{struct.unpack_from('>I', rom, 0x0026C0)[0]:08X}",
            "fill_pattern_4bpp": f"0x{struct.unpack_from('>I', rom, 0x0026C6)[0]:08X}",
            "entity_base": "0xFF19E8",
            "stat_offset": "0x8C",
        },
        {
            "name": "target_enemy_hp",
            "vdp_cmd": f"0x{struct.unpack_from('>I', rom, 0x00270A)[0]:08X}",
            "fill_pattern_4bpp": f"0x{struct.unpack_from('>I', rom, 0x002710)[0]:08X}",
            "entity_ptr": "0xFF1850",
            "timer_addr": "0xFF184F",
            "stat_offset": "0x88",
        },
    ]

    return {
        "milestone": "M15.300",
        "rom_path": str(rom_path),
        "gauge_renderer": {
            "entry_addr": "0x00267C",
            "kernel_addr": "0x00271C",
            "max_rows": 0x30,
            "scale_numerator": 0x2F,
            "erase_or_mask": f"0x{struct.unpack_from('>I', rom, 0x002752)[0]:08X}",
            "cache_values_addr": "0xFF1840",
            "cache_rows_addr": "0xFF184C",
        },
        "gauges": gauges,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_hud_gauge_bars(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['gauges'])} HUD gauge specs to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
