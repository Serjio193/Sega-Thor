#!/usr/bin/env python3
"""
rom_ali_item_pickup_inventory_test.py — M15.386: Unit tests for ROM Ali Item Pickup Dispatcher & Inventory
Tests:
  1. test_weapon_inventory_storage_and_full_bag_reject
  2. test_food_inventory_storage_and_full_bag_reject
  3. test_spirit_jewel_level_and_sp_boost
  4. test_gem_key_and_rank_upgrade
  5. test_chest_collection_bit_recorder
  6. test_item_classification_sfx
"""

import unittest


def pickup_item(item_id, weapon_slots, food_slots, gem_counts, spirit_levels,
                keys_bitmask, rank_info, sp_capacity):
    """
    Simulates Beyond Oasis ROM routine 0x007A28..0x007C3C.
    """
    if item_id <= 0x0C:
        # Weapon item: search 8 weapon slots
        stored = False
        for i in range(len(weapon_slots)):
            if weapon_slots[i] == 0:
                weapon_slots[i] = item_id
                stored = True
                break
        if not stored:
            # Full bag reject 0x007C20
            return {"status": "full_bag_reject", "anim": 0x44, "state": 0x3C, "sfx": 0xC00255}
        return {"status": "stored_weapon"}

    elif item_id <= 0x10:
        # Gem counter 0x007BD4
        idx = item_id - 0x0D
        gem_counts[idx] = gem_counts.get(idx, 0) + 1
        return {"status": "gem_incremented", "index": idx, "count": gem_counts[idx]}

    elif item_id == 0x11:
        # Rank upgrade 0x007BF6
        level = rank_info["level"]
        if level < 200:
            level += 1
            rank_info["level"] = level
            rank_info["max_hp"] = level * 2 + 0xC6
            rank_info["max_sp"] += 2
        return {"status": "rank_upgraded", "rank": rank_info}

    elif item_id <= 0x15:
        # Spirit jewel 0x007BA4
        idx = item_id - 0x12
        if spirit_levels[idx] < 15:
            spirit_levels[idx] += 1
        if sp_capacity["active"]:
            sp_capacity["current"] += 5
            sp_capacity["max"] += 5
        return {"status": "spirit_jewel_upgraded", "index": idx, "level": spirit_levels[idx]}

    elif item_id <= 0x19:
        # Magic key bitmask 0x007BE8
        bit = item_id - 0x16
        keys_bitmask[0] |= (1 << bit)
        return {"status": "key_collected", "bit": bit}

    elif item_id <= 0x3F:
        # Food/consumable item: search 16 food slots 0x007B84
        stored = False
        for i in range(len(food_slots)):
            if food_slots[i] == 0:
                food_slots[i] = item_id
                stored = True
                break
        if not stored:
            # Full bag reject 0x007C20
            return {"status": "full_bag_reject", "anim": 0x44, "state": 0x3C, "sfx": 0xC00255}
        return {"status": "stored_food"}

    return {"status": "direct_consume"}


def record_chest_collection(region_table, chest_id):
    # 0x007AF0..0x007B28
    byte_offset = chest_id >> 3
    bit_offset = chest_id & 7
    while len(region_table) <= byte_offset:
        region_table.append(0)
    region_table[byte_offset] |= (1 << bit_offset)


def classify_item_sfx(item_type):
    # 0x007A6C..0x007AF0
    if item_type == 3:
        return {"sfx": 0x508, "state": 0x44, "anim": 0x4A}
    elif item_type == 4:
        return {"sfx": 0x50D, "state": 0x44, "anim": 0x4A}
    elif item_type == 0:
        return {"sfx": 0xC0035F}
    elif item_type == 1:
        return {"sfx": 0xC00360}
    elif item_type == 2:
        return {"sfx": 0xC00361}
    return {}


class TestRomAliItemPickupInventory(unittest.TestCase):
    def test_weapon_inventory_storage_and_full_bag_reject(self):
        weapons = [1, 2, 0, 0, 0, 0, 0, 0]
        res = pickup_item(5, weapons, [], {}, {}, [0], {}, {"active": False})
        self.assertEqual(res["status"], "stored_weapon")
        self.assertEqual(weapons[2], 5)

        full_weapons = [1, 2, 3, 4, 5, 6, 7, 8]
        res_full = pickup_item(9, full_weapons, [], {}, {}, [0], {}, {"active": False})
        self.assertEqual(res_full["status"], "full_bag_reject")
        self.assertEqual(res_full["anim"], 0x44)
        self.assertEqual(res_full["state"], 0x3C)
        self.assertEqual(res_full["sfx"], 0xC00255)

    def test_food_inventory_storage_and_full_bag_reject(self):
        foods = [0x1A] * 15 + [0]
        res = pickup_item(0x20, [], foods, {}, {}, [0], {}, {"active": False})
        self.assertEqual(res["status"], "stored_food")
        self.assertEqual(foods[15], 0x20)

        full_foods = [0x1A] * 16
        res_full = pickup_item(0x20, [], full_foods, {}, {}, [0], {}, {"active": False})
        self.assertEqual(res_full["status"], "full_bag_reject")
        self.assertEqual(res_full["sfx"], 0xC00255)

    def test_spirit_jewel_level_and_sp_boost(self):
        levels = [0, 0, 0, 0]
        sp = {"active": True, "current": 20, "max": 20}
        res = pickup_item(0x13, [], [], {}, levels, [0], {}, sp)
        self.assertEqual(res["status"], "spirit_jewel_upgraded")
        self.assertEqual(levels[1], 1)
        self.assertEqual(sp["current"], 25)
        self.assertEqual(sp["max"], 25)

        # Cap check at 15
        levels[1] = 15
        pickup_item(0x13, [], [], {}, levels, [0], {}, sp)
        self.assertEqual(levels[1], 15)

    def test_gem_key_and_rank_upgrade(self):
        gems = {}
        pickup_item(0x0E, [], [], gems, {}, [0], {}, {"active": False})
        self.assertEqual(gems[1], 1)

        keys = [0]
        pickup_item(0x18, [], [], {}, {}, keys, {}, {"active": False})
        self.assertEqual(keys[0], 1 << 2)

        rank = {"level": 1, "max_hp": 200, "max_sp": 50}
        pickup_item(0x11, [], [], {}, {}, [0], rank, {"active": False})
        self.assertEqual(rank["level"], 2)
        self.assertEqual(rank["max_hp"], 2 * 2 + 0xC6)  # 4 + 198 = 202
        self.assertEqual(rank["max_sp"], 52)

    def test_chest_collection_bit_recorder(self):
        table = []
        record_chest_collection(table, 13)  # byte 1, bit 5
        self.assertEqual(table[1], 1 << 5)

        record_chest_collection(table, 2)  # byte 0, bit 2
        self.assertEqual(table[0], 1 << 2)

    def test_item_classification_sfx(self):
        self.assertEqual(classify_item_sfx(3), {"sfx": 0x508, "state": 0x44, "anim": 0x4A})
        self.assertEqual(classify_item_sfx(4), {"sfx": 0x50D, "state": 0x44, "anim": 0x4A})
        self.assertEqual(classify_item_sfx(0), {"sfx": 0xC0035F})
        self.assertEqual(classify_item_sfx(1), {"sfx": 0xC00360})
        self.assertEqual(classify_item_sfx(2), {"sfx": 0xC00361})


if __name__ == "__main__":
    unittest.main()
