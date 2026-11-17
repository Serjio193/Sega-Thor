"""Extract CRAM Palette Fade Engine & 9-bit BGR333 Scaling (0x002D58..0x00302D).

Milestone: M15.295.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x002D84: Blocking Fade In ($FF134C -> $FF18A6 64 colors, step 0..7 via 0x002EE2)
  - 0x002DB6: Async Fade In ($FF164D bit 4)
  - 0x002E1E: Blocking Fade Out (step 7..0 via 0x002F6E, restores $FF18A6 -> $FF134C)
  - 0x002E78: Async Fade Out ($FF1926 = 0)
  - 0x002FDA..0x00302C: Per-channel BGR333 scaling ((c & mask) * step / 7) & mask, sets $FF164D bit 0
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_palette_fade_m15_295.json")


def scale_bgr333_color(color: int, step: int) -> int:
    """Apply exact ROM 0x002FE2 CRAM scaling for step in 0..7."""
    s = step & 0x7
    r = (((color & 0x00E) * s) // 7) & 0x00E
    g = (((color & 0x0E0) * s) // 7) & 0x0E0
    b = (((color & 0xE00) * s) // 7) & 0xE00
    return r | g | b


def extract_palette_fade(rom_path: Path) -> dict:
    """Verify ROM palette fade constants and channel masks."""
    rom = rom_path.read_bytes()
    fade_in_limit = struct.unpack_from(">H", rom, 0x002F62)[0]
    fade_out_start = struct.unpack_from(">H", rom, 0x002F70)[0]
    divisor = struct.unpack_from(">H", rom, 0x002FF2)[0]
    color_count = rom[0x002DAB]

    return {
        "milestone": "M15.295",
        "rom_path": str(rom_path),
        "palette_fade": {
            "cram_shadow_addr": "0xFF134C",
            "fade_backup_addr": "0xFF18A6",
            "ram_fade_step": "0xFF189A",
            "ram_fade_delay": "0xFF1898",
            "ram_vdp_flags": "0xFF164D",
            "fade_in_blocking_addr": "0x002D84",
            "fade_in_async_addr": "0x002DB6",
            "fade_out_blocking_addr": "0x002E1E",
            "fade_out_async_addr": "0x002E78",
            "scale_kernel_addr": "0x002FDA",
            "color_count": color_count + 1,
            "fade_in_step_end": fade_in_limit,
            "fade_out_step_start": fade_out_start,
            "channel_divisor": divisor,
            "masks": {"r": "0x00E", "g": "0x0E0", "b": "0xE00"},
        },
        "sample_0x0EEE_ramp": [f"0x{scale_bgr333_color(0x0EEE, s):03X}" for s in range(8)],
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_palette_fade(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote CRAM fade spec (ramp={data['sample_0x0EEE_ramp']}) to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
