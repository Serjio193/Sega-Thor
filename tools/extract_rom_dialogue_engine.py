"""Extract Dialogue / Text Engine, control codes, font ref, and 116-entry string table.

Milestone: M15.281.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x00C386: Font decompression (0x16943C -> $FF2FA8 via 0x003820)
  - 0x00C3F8: Dialogue ID ($FF1870) lookup in 0x051514 self-relative u16 table
  - 0x00C40E..0x00C534: Control code dispatcher (< 0x20: 0x00,0x01,0x02,0x03,0x05,0x06,0x07)
  - 0x00C6BE: Glyph DMA emitter ((ch - 0x20) << 5 into $FF2FA8, 16-word VDP DMA)
  - 0x03D160: Epilogue/credits text drawer (0x16943C -> $FF3FAC, control 0x00/0x01)
  - 0x051514: 116-entry dialogue offset table (ends at 0x0515FC)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_dialogue_engine_m15_281.json")

FONT_COMPRESSED_ADDR = 0x16943C
DIALOGUE_TABLE_ADDR = 0x051514

CONTROL_CODES = [
    {"code": 0x00, "name": "end_dialogue", "handler": "0x00C4AA", "description": "Set $FF1887=0xFF, wait button, clear $FF1887, close box"},
    {"code": 0x01, "name": "newline", "handler": "0x00C4BC", "description": "Scroll/advance line (0x00C566), yield 1 frame (0x00C84E)"},
    {"code": 0x02, "name": "wait_button_newline", "handler": "0x00C4C8", "description": "Wait button (0x00C536) then newline (0x00C4BC)"},
    {"code": 0x03, "name": "delay_frames", "handler": "0x00C4CE", "description": "Read u8 frame count, yield N frames via 0x00C84E"},
    {"code": 0x05, "name": "wait_button", "handler": "0x00C4E4", "description": "Wait button (0x00C536) and continue"},
    {"code": 0x06, "name": "clear_page", "handler": "0x00C4EC", "description": "Scroll 4 lines, reset $FF187E/$FF1880 and line offsets $FF1876..$FF187C"},
    {"code": 0x07, "name": "wait_button_clear_page", "handler": "0x00C530", "description": "Wait button (0x00C536) then clear_page (0x00C4EC)"},
]


def decode_dialogue_bytes(raw: bytes) -> str:
    """Decode ROM dialogue bytes into readable ASCII with <XX> control tags."""
    out = []
    i = 0
    while i < len(raw):
        b = raw[i]
        if b == 0:
            break
        if b == 0x03 and i + 1 < len(raw):
            out.append(f"<03:{raw[i + 1]:02X}>")
            i += 2
            continue
        if 32 <= b < 127:
            out.append(chr(b))
        else:
            out.append(f"<{b:02X}>")
        i += 1
    return "".join(out)


def extract_dialogue_engine(rom_path: Path) -> dict:
    """Extract dialogue control codes and all 116 dialogue string entries."""
    rom = rom_path.read_bytes()
    first_off = struct.unpack_from(">H", rom, DIALOGUE_TABLE_ADDR)[0]
    count = first_off // 2

    entries = []
    for idx in range(count):
        entry_addr = DIALOGUE_TABLE_ADDR + idx * 2
        rel_off = struct.unpack_from(">H", rom, entry_addr)[0]
        target_addr = entry_addr + rel_off
        chunk = rom[target_addr:target_addr + 1024]
        nul = chunk.find(0)
        if nul != -1:
            chunk = chunk[:nul]
        entries.append({
            "id": idx,
            "entry_addr": f"0x{entry_addr:06X}",
            "rel_offset": rel_off,
            "text_addr": f"0x{target_addr:06X}",
            "byte_length": len(chunk),
            "text": decode_dialogue_bytes(chunk),
        })

    return {
        "milestone": "M15.281",
        "rom_path": str(rom_path),
        "font": {
            "compressed_addr": f"0x{FONT_COMPRESSED_ADDR:06X}",
            "decompressor_addr": "0x003820",
            "glyph_stride_bytes": 32,
            "ascii_base": 0x20,
            "ram_buffer_dialogue": "0xFF2FA8",
            "ram_buffer_epilogue": "0xFF3FAC",
        },
        "engine": {
            "dialogue_entry_addr": "0x00C386",
            "dialogue_lookup_addr": "0x00C3F8",
            "glyph_emitter_addr": "0x00C6BE",
            "epilogue_drawer_addr": "0x03D160",
            "table_addr": f"0x{DIALOGUE_TABLE_ADDR:06X}",
            "entry_count": count,
            "ram_dialogue_id": "0xFF1870",
            "ram_col_cursor": "0xFF187E",
            "ram_line_cursor": "0xFF1880",
            "ram_line_vram_offsets": "0xFF1876..0xFF187C",
        },
        "control_codes": CONTROL_CODES,
        "entries": entries,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_dialogue_engine(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['entries'])} dialogue entries to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
