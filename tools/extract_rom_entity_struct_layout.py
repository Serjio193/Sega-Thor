"""Extract Entity Struct Constructor & 0x03F2FA 32-Byte Descriptor Field Map (0x008D06).

Milestone: M15.302.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x008D06..0x008DA3: Entity constructor (clears 0x5D words if < $FF2954 else 0x2C words,
                        decodes 32B descriptor at 0x03F2FA + (entity_id - 2) * 16)
  - 0x008DA4: Lightweight pointer-only re-init (for Continue / Checkpoint restore)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_entity_struct_layout_m15_302.json")

DESCRIPTOR_FIELDS = [
    {"rom_off": "0x00", "size": 2, "entity_offsets": ["0x4A"], "role": "height_z"},
    {"rom_off": "0x02", "size": 1, "entity_offsets": ["0x46", "0x42", "0x44"], "role": "radius_extent"},
    {"rom_off": "0x03", "size": 1, "entity_offsets": ["0x48"], "role": "hitbox_half_w"},
    {"rom_off": "0x04", "size": 1, "entity_offsets": ["0x49"], "role": "hitbox_half_h"},
    {"rom_off": "0x05", "size": 1, "entity_offsets": ["0x3D"], "role": "weight_flags"},
    {"rom_off": "0x06", "size": 1, "entity_offsets": ["0x4C"], "role": "shadow_z_offset"},
    {"rom_off": "0x07", "size": 1, "entity_offsets": ["0x3A"], "role": "trait_flags (bit0=spirit)"},
    {"rom_off": "0x08", "size": 1, "entity_offsets": ["0x36"], "role": "collision_flags"},
    {"rom_off": "0x09", "size": 1, "entity_offsets": ["0x37"], "role": "combat_flags"},
    {"rom_off": "0x0A", "size": 1, "entity_offsets": ["0x38"], "role": "render_flags"},
    {"rom_off": "0x0B", "size": 1, "entity_offsets": ["0x3C"], "role": "behavior_flags"},
    {"rom_off": "0x0C", "size": 4, "entity_offsets": ["0x1A"], "role": "vram_tile_ptr"},
    {"rom_off": "0x10", "size": 4, "entity_offsets": ["0x1E"], "role": "sprite_frame_ptr"},
    {"rom_off": "0x14", "size": 4, "entity_offsets": ["0x26"], "role": "anim_script_ptr"},
    {"rom_off": "0x18", "size": 4, "entity_offsets": ["0x22"], "role": "hitbox_table_ptr"},
    {"rom_off": "0x1C", "size": 4, "entity_offsets": ["0x5E"], "role": "secondary_ai_ptr (<$FF2954 only)"},
]


def extract_entity_struct_layout(rom_path: Path) -> dict:
    """Extract entity constructor constants and sample Ali (id=2) descriptor."""
    rom = rom_path.read_bytes()
    table_addr = struct.unpack_from(">I", rom, 0x008D2C)[0]
    proj_threshold = struct.unpack_from(">I", rom, 0x008D14)[0]
    ali_base = table_addr  # entity_id 2 -> (2 - 2) * 16 = 0

    return {
        "milestone": "M15.302",
        "rom_path": str(rom_path),
        "constructor": {
            "entry_addr": "0x008D06",
            "reinit_ptrs_addr": "0x008DA4",
            "master_table_addr": f"0x{table_addr:06X}",
            "descriptor_stride_bytes": 32,
            "full_entity_clear_words": 0x5D,
            "light_entity_clear_words": 0x2C,
            "light_entity_ram_threshold": f"0x{proj_threshold:06X}",
        },
        "descriptor_fields": DESCRIPTOR_FIELDS,
        "sample_ali_id2": {
            "height_z": struct.unpack_from(">H", rom, ali_base + 0x00)[0],
            "radius_extent": rom[ali_base + 0x02],
            "hitbox_half_w": rom[ali_base + 0x03],
            "hitbox_half_h": rom[ali_base + 0x04],
            "vram_tile_ptr": f"0x{struct.unpack_from('>I', rom, ali_base + 0x0C)[0]:06X}",
            "sprite_frame_ptr": f"0x{struct.unpack_from('>I', rom, ali_base + 0x10)[0]:06X}",
            "anim_script_ptr": f"0x{struct.unpack_from('>I', rom, ali_base + 0x14)[0]:06X}",
            "hitbox_table_ptr": f"0x{struct.unpack_from('>I', rom, ali_base + 0x18)[0]:06X}",
        },
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_entity_struct_layout(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['descriptor_fields'])} descriptor fields (table={data['constructor']['master_table_addr']}) to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
