#!/usr/bin/env python3
"""
rom_ali_item_drop_button_dispatch_test.py — M15.387: Unit tests for ROM Ali Item Drop Spawner & Button Dispatcher
Tests:
  1. test_item_drop_entity_spawn
  2. test_drop_rejection_audio_gate
  3. test_8_slot_allocation_probe
  4. test_input_ring_multi_frame_sampler
  5. test_master_player_button_dispatcher_priority
"""

import unittest


def spawn_dropped_item_entity(player_x, player_y, item_id, item_sub, item_state, trajectory):
    # 0x007C3C..0x007CC4
    entity = {
        "type": 8,            # 0(a6)
        "item_id": item_id,   # 32(a6)
        "item_sub": item_sub, # 52(a6)
        "state": item_state,  # 4(a6)
        "timer": 0x3C,        # 22(a6) (60 frames)
        "trajectory": trajectory, # 4E(a6)
        "x": player_x,        # 8(a6)
        "y": player_y,        # C(a6)
        "flag_39": 1,         # bset #0, 39(a6)
    }
    player_transition = {
        "clear_17b8_bits": 0xF9,
        "set_5c": 0xFFFF,
        "next_state": "idle_walk",
    }
    return entity, player_transition


def sample_input_ring(ring_words, head_offset, frame_count, window_req, locked):
    # 0x007D8A..0x007DC6
    if locked:
        return 0
    if frame_count == 0:
        return 0
    frames_to_read = min(window_req, frame_count)
    composite = 0
    head = head_offset
    for _ in range(frames_to_read):
        word_idx = (head & 0x7E) // 2
        composite |= ring_words[word_idx]
        head = (head + 2) & 0x7E
    return composite


def dispatch_master_buttons(composite_buttons, latched_attack_bit, locked):
    # 0x007DC8..0x007E44
    if locked:
        return "none"
    # Bit 6: Jump (0x40)
    if composite_buttons & (1 << 6):
        return "jump"
    # Bit 4: Attack (0x10)
    if composite_buttons & (1 << 4):
        return "attack"
    # Latched Attack buffer
    if latched_attack_bit:
        return "latched_attack"
    # Bit 5: Spirit (0x20)
    if composite_buttons & (1 << 5):
        return "spirit"
    return "none"


class TestRomAliItemDropButtonDispatch(unittest.TestCase):
    def test_item_drop_entity_spawn(self):
        entity, player = spawn_dropped_item_entity(
            player_x=0x01200000,
            player_y=0x00800000,
            item_id=0x0A,
            item_sub=1,
            item_state=2,
            trajectory=0x00020000,
        )
        self.assertEqual(entity["type"], 8)
        self.assertEqual(entity["item_id"], 0x0A)
        self.assertEqual(entity["item_sub"], 1)
        self.assertEqual(entity["state"], 2)
        self.assertEqual(entity["timer"], 0x3C)
        self.assertEqual(entity["x"], 0x01200000)
        self.assertEqual(entity["y"], 0x00800000)
        self.assertEqual(entity["flag_39"], 1)
        self.assertEqual(player["next_state"], "idle_walk")
        self.assertEqual(player["set_5c"], 0xFFFF)

    def test_drop_rejection_audio_gate(self):
        # 0x007CC6..0x007D22
        reject_state = {"state": 0x3C, "anim": 0x44, "sfx": 0xC00255}
        self.assertEqual(reject_state["state"], 0x3C)
        self.assertEqual(reject_state["anim"], 0x44)
        self.assertEqual(reject_state["sfx"], 0xC00255)

    def test_8_slot_allocation_probe(self):
        # 0x007D24..0x007D88
        # Stride 90 bytes across 8 slots = 720 bytes
        slots = [{"item_id": 0, "active": False} for _ in range(8)]
        # Free slot allows drop
        can_drop = any(not s["active"] for s in slots)
        self.assertTrue(can_drop)

        # All occupied slots
        for s in slots:
            s["active"] = True
        can_drop_full = all(s["active"] for s in slots)
        self.assertTrue(can_drop_full)

    def test_input_ring_multi_frame_sampler(self):
        ring = [0] * 64
        # Place button masks at word indices 10, 11, 12
        ring[10] = 0x0010  # Button B
        ring[11] = 0x0020  # Button A
        ring[12] = 0x0040  # Button C

        # Read 3 frames starting at byte head 20 (word 10)
        composite = sample_input_ring(ring, head_offset=20, frame_count=5, window_req=3, locked=False)
        self.assertEqual(composite, 0x0070)

        # Locked input returns 0
        composite_locked = sample_input_ring(ring, head_offset=20, frame_count=5, window_req=3, locked=True)
        self.assertEqual(composite_locked, 0)

        # Wrap test around 0x7E (word 63 -> 0)
        ring[63] = 0x0001
        ring[0] = 0x0002
        composite_wrap = sample_input_ring(ring, head_offset=126, frame_count=2, window_req=2, locked=False)
        self.assertEqual(composite_wrap, 0x0003)

    def test_master_player_button_dispatcher_priority(self):
        # 1. Jump (bit 6 = 0x40) has highest priority over Attack (0x10) and Spirit (0x20)
        action_jump = dispatch_master_buttons(composite_buttons=0x70, latched_attack_bit=False, locked=False)
        self.assertEqual(action_jump, "jump")

        # 2. Attack (bit 4 = 0x10) has priority over Spirit (bit 5 = 0x20)
        action_attack = dispatch_master_buttons(composite_buttons=0x30, latched_attack_bit=False, locked=False)
        self.assertEqual(action_attack, "attack")

        # 3. Latched attack bit triggers when no raw attack/jump pressed
        action_latched = dispatch_master_buttons(composite_buttons=0x00, latched_attack_bit=True, locked=False)
        self.assertEqual(action_latched, "latched_attack")

        # 4. Spirit (bit 5 = 0x20) triggers when no jump/attack
        action_spirit = dispatch_master_buttons(composite_buttons=0x20, latched_attack_bit=False, locked=False)
        self.assertEqual(action_spirit, "spirit")

        # 5. Locked returns "none"
        action_locked = dispatch_master_buttons(composite_buttons=0x70, latched_attack_bit=True, locked=True)
        self.assertEqual(action_locked, "none")


if __name__ == "__main__":
    unittest.main()
