#!/usr/bin/env python3
"""
Unit tests for ROM World Interactive Objects: Spirit Crystal Orb Switch, Breakable Wall Block
& Floor Spike/Saw Hazard (0x016CBE..0x016EBC).
Milestone M15.519.
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
    / "rom_world_crystal_wall_spike_m15_519.json"
)


# ==============================================================================
# Emulation Routines
# ==============================================================================

def step_crystal_orb_switch(
    entity: dict,
    persistent_flags: dict[int, bool],
    sfx_log: list[int],
) -> None:
    """
    Emulates 0x016CBE..0x016D5A Spirit Crystal Orb Switch.
    """
    flag_id = entity.get("persistent_id", 0)
    is_set = persistent_flags.get(flag_id, False)
    submode = entity.get("submode_66", 0)

    if submode == 0:
        entity["submode_66"] = 1
        entity["z"] = entity.get("param_8c", 0)
        if is_set:
            entity["flag_38"] = entity.get("flag_38", 0) | (1 << 3)
            entity["anim"] = 2
            entity["flag_3c"] = entity.get("flag_3c", 0) | (1 << 1)
        else:
            entity["anim"] = 0
        return

    # submode != 0
    if is_set:
        return

    cd = entity.get("cooldown_82", 0)
    if cd > 0:
        entity["cooldown_82"] = cd - 1
        return

    dmg = entity.get("damage_86", 0)
    if dmg == 0:
        return

    entity["damage_86"] = 0
    entity["cooldown_82"] = 10  # 10-frame cooldown

    # bit 13 test: spirit weapon damage (0x2000)
    if (dmg & (1 << 13)) != 0:
        sfx_log.append(0x9003A1)
        persistent_flags[flag_id] = True
        entity["anim"] = 2
        entity["flag_3c"] = entity.get("flag_3c", 0) | (1 << 1)
        entity["cooldown_82"] = 0xFF  # locked on
    else:
        sfx_log.append(0x900238)


def step_breakable_wall(
    entity: dict,
    persistent_flags: dict[int, bool],
    sfx_log: list[int],
    debris_log: list[dict],
) -> None:
    """
    Emulates 0x016D5C..0x016DCC Breakable Wall Block.
    """
    flag_id = entity.get("persistent_id", 0)
    submode = entity.get("submode_66", 0)

    if submode == 0:
        entity["submode_66"] = 1
        hp = entity.get("param_a0", 0)
        entity["hp_8a"] = hp
        entity["hp_max_88"] = hp
        return

    # submode != 0: if persistent flag already active, do nothing
    if persistent_flags.get(flag_id, False):
        return

    cd = entity.get("cooldown_82", 0)
    if cd != 0:
        entity["cooldown_82"] = cd - 1
        return

    dmg = entity.get("damage_86", 0)
    if dmg == 0:
        return

    entity["damage_86"] = 0
    entity["cooldown_82"] = 6  # 6-frame cooldown

    # bit 13 test
    if (dmg & (1 << 13)) != 0:
        pts = dmg & 0xFFF
        cur_hp = entity.get("hp_8a", 0)
        new_hp = cur_hp - pts
        entity["hp_8a"] = max(0, new_hp)
        if new_hp <= 0:
            persistent_flags[flag_id] = True
            entity["z"] = entity.get("z", 0) + 16
            debris_log.append({"debris_type": 1, "z": entity["z"]})
    else:
        sfx_log.append(0x900238)


def step_floor_spike_hazard(
    spike: dict,
    ali: dict,
    in_aabb: bool,
    persistent_flags: dict[int, bool],
    sfx_log: list[int],
) -> bool:
    """
    Emulates 0x016DCE..0x016EBA Floor Spike / Saw Hazard & Blinking Despawn.
    """
    submode = spike.get("submode_66", 0)

    if submode == 0:
        # Sound timer
        st = spike.get("sound_timer_aa", 0) - 1
        spike["sound_timer_aa"] = st
        if st < 0:
            spike["sound_timer_aa"] = 0x34  # 52 frames
            sfx_log.append(0x600262)

        if not in_aabb:
            return False

        # Guard checks on Ali
        if (ali.get("flag_38", 0) & (1 << 3)) != 0:
            return False
        if ali.get("cooldown_82", 0) != 0:
            return False
        if ali.get("hitstun_83", 0) >= 2:
            return False

        # Vulnerable hit
        ali["attacker_ptr"] = spike.get("ptr", 0)
        ali["cooldown_82"] = 0
        damage = 0x200F
        ali["flag_9d"] = 0

        dx = ali.get("x", 0) - spike.get("x", 0)
        dy = ali.get("y", 0) - spike.get("y", 0)
        angle = 0 if dx >= 0 else 128
        ali["knockback_angle_84"] = angle

        # Directional velocities (x=$400, y=$3C0)
        ali["vx_4e"] = 0x400 if dx >= 0 else -0x400
        ali["vy_52"] = 0x3C0 if dy >= 0 else -0x3C0
        ali["vz_56"] = 0x00020000

        # Airborne check
        if ali.get("hitstun_83", 0) != 0 or ali.get("z_10", 0) != 0:
            damage |= 0x8000  # $A00F

        ali["damage_86"] = damage
        ali["hitstun_83"] = 10
        sfx_log.append(0x8004D9)
        return True

    # submode != 0: Blinking / Despawn
    timer = spike.get("timer_2", 0) + 1
    spike["timer_2"] = timer
    if timer < 15:
        spike["flag_36"] = spike.get("flag_36", 0) ^ 0x02  # toggle bit 1
        return False
    else:
        spike["type_0"] = 0  # despawn
        flag_id = spike.get("persistent_id", 0)
        persistent_flags[flag_id] = True
        return False


# ==============================================================================
# Unit Tests
# ==============================================================================

class TestROMWorldCrystalWallSpike(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure_and_counts(self):
        meta = self.evidence["metadata"]
        self.assertEqual(meta["milestone"], "M15.519")
        self.assertEqual(meta["range_start"], "0x016CBE")
        self.assertEqual(meta["range_end"], "0x016EBC")
        self.assertEqual(meta["range_size_bytes"], 510)
        self.assertEqual(meta["instruction_count"], 132)
        self.assertEqual(meta["routine_count"], 3)

        routines = {r["name"]: r for r in self.evidence["routines"]}
        self.assertIn("AI_World_SpiritCrystalOrbSwitch", routines)
        self.assertIn("AI_World_BreakableWallBlock", routines)
        self.assertIn("AI_World_FloorSpikeSawHazard", routines)

        self.assertEqual(routines["AI_World_SpiritCrystalOrbSwitch"]["instruction_count"], 41)
        self.assertEqual(routines["AI_World_SpiritCrystalOrbSwitch"]["size_bytes"], 158)
        self.assertEqual(routines["AI_World_BreakableWallBlock"]["instruction_count"], 31)
        self.assertEqual(routines["AI_World_BreakableWallBlock"]["size_bytes"], 114)
        self.assertEqual(routines["AI_World_FloorSpikeSawHazard"]["instruction_count"], 60)
        self.assertEqual(routines["AI_World_FloorSpikeSawHazard"]["size_bytes"], 238)

    # --------------------------------------------------------------------------
    # Spirit Crystal Orb Switch Tests
    # --------------------------------------------------------------------------

    def test_crystal_orb_init_dormant(self):
        orb = {"submode_66": 0, "param_8c": 24, "persistent_id": 5}
        p_flags = {5: False}
        sfx = []
        step_crystal_orb_switch(orb, p_flags, sfx)
        self.assertEqual(orb["submode_66"], 1)
        self.assertEqual(orb["z"], 24)
        self.assertEqual(orb["anim"], 0)
        self.assertEqual(p_flags[5], False)

    def test_crystal_orb_init_already_activated(self):
        orb = {"submode_66": 0, "param_8c": 32, "persistent_id": 7, "flag_38": 0, "flag_3c": 0}
        p_flags = {7: True}
        sfx = []
        step_crystal_orb_switch(orb, p_flags, sfx)
        self.assertEqual(orb["submode_66"], 1)
        self.assertEqual(orb["z"], 32)
        self.assertEqual(orb["anim"], 2)
        self.assertTrue(orb["flag_38"] & (1 << 3))
        self.assertTrue(orb["flag_3c"] & (1 << 1))

    def test_crystal_orb_cooldown_decrement(self):
        orb = {"submode_66": 1, "persistent_id": 3, "cooldown_82": 5, "damage_86": 0x2005}
        p_flags = {3: False}
        sfx = []
        step_crystal_orb_switch(orb, p_flags, sfx)
        self.assertEqual(orb["cooldown_82"], 4)
        self.assertEqual(orb["damage_86"], 0x2005)  # damage ignored during cooldown
        self.assertEqual(sfx, [])

    def test_crystal_orb_damage_physical_clink(self):
        orb = {"submode_66": 1, "persistent_id": 4, "cooldown_82": 0, "damage_86": 0x000F, "anim": 0}
        p_flags = {4: False}
        sfx = []
        step_crystal_orb_switch(orb, p_flags, sfx)
        self.assertEqual(orb["damage_86"], 0)
        self.assertEqual(orb["cooldown_82"], 10)
        self.assertEqual(orb["anim"], 0)
        self.assertEqual(p_flags[4], False)
        self.assertEqual(sfx, [0x900238])  # Clink SFX

    def test_crystal_orb_damage_spirit_activation(self):
        orb = {"submode_66": 1, "persistent_id": 8, "cooldown_82": 0, "damage_86": 0x200F, "flag_3c": 0}
        p_flags = {8: False}
        sfx = []
        step_crystal_orb_switch(orb, p_flags, sfx)
        self.assertEqual(orb["damage_86"], 0)
        self.assertEqual(orb["cooldown_82"], 0xFF)  # locked on
        self.assertEqual(orb["anim"], 2)
        self.assertTrue(orb["flag_3c"] & (1 << 1))
        self.assertEqual(p_flags[8], True)  # persistent flag set
        self.assertEqual(sfx, [0x9003A1])  # Chime activation SFX

    def test_crystal_orb_locked_on_when_activated(self):
        orb = {"submode_66": 1, "persistent_id": 8, "cooldown_82": 0xFF, "damage_86": 0x200F, "anim": 2}
        p_flags = {8: True}
        sfx = []
        step_crystal_orb_switch(orb, p_flags, sfx)
        self.assertEqual(orb["damage_86"], 0x200F)  # ignored since persistent bit is set
        self.assertEqual(sfx, [])

    # --------------------------------------------------------------------------
    # Breakable Wall Block Tests
    # --------------------------------------------------------------------------

    def test_breakable_wall_init(self):
        wall = {"submode_66": 0, "param_a0": 100, "persistent_id": 12}
        p_flags = {12: False}
        sfx, debris = [], []
        step_breakable_wall(wall, p_flags, sfx, debris)
        self.assertEqual(wall["submode_66"], 1)
        self.assertEqual(wall["hp_8a"], 100)
        self.assertEqual(wall["hp_max_88"], 100)

    def test_breakable_wall_cooldown_decrement(self):
        wall = {"submode_66": 1, "persistent_id": 12, "cooldown_82": 3, "damage_86": 0x2020, "hp_8a": 100}
        p_flags = {12: False}
        sfx, debris = [], []
        step_breakable_wall(wall, p_flags, sfx, debris)
        self.assertEqual(wall["cooldown_82"], 2)
        self.assertEqual(wall["hp_8a"], 100)
        self.assertEqual(wall["damage_86"], 0x2020)

    def test_breakable_wall_damage_physical_clink(self):
        wall = {"submode_66": 1, "persistent_id": 12, "cooldown_82": 0, "damage_86": 0x0030, "hp_8a": 80}
        p_flags = {12: False}
        sfx, debris = [], []
        step_breakable_wall(wall, p_flags, sfx, debris)
        self.assertEqual(wall["damage_86"], 0)
        self.assertEqual(wall["cooldown_82"], 6)
        self.assertEqual(wall["hp_8a"], 80)
        self.assertEqual(sfx, [0x900238])  # Clink SFX
        self.assertEqual(debris, [])

    def test_breakable_wall_damage_spirit_survives(self):
        # 0x201E: spirit flag (bit 13) + 30 damage points
        wall = {"submode_66": 1, "persistent_id": 15, "cooldown_82": 0, "damage_86": 0x201E, "hp_8a": 50, "z": 0}
        p_flags = {15: False}
        sfx, debris = [], []
        step_breakable_wall(wall, p_flags, sfx, debris)
        self.assertEqual(wall["damage_86"], 0)
        self.assertEqual(wall["cooldown_82"], 6)
        self.assertEqual(wall["hp_8a"], 20)  # 50 - 30 = 20
        self.assertEqual(p_flags[15], False)
        self.assertEqual(debris, [])

    def test_breakable_wall_damage_spirit_shatters(self):
        wall = {"submode_66": 1, "persistent_id": 15, "cooldown_82": 0, "damage_86": 0x2030, "hp_8a": 20, "z": 10}
        p_flags = {15: False}
        sfx, debris = [], []
        step_breakable_wall(wall, p_flags, sfx, debris)
        self.assertEqual(wall["damage_86"], 0)
        self.assertEqual(wall["cooldown_82"], 6)
        self.assertEqual(wall["hp_8a"], 0)
        self.assertEqual(p_flags[15], True)
        self.assertEqual(wall["z"], 26)  # raised by 16 px
        self.assertEqual(debris, [{"debris_type": 1, "z": 26}])

    # --------------------------------------------------------------------------
    # Floor Spike Saw Hazard Tests
    # --------------------------------------------------------------------------

    def test_floor_spike_sound_timer_and_buzz_sfx(self):
        spike = {"submode_66": 0, "sound_timer_aa": 0}
        ali = {}
        sfx = []
        step_floor_spike_hazard(spike, ali, in_aabb=False, persistent_flags={}, sfx_log=sfx)
        self.assertEqual(spike["sound_timer_aa"], 0x34)  # reset to 52 frames
        self.assertEqual(sfx, [0x600262])  # blade buzz SFX

    def test_floor_spike_aabb_miss(self):
        spike = {"submode_66": 0, "sound_timer_aa": 10}
        ali = {"flag_38": 0}
        sfx = []
        hit = step_floor_spike_hazard(spike, ali, in_aabb=False, persistent_flags={}, sfx_log=sfx)
        self.assertFalse(hit)
        self.assertNotIn("damage_86", ali)

    def test_floor_spike_ali_guarded_or_invulnerable(self):
        spike = {"submode_66": 0, "sound_timer_aa": 10}
        # Guard bit set
        ali_guarded = {"flag_38": (1 << 3), "cooldown_82": 0, "hitstun_83": 0}
        hit = step_floor_spike_hazard(spike, ali_guarded, in_aabb=True, persistent_flags={}, sfx_log=[])
        self.assertFalse(hit)

        # Invulnerable
        ali_invuln = {"flag_38": 0, "cooldown_82": 4, "hitstun_83": 0}
        hit = step_floor_spike_hazard(spike, ali_invuln, in_aabb=True, persistent_flags={}, sfx_log=[])
        self.assertFalse(hit)

        # In heavy hitstun (>= 2)
        ali_stunned = {"flag_38": 0, "cooldown_82": 0, "hitstun_83": 3}
        hit = step_floor_spike_hazard(spike, ali_stunned, in_aabb=True, persistent_flags={}, sfx_log=[])
        self.assertFalse(hit)

    def test_floor_spike_ali_grounded_hit(self):
        spike = {"submode_66": 0, "sound_timer_aa": 10, "ptr": 0xFF2200, "x": 100, "y": 200}
        ali = {
            "flag_38": 0,
            "cooldown_82": 0,
            "hitstun_83": 0,
            "z_10": 0,
            "x": 110,
            "y": 205,
        }
        sfx = []
        hit = step_floor_spike_hazard(spike, ali, in_aabb=True, persistent_flags={}, sfx_log=sfx)
        self.assertTrue(hit)
        self.assertEqual(ali["attacker_ptr"], 0xFF2200)
        self.assertEqual(ali["damage_86"], 0x200F)  # grounded spike hazard damage
        self.assertEqual(ali["hitstun_83"], 10)
        self.assertEqual(ali["vx_4e"], 0x400)
        self.assertEqual(ali["vy_52"], 0x3C0)
        self.assertEqual(ali["vz_56"], 0x00020000)
        self.assertEqual(sfx, [0x8004D9])  # impale SFX

    def test_floor_spike_ali_airborne_hit(self):
        spike = {"submode_66": 0, "sound_timer_aa": 10, "ptr": 0xFF2200, "x": 100, "y": 200}
        ali = {
            "flag_38": 0,
            "cooldown_82": 0,
            "hitstun_83": 0,
            "z_10": 15,  # airborne
            "x": 90,
            "y": 195,
        }
        sfx = []
        hit = step_floor_spike_hazard(spike, ali, in_aabb=True, persistent_flags={}, sfx_log=sfx)
        self.assertTrue(hit)
        self.assertEqual(ali["damage_86"], 0xA00F)  # airborne damage (0x200F | 0x8000)
        self.assertEqual(ali["vx_4e"], -0x400)
        self.assertEqual(ali["vy_52"], -0x3C0)
        self.assertEqual(sfx, [0x8004D9])

    def test_floor_spike_blinking_toggle(self):
        spike = {"submode_66": 1, "timer_2": 4, "flag_36": 0x00}
        p_flags = {}
        sfx = []
        step_floor_spike_hazard(spike, {}, in_aabb=False, persistent_flags=p_flags, sfx_log=sfx)
        self.assertEqual(spike["timer_2"], 5)
        self.assertEqual(spike["flag_36"], 0x02)  # bit 1 toggled on

        step_floor_spike_hazard(spike, {}, in_aabb=False, persistent_flags=p_flags, sfx_log=sfx)
        self.assertEqual(spike["timer_2"], 6)
        self.assertEqual(spike["flag_36"], 0x00)  # bit 1 toggled off

    def test_floor_spike_despawn_and_persistent_bit(self):
        spike = {"submode_66": 1, "timer_2": 14, "type_0": 0x45, "persistent_id": 20}
        p_flags = {20: False}
        sfx = []
        step_floor_spike_hazard(spike, {}, in_aabb=False, persistent_flags=p_flags, sfx_log=sfx)
        self.assertEqual(spike["timer_2"], 15)
        self.assertEqual(spike["type_0"], 0)  # despawned
        self.assertEqual(p_flags[20], True)  # persistent bit set


if __name__ == "__main__":
    unittest.main()
