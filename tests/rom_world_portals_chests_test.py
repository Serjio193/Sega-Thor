#!/usr/bin/env python3
"""
Unit tests for ROM World Interactive Objects: Room Doors, Vortex Portals, Spirit Locks & Treasure Chests
(0x017A96..0x017E28).
Milestone M15.524.
"""

from __future__ import annotations

import json
from pathlib import Path
import unittest

ROOT_DIR = Path(__file__).resolve().parent.parent
EVIDENCE_PATH = (
    ROOT_DIR
    / "data"
    / "thor-evidence"
    / "rom_world_portals_chests_m15_524.json"
)


# ==============================================================================
# Emulation Routines
# ==============================================================================

def step_room_door_portal(
    door: dict,
    ali: dict,
    in_aabb: bool,
    system_vars: dict,
    sfx_log: list[int],
    transition_log: list[str],
) -> None:
    """
    Emulates 0x017A96..0x017CA4 Room Door / Portal & Warp Trigger.
    """
    entity_type = door.get("type_0", 0)

    # Standard Doorway (type != 0x7C)
    if entity_type != 0x7C:
        if not in_aabb:
            return

        target_room = door.get("target_room_4e", 0)
        curr_room = system_vars.get("current_room_ff0e40", 0)

        # Cross-Room Transition
        if target_room != curr_room:
            if ali.get("type_0", 0) != 2:
                return
            if system_vars.get("cutscene_lock_ff1985", 0) != 0:
                return
            if ali.get("state_4", 0) == 0x5A:
                return

            # Special room 0x90B event
            if curr_room == 0x90B:
                dc6 = system_vars.get("room_flag_ff0dc6", 0)
                if (dc6 & 1) == 0:
                    transition_log.append("0x00C2EC(d5=0, d6=0x5000, d7=0x5800, d0=0x18)")
                    system_vars["room_flag_ff0dc6"] = dc6 | 1

            sfx_log.append(0x800230)
            system_vars["transition_flag_ff17bb"] = 0xFF
            system_vars["transition_flag_ff17bf"] = 0xFF
            transition_log.append("0x00CA24")
            return
        else:
            # Same-Room Teleporter
            if ali.get("type_0", 0) != 2:
                return
            if system_vars.get("cutscene_lock_ff1985", 0) != 0:
                return
            if ali.get("state_4", 0) == 0x5A:
                return

            ali["x"] = door.get("teleport_x_52", 0)
            ali["y"] = door.get("teleport_y_54", 0)
            transition_log.append("0x009D00")
            sfx_log.append(0x800230)
            return

    # Vortex Portal (type == 0x7C)
    else:
        # Check submode / flags
        flag_4c = door.get("flag_4c", 0)
        sub_d0 = flag_4c & 0x7F
        if sub_d0 == 1:
            door["flag_4c"] = 0
            door["anim_2a"] = 2
        elif sub_d0 == 2:
            door["flag_4c"] = 0x80

        if door.get("anim_2a", 0) == 0:
            return

        if system_vars.get("cutscene_lock_ff1985", 0) != 0 or system_vars.get("dialogue_lock_ff1987", 0) != 0:
            return

        excluded_states = {0x0E, 0x28, 0x2A, 0x2C, 0x2E, 0x6C, 0x5A, 0x32, 0x34, 0x3E, 0x58}
        if ali.get("state_4", 0) in excluded_states:
            return

        dx = door.get("x", 0) - ali.get("x", 0)
        dy = door.get("y", 0) - ali.get("y", 0)
        dist = max(abs(dx), abs(dy))

        if dist <= 1:
            # Absorption Warp
            if ali.get("state_4", 0) == 0x6E or ali.get("z_10", 0) != 0:
                return
            sfx_log.append(0x800230)
            ali["target_room_ae"] = door.get("target_room_4e", 0)
            ali["target_x_b0"] = door.get("teleport_x_52", 0)
            ali["target_y_b2"] = door.get("teleport_y_54", 0)
            ali["anim_2a"] = 0x78
            ali["state_4"] = 0x6E
            ali["facing_16"] = 0
            return
        elif dist <= 56:
            # Suction pull
            pull_x = dx
            pull_y = dy
            if dist > 24:
                pull_x >>= 1
                pull_y >>= 1
            if dist > 40:
                pull_x >>= 1
                pull_y >>= 1
            ali["delta_x_72"] = ali.get("delta_x_72", 0) + pull_x
            ali["delta_y_76"] = ali.get("delta_y_76", 0) + pull_y


def step_spirit_lock_persistent_despawner(
    lock: dict,
    sparkle_contact: bool,
    sparkle_type: int,
    system_vars: dict,
    sfx_log: list[int],
    callback_log: list[str],
) -> None:
    """
    Emulates 0x017CA6..0x017D10 Spirit/Key Lock Trigger & Persistent Despawner.
    """
    # Visibility check via bit 7 of +$4C
    if (lock.get("flag_4c", 0) & 0x80) != 0:
        lock["flag_36"] = lock.get("flag_36", 0) | (1 << 1)
        if system_vars.get("cutscene_lock_ff1985", 0) != 0:
            lock["flag_36"] = lock.get("flag_36", 0) & ~(1 << 1)

    # Spirit Sparkle Contact ($FF0DBA:1)
    if (system_vars.get("spirit_active_ff0dba", 0) & 2) != 0:
        if sparkle_contact and sparkle_type == 0x16:
            lock["flag_36"] = lock.get("flag_36", 0) & ~(1 << 1)
            lock["anim_2a"] = 2
            sfx_log.append(0x80022F)


def despawn_persistent_object(
    entity: dict,
    callback_log: list[str],
) -> None:
    """
    Emulates 0x017D06..0x017D10 Persistent Despawner.
    """
    entity["type_0"] = 0
    callback_log.append("0x017FF2")


def step_treasure_chest(
    chest: dict,
    sfx_log: list[int],
    callback_log: list[str],
    spawn_loot_log: list[dict],
) -> None:
    """
    Emulates 0x017D12..0x017E26 Treasure Chest.
    """
    submode = chest.get("submode_66", 0)

    # Init
    if submode == 0:
        chest["submode_66"] = 0xFF
        z = chest.get("param_a0", 0)
        auto_open = (z & 0x80) != 0
        z &= 0x7F
        chest["z_10"] = z

        if auto_open:
            chest["state_4"] = 0x0A
            chest["anim_2a"] = chest.get("anim_2a", 0) + 2
            chest["timer_2"] = 1
        return

    # Ballistic descent
    z = chest.get("z_10", 0)
    if z != 0:
        vz = chest.get("vz_56", 0) - 0xC000
        chest["vz_56"] = vz
        z += vz
        chest["z_10"] = z

        if z < 0:
            chest["z_10"] = 0
            if chest.get("bounced_a6", 0) == 0:
                chest["bounced_a6"] = 0xFF
                chest["z_10"] = 1
                chest["vz_56"] = 0x00030000
                sfx_log.append(0x400376)
            else:
                chest["vz_56"] = 0

    state = chest.get("state_4", 0)

    # Auto-open sequence (state 0x0A)
    if state == 0x0A:
        if chest.get("param_9e", 0) != 0:
            callback_log.append("0x017FF2")

        loot_id = chest.get("param_8c", 0)
        if loot_id != 0:
            loot_item = {"item_id": loot_id, "state_4": 0}
            if chest.get("param_9e", 0) == 0:
                loot_item["param_4e"] = chest.get("param_a2", 0)
                loot_item["param_50"] = chest.get("param_a4", 0)
            spawn_loot_log.append(loot_item)

        chest["type_0"] = 0
        return

    # Hit-to-open (state 0)
    if state == 0:
        if chest.get("damage_86", 0) != 0:
            chest["state_4"] = 1
            chest["timer_2"] = 2
            chest["anim_2a"] = chest.get("anim_2a", 0) + 2
            sfx_log.append(0x900232)
        return

    # Opening countdown (state > 0)
    if state != 0:
        timer = chest.get("timer_2", 0) - 1
        chest["timer_2"] = timer
        if timer == 0:
            if chest.get("param_9e", 0) != 0:
                callback_log.append("0x017FF2")

            loot_id = chest.get("param_8c", 0)
            if loot_id != 0:
                loot_item = {"item_id": loot_id}
                if chest.get("param_9e", 0) == 0:
                    loot_item["param_4e"] = chest.get("param_a2", 0)
                    loot_item["param_50"] = chest.get("param_a4", 0)
                spawn_loot_log.append(loot_item)

            chest["type_0"] = 0


# ==============================================================================
# Unit Tests
# ==============================================================================

class TestROMWorldPortalsChests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure_and_counts(self):
        meta = self.evidence["metadata"]
        self.assertEqual(meta["milestone"], "M15.524")
        self.assertEqual(meta["range_start"], "0x017A96")
        self.assertEqual(meta["range_end"], "0x017E28")
        self.assertEqual(meta["range_size_bytes"], 914)
        self.assertEqual(meta["instruction_count"], 225)
        self.assertEqual(meta["routine_count"], 3)

        routines = {r["name"]: r for r in self.evidence["routines"]}
        self.assertIn("AI_World_RoomDoorPortalWarpTrigger", routines)
        self.assertIn("AI_World_SpiritLockPersistentDespawner", routines)
        self.assertIn("AI_World_TreasureChest", routines)

        self.assertEqual(routines["AI_World_RoomDoorPortalWarpTrigger"]["instruction_count"], 127)
        self.assertEqual(routines["AI_World_RoomDoorPortalWarpTrigger"]["size_bytes"], 528)
        self.assertEqual(routines["AI_World_SpiritLockPersistentDespawner"]["instruction_count"], 28)
        self.assertEqual(routines["AI_World_SpiritLockPersistentDespawner"]["size_bytes"], 108)
        self.assertEqual(routines["AI_World_TreasureChest"]["instruction_count"], 70)
        self.assertEqual(routines["AI_World_TreasureChest"]["size_bytes"], 278)

    # --------------------------------------------------------------------------
    # Room Door & Portal Tests
    # --------------------------------------------------------------------------

    def test_door_same_room_teleport(self):
        door = {"type_0": 0x40, "target_room_4e": 0x105, "teleport_x_52": 320, "teleport_y_54": 480}
        ali = {"type_0": 2, "state_4": 0, "x": 100, "y": 100}
        sys_vars = {"current_room_ff0e40": 0x105, "cutscene_lock_ff1985": 0}
        sfx, trans = [], []

        step_room_door_portal(door, ali, in_aabb=True, system_vars=sys_vars, sfx_log=sfx, transition_log=trans)
        self.assertEqual(ali["x"], 320)
        self.assertEqual(ali["y"], 480)
        self.assertEqual(sfx, [0x800230])
        self.assertEqual(trans, ["0x009D00"])

    def test_door_cross_room_transition(self):
        door = {"type_0": 0x40, "target_room_4e": 0x106}
        ali = {"type_0": 2, "state_4": 0}
        sys_vars = {"current_room_ff0e40": 0x105, "cutscene_lock_ff1985": 0}
        sfx, trans = [], []

        step_room_door_portal(door, ali, in_aabb=True, system_vars=sys_vars, sfx_log=sfx, transition_log=trans)
        self.assertEqual(sys_vars["transition_flag_ff17bb"], 0xFF)
        self.assertEqual(sys_vars["transition_flag_ff17bf"], 0xFF)
        self.assertEqual(sfx, [0x800230])
        self.assertEqual(trans, ["0x00CA24"])

    def test_door_special_room_0x90b_event(self):
        door = {"type_0": 0x40, "target_room_4e": 0x200}
        ali = {"type_0": 2, "state_4": 0}
        sys_vars = {"current_room_ff0e40": 0x90B, "room_flag_ff0dc6": 0}
        sfx, trans = [], []

        step_room_door_portal(door, ali, in_aabb=True, system_vars=sys_vars, sfx_log=sfx, transition_log=trans)
        self.assertEqual(sys_vars["room_flag_ff0dc6"], 1)
        self.assertIn("0x00C2EC(d5=0, d6=0x5000, d7=0x5800, d0=0x18)", trans)
        self.assertIn("0x00CA24", trans)

    def test_vortex_portal_state_exclusion(self):
        door = {"type_0": 0x7C, "anim_2a": 1, "x": 200, "y": 200}
        ali_excluded = {"state_4": 0x5A, "x": 210, "y": 210}  # Damage flinch state
        sys_vars = {}
        sfx, trans = [], []

        step_room_door_portal(door, ali_excluded, in_aabb=False, system_vars=sys_vars, sfx_log=sfx, transition_log=trans)
        self.assertEqual(sfx, [])
        self.assertNotIn("delta_x_72", ali_excluded)

    def test_vortex_portal_suction_distance_stages(self):
        door = {"type_0": 0x7C, "anim_2a": 1, "x": 200, "y": 200}
        # In half-speed suction range (dist = 16)
        ali = {"state_4": 0, "x": 184, "y": 200}
        step_room_door_portal(door, ali, in_aabb=False, system_vars={}, sfx_log=[], transition_log=[])
        self.assertEqual(ali["delta_x_72"], 16)  # dx = 16

    def test_vortex_portal_absorption_warp(self):
        door = {"type_0": 0x7C, "anim_2a": 1, "x": 200, "y": 200, "target_room_4e": 0x500, "teleport_x_52": 160, "teleport_y_54": 120}
        ali = {"state_4": 0, "x": 200, "y": 201, "z_10": 0}
        sfx, trans = [], []

        step_room_door_portal(door, ali, in_aabb=False, system_vars={}, sfx_log=sfx, transition_log=trans)
        self.assertEqual(ali["state_4"], 0x6E)
        self.assertEqual(ali["anim_2a"], 0x78)
        self.assertEqual(ali["target_room_ae"], 0x500)
        self.assertEqual(ali["target_x_b0"], 160)
        self.assertEqual(ali["target_y_b2"], 120)
        self.assertEqual(sfx, [0x800230])

    # --------------------------------------------------------------------------
    # Spirit Lock & Persistent Despawner Tests
    # --------------------------------------------------------------------------

    def test_spirit_lock_visibility_toggle(self):
        lock = {"flag_4c": 0x80, "flag_36": 0}
        step_spirit_lock_persistent_despawner(lock, False, 0, {"cutscene_lock_ff1985": 0}, [], [])
        self.assertTrue(lock["flag_36"] & 2)

        # Cutscene hides it
        step_spirit_lock_persistent_despawner(lock, False, 0, {"cutscene_lock_ff1985": 1}, [], [])
        self.assertEqual(lock["flag_36"] & 2, 0)

    def test_spirit_lock_sparkle_contact_unlock(self):
        lock = {"flag_36": 2, "anim_2a": 0}
        sfx = []
        sys_vars = {"spirit_active_ff0dba": 2}
        step_spirit_lock_persistent_despawner(lock, sparkle_contact=True, sparkle_type=0x16, system_vars=sys_vars, sfx_log=sfx, callback_log=[])
        self.assertEqual(lock["flag_36"] & 2, 0)
        self.assertEqual(lock["anim_2a"], 2)
        self.assertEqual(sfx, [0x80022F])

    def test_persistent_despawner_callback(self):
        obj = {"type_0": 0x30}
        cb = []
        despawn_persistent_object(obj, cb)
        self.assertEqual(obj["type_0"], 0)
        self.assertEqual(cb, ["0x017FF2"])

    # --------------------------------------------------------------------------
    # Treasure Chest Tests
    # --------------------------------------------------------------------------

    def test_treasure_chest_init_ground_vs_auto_open(self):
        # Standard closed chest
        c1 = {"submode_66": 0, "param_a0": 30}
        step_treasure_chest(c1, [], [], [])
        self.assertEqual(c1["submode_66"], 0xFF)
        self.assertEqual(c1["z_10"], 30)
        self.assertNotIn("state_4", c1)

        # Auto-open on spawn (bit 7 set: 0x80 | 10)
        c2 = {"submode_66": 0, "param_a0": 0x8A}
        step_treasure_chest(c2, [], [], [])
        self.assertEqual(c2["z_10"], 10)
        self.assertEqual(c2["state_4"], 0x0A)
        self.assertEqual(c2["timer_2"], 1)

    def test_treasure_chest_ballistic_descent_and_bounce(self):
        chest = {"submode_66": 0xFF, "z_10": 0x2000, "vz_56": -0x5000, "bounced_a6": 0}
        sfx = []
        step_treasure_chest(chest, sfx_log=sfx, callback_log=[], spawn_loot_log=[])
        self.assertEqual(chest["bounced_a6"], 0xFF)
        self.assertEqual(chest["z_10"], 1)
        self.assertEqual(chest["vz_56"], 0x00030000)
        self.assertEqual(sfx, [0x400376])

    def test_treasure_chest_hit_to_open(self):
        chest = {"submode_66": 0xFF, "z_10": 0, "state_4": 0, "damage_86": 0x200F, "anim_2a": 0}
        sfx = []
        step_treasure_chest(chest, sfx_log=sfx, callback_log=[], spawn_loot_log=[])
        self.assertEqual(chest["state_4"], 1)
        self.assertEqual(chest["timer_2"], 2)
        self.assertEqual(chest["anim_2a"], 2)
        self.assertEqual(sfx, [0x900232])

    def test_treasure_chest_loot_spawn_and_despawn(self):
        chest = {
            "submode_66": 0xFF,
            "z_10": 0,
            "state_4": 1,
            "timer_2": 1,
            "param_9e": 0,
            "param_8c": 0x12,  # Loot ID
            "param_a2": 5,
            "param_a4": 10,
        }
        cb, loot = [], []
        step_treasure_chest(chest, sfx_log=[], callback_log=cb, spawn_loot_log=loot)
        self.assertEqual(chest["type_0"], 0)
        self.assertEqual(len(loot), 1)
        self.assertEqual(loot[0]["item_id"], 0x12)
        self.assertEqual(loot[0]["param_4e"], 5)
        self.assertEqual(loot[0]["param_50"], 10)


if __name__ == "__main__":
    unittest.main()
