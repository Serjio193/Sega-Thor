"""Extract VDP Full Clear, Armlet Glow & Palette-Cycling Animator (0x002B28, 0x008DD4..0x008E8E).

Milestone: M15.306.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x002B28..0x002C20: VDP full clear (CRAM backdrop 0, HScroll 0xDC00, VSRAM 40 words,
                        Tile 0 0x0000, SAT 0xD000, Plane A 0xC000, Plane B 0xE000 via DMA fill 0x8F01/0x9780)
  - 0x008DD4..0x008E21: Armlet glow intensity ($FF183E): ramps 0..3 when spirit active (pulses 3/4
                        if $FF199E >= 20), decays 3..0 when inactive
  - 0x008E22..0x008E8E: CRAM palette-cycling animator ($FF17A2 linked list -> $FF134C)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_vdp_clear_and_palette_cycle_m15_306.json")


def step_armlet_glow(glow: int, spirit_active: bool, timer_ff199e: int, pulse_ff199f: int) -> int:
    """Apply exact ROM 0x008DD4 armlet glow state transition."""
    if spirit_active:
        if glow < 3:
            return glow + 1
        glow = 3
        if timer_ff199e >= 0x14 and (pulse_ff199f & 1) == 0:
            return glow + 1
        return glow
    if glow == 0:
        return 0
    if glow < 4:
        return glow - 1
    return 2


def extract_vdp_clear_and_palette_cycle(rom_path: Path) -> dict:
    """Extract VDP clear DMA regions, armlet glow thresholds, and palette cycler specs."""
    rom = rom_path.read_bytes()
    return {
        "milestone": "M15.306",
        "rom_path": str(rom_path),
        "vdp_clear": {
            "entry_addr": "0x002B28",
            "dma_fill_addr": "0x002BD2",
            "dma_autoinc_reg": f"0x{struct.unpack_from('>H', rom, 0x002BE2)[0]:04X}",
            "dma_mode_fill": f"0x{struct.unpack_from('>H', rom, 0x002C04)[0]:04X}",
            "regions": [
                {"name": "hscroll", "vdp_cmd": "0x5C000003", "addr": "0xDC00"},
                {"name": "vsram", "vdp_cmd": "0x40000010", "words": 40},
                {"name": "tile_zero", "vdp_cmd": "0x40000000", "bytes_minus_1": 0x1F},
                {"name": "sat", "vdp_cmd": "0x50000003", "addr": "0xD000"},
                {"name": "plane_a", "vdp_cmd": "0x40000003", "bytes_minus_1": 0xFFF},
                {"name": "plane_b", "vdp_cmd": "0x60000003", "bytes_minus_1": 0xFFF},
            ],
        },
        "armlet_glow": {
            "entry_addr": "0x008DD4",
            "ram_glow": "0xFF183E",
            "ram_spirit_active": "0xFF1AA4",
            "pulse_threshold_ff199e": rom[0x008DF3],
        },
        "palette_cycler": {
            "entry_addr": "0x008E22",
            "ram_enable_flag": "0xFF164F",
            "ram_list_ptr": "0xFF17A2",
            "cram_shadow_addr": "0xFF134C",
        },
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_vdp_clear_and_palette_cycle(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote VDP clear ({len(data['vdp_clear']['regions'])} regions) & palette cycler spec to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
