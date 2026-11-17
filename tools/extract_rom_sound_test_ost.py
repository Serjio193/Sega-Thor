"""Extract Secret Sound Test Menu & 34-Track Named OST Table (0x03E7F4..0x03F047).

Milestone: M15.294.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x03E7F4: Secret Sound Test screen entry (unlocked via New Game A+Start 0x90000000)
  - 0x03ED48: Sound Test UI labels ("SOUND TEST", "BGM", "SFX", "PCM", "EXIT")
  - 0x03ED7E..0x03ED9F: 34-byte BGM track ID display order (maps menu index 0..33 -> BGM 0x01xx)
  - 0x03EDA0..0x03F047: 34 NUL-terminated Yuzo Koshiro OST track names
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_sound_test_ost_m15_294.json")

TRACK_ID_MAP_ADDR = 0x03ED7E
TRACK_NAMES_ADDR = 0x03EDA0
TRACK_COUNT = 34


def read_cstr(rom: bytes, addr: int) -> tuple[str, int]:
    """Read NUL-terminated ASCII string and return (text, next_addr)."""
    end = rom.index(0, addr)
    return rom[addr:end].decode("ascii", errors="replace"), end + 1


def extract_sound_test_ost(rom_path: Path) -> dict:
    """Extract Sound Test labels, 34 BGM IDs, and OST track titles."""
    rom = rom_path.read_bytes()

    labels = {}
    pos = 0x03ED48
    for key in ["title", "bgm", "sfx", "pcm", "exit"]:
        text, pos = read_cstr(rom, pos)
        labels[key] = text

    tracks = []
    name_pos = TRACK_NAMES_ADDR
    for idx in range(TRACK_COUNT):
        track_byte = rom[TRACK_ID_MAP_ADDR + idx]
        raw_name, name_pos = read_cstr(rom, name_pos)
        tracks.append({
            "menu_index": idx,
            "track_byte": track_byte,
            "bgm_cmd": f"0x01{track_byte:02X}",
            "title": raw_name.strip(),
        })

    return {
        "milestone": "M15.294",
        "rom_path": str(rom_path),
        "sound_test": {
            "entry_addr": "0x03E7F4",
            "unlock_chord_ff165e": "0x90000000",
            "labels_addr": "0x03ED48",
            "track_id_map_addr": f"0x{TRACK_ID_MAP_ADDR:06X}",
            "track_names_addr": f"0x{TRACK_NAMES_ADDR:06X}",
            "track_count": TRACK_COUNT,
        },
        "labels": labels,
        "tracks": tracks,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_sound_test_ost(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote Sound Test labels and {len(data['tracks'])} OST tracks to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
