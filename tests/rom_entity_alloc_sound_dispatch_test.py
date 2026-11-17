#!/usr/bin/env python3
"""
Unit tests for ROM Projectile/Effect Entity Pool Allocators & Sound Driver Command Dispatchers
(0x00D9A4..0x00DB1A, 374 bytes) for milestone M15.442.
"""

import json
from pathlib import Path
import pytest

EVIDENCE_FILE = Path("data/thor-evidence/rom_entity_alloc_sound_dispatch_m15_442.json")
ROM_FILE = Path("local-roms/Beyond Oasis (USA).md")

START_ADDR = 0x00D9A4
END_ADDR = 0x00DB1A
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

class TestRomEntityAllocSoundDispatch:
    def test_01_rom_slice_and_header(self, rom_bytes, evidence):
        assert evidence["milestone"] == "M15.442"
        assert evidence["byte_count"] == EXPECTED_LEN
        assert evidence["start_address"] == f"0x{START_ADDR:06X}"
        assert evidence["end_address"] == f"0x{END_ADDR:06X}"

        slice_bytes = rom_bytes[START_ADDR:END_ADDR]
        assert len(slice_bytes) == EXPECTED_LEN
        assert slice_bytes[:16].hex() == evidence["first_bytes"]
        assert slice_bytes[-16:].hex() == evidence["last_bytes"]

        # First instruction at 0x00D9A4: movem.l d0-d7/a0-a6, -(a7) (48 E7 FF FE)
        assert slice_bytes[:4] == bytes([0x48, 0xE7, 0xFF, 0xFE])
        # Last instruction at 0x00DB18: rts (4E 75)
        assert slice_bytes[-2:] == bytes([0x4E, 0x75])

    def test_02_projectile_and_spark_allocators(self, evidence):
        instructions = evidence["instructions"]
        by_addr = {ins["address"]: ins for ins in instructions}

        # Projectile pool base $FF2ABC, 8 slots, base tile index $0694
        assert "0x00D9C2" in by_addr
        assert by_addr["0x00D9C2"]["op_str"] == "#$694, d0"
        assert "0x00D9C6" in by_addr
        assert by_addr["0x00D9C6"]["op_str"] == "$ff2abc.l, a6"
        assert "0x00D9CC" in by_addr
        assert by_addr["0x00D9CC"]["op_str"] == "#$7, d1"
        assert "0x00D9D8" in by_addr
        assert by_addr["0x00D9D8"]["op_str"] == "#$9, d0"

        # Spark pool base $FF2D8C, 6 slots
        assert "0x00D9F4" in by_addr
        assert by_addr["0x00D9F4"]["op_str"] == "$ff2d8c.l, a6"
        assert "0x00D9FA" in by_addr
        assert by_addr["0x00D9FA"]["op_str"] == "#$5, d1"

    def test_03_general_entity_and_npc_allocators(self, evidence):
        instructions = evidence["instructions"]
        by_addr = {ins["address"]: ins for ins in instructions}

        # General entity pool bases ($FF1C1C for 18 slots, $FF1CD8 for 17 slots)
        assert "0x00DA1E" in by_addr
        assert by_addr["0x00DA1E"]["op_str"] == "$ff1c1c.l, a6"
        assert "0x00DA2E" in by_addr
        assert by_addr["0x00DA2E"]["op_str"] == "$ff1cd8.l, a6"

        # NPC type #$2E and timer #$3C
        assert "0x00DAA0" in by_addr
        assert by_addr["0x00DAA0"]["op_str"] == "#$2e, $0(a6)"
        assert "0x00DAAA" in by_addr
        assert by_addr["0x00DAAA"]["op_str"] == "#$3c, $2(a6)"

        # Offset relative to player entity at $FF19E8
        assert "0x00DAB2" in by_addr
        assert by_addr["0x00DAB2"]["op_str"] == "#$ff19e8, d0"

    def test_04_sound_driver_dispatchers(self, evidence):
        instructions = evidence["instructions"]
        by_addr = {ins["address"]: ins for ins in instructions}

        # sound_play_music_track: stores d0 in $FF16F2 and jumps to $60004
        assert "0x00DAC2" in by_addr
        assert by_addr["0x00DAC2"]["op_str"] == "d0, $ff16f2.l"
        assert "0x00DAC8" in by_addr
        assert by_addr["0x00DAC8"]["op_str"] == "$60004.l"

        # sound_pause_music_track: cmd 7
        assert "0x00DAF2" in by_addr
        assert by_addr["0x00DAF2"]["op_str"] == "#$7, d0"

        # sound_stop_music_track: cmd 0
        assert "0x00DB0C" in by_addr
        assert by_addr["0x00DB0C"]["op_str"] == "#$0, d0"

    def test_05_pool_math_and_allocator_simulation(self):
        # Simulation of alloc_projectile_entity_slot
        def sim_alloc_projectile(pool: list[int]):
            base_addr = 0xFF2ABC
            stride = 0x5A
            base_tile = 0x0694
            tile_step = 9
            for i, status in enumerate(pool):
                if status == 0:
                    addr = base_addr + i * stride
                    tile = base_tile + i * tile_step
                    return True, addr, tile
            return False, 0, 0

        # Slot 0 free
        ok, addr, tile = sim_alloc_projectile([0] * 8)
        assert ok
        assert addr == 0xFF2ABC
        assert tile == 0x0694

        # Slot 3 free
        ok, addr, tile = sim_alloc_projectile([1, 1, 1, 0, 0, 0, 0, 0])
        assert ok
        assert addr == 0xFF2ABC + 3 * 0x5A
        assert tile == 0x0694 + 3 * 9

        # Full pool
        ok, _, _ = sim_alloc_projectile([1] * 8)
        assert not ok

        # Simulation of NPC offset
        player_base = 0xFF19E8
        slot_18_0 = 0xFF1C1C
        assert slot_18_0 - player_base == 0x0234
