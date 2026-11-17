"""Extract Continue Game SRAM Load, Backup Fallback & Entity Restore (0x00161C..0x001730).

Milestone: M15.292.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x00161C: Continue entry ($A130F1 = 3 read, tries primary 0x001112 then backup 0x00114A)
  - 0x001648: Corrupt-both fallback (lock SRAM $A130F1 = 2, soft reset 0x00020E)
  - 0x001660: Copy 0xBC words (376 bytes) from $FF0BFE to $FF19E8 (Ali + Active Spirit)
  - 0x0016B2: Reload active spirit graphics/resources via 0x014522
  - 0x0016D2: Set post-load invulnerability $FF198E = 0x1E (30 frames)
  - 0x0016DA..0x0016EE: Restore room $FF0E42 and Ali (X, Y) -> 0x00CA24
  - 0x0016F4: Decode 0x12A words (596 bytes) from odd-byte SRAM with u16_be sum checksum
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_continue_game_load_m15_292.json")


def extract_continue_game_load(rom_path: Path) -> dict:
    """Extract Continue Game SRAM fallback and entity restore constants."""
    rom = rom_path.read_bytes()
    entity_copy_words = struct.unpack_from(">H", rom, 0x00166E)[0] + 1
    invuln_frames = rom[0x0016D5]
    sram_words = struct.unpack_from(">H", rom, 0x001700)[0] + 1

    return {
        "milestone": "M15.292",
        "rom_path": str(rom_path),
        "continue_load": {
            "entry_addr": "0x00161C",
            "primary_slot_addr": "0x001112",
            "backup_slot_addr": "0x00114A",
            "checksum_loader_addr": "0x0016F4",
            "corrupt_reset_addr": "0x00020E",
            "sram_payload_words": sram_words,
            "sram_payload_bytes": sram_words * 2,
            "entity_copy_words": entity_copy_words,
            "entity_copy_bytes": entity_copy_words * 2,
            "ram_save_buf": "0xFF0BFE",
            "ram_player_entity": "0xFF19E8",
            "ram_spirit_entity": "0xFF1AA4",
            "ram_room_id": "0xFF0E42",
            "spirit_resource_loader_addr": "0x014522",
            "post_load_invuln_frames": invuln_frames,
            "gameplay_entry_addr": "0x00CA24",
        },
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_continue_game_load(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote Continue Game load spec ({data['continue_load']['sram_payload_bytes']}B SRAM, {data['continue_load']['post_load_invuln_frames']}f invuln) to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
