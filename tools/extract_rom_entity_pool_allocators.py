"""Extract WRAM Entity, Effect & Projectile Object Pool Allocators (0x00D9BE..0x00DA60).

Milestone: M15.307.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x00D9BE: Effect/Sprite pool ($FF2ABC, 8 slots, stride 0x5A, VRAM tile 0x694 + idx * 9)
  - 0x00D9F0: Special/Particle pool ($FF2D8C, 6 slots, stride 0x5A)
  - 0x00DA1A: Full entity pool incl. companion ($FF1C1C, 18 slots, stride 0xBC)
  - 0x00DA2A: Enemy/Object pool ($FF1CD8, 17 slots, stride 0xBC; clears $FF184F if slot == $FF1850)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_entity_pool_allocators_m15_307.json")


def extract_entity_pool_allocators(rom_path: Path) -> dict:
    """Extract all 4 WRAM object pool bases, capacities, strides, and VRAM tile step."""
    rom = rom_path.read_bytes()
    pools = [
        {
            "name": "effect_pool_8",
            "allocator_addr": "0x00D9BE",
            "ram_base": f"0x{struct.unpack_from('>I', rom, 0x00D9C8)[0]:06X}",
            "slot_count": rom[0x00D9CD] + 1,
            "slot_stride_bytes": 0x5A,
            "vram_tile_base": f"0x{struct.unpack_from('>H', rom, 0x00D9C4)[0]:04X}",
            "vram_tile_step": 9,
        },
        {
            "name": "special_pool_6",
            "allocator_addr": "0x00D9F0",
            "ram_base": f"0x{struct.unpack_from('>I', rom, 0x00D9F6)[0]:06X}",
            "slot_count": rom[0x00D9FB] + 1,
            "slot_stride_bytes": 0x5A,
        },
        {
            "name": "full_entity_pool_18",
            "allocator_addr": "0x00DA1A",
            "ram_base": f"0x{struct.unpack_from('>I', rom, 0x00DA20)[0]:06X}",
            "slot_count": rom[0x00DA25] + 1,
            "slot_stride_bytes": 0xBC,
        },
        {
            "name": "enemy_entity_pool_17",
            "allocator_addr": "0x00DA2A",
            "ram_base": f"0x{struct.unpack_from('>I', rom, 0x00DA30)[0]:06X}",
            "slot_count": rom[0x00DA35] + 1,
            "slot_stride_bytes": 0xBC,
            "clears_enemy_hud_if_reused": True,
        },
    ]

    return {
        "milestone": "M15.307",
        "rom_path": str(rom_path),
        "pools": pools,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_entity_pool_allocators(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['pools'])} entity/effect WRAM pools to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
