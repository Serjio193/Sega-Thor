#!/usr/bin/env python3
"""
Unit tests for ROM Visible Tilemap Plane VRAM Streamer & Full-Screen Blank Tile Plane Clearer
(0x00D7C0..0x00D9A4, 484 bytes) for milestone M15.441.
"""

import json
from pathlib import Path
import pytest

EVIDENCE_FILE = Path("data/thor-evidence/rom_visible_plane_streamer_m15_441.json")
ROM_FILE = Path("local-roms/Beyond Oasis (USA).md")

START_ADDR = 0x00D7C0
END_ADDR = 0x00D9A4
EXPECTED_LEN = END_ADDR - START_ADDR

@pytest.fixture(scope="module")
def evidence():
    if not EVIDENCE_FILE.exists():
        pytest.fail(f"Evidence file missing: {EVIDENCE_FILE}")
    with open(EVIDENCE_FILE, "r", encoding="utf-8") as f:
        return json.load(f)

@pytest.fixture(scope="module")
def rom_bytes():
    if not ROM_FILE.exists():
        pytest.fail(f"ROM file missing: {ROM_FILE}")
    with open(ROM_FILE, "rb") as f:
        return f.read()

class TestRomVisiblePlaneStreamer:
    def test_01_rom_slice_and_header(self, rom_bytes, evidence):
        assert evidence["milestone"] == "M15.441"
        assert evidence["byte_count"] == EXPECTED_LEN
        assert evidence["start_address"] == f"0x{START_ADDR:06X}"
        assert evidence["end_address"] == f"0x{END_ADDR:06X}"

        slice_bytes = rom_bytes[START_ADDR:END_ADDR]
        assert len(slice_bytes) == EXPECTED_LEN
        assert slice_bytes[:16].hex() == evidence["first_bytes"]
        assert slice_bytes[-16:].hex() == evidence["last_bytes"]

        # First instruction at 0x00D7C0: clr.w d0 (0x4240)
        assert slice_bytes[:2] == bytes([0x42, 0x40])
        # Last instruction of blank fill raw at 0x00D9A2: rts (0x4E75)
        assert slice_bytes[-2:] == bytes([0x4E, 0x75])

    def test_02_streamer_context_zeroing_and_flag_branch(self, evidence):
        instructions = evidence["instructions"]
        by_addr = {ins["address"]: ins for ins in instructions}

        # Clear context offsets
        assert "0x00D7C0" in by_addr
        assert by_addr["0x00D7C0"]["mnemonic"] == "clr.w"
        assert by_addr["0x00D7C2"]["op_str"] == "d0, $e(a3)"
        assert by_addr["0x00D7C6"]["op_str"] == "d0, $10(a3)"

        # Flag test on bit 0 of $27(a3) branching to blank tile filler at $00D95E
        assert "0x00D7DE" in by_addr
        assert by_addr["0x00D7DE"]["mnemonic"] == "btst.b"
        assert by_addr["0x00D7DE"]["op_str"] == "#$0, $27(a3)"
        assert "0x00D7E4" in by_addr
        assert by_addr["0x00D7E4"]["mnemonic"] == "beq.w"
        assert by_addr["0x00D7E4"]["op_str"] == "$d95e"

    def test_03_vram_write_command_and_transfer_loop(self, evidence):
        instructions = evidence["instructions"]
        by_addr = {ins["address"]: ins for ins in instructions}

        # VDP ports setup
        assert "0x00D850" in by_addr
        assert by_addr["0x00D850"]["op_str"] == "$c00000.l, a2"
        assert "0x00D856" in by_addr
        assert by_addr["0x00D856"]["op_str"] == "$4(a2), a4"

        # VRAM data write loop
        assert "0x00D8C4" in by_addr
        assert by_addr["0x00D8C4"]["mnemonic"] == "move.w"
        assert by_addr["0x00D8C4"]["op_str"] == "(a1)+, (a2)"

        # Streamer exit rts
        assert "0x00D94E" in by_addr
        assert by_addr["0x00D94E"]["mnemonic"] == "rts"

    def test_04_blank_fill_routines(self, evidence):
        instructions = evidence["instructions"]
        by_addr = {ins["address"]: ins for ins in instructions}

        # Blank fill safe wrapper at 0x00D950
        assert "0x00D950" in by_addr
        assert by_addr["0x00D950"]["mnemonic"] == "movem.l"
        assert "0x00D954" in by_addr
        assert by_addr["0x00D954"]["op_str"] == "$d962"
        assert "0x00D95C" in by_addr
        assert by_addr["0x00D95C"]["mnemonic"] == "rts"

        # Blank tile fill loop writing #$0780
        assert "0x00D990" in by_addr
        assert by_addr["0x00D990"]["op_str"] == "#$780, (a2)"

        # Column loop dbra d3, $d990
        assert "0x00D994" in by_addr
        assert by_addr["0x00D994"]["mnemonic"] == "dbra"

        # Row loop dbra d2, $d98a
        assert "0x00D99A" in by_addr
        assert by_addr["0x00D99A"]["mnemonic"] == "dbra"

        # Final exit rts
        assert "0x00D9A2" in by_addr
        assert by_addr["0x00D9A2"]["mnemonic"] == "rts"

    def test_05_vram_command_and_blank_fill_simulation(self):
        # Simulation of VDP VRAM write command construction:
        # High word: ((vram_addr & 0x3FFF) | 0x4000)
        # Low word:  ((vram_addr >> 14) & 0x0003)
        def sim_vram_write_cmd(vram_addr: int) -> int:
            high = ((vram_addr & 0x3FFF) | 0x4000) & 0xFFFF
            low = ((vram_addr >> 14) & 0x0003) & 0xFFFF
            return (high << 16) | low

        # Plane A base $C000:
        # High = (0xC000 & 0x3FFF) | 0x4000 = 0x4000
        # Low  = (0xC000 >> 14) & 3 = 3
        # Cmd  = 0x40000003
        assert sim_vram_write_cmd(0xC000) == 0x40000003

        # Plane B base $E000:
        # High = (0xE000 & 0x3FFF) | 0x4000 = 0x6000
        # Low  = (0xE000 >> 14) & 3 = 3
        # Cmd  = 0x60000003
        assert sim_vram_write_cmd(0xE000) == 0x60000003

        # Blank tile fill dimensions
        cols = 40  # 320 px / 8 px per tile
        rows = 28  # 224 px / 8 px per tile
        total_tiles = cols * rows
        assert total_tiles == 1120
        assert total_tiles * 2 == 2240  # bytes
