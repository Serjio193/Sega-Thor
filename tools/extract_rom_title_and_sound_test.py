"""Extract Title Screen, Attract/Demo Timeout, and Secret Input Codes (0x0032F8..0x003586).

Milestone: M15.283.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x0032F8: Mode 0x04 entry (loads font 0x16943C to VRAM 0xF020, title gfx 0x180C56)
  - 0x0033A6: Attract/Demo timeout ($FF1654 = 0x2D0 = 720 frames = 12s at 60Hz)
  - 0x003428: Demo/Attract mode entry ($FF10AC = 0x08, $FF1996 = 0xFF)
  - 0x003464: Start button ($FF165F & 0x80) secret chord dispatch ($FF165E == 0x90, 0xD5, 0x8A)
  - 0x0034BE: Sound Test / Debug menu ($FF165E == 0x90, SFX 0xA003AA -> jmp 0x03E4FA)
  - 0x0034E4: Alternate Debug / Stage Select ($FF165E == 0xD5, SFX 0xA00205 -> 0x001E98 -> jmp 0x03E4FA)
  - 0x00350E: Blinking "PRESS START" prompt (timer & 0x7F < 0x60)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_title_and_sound_test_m15_283.json")


def read_cstr(rom: bytes, addr: int, max_len: int = 64) -> str:
    """Read NUL-terminated ASCII string from ROM."""
    chunk = rom[addr:addr + max_len]
    nul = chunk.find(0)
    if nul != -1:
        chunk = chunk[:nul]
    return chunk.decode("ascii", errors="replace")


def extract_title_and_sound_test(rom_path: Path) -> dict:
    """Extract title screen constants, strings, and secret input chords."""
    rom = rom_path.read_bytes()
    attract_timeout = struct.unpack_from(">H", rom, 0x0033A8)[0]
    blink_mask = struct.unpack_from(">H", rom, 0x00351A)[0]
    blink_threshold = struct.unpack_from(">H", rom, 0x00351E)[0]

    return {
        "milestone": "M15.283",
        "rom_path": str(rom_path),
        "title_screen": {
            "entry_addr": "0x0032F8",
            "font_addr": "0x16943C",
            "font_vram_addr": "0xF020",
            "title_gfx_addr": "0x180C56",
            "title_bgm_id": "0x0104",
            "attract_timeout_frames": attract_timeout,
            "blink_mask": blink_mask,
            "blink_on_threshold": blink_threshold,
            "prompt_text": read_cstr(rom, 0x003534),
            "copyright_text": read_cstr(rom, 0x00355A),
            "music_credit_text": read_cstr(rom, 0x00356B),
        },
        "input_chords": [
            {
                "pad_byte_ff165e": "0x90",
                "handler_addr": "0x0034BE",
                "sound_cmd": "0xA003AA",
                "target_addr": "0x03E4FA",
                "role": "Sound Test / Secret Menu (A + Start)",
            },
            {
                "pad_byte_ff165e": "0xD5",
                "handler_addr": "0x0034E4",
                "sound_cmd": "0xA00205",
                "helper_addr": "0x001E98",
                "target_addr": "0x03E4FA",
                "role": "Alternate Debug / Stage Select",
            },
            {
                "pad_byte_ff165e": "0x8A",
                "handler_addr": "0x003428",
                "next_mode": "0x08",
                "ram_demo_flag_ff1996": "0xFF",
                "role": "Immediate Demo / Attract Mode",
            },
            {
                "pad_byte_ff165e": "any_other_with_start",
                "handler_addr": "0x00347C",
                "sound_cmd": "0x00000002",
                "next_mode": "0x0C",
                "ram_demo_flag_ff1996": "0x00",
                "role": "Normal Start -> File Select / Gameplay",
            },
        ],
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_title_and_sound_test(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote title screen & {len(data['input_chords'])} input chords to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
