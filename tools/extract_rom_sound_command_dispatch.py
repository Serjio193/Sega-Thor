#!/usr/bin/env python3
"""
extract_rom_sound_command_dispatch.py — Milestone M15.270

Extracts ROM semantic knowledge for the Beyond Oasis Sound Driver (`0x060000..`):
1. Driver Jump Table (`0x060000..0x06000C`):
   - `0x060000`: `bra.w 0x060090` (VBlank / Init driver tick)
   - `0x060004`: `bra.w 0x06042A` (Enqueue sound / BGM command `d0.L`)
   - `0x060008`: `bra.w 0x061232` (Z80 / FM / PSG hardware tick)
2. Command Dispatcher (`0x06042A..0x0604BC`):
   - System / BGM Control (`d0 & 0xFF00 == 0x0000`, commands `0x00..0x08`)
   - Category Dispatch (`d0 & 0xFF00 == 0x0100..0x0500`):
     * `0x0100`: `0x060B50` (BGM Track)
     * `0x0200`: `0x06112E` (SFX Bank 2, priority in `d0[23:16]`)
     * `0x0300`: `0x060F1C` (SFX Bank 3, e.g. `0x80034F`, `0x800399`)
     * `0x0400`: `0x060F54` (SFX Bank 4, e.g. `0x8004D9`)
     * `0x0500`: `0x060CDA` (Category 5)
3. Sound Driver WRAM & Sequence Bank:
   - Driver state base `$FF001A`, BGM/SFX slots `$FF0628` / `$FF06F2`.
"""

import json
import struct
import sys
from pathlib import Path

ROM_PATH_DEFAULT = "local-roms/Beyond Oasis (USA).md"
OUTPUT_PATH_DEFAULT = "data/thor-evidence/rom_sound_command_dispatch_m15_270.json"


def read_u16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">H", rom, addr)[0]


def read_s16_be(rom: bytes, addr: int) -> int:
    return struct.unpack_from(">h", rom, addr)[0]


def decode_bra_w(rom: bytes, pc: int) -> int:
    assert read_u16_be(rom, pc) == 0x6000, f"Expected bra.w at 0x{pc:06X}"
    return pc + 2 + read_s16_be(rom, pc + 2)


def decode_beq_w(rom: bytes, pc: int) -> int:
    assert read_u16_be(rom, pc) == 0x6700, f"Expected beq.w at 0x{pc:06X}"
    return pc + 2 + read_s16_be(rom, pc + 2)


def main() -> int:
    rom_path = Path(ROM_PATH_DEFAULT)
    out_path = Path(OUTPUT_PATH_DEFAULT)
    if not rom_path.exists():
        print(f"Error: ROM not found at {rom_path}")
        return 1

    rom = rom_path.read_bytes()

    entrypoints = {
        "driver_init_or_tick": f"0x{decode_bra_w(rom, 0x060000):06X}",
        "enqueue_command": f"0x{decode_bra_w(rom, 0x060004):06X}",
        "hardware_tick": f"0x{decode_bra_w(rom, 0x060008):06X}",
    }

    system_commands = {
        "0x00": f"0x{decode_beq_w(rom, 0x060448):06X}",
        "0x01": f"0x{decode_beq_w(rom, 0x060450):06X}",
        "0x02": f"0x{decode_beq_w(rom, 0x060458):06X}",
        "0x03": f"0x{decode_beq_w(rom, 0x060460):06X}",
        "0x04": f"0x{decode_beq_w(rom, 0x060468):06X}",
        "0x05": f"0x{decode_beq_w(rom, 0x060470):06X}",
        "0x06": f"0x{decode_beq_w(rom, 0x060478):06X}",
        "0x07": f"0x{decode_beq_w(rom, 0x060480):06X}",
        "0x08": f"0x{decode_beq_w(rom, 0x060488):06X}",
    }

    category_commands = {
        "0x0100_bgm": f"0x{decode_beq_w(rom, 0x060494):06X}",
        "0x0200_sfx_bank2": f"0x{decode_beq_w(rom, 0x06049C):06X}",
        "0x0300_sfx_bank3": f"0x{decode_beq_w(rom, 0x0604A4):06X}",
        "0x0400_sfx_bank4": f"0x{decode_beq_w(rom, 0x0604AC):06X}",
        "0x0500_category5": f"0x{decode_beq_w(rom, 0x0604B4):06X}",
    }

    evidence = {
        "milestone": "M15.270",
        "description": "ROM Sound Driver Entrypoints (0x060000..0x060008) and Command Dispatcher (0x06042A)",
        "entrypoints": entrypoints,
        "system_commands_0x00_0x08": system_commands,
        "category_commands_0x0100_0x0500": category_commands,
        "wram_addresses": {
            "driver_state_base": "0x00FF001A",
            "bgm_channel_a": "0x00FF0628",
            "bgm_channel_b": "0x00FF06F2",
        },
    }

    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(evidence, indent=2), encoding="utf-8")
    print(f"Emitted {out_path} ({len(system_commands)} sys cmds, {len(category_commands)} categories)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
