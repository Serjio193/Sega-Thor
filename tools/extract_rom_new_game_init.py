"""Extract New Game Initialization & Starting Stats/Room/Chords (0x001300..0x00144F).

Milestone: M15.291.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x001300: Clear 0x12A words (596 bytes) from $FF0BFE
  - 0x001310..0x0013CD: 4 P1+P2 secret start chords on $FF165E.L (0xD6000100, 0xC9000200, 0xCA000100, 0x90000000)
  - 0x0013CE..0x00142E: Player initial stats (id=2, facing=2, MaxHP=HP=200 (0xC8), MaxSP=SP=0, atk=10 (0x0A), spirit=0)
  - 0x001432..0x00144A: Intro cutscene (0x03BF86) -> Room 0x001B, Cam (0x60, 0), Ali (256, 80) -> 0x00CA38
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_new_game_init_m15_291.json")


def extract_new_game_init(rom_path: Path) -> dict:
    """Extract New Game starting stats, room/spawn, and secret chords."""
    rom = rom_path.read_bytes()
    max_hp = struct.unpack_from(">H", rom, 0x0013FA)[0]
    hp = struct.unpack_from(">H", rom, 0x001400)[0]
    base_atk = struct.unpack_from(">H", rom, 0x001414)[0]
    start_room = struct.unpack_from(">H", rom, 0x00143A)[0]
    cam_packed = struct.unpack_from(">I", rom, 0x00143E)[0]
    pos_packed = struct.unpack_from(">I", rom, 0x001446)[0]

    chords = [
        {"mask_ff165e": f"0x{struct.unpack_from('>I', rom, 0x001312)[0]:08X}", "target": "0x03C8DE", "role": "Secret menu / sound test"},
        {"mask_ff165e": f"0x{struct.unpack_from('>I', rom, 0x001336)[0]:08X}", "target": "0x001450", "role": "Debug / super start"},
        {"mask_ff165e": f"0x{struct.unpack_from('>I', rom, 0x00137E)[0]:08X}", "target": "0x001394", "role": "Special flag $FF0D7A=0x80"},
        {"mask_ff165e": f"0x{struct.unpack_from('>I', rom, 0x00139E)[0]:08X}", "target": "0x03E7F4", "role": "A+Start alternate screen"},
    ]

    return {
        "milestone": "M15.291",
        "rom_path": str(rom_path),
        "new_game": {
            "entry_addr": "0x001300",
            "clear_base": "0xFF0BFE",
            "clear_words": 0x12A,
            "world_flags_ff0d80": "0x01FF",
            "entity_id": 2,
            "facing": 2,
            "max_hp": max_hp,
            "hp": hp,
            "max_sp": 0,
            "sp": 0,
            "base_attack_ff0d7e": base_atk,
            "active_spirit_ff1aa4": 0,
            "intro_cutscene_addr": "0x03BF86",
            "start_room_id": start_room,
            "start_cam_x": (cam_packed >> 16) & 0xFFFF,
            "start_cam_y": cam_packed & 0xFFFF,
            "start_x": (pos_packed >> 16) & 0xFFFF,
            "start_y": pos_packed & 0xFFFF,
            "gameplay_entry_addr": "0x00CA38",
        },
        "secret_chords": chords,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_new_game_init(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote New Game init (Room 0x{data['new_game']['start_room_id']:04X}, HP={data['new_game']['hp']}) to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
