"""Extract Sound Driver 11-Channel FM/PSG/SFX RAM Map & Fade/Mute (0x0604E6..0x06064B).

Milestone: M15.301.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x0604E6: System cmd 0x00 (Stop/Mute All, clears $FF0010..$FF0014)
  - 0x060516..0x060648: System cmd 0x07 (Fade/Attenuate all 11 channels: 6 FM, 3 PSG, 2 SFX)
    - 6 FM channels ($FF0022..$FF0414, stride 0xCA = 202 bytes)
    - 3 PSG channels ($FF049E..$FF05B2, stride 0x8A = 138 bytes)
    - 2 SFX channels ($FF0628, $FF06F2, stride 0xCA = 202 bytes)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_sound_channels_m15_301.json")

LEA_INSN_ADDRS = [
    ("fm_1", "fm", 0x060530),
    ("fm_2", "fm", 0x06054A),
    ("fm_3", "fm", 0x060564),
    ("fm_4", "fm", 0x06057E),
    ("fm_5", "fm", 0x060598),
    ("fm_6_dac", "fm", 0x0605B2),
    ("psg_1", "psg", 0x0605CC),
    ("psg_2", "psg", 0x0605E6),
    ("psg_3_noise", "psg", 0x060600),
    ("sfx_fm", "sfx", 0x060614),
    ("sfx_psg", "sfx", 0x06062E),
]


def extract_sound_channels(rom_path: Path) -> dict:
    """Extract all 11 sound driver channel RAM bases and fade/mute params."""
    rom = rom_path.read_bytes()
    channels = []
    for idx, (name, kind, insn_addr) in enumerate(LEA_INSN_ADDRS):
        ram_addr = struct.unpack_from(">I", rom, insn_addr + 2)[0]
        channels.append({
            "index": idx,
            "name": name,
            "kind": kind,
            "ram_addr": f"0x{ram_addr:06X}",
        })

    return {
        "milestone": "M15.301",
        "rom_path": str(rom_path),
        "sound_channels": {
            "stop_all_addr": "0x0604E6",
            "fade_all_addr": "0x060516",
            "fm_stride_bytes": 0xCA,
            "psg_stride_bytes": 0x8A,
            "channel_count": len(channels),
            "fade_init_step": rom[0x060519],
            "fade_init_timer": rom[0x06051F],
        },
        "channels": channels,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_sound_channels(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['channels'])} sound driver channels to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
