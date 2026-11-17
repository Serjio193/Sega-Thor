#!/usr/bin/env python3
"""
rom_ali_input_seq_durability_test.py — M15.391: Unit tests for ROM Ali Motion Sequence Matcher & Weapon Durability
Tests:
  1. test_command_motion_sequence_matcher
  2. test_shockwave_projectile_spawner
  3. test_weapon_durability_decrement_and_break
  4. test_exp_bonus_trigger
"""

import unittest


def match_motion_sequence(ring_words, head_offset, frame_count, mask, target_val):
    # 0x0083F8..0x00846A: 3-stage recognizer (Press -> Release -> Re-press)
    if frame_count == 0:
        return False
    frames_to_check = min(12, frame_count)
    stage = 0  # 0: waiting for first press, 1: waiting for neutral release, 2: waiting for re-press
    head = head_offset
    for _ in range(frames_to_check):
        word_idx = (head & 0x7E) // 2
        w = ring_words[word_idx]
        masked = w & mask
        if stage == 0:
            if masked == target_val:
                stage = 1
        elif stage == 1:
            if masked == 0:
                stage = 2
        elif stage == 2:
            if masked == target_val:
                return True
        head = (head + 2) & 0x7E
    return False


def spawn_shockwave_entity(player_x, player_y, facing_dir):
    # 0x00846C..0x0084FA
    offsets_table = {
        0: 0xF800,
        1: 0x0B00,
        2: 0x0817,
        3: 0xF500,
    }
    offset_word = offsets_table.get(facing_dir % 4, 0)
    d1 = (offset_word >> 8) & 0xFF
    d2 = offset_word & 0xFF
    # Direction projection 0xE2D4 with base 0x800
    table_8way = {
        0: (0, -0x800),
        1: (0x5A8, -0x5A8),
        2: (0x800, 0),
        3: (0x5A8, 0x5A8),
        4: (0, 0x800),
        5: (-0x5A8, 0x5A8),
        6: (-0x800, 0),
        7: (-0x5A8, -0x5A8),
    }
    vx, vy = table_8way.get(facing_dir % 8, (0, 0))
    return {
        "type": 0x16,
        "x": player_x,
        "y": player_y,
        "elevation": 19,
        "facing": facing_dir,
        "offset_word": offset_word,
        "vx": vx,
        "vy": vy,
    }


def step_weapon_durability(slot_idx, inventory_slots):
    # 0x008530..0x0085C2
    item = inventory_slots[slot_idx]
    item_id = item["id"]
    durability = item["durability"]

    if item_id < 2 or item_id > 12:
        return {"action": "ignored"}
    if durability == 0xFF:
        return {"action": "infinite_durability"}

    new_durability = durability - 1
    if new_durability > 0:
        item["durability"] = new_durability
        return {"action": "durability_decremented", "durability": new_durability}
    else:
        # Weapon breaks!
        inventory_slots[slot_idx] = {"id": 0, "durability": 0}
        return {
            "action": "weapon_broken",
            "active_slot": 0,
            "revert_weapon_id": 1,
            "revert_weapon_type": 0x0A,
            "hud_display_id": 1,
            "hud_toast_timer": 0x5A,
            "swap_flag": True,
            "sfx": 0xC00269,
        }


def check_exp_bonus(exp, threshold):
    # 0x0085C4..0x0085E2
    if exp >= threshold:
        return {"bonus_triggered": True, "exp": 0, "bonus_flag": 0xFF}
    return {"bonus_triggered": False, "exp": exp, "bonus_flag": 0}


class TestRomAliInputSeqDurability(unittest.TestCase):
    def test_command_motion_sequence_matcher(self):
        # 1. Successful Double-Tap: Press (Right: 0x08) -> Neutral (0) -> Re-press (Right: 0x08)
        ring = [0] * 64
        ring[0] = 0x08
        ring[1] = 0x00
        ring[2] = 0x08
        matched = match_motion_sequence(ring, head_offset=0, frame_count=5, mask=0x08, target_val=0x08)
        self.assertTrue(matched)

        # 2. Continuous hold (no neutral release) -> False
        ring_hold = [0] * 64
        ring_hold[0] = 0x08
        ring_hold[1] = 0x08
        ring_hold[2] = 0x08
        matched_hold = match_motion_sequence(ring_hold, head_offset=0, frame_count=5, mask=0x08, target_val=0x08)
        self.assertFalse(matched_hold)

        # 3. Single tap without re-press -> False
        ring_single = [0] * 64
        ring_single[0] = 0x08
        ring_single[1] = 0x00
        ring_single[2] = 0x00
        matched_single = match_motion_sequence(ring_single, head_offset=0, frame_count=5, mask=0x08, target_val=0x08)
        self.assertFalse(matched_single)

    def test_shockwave_projectile_spawner(self):
        proj = spawn_shockwave_entity(player_x=100, player_y=200, facing_dir=2)
        self.assertEqual(proj["type"], 0x16)
        self.assertEqual(proj["elevation"], 19)
        self.assertEqual(proj["offset_word"], 0x0817)
        self.assertEqual(proj["vx"], 0x800)
        self.assertEqual(proj["vy"], 0)

    def test_weapon_durability_decrement_and_break(self):
        slots = [
            {"id": 2, "durability": 5},   # Sword with 5 durability
            {"id": 6, "durability": 0xFF}, # Infinite Bow
            {"id": 3, "durability": 1},   # Sword about to break
        ]

        # 1. Decrement normal sword
        res1 = step_weapon_durability(0, slots)
        self.assertEqual(res1["action"], "durability_decremented")
        self.assertEqual(res1["durability"], 4)

        # 2. Infinite ammo
        res2 = step_weapon_durability(1, slots)
        self.assertEqual(res2["action"], "infinite_durability")

        # 3. Weapon breaks
        res3 = step_weapon_durability(2, slots)
        self.assertEqual(res3["action"], "weapon_broken")
        self.assertEqual(res3["revert_weapon_id"], 1)
        self.assertEqual(res3["revert_weapon_type"], 0x0A)
        self.assertEqual(res3["hud_toast_timer"], 0x5A)
        self.assertEqual(res3["sfx"], 0xC00269)
        self.assertEqual(slots[2]["id"], 0)

    def test_exp_bonus_trigger(self):
        # Threshold met
        res_met = check_exp_bonus(exp=200, threshold=200)
        self.assertTrue(res_met["bonus_triggered"])
        self.assertEqual(res_met["exp"], 0)
        self.assertEqual(res_met["bonus_flag"], 0xFF)

        # Threshold not met
        res_not = check_exp_bonus(exp=150, threshold=200)
        self.assertFalse(res_not["bonus_triggered"])
        self.assertEqual(res_not["exp"], 150)


if __name__ == "__main__":
    unittest.main()
