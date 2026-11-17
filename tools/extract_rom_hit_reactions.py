"""Extract Hit-Reaction Knockback Physics, Shield Parry & Hit-Spark Pool (0x00BF26..0x00C060).

Milestone: M15.298.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x00BF26: 4-entry signed self-relative hit-reaction table (0, 2, 4, 6)
  - 0x00BF2E (react 0): Light hit (0x300, 0x2C0, vz=0 if grounded; else $86 bit 7 -> 0x00BF58)
  - 0x00BF98 (react 2): Vertical launcher (vx=0, vy=0, vz=0x70000)
  - 0x00BF58 (react 4): Medium knockback (0x400, 0x3C0, vz=0x28000)
  - 0x00BF78 (react 6): Heavy launch knockback (0x700, 0x680, vz=0x30000)
  - 0x00BFAE: Shield parry (SFX 0x00900202, Ali weapon recoil 0x008530 if $FF198C in {6, 0xA})
  - 0x00C012: 4-slot 8-byte hit-spark pool at $FF19C8 (type=2, timer=5)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_hit_reactions_m15_298.json")

REACTION_TABLE_ADDR = 0x00BF26


def read_s16(rom: bytes, addr: int) -> int:
    """Read big-endian signed 16-bit integer from ROM."""
    return struct.unpack_from(">h", rom, addr)[0]


def extract_hit_reactions(rom_path: Path) -> dict:
    """Extract all 4 hit-reaction physics profiles, shield parry, and hit-spark pool."""
    rom = rom_path.read_bytes()

    reactions = [
        {
            "index": 0,
            "selector": 0,
            "name": "light_ground_or_juggle",
            "target_addr": f"0x{REACTION_TABLE_ADDR + 0 + read_s16(rom, REACTION_TABLE_ADDR + 0):06X}",
            "horiz_x": struct.unpack_from(">H", rom, 0x00BF38)[0],
            "horiz_y": struct.unpack_from(">H", rom, 0x00BF3C)[0],
            "vz_fixed": 0,
            "airborne_fallthrough_to": "0x00BF58",
        },
        {
            "index": 1,
            "selector": 2,
            "name": "vertical_launcher",
            "target_addr": f"0x{REACTION_TABLE_ADDR + 2 + read_s16(rom, REACTION_TABLE_ADDR + 2):06X}",
            "horiz_x": 0,
            "horiz_y": 0,
            "vz_fixed": struct.unpack_from(">I", rom, 0x00BFA2)[0],
        },
        {
            "index": 2,
            "selector": 4,
            "name": "medium_knockback",
            "target_addr": f"0x{REACTION_TABLE_ADDR + 4 + read_s16(rom, REACTION_TABLE_ADDR + 4):06X}",
            "horiz_x": struct.unpack_from(">H", rom, 0x00BF5A)[0],
            "horiz_y": struct.unpack_from(">H", rom, 0x00BF5E)[0],
            "vz_fixed": struct.unpack_from(">I", rom, 0x00BF70)[0],
        },
        {
            "index": 3,
            "selector": 6,
            "name": "heavy_knockback",
            "target_addr": f"0x{REACTION_TABLE_ADDR + 6 + read_s16(rom, REACTION_TABLE_ADDR + 6):06X}",
            "horiz_x": struct.unpack_from(">H", rom, 0x00BF7A)[0],
            "horiz_y": struct.unpack_from(">H", rom, 0x00BF7E)[0],
            "vz_fixed": struct.unpack_from(">I", rom, 0x00BF90)[0],
        },
    ]

    return {
        "milestone": "M15.298",
        "rom_path": str(rom_path),
        "reactions": reactions,
        "shield_parry": {
            "handler_addr": "0x00BFAE",
            "sfx_id": f"0x{struct.unpack_from('>I', rom, 0x00C004)[0]:08X}",
            "recoil_weapons_ff198c": [6, 10],
            "recoil_helper_addr": "0x008530",
        },
        "hit_spark_pool": {
            "allocator_addr": "0x00C012",
            "pool_base_addr": "0xFF19C8",
            "slot_count": 4,
            "slot_stride_bytes": 8,
            "spark_type": struct.unpack_from(">H", rom, 0x00C03E)[0],
            "spark_timer_frames": struct.unpack_from(">H", rom, 0x00C042)[0],
        },
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_hit_reactions(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['reactions'])} hit reactions and hit-spark pool to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
