"""Extract V-Int Sound Tick & HUD Slide Offset State (0x002380..0x0023CA).

Milestone: M15.312.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x002380: Sound driver V-Int tick (jsr 0x060000)
  - 0x002386..0x0023CA: HUD slide offset ($FF185A) & direction ($FF1859):
      * dir == 0: stop at 0x20, else +2 mod 0x158
      * dir != 0: stop at 0x138, else -2 wrap < 0 -> 0x158
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_vint_hud_slide_m15_312.json")


def step_hud_slide_offset(offset: int, reverse_dir: bool) -> int:
    """Apply exact ROM 0x002386..0x0023C0 HUD slide update."""
    if not reverse_dir:
        if offset == 0x20:
            return offset
        offset += 2
        if offset >= 0x158:
            offset = 0
        return offset
    if offset == 0x138:
        return offset
    offset -= 2
    if offset < 0:
        offset = 0x158
    return offset


def extract_vint_hud_slide(rom_path: Path) -> dict:
    """Extract V-Int HUD slide limits and sound tick call."""
    rom = rom_path.read_bytes()
    return {
        "milestone": "M15.312",
        "rom_path": str(rom_path),
        "vint_tail": {
            "entry_addr": "0x002380",
            "sound_tick_target": f"0x{struct.unpack_from('>I', rom, 0x002382)[0]:06X}",
            "ram_slide_dir": "0xFF1859",
            "ram_slide_offset": "0xFF185A",
            "fwd_stop_offset": struct.unpack_from(">H", rom, 0x002398)[0],
            "rev_stop_offset": struct.unpack_from(">H", rom, 0x0023B0)[0],
            "wrap_limit": struct.unpack_from(">H", rom, 0x0023A2)[0],
            "step_pixels": 2,
        },
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_vint_hud_slide(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote V-Int HUD slide spec (wrap=0x{data['vint_tail']['wrap_limit']:X}) to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
