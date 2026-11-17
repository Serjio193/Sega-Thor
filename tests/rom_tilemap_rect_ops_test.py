#!/usr/bin/env python3
"""
Unit tests for ROM Tilemap Rectangle Fill & Block Copy Subsystem
(0x00DB1A..0x00DC26, 268 bytes) for milestone M15.443.
"""

import json
from pathlib import Path
import pytest

EVIDENCE_FILE = Path("data/thor-evidence/rom_tilemap_rect_ops_m15_443.json")
ROM_FILE = Path("local-roms/Beyond Oasis (USA).md")

START_ADDR = 0x00DB1A
END_ADDR = 0x00DC26
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

class TestRomTilemapRectOps:
    def test_01_rom_slice_and_header(self, rom_bytes, evidence):
        assert evidence["milestone"] == "M15.443"
        assert evidence["byte_count"] == EXPECTED_LEN
        assert evidence["start_address"] == f"0x{START_ADDR:06X}"
        assert evidence["end_address"] == f"0x{END_ADDR:06X}"

        slice_bytes = rom_bytes[START_ADDR:END_ADDR]
        assert len(slice_bytes) == EXPECTED_LEN
        assert slice_bytes[:16].hex() == evidence["first_bytes"]
        assert slice_bytes[-16:].hex() == evidence["last_bytes"]

        # First instruction at 0x00DB1A: movem.l d1-d6/a0-a1, -(a7) (48 E7 7E C0)
        assert slice_bytes[:4] == bytes([0x48, 0xE7, 0x7E, 0xC0])
        # Last instruction of tilemap_copy_rect_byte at 0x00DC24: rts (4E 75)
        assert slice_bytes[-2:] == bytes([0x4E, 0x75])

    def test_02_fill_rect_byte_routine(self, evidence):
        instructions = evidence["instructions"]
        by_addr = {ins["address"]: ins for ins in instructions}

        # Check Plane selection via $FF16F0 bit 7
        assert "0x00DB1E" in by_addr
        assert by_addr["0x00DB1E"]["op_str"] == "$ff1766.l, a0"
        assert "0x00DB24" in by_addr
        assert by_addr["0x00DB24"]["op_str"] == "#$7, $ff16f0.l"
        assert "0x00DB2E" in by_addr
        assert by_addr["0x00DB2E"]["op_str"] == "$ff1770.l, a0"

        # Byte fill loop
        assert "0x00DB4E" in by_addr
        assert by_addr["0x00DB4E"]["op_str"] == "d0, (a1)+"
        assert "0x00DB62" in by_addr
        assert by_addr["0x00DB62"]["mnemonic"] == "rts"

    def test_03_copy_rect_word_and_sync(self, evidence):
        instructions = evidence["instructions"]
        by_addr = {ins["address"]: ins for ins in instructions}

        # Subroutine entry calls byte copy helper $dbca
        assert "0x00DB64" in by_addr
        assert by_addr["0x00DB64"]["op_str"] == "$dbca"

        # Word copy loop
        assert "0x00DBA6" in by_addr
        assert by_addr["0x00DBA6"]["mnemonic"] == "move.w"
        assert by_addr["0x00DBA6"]["op_str"] == "(a0)+, (a1)+"

        # VRAM upload sync call $dc26
        assert "0x00DBC0" in by_addr
        assert by_addr["0x00DBC0"]["op_str"] == "$dc26"
        assert "0x00DBC8" in by_addr
        assert by_addr["0x00DBC8"]["mnemonic"] == "rts"

    def test_04_copy_rect_byte(self, evidence):
        instructions = evidence["instructions"]
        by_addr = {ins["address"]: ins for ins in instructions}

        # Compare a3 with #$FF1716 (Plane A context base)
        assert "0x00DBD4" in by_addr
        assert by_addr["0x00DBD4"]["op_str"] == "#$ff1716, a3"

        # Byte copy loop
        assert "0x00DC0E" in by_addr
        assert by_addr["0x00DC0E"]["mnemonic"] == "move.b"
        assert by_addr["0x00DC0E"]["op_str"] == "(a0)+, (a1)+"
        assert "0x00DC24" in by_addr
        assert by_addr["0x00DC24"]["mnemonic"] == "rts"

    def test_05_rect_offset_math_simulation(self):
        # Simulation of rectangular stride offset calculation:
        # offset = (y1 << shift) + x1
        def sim_rect_offset(x1: int, y1: int, shift: int) -> int:
            return (y1 << shift) + x1

        # Plane A standard shift = 7 (128 bytes per row)
        assert sim_rect_offset(0, 0, 7) == 0
        assert sim_rect_offset(4, 2, 7) == 260
        assert sim_rect_offset(10, 5, 7) == 650

        # Word tilemap stride offset: (y1 << shift) + 2 * x1
        def sim_word_tile_offset(x1: int, y1: int, shift: int) -> int:
            return (y1 << shift) + (x1 * 2)

        assert sim_word_tile_offset(0, 0, 8) == 0
        assert sim_word_tile_offset(4, 2, 8) == 520
