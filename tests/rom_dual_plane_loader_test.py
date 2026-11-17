#!/usr/bin/env python3
"""
Unit tests for ROM Dual Tilemap Plane Header Unpacker, VRAM Plane A/B Layout Configurator,
Stride Shift Calculator & Full Plane Initializer (0x00D542..0x00D7C0, 638 bytes)
for milestone M15.440.
"""

import json
from pathlib import Path
import pytest

EVIDENCE_FILE = Path("data/thor-evidence/rom_dual_plane_loader_m15_440.json")
ROM_FILE = Path("local-roms/Beyond Oasis (USA).md")

START_ADDR = 0x00D542
END_ADDR = 0x00D7C0
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

class TestRomDualPlaneLoader:
    def test_01_rom_slice_and_header(self, rom_bytes, evidence):
        assert evidence["milestone"] == "M15.440"
        assert evidence["byte_count"] == EXPECTED_LEN
        assert evidence["start_address"] == f"0x{START_ADDR:06X}"
        assert evidence["end_address"] == f"0x{END_ADDR:06X}"

        slice_bytes = rom_bytes[START_ADDR:END_ADDR]
        assert len(slice_bytes) == EXPECTED_LEN
        assert slice_bytes[:16].hex() == evidence["first_bytes"]
        assert slice_bytes[-16:].hex() == evidence["last_bytes"]

        # First instruction: movea.l (a4), a0
        assert slice_bytes[:2] == bytes([0x20, 0x54])
        # Last instruction of helper: rts (0x4E75)
        assert slice_bytes[-2:] == bytes([0x4E, 0x75])

    def test_02_plane_a_and_b_configuration(self, evidence):
        instructions = evidence["instructions"]
        by_addr = {ins["address"]: ins for ins in instructions}

        # Plane A VRAM base 0xC000 stored at 0x1A(a0)
        assert "0x00D56C" in by_addr
        assert by_addr["0x00D56C"]["op_str"] == "#$c000, $1a(a0)"

        # Plane A call to log2_shift helper
        assert "0x00D582" in by_addr
        assert by_addr["0x00D582"]["op_str"] == "$d7b0"

        # Plane B VRAM base 0xE000 stored at 0x1A(a0)
        assert "0x00D5D0" in by_addr
        assert by_addr["0x00D5D0"]["op_str"] == "#$e000, $1a(a0)"

        # Plane B call to log2_shift helper
        assert "0x00D5E8" in by_addr
        assert by_addr["0x00D5E8"]["op_str"] == "$d7b0"

    def test_03_vdp_scroll_and_plane_upload_logic(self, evidence):
        instructions = evidence["instructions"]
        by_addr = {ins["address"]: ins for ins in instructions}

        # Full-screen H-scroll and V-scroll register clears
        assert "0x00D668" in by_addr
        assert by_addr["0x00D668"]["op_str"] == "d1, $ff1344.l"
        assert "0x00D688" in by_addr
        assert by_addr["0x00D688"]["op_str"] == "d1, $ff1348.l"

        # VRAM H-scroll table write command 0x5C000003
        assert "0x00D6B2" in by_addr
        assert by_addr["0x00D6B2"]["op_str"] == "#$5c000003, $c00004.l"

        # VSRAM write command 0x40000010
        assert "0x00D6C6" in by_addr
        assert by_addr["0x00D6C6"]["op_str"] == "#$40000010, $c00004.l"

        # Plane A upload call bsr.w $d7c0
        assert "0x00D6FE" in by_addr
        assert by_addr["0x00D6FE"]["op_str"] == "$d7c0"

        # Plane B upload call bsr.w $d7c0
        assert "0x00D708" in by_addr
        assert by_addr["0x00D708"]["op_str"] == "$d7c0"

    def test_04_camera_bounds_and_flags(self, evidence):
        instructions = evidence["instructions"]
        by_addr = {ins["address"]: ins for ins in instructions}

        # Max X scroll limit stored in $FF177A
        assert "0x00D786" in by_addr
        assert by_addr["0x00D786"]["op_str"] == "d0, $ff177a.l"

        # Max Y scroll limit stored in $FF177C
        assert "0x00D796" in by_addr
        assert by_addr["0x00D796"]["op_str"] == "d0, $ff177c.l"

        # Status flags initialization
        assert "0x00D79C" in by_addr
        assert by_addr["0x00D79C"]["op_str"] == "$ff17c1.l"  # sf.b
        assert "0x00D7A2" in by_addr
        assert by_addr["0x00D7A2"]["op_str"] == "$ff17c2.l"  # st.b
        assert "0x00D7A8" in by_addr
        assert by_addr["0x00D7A8"]["op_str"] == "$ff17c3.l"  # st.b

        # Main routine returns at 0x00D7AE
        assert "0x00D7AE" in by_addr
        assert by_addr["0x00D7AE"]["mnemonic"] == "rts"

    def test_05_log2_shift_and_clamp_math_simulation(self):
        # Simulation of routine 0x00D7B0 (log2 byte stride shift helper)
        def sim_log2_byte_stride(val: int) -> int:
            d1 = 0
            d0 = (val + val) & 0xFFFF
            if d0 == 0:
                return 0
            while True:
                carry = (d0 & 1) != 0
                d0 = (d0 >> 1) & 0xFFFF
                if carry:
                    return d1
                d1 += 1

        # Test byte stride shift exponent: 2^d1 == val * 2 bytes
        assert sim_log2_byte_stride(0x20) == 6    # 32 words -> 64 bytes (2^6)
        assert sim_log2_byte_stride(0x40) == 7    # 64 words -> 128 bytes (2^7)
        assert sim_log2_byte_stride(0x80) == 8    # 128 words -> 256 bytes (2^8)
        assert sim_log2_byte_stride(0x100) == 9   # 256 words -> 512 bytes (2^9)
        assert sim_log2_byte_stride(0x200) == 10  # 512 words -> 1024 bytes (2^10)
        assert sim_log2_byte_stride(0x400) == 11  # 1024 words -> 2048 bytes (2^11)
        assert sim_log2_byte_stride(0x800) == 12  # 2048 words -> 4096 bytes (2^12)

        # Simulation of screen clamping limits
        def sim_screen_limits(width_tiles: int, height_tiles: int):
            max_x = (width_tiles - 40) * 8
            max_y = (height_tiles - 28) * 8
            return max_x, max_y

        assert sim_screen_limits(64, 32) == (192, 32)
        assert sim_screen_limits(128, 64) == (704, 288)
        assert sim_screen_limits(40, 28) == (0, 0)
