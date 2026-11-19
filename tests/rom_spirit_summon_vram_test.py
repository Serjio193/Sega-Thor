#!/usr/bin/env python3
"""
Deterministic unit tests for ROM Companion Spirit VRAM Graphics Decompressor & DMA Uploader,
Special Lock Reset & 4-Spirit Summon Dispatcher (0x014522..0x0147E0).
Milestone M15.502.
"""

from __future__ import annotations

import json
from pathlib import Path
import unittest

ROOT_DIR = Path(__file__).resolve().parent.parent
EVIDENCE_PATH = ROOT_DIR / "data" / "thor-evidence" / "rom_spirit_summon_vram_m15_502.json"

# Spirit Type Constants
TYPE_DYTTO = 0x0018
TYPE_EFREET = 0x001A
TYPE_SHADE = 0x001C
TYPE_BOW = 0x001E

# VRAM & DMA Constants
DMA_CMD_SRC_WORD = 0x007F97D4
DMA_LENGTH_WORD = 0xF500

SPIRIT_GFX_TABLE = {
    TYPE_DYTTO: {"name": "Dytto", "rom_art": 0x00141580, "vram_tile": 0x04D0},
    TYPE_EFREET: {"name": "Efreet", "rom_art": 0x001425AE, "vram_tile": 0x0450},
    TYPE_SHADE: {"name": "Shade", "rom_art": 0x00143262, "vram_tile": 0x0280},
    TYPE_BOW: {"name": "Bow", "rom_art": 0x00143A42, "vram_tile": 0x0360},
}


class MockVdpDmaQueue:
    def __init__(self, base_addr: int = 0x00FF18A0):
        self.queue_ptr = base_addr
        self.entries: list[tuple[int, int, int]] = []

    def queue_dma(self, cmd_word: int, length_word: int, vram_tile: int) -> None:
        self.entries.append((cmd_word, length_word, vram_tile))
        self.queue_ptr += 8


def lookup_spirit_vram_and_dma(spirit_type: int) -> tuple[int, int, int, int] | None:
    """Emulates 0x014522..0x014628 VRAM lookup."""
    info = SPIRIT_GFX_TABLE.get(spirit_type)
    if info is None:
        return None
    return (info["rom_art"], DMA_CMD_SRC_WORD, DMA_LENGTH_WORD, info["vram_tile"])


def dispatch_spirit_summon(
    summon_index: int,
    ram_state: dict[str, int],
) -> str | None:
    """Emulates 0x014628..0x014658 overlay reset and summon dispatch."""
    # Reset overlay registers
    ram_state["spirit_hold_counter"] = 0
    ram_state["spirit_dtap_timer"] = 0
    ram_state["spirit_prev_button_a"] = 0xFF

    if summon_index == 0:
        return "Dytto"
    elif summon_index == 1:
        return "Efreet"
    elif summon_index == 2:
        return "Shade"
    elif summon_index == 3:
        return "Bow"
    else:
        return None  # RTS at 0x014656


class MockSpiritEntity:
    def __init__(self, x: int = 0x00A00000, y: int = 0x00B00000, z: int = 0x00020000, ground_z: int = 0x0002):
        self.type_id = 0
        self.action_state = 0
        self.substate_2a = 0
        self.anim_16 = 0
        self.tile_attr_18 = 0
        self.pos_x = x
        self.pos_y = y
        self.pos_z = z
        self.ground_z = ground_z
        self.max_sp_88 = 0
        self.cur_sp_8a = 0
        self.drain_interval_8c = 0
        self.drain_timer_8e = 0
        self.status_flags_36 = 0
        self.parent_ptr = 0
        self.sfx_played: int | None = None
        self.dma_queued = False
        self.bow_root_called = False

    def spawn_dytto(self, dma_queue: MockVdpDmaQueue, ram: dict[str, int]) -> None:
        # Preserve coords
        d0, d1, d2, d3 = self.pos_x, self.pos_y, self.pos_z, self.ground_z
        self.type_id = TYPE_DYTTO
        self.anim_16 = 2
        self.tile_attr_18 = 0x04F8
        self.max_sp_88 = 100
        self.cur_sp_8a = 100
        self.drain_interval_8c = 10
        self.drain_timer_8e = 10
        # Restore coords
        self.pos_x, self.pos_y, self.pos_z, self.ground_z = d0, d1, d2, d3
        self.action_state = 2
        self.substate_2a = 4
        # Call 0x014522
        vram_info = lookup_spirit_vram_and_dma(self.type_id)
        if vram_info:
            dma_queue.queue_dma(vram_info[1], vram_info[2], vram_info[3])
            self.dma_queued = True
        ram["spawn_lock"] = 0

    def spawn_efreet(self, dma_queue: MockVdpDmaQueue, ram: dict[str, int], entity_addr: int = 0xFF1AA4) -> None:
        d0, d1, d2, d3 = self.pos_x, self.pos_y, self.pos_z, self.ground_z
        d2 += 0x00080000  # +8.0 px fixed point
        self.type_id = TYPE_EFREET
        self.tile_attr_18 = 0x04F8
        self.max_sp_88 = 100
        self.cur_sp_8a = 100
        self.drain_interval_8c = 10
        self.drain_timer_8e = 10
        self.pos_x, self.pos_y, self.pos_z, self.ground_z = d0, d1, d2, d3
        self.action_state = 4
        self.substate_2a = 4
        self.anim_16 = 2
        self.parent_ptr = entity_addr
        vram_info = lookup_spirit_vram_and_dma(self.type_id)
        if vram_info:
            dma_queue.queue_dma(vram_info[1], vram_info[2], vram_info[3])
            self.dma_queued = True
        ram["spawn_lock"] = 0

    def spawn_shade(self, dma_queue: MockVdpDmaQueue, ram: dict[str, int]) -> None:
        d0, d1, d2, d3 = self.pos_x, self.pos_y, self.pos_z, self.ground_z
        self.type_id = TYPE_SHADE
        self.tile_attr_18 = 0x04F8
        self.max_sp_88 = 100
        self.cur_sp_8a = 100
        self.drain_interval_8c = 10
        self.drain_timer_8e = 10
        self.pos_x, self.pos_y, self.pos_z, self.ground_z = d0, d1, d2, d3
        self.action_state = 2
        self.substate_2a = 0
        self.anim_16 = 0
        vram_info = lookup_spirit_vram_and_dma(self.type_id)
        if vram_info:
            dma_queue.queue_dma(vram_info[1], vram_info[2], vram_info[3])
            self.dma_queued = True
        ram["spawn_lock"] = 0

    def spawn_bow(self, dma_queue: MockVdpDmaQueue, ram: dict[str, int]) -> None:
        self.type_id = TYPE_BOW
        vram_info = lookup_spirit_vram_and_dma(self.type_id)
        if vram_info:
            dma_queue.queue_dma(vram_info[1], vram_info[2], vram_info[3])
            self.dma_queued = True
        self.bow_root_called = True
        self.action_state = 0x0E
        self.substate_2a = 0x1E
        self.anim_16 = 0
        self.tile_attr_18 = 0x07A8
        self.status_flags_36 &= ~0x01
        self.sfx_played = 0x0080039F
        ram["spawn_lock"] = 0


class TestROMSpiritSummonVram(unittest.TestCase):
    def setUp(self) -> None:
        self.assertTrue(EVIDENCE_PATH.exists(), f"Evidence file not found: {EVIDENCE_PATH}")
        self.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_metadata_and_range(self) -> None:
        self.assertEqual(self.evidence["milestone"], "M15.502")
        self.assertEqual(self.evidence["range"]["start"], "0x014522")
        self.assertEqual(self.evidence["range"]["end"], "0x0147E0")
        self.assertEqual(self.evidence["range"]["size_bytes"], 702)
        self.assertEqual(self.evidence["instruction_count"], 148)
        self.assertEqual(len(self.evidence["instructions"]), 148)
        self.assertEqual(len(self.evidence["routines"]), 6)

    def test_routine_boundaries_and_sizes(self) -> None:
        expected_routines = [
            ("Spirits_DecompressAndQueueVramGfx", "0x014522", "0x014628", 262),
            ("Spirits_ClearOverlayStateAndDispatch", "0x014628", "0x014658", 48),
            ("Spirits_SpawnDytto", "0x014658", "0x0146C0", 104),
            ("Spirits_SpawnEfreet", "0x0146C0", "0x014732", 114),
            ("Spirits_SpawnShade", "0x014732", "0x014798", 102),
            ("Spirits_SpawnBow", "0x014798", "0x0147E0", 72),
        ]
        actual_routines = [
            (r["name"], r["start"], r["end"], r["size_bytes"])
            for r in self.evidence["routines"]
        ]
        self.assertEqual(actual_routines, expected_routines)
        total_routine_bytes = sum(r["size_bytes"] for r in self.evidence["routines"])
        self.assertEqual(total_routine_bytes, 702)

    def test_spirits_vram_table_lookup_and_dma_descriptors(self) -> None:
        vram_table = self.evidence["spirits_vram_table"]
        self.assertEqual(len(vram_table), 4)

        # Dytto
        dytto = vram_table["0x0018"]
        self.assertEqual(dytto["name"], "Dytto")
        self.assertEqual(dytto["rom_art_address"], "0x00141580")
        self.assertEqual(dytto["vram_tile"], "0x04D0")
        self.assertEqual(dytto["dma_length_word"], "0xF500")
        self.assertEqual(dytto["dma_cmd_word"], "0x007F97D4")

        # Efreet
        efreet = vram_table["0x001A"]
        self.assertEqual(efreet["name"], "Efreet")
        self.assertEqual(efreet["rom_art_address"], "0x001425AE")
        self.assertEqual(efreet["vram_tile"], "0x0450")
        self.assertEqual(efreet["dma_length_word"], "0xF500")

        # Shade
        shade = vram_table["0x001C"]
        self.assertEqual(shade["name"], "Shade")
        self.assertEqual(shade["rom_art_address"], "0x00143262")
        self.assertEqual(shade["vram_tile"], "0x0280")
        self.assertEqual(shade["dma_length_word"], "0xF500")

        # Bow
        bow = vram_table["0x001E"]
        self.assertEqual(bow["name"], "Bow")
        self.assertEqual(bow["rom_art_address"], "0x00143A42")
        self.assertEqual(bow["vram_tile"], "0x0360")
        self.assertEqual(bow["dma_length_word"], "0xF500")

    def test_dma_queue_write_simulation(self) -> None:
        queue = MockVdpDmaQueue(base_addr=0x00FF18A0)

        # Simulate queuing for Dytto
        res_dytto = lookup_spirit_vram_and_dma(TYPE_DYTTO)
        self.assertIsNotNone(res_dytto)
        queue.queue_dma(res_dytto[1], res_dytto[2], res_dytto[3])
        self.assertEqual(queue.queue_ptr, 0x00FF18A8)
        self.assertEqual(queue.entries[-1], (0x007F97D4, 0xF500, 0x04D0))

        # Simulate queuing for Efreet
        res_efreet = lookup_spirit_vram_and_dma(TYPE_EFREET)
        self.assertIsNotNone(res_efreet)
        queue.queue_dma(res_efreet[1], res_efreet[2], res_efreet[3])
        self.assertEqual(queue.queue_ptr, 0x00FF18B0)
        self.assertEqual(queue.entries[-1], (0x007F97D4, 0xF500, 0x0450))

        # Invalid type returns None
        self.assertIsNone(lookup_spirit_vram_and_dma(0x0020))

    def test_overlay_state_reset_and_summon_dispatcher(self) -> None:
        ram = {
            "spirit_hold_counter": 45,
            "spirit_dtap_timer": 12,
            "spirit_prev_button_a": 0x00,
        }

        # Index 0 -> Dytto
        target = dispatch_spirit_summon(0, ram)
        self.assertEqual(target, "Dytto")
        self.assertEqual(ram["spirit_hold_counter"], 0)
        self.assertEqual(ram["spirit_dtap_timer"], 0)
        self.assertEqual(ram["spirit_prev_button_a"], 0xFF)

        # Index 1 -> Efreet
        ram["spirit_hold_counter"] = 99
        target = dispatch_spirit_summon(1, ram)
        self.assertEqual(target, "Efreet")
        self.assertEqual(ram["spirit_hold_counter"], 0)

        # Index 2 -> Shade
        target = dispatch_spirit_summon(2, ram)
        self.assertEqual(target, "Shade")

        # Index 3 -> Bow
        target = dispatch_spirit_summon(3, ram)
        self.assertEqual(target, "Bow")

        # Index >= 4 -> None (RTS at 0x014656)
        target = dispatch_spirit_summon(4, ram)
        self.assertIsNone(target)
        target = dispatch_spirit_summon(255, ram)
        self.assertIsNone(target)

    def test_dytto_spawn_behavior(self) -> None:
        entity = MockSpiritEntity(x=0x12340000, y=0x56780000, z=0x00010000, ground_z=0x0001)
        queue = MockVdpDmaQueue()
        ram = {"spawn_lock": 1}

        entity.spawn_dytto(queue, ram)

        self.assertEqual(entity.type_id, TYPE_DYTTO)
        self.assertEqual(entity.action_state, 2)
        self.assertEqual(entity.substate_2a, 4)
        self.assertEqual(entity.anim_16, 2)
        self.assertEqual(entity.tile_attr_18, 0x04F8)
        self.assertEqual(entity.max_sp_88, 100)
        self.assertEqual(entity.cur_sp_8a, 100)
        self.assertEqual(entity.drain_interval_8c, 10)
        self.assertEqual(entity.drain_timer_8e, 10)
        self.assertEqual(entity.pos_x, 0x12340000)
        self.assertEqual(entity.pos_y, 0x56780000)
        self.assertEqual(entity.pos_z, 0x00010000)
        self.assertTrue(entity.dma_queued)
        self.assertEqual(queue.entries[-1], (0x007F97D4, 0xF500, 0x04D0))
        self.assertEqual(ram["spawn_lock"], 0)

    def test_efreet_spawn_behavior_and_z_offset(self) -> None:
        orig_z = 0x00050000
        entity = MockSpiritEntity(x=0x11110000, y=0x22220000, z=orig_z, ground_z=0x0005)
        queue = MockVdpDmaQueue()
        ram = {"spawn_lock": 1}

        entity.spawn_efreet(queue, ram, entity_addr=0xFF1AA4)

        self.assertEqual(entity.type_id, TYPE_EFREET)
        self.assertEqual(entity.action_state, 4)
        self.assertEqual(entity.substate_2a, 4)
        self.assertEqual(entity.anim_16, 2)
        self.assertEqual(entity.pos_z, orig_z + 0x00080000)  # Elevated by 8.0 px fixed point!
        self.assertEqual(entity.parent_ptr, 0xFF1AA4)
        self.assertTrue(entity.dma_queued)
        self.assertEqual(queue.entries[-1], (0x007F97D4, 0xF500, 0x0450))
        self.assertEqual(ram["spawn_lock"], 0)

    def test_shade_spawn_behavior(self) -> None:
        entity = MockSpiritEntity()
        queue = MockVdpDmaQueue()
        ram = {"spawn_lock": 1}

        entity.spawn_shade(queue, ram)

        self.assertEqual(entity.type_id, TYPE_SHADE)
        self.assertEqual(entity.action_state, 2)
        self.assertEqual(entity.substate_2a, 0)
        self.assertEqual(entity.anim_16, 0)
        self.assertEqual(entity.tile_attr_18, 0x04F8)
        self.assertTrue(entity.dma_queued)
        self.assertEqual(queue.entries[-1], (0x007F97D4, 0xF500, 0x0280))
        self.assertEqual(ram["spawn_lock"], 0)

    def test_bow_spawn_behavior_and_sfx(self) -> None:
        entity = MockSpiritEntity()
        entity.status_flags_36 = 0x01
        queue = MockVdpDmaQueue()
        ram = {"spawn_lock": 1}

        entity.spawn_bow(queue, ram)

        self.assertEqual(entity.type_id, TYPE_BOW)
        self.assertEqual(entity.action_state, 0x0E)
        self.assertEqual(entity.substate_2a, 0x1E)
        self.assertEqual(entity.anim_16, 0)
        self.assertEqual(entity.tile_attr_18, 0x07A8)
        self.assertEqual(entity.status_flags_36 & 0x01, 0)
        self.assertEqual(entity.sfx_played, 0x0080039F)
        self.assertTrue(entity.bow_root_called)
        self.assertTrue(entity.dma_queued)
        self.assertEqual(queue.entries[-1], (0x007F97D4, 0xF500, 0x0360))
        self.assertEqual(ram["spawn_lock"], 0)


if __name__ == "__main__":
    unittest.main()
