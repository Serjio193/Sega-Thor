"""Extract Checkpoint Snapshot & Game-Over Retry Restore (0x0017DA..0x001900).

Milestone: M15.293.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x0017DA: Checkpoint snapshot (skips if demo $FF1996 != 0; saves room/cam/pos to $FF0E42,
              0xBC words Ali+Spirit to $FF0BFE, 0x12A words save buffer to $FF0E52)
  - 0x001828: Game-Over retry restore (preserves $FF0D76/$FF1A7E/$FF1A70 MaxHP,
              restores $FF0E52 -> $FF0BFE -> $FF19E8, sets HP = MaxHP, reloads spirit 0x014522,
              increments continue count $FF0E3E, sets 30f invulnerability $FF198E = 0x1E)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_checkpoint_retry_m15_293.json")


def extract_checkpoint_retry(rom_path: Path) -> dict:
    """Extract checkpoint snapshot and Game-Over retry constants."""
    rom = rom_path.read_bytes()
    entity_words = struct.unpack_from(">H", rom, 0x001804)[0] + 1
    snapshot_words = struct.unpack_from(">H", rom, 0x00181A)[0] + 1
    invuln_frames = rom[0x0018FF]

    return {
        "milestone": "M15.293",
        "rom_path": str(rom_path),
        "checkpoint_retry": {
            "snapshot_addr": "0x0017DA",
            "retry_restore_addr": "0x001828",
            "ram_room_cam_pos": "0xFF0E42",
            "ram_save_buf": "0xFF0BFE",
            "ram_retry_buf": "0xFF0E52",
            "ram_player_entity": "0xFF19E8",
            "ram_spirit_entity": "0xFF1AA4",
            "ram_continue_count": "0xFF0E3E",
            "entity_copy_words": entity_words,
            "entity_copy_bytes": entity_words * 2,
            "snapshot_words": snapshot_words,
            "snapshot_bytes": snapshot_words * 2,
            "preserved_on_retry": ["0xFF0D76", "0xFF1A7E", "0xFF1A70 (MaxHP)"],
            "retry_restores_full_hp": True,
            "spirit_resource_loader_addr": "0x014522",
            "post_retry_invuln_frames": invuln_frames,
        },
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_checkpoint_retry(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote Checkpoint/Retry spec ({data['checkpoint_retry']['snapshot_bytes']}B snapshot, {data['checkpoint_retry']['post_retry_invuln_frames']}f invuln) to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
