#!/usr/bin/env python3
"""
Unit tests for ROM World Interactive Objects: Patrolling/Pacing Trap, Persistent Gate & Hidden Bridge,
and Bomb-Destructible Barrier (0x01778C..0x017A96).
Milestone M15.523.
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
    / "rom_world_patrol_gate_bomb_m15_523.json"
)


# ==============================================================================
# Emulation Routines
# ==============================================================================

def step_patrolling_pacing_trap(
    trap: dict,
    tile_scan_ok: bool,
    sfx_log: list[int],
    anim_log: list[int],
) -> None:
    """
    Emulates 0x01778C..0x0178E8 Patrolling / Pacing Trap.
    """
    state = trap.get("state_a6", 0)

    # State 0: Home Init
    if state == 0:
        trap["home_x_a8"] = trap.get("x", 0)
        trap["home_y_aa"] = trap.get("y", 0)
        trap["z_10"] = trap.get("param_8c", 0)
        trap["anim_2a"] = 0
        trap["state_a6"] = 1
        return

    # State 1: Tile Probing & Motion Launch
    if state == 1:
        if not tile_scan_ok:
            return  # bcc -> rts

        # Directional velocity from speed param_a0
        speed = trap.get("param_a0", 0x400)
        facing = trap.get("facing_16", 0)
        trap["vx_4e"] = speed if facing in (0, 1) else -speed
        trap["vy_52"] = speed if facing in (0, 2) else -speed
        trap["anim_2a"] = 2
        trap["timer_2"] = trap.get("param_9e", 0)
        trap["state_a6"] = 2
        sfx_log.append(0x800399)
        return

    # State 2: Acceleration, Deceleration & Travel
    if state == 2:
        anim_log.append(0)  # jsr $BC8E.l
        timer = trap.get("timer_2", 0) - 1
        trap["timer_2"] = timer

        if timer <= 0:
            trap["flag_17"] = trap.get("flag_17", 0) ^ 0x02  # bchg #1, $17
            trap["anim_2a"] = 4
            trap["state_a6"] = 3
            return

        midpoint = trap.get("param_9e", 0) >> 1
        accel_step = 0x20
        if timer >= midpoint:
            # Accelerating
            trap["vx_4e"] = trap.get("vx_4e", 0) + accel_step
            trap["vy_52"] = trap.get("vy_52", 0) + accel_step
        else:
            # Decelerating
            trap["vx_4e"] = trap.get("vx_4e", 0) - accel_step
            trap["vy_52"] = trap.get("vy_52", 0) - accel_step

        trap["delta_x_72"] = trap.get("delta_x_72", 0) + trap["vx_4e"]
        trap["delta_y_76"] = trap.get("delta_y_76", 0) + trap["vy_52"]
        return

    # State 3: Homing Return
    if state == 3:
        anim_log.append(0)  # jsr $BC8E.l
        dx = trap.get("home_x_a8", 0) - trap.get("x", 0)
        dy = trap.get("home_y_aa", 0) - trap.get("y", 0)

        if dx != 0 or dy != 0:
            step_x = 0x100 if dx > 0 else (-0x100 if dx < 0 else 0)
            step_y = 0x100 if dy > 0 else (-0x100 if dy < 0 else 0)
            trap["delta_x_72"] = step_x
            trap["delta_y_76"] = step_y
            trap["x"] = trap.get("x", 0) + (1 if dx > 0 else -1)
            trap["y"] = trap.get("y", 0) + (1 if dy > 0 else -1)
        else:
            # Arrived home
            trap["anim_2a"] = 0
            trap["flag_17"] = trap.get("flag_17", 0) ^ 0x02
            trap["state_a6"] = 1


def step_persistent_gate_bridge(
    gate: dict,
    persistent_flags: dict[int, bool],
    living_enemies: int,
    sfx_log: list[int],
    unlock_callback_log: list[str],
) -> None:
    """
    Emulates 0x0178EA..0x0179D2 Persistent Gate & Hidden Bridge.
    """
    submode = gate.get("submode_66", 0)

    # Submode 0: Init
    if submode == 0:
        gate["submode_66"] = 1
        param_8c = gate.get("param_8c", 0)
        gate["anim_2a"] = param_8c * 2
        gate["z_10"] = gate.get("z_10", 0) + 1
        return

    mode = gate.get("mode_9e", 0)
    flag_id = gate.get("persistent_id", 0)

    # Jump Table Entry 0: Persistent Bit Elevation / Visibility Toggle
    if mode == 0:
        old_z = gate.get("z_10", 0)
        is_set = persistent_flags.get(flag_id, False)

        if is_set:
            gate["flag_36"] = gate.get("flag_36", 0) | (1 << 1)   # visible
            gate["flag_38"] = gate.get("flag_38", 0) & ~(1 << 2)  # clear solid
            gate["z_10"] = 0x2710  # 10000 px
        else:
            gate["flag_36"] = gate.get("flag_36", 0) & ~(1 << 1)  # hidden
            gate["flag_38"] = gate.get("flag_38", 0) | (1 << 2)   # solid
            gate["z_10"] = 0

        if gate["z_10"] != old_z:
            sfx_log.append(0xB00385)  # Gate rumble SFX
        return

    # Jump Table Entry 1: Living Enemy Threshold
    if mode == 1:
        timer = gate.get("timer_2", 0)
        if timer < 10:
            gate["timer_2"] = timer + 1
            return

        threshold = gate.get("param_a0", 0)
        if living_enemies <= threshold:
            unlock_callback_log.append("0x017D06")
            sfx_log.append(0xA00228)  # Secret chime SFX
        return

    # Jump Table Entry 2: Delayed Trigger
    if mode == 2:
        timer = gate.get("timer_2", 0)
        if timer != 0:
            timer += 1
            gate["timer_2"] = timer
            if timer >= 15:
                unlock_callback_log.append("0x017D06")
                sfx_log.append(0xA00228)


def step_bomb_destructible_barrier(
    barrier: dict,
    attacker: dict,
    sfx_log: list[int],
    callback_log: list[str],
) -> None:
    """
    Emulates 0x0179D4..0x017A94 Bomb-Destructible Barrier.
    """
    submode = barrier.get("submode_66", 0)

    # Submode 0: Init
    if submode == 0:
        # Calls 0x0178F2 logic
        barrier["submode_66"] = 1
        barrier["anim_2a"] = barrier.get("param_8c", 0) * 2
        barrier["z_10"] = barrier.get("z_10", 0) + 1

        if barrier.get("mode_9e", 0) != 0:
            barrier["submode_66"] = 3
            barrier["flag_36"] = barrier.get("flag_36", 0) ^ 0x02  # toggle bit 1
        return

    # Submode 1: Standard Bomb Barrier
    if submode == 1:
        dmg = barrier.get("damage_86", 0)
        if dmg == 0:
            return

        barrier["damage_86"] = 0
        is_bomb = (
            attacker.get("ptr", 0) == 0x00FF1AA4
            and attacker.get("type_0", 0) == 0x1E
        )

        if is_bomb:
            callback_log.append("0x017FF2")
            barrier["cooldown_82"] = 0xFF
            barrier["anim_2a"] = 6
            barrier["submode_66"] = 2
            sfx_log.append(0x900202)  # Bomb explosion SFX
        else:
            barrier["cooldown_82"] = 0
            sfx_log.append(0x900238)  # Clink SFX
        return

    # Submode 2: Destruction Animation Playing
    if submode == 2:
        if (barrier.get("flag_37", 0) & (1 << 1)) != 0:  # anim complete bit
            barrier["type_0"] = 0  # despawn
        return

    # Submode 3: Alternate Instant-Despawn Bomb Barrier
    if submode == 3:
        dmg = barrier.get("damage_86", 0)
        if dmg == 0:
            return

        barrier["damage_86"] = 0
        is_bomb = (
            attacker.get("ptr", 0) == 0x00FF1AA4
            and attacker.get("type_0", 0) == 0x1E
        )

        if is_bomb:
            callback_log.append("0x017FF2")
            barrier["type_0"] = 0  # instant despawn
            sfx_log.append(0x900202)
        else:
            barrier["cooldown_82"] = 0
            sfx_log.append(0x900238)


# ==============================================================================
# Unit Tests
# ==============================================================================

class TestROMWorldPatrolGateBomb(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure_and_counts(self):
        meta = self.evidence["metadata"]
        self.assertEqual(meta["milestone"], "M15.523")
        self.assertEqual(meta["range_start"], "0x01778C")
        self.assertEqual(meta["range_end"], "0x017A96")
        self.assertEqual(meta["range_size_bytes"], 778)
        self.assertEqual(meta["instruction_count"], 204)
        self.assertEqual(meta["routine_count"], 3)

        routines = {r["name"]: r for r in self.evidence["routines"]}
        self.assertIn("AI_World_PatrollingPacingTrap", routines)
        self.assertIn("AI_World_PersistentGateHiddenBridge", routines)
        self.assertIn("AI_World_BombDestructibleBarrier", routines)

        self.assertEqual(routines["AI_World_PatrollingPacingTrap"]["instruction_count"], 93)
        self.assertEqual(routines["AI_World_PatrollingPacingTrap"]["size_bytes"], 350)
        self.assertEqual(routines["AI_World_PersistentGateHiddenBridge"]["instruction_count"], 59)
        self.assertEqual(routines["AI_World_PersistentGateHiddenBridge"]["size_bytes"], 234)
        self.assertEqual(routines["AI_World_BombDestructibleBarrier"]["instruction_count"], 52)
        self.assertEqual(routines["AI_World_BombDestructibleBarrier"]["size_bytes"], 194)

        tables = self.evidence["embedded_tables"]
        self.assertEqual(len(tables["patrol_tile_scan_table_0x01783e"]), 16)
        self.assertEqual(len(tables["gate_jump_table_0x017920"]), 3)

    # --------------------------------------------------------------------------
    # Patrolling / Pacing Trap Tests
    # --------------------------------------------------------------------------

    def test_patrol_trap_state_0_home_init(self):
        trap = {"state_a6": 0, "x": 160, "y": 240, "param_8c": 12}
        sfx, anim = [], []
        step_patrolling_pacing_trap(trap, tile_scan_ok=True, sfx_log=sfx, anim_log=anim)
        self.assertEqual(trap["home_x_a8"], 160)
        self.assertEqual(trap["home_y_aa"], 240)
        self.assertEqual(trap["z_10"], 12)
        self.assertEqual(trap["anim_2a"], 0)
        self.assertEqual(trap["state_a6"], 1)

    def test_patrol_trap_state_1_tile_scan_and_launch(self):
        # Tile scan blocked
        trap = {"state_a6": 1, "facing_16": 0}
        sfx, anim = [], []
        step_patrolling_pacing_trap(trap, tile_scan_ok=False, sfx_log=sfx, anim_log=anim)
        self.assertEqual(trap["state_a6"], 1)
        self.assertEqual(sfx, [])

        # Tile scan clear -> launch
        trap2 = {"state_a6": 1, "facing_16": 1, "param_a0": 0x400, "param_9e": 30}
        step_patrolling_pacing_trap(trap2, tile_scan_ok=True, sfx_log=sfx, anim_log=anim)
        self.assertEqual(trap2["state_a6"], 2)
        self.assertEqual(trap2["anim_2a"], 2)
        self.assertEqual(trap2["timer_2"], 30)
        self.assertEqual(sfx, [0x800399])

    def test_patrol_trap_state_2_accel_decel_travel(self):
        trap = {
            "state_a6": 2,
            "timer_2": 20,
            "param_9e": 20,
            "vx_4e": 0x400,
            "vy_52": 0x400,
            "delta_x_72": 0,
            "delta_y_76": 0,
            "flag_17": 0,
        }
        sfx, anim = [], []
        # Accelerating phase
        step_patrolling_pacing_trap(trap, tile_scan_ok=True, sfx_log=sfx, anim_log=anim)
        self.assertEqual(trap["timer_2"], 19)
        self.assertEqual(trap["vx_4e"], 0x420)
        self.assertEqual(trap["delta_x_72"], 0x420)

        # Transition at timer expire
        trap["timer_2"] = 1
        step_patrolling_pacing_trap(trap, tile_scan_ok=True, sfx_log=sfx, anim_log=anim)
        self.assertEqual(trap["state_a6"], 3)
        self.assertEqual(trap["anim_2a"], 4)
        self.assertEqual(trap["flag_17"], 2)

    def test_patrol_trap_state_3_homing_return(self):
        trap = {
            "state_a6": 3,
            "home_x_a8": 100,
            "home_y_aa": 100,
            "x": 102,
            "y": 102,
            "flag_17": 2,
        }
        sfx, anim = [], []
        # Stepping home
        step_patrolling_pacing_trap(trap, tile_scan_ok=True, sfx_log=sfx, anim_log=anim)
        self.assertEqual(trap["state_a6"], 3)
        self.assertEqual(trap["x"], 101)

        # Reached home
        trap["x"] = 100
        trap["y"] = 100
        step_patrolling_pacing_trap(trap, tile_scan_ok=True, sfx_log=sfx, anim_log=anim)
        self.assertEqual(trap["state_a6"], 1)
        self.assertEqual(trap["anim_2a"], 0)
        self.assertEqual(trap["flag_17"], 0)

    # --------------------------------------------------------------------------
    # Persistent Gate & Hidden Bridge Tests
    # --------------------------------------------------------------------------

    def test_persistent_gate_submode_0_init(self):
        gate = {"submode_66": 0, "param_8c": 2, "z_10": 0}
        sfx, unlock = [], []
        step_persistent_gate_bridge(gate, persistent_flags={}, living_enemies=5, sfx_log=sfx, unlock_callback_log=unlock)
        self.assertEqual(gate["submode_66"], 1)
        self.assertEqual(gate["anim_2a"], 4)
        self.assertEqual(gate["z_10"], 1)

    def test_persistent_gate_entry_0_toggle_unlocked_vs_locked(self):
        # Unlocked: persistent flag set -> elevated / non-solid
        gate = {"submode_66": 1, "mode_9e": 0, "persistent_id": 14, "z_10": 0}
        p_flags = {14: True}
        sfx, unlock = [], []
        step_persistent_gate_bridge(gate, p_flags, living_enemies=0, sfx_log=sfx, unlock_callback_log=unlock)
        self.assertEqual(gate["z_10"], 0x2710)
        self.assertTrue(gate["flag_36"] & (1 << 1))
        self.assertEqual(gate["flag_38"] & (1 << 2), 0)
        self.assertEqual(sfx, [0xB00385])

        # Locked: persistent flag clear -> z=0, solid
        p_flags[14] = False
        sfx.clear()
        step_persistent_gate_bridge(gate, p_flags, living_enemies=0, sfx_log=sfx, unlock_callback_log=unlock)
        self.assertEqual(gate["z_10"], 0)
        self.assertEqual(gate["flag_36"] & (1 << 1), 0)
        self.assertTrue(gate["flag_38"] & (1 << 2))
        self.assertEqual(sfx, [0xB00385])

    def test_persistent_gate_entry_1_enemy_clear_threshold(self):
        gate = {"submode_66": 1, "mode_9e": 1, "timer_2": 10, "param_a0": 0}
        sfx, unlock = [], []

        # Enemies remaining > 0
        step_persistent_gate_bridge(gate, persistent_flags={}, living_enemies=2, sfx_log=sfx, unlock_callback_log=unlock)
        self.assertEqual(unlock, [])

        # All enemies defeated
        step_persistent_gate_bridge(gate, persistent_flags={}, living_enemies=0, sfx_log=sfx, unlock_callback_log=unlock)
        self.assertEqual(unlock, ["0x017D06"])
        self.assertEqual(sfx, [0xA00228])

    def test_persistent_gate_delayed_trigger(self):
        gate = {"submode_66": 1, "mode_9e": 2, "timer_2": 14}
        sfx, unlock = [], []
        step_persistent_gate_bridge(gate, persistent_flags={}, living_enemies=0, sfx_log=sfx, unlock_callback_log=unlock)
        self.assertEqual(gate["timer_2"], 15)
        self.assertEqual(unlock, ["0x017D06"])
        self.assertEqual(sfx, [0xA00228])

    # --------------------------------------------------------------------------
    # Bomb-Destructible Barrier Tests
    # --------------------------------------------------------------------------

    def test_bomb_barrier_submode_0_init(self):
        # mode_9e == 0
        b1 = {"submode_66": 0, "mode_9e": 0, "param_8c": 1}
        step_bomb_destructible_barrier(b1, attacker={}, sfx_log=[], callback_log=[])
        self.assertEqual(b1["submode_66"], 1)

        # mode_9e != 0 -> submode 3 with flipped bit 1
        b2 = {"submode_66": 0, "mode_9e": 1, "param_8c": 1, "flag_36": 0}
        step_bomb_destructible_barrier(b2, attacker={}, sfx_log=[], callback_log=[])
        self.assertEqual(b2["submode_66"], 3)
        self.assertEqual(b2["flag_36"], 2)

    def test_bomb_barrier_non_bomb_attack_deflected(self):
        barrier = {"submode_66": 1, "damage_86": 0x200F}
        attacker = {"ptr": 0xFF19E8, "type_0": 1}  # Ali physical strike
        sfx, cb = [], []
        step_bomb_destructible_barrier(barrier, attacker, sfx_log=sfx, callback_log=cb)
        self.assertEqual(barrier["damage_86"], 0)
        self.assertEqual(barrier["cooldown_82"], 0)
        self.assertEqual(barrier["submode_66"], 1)
        self.assertEqual(sfx, [0x900238])  # Clink SFX
        self.assertEqual(cb, [])

    def test_bomb_barrier_submode_1_bomb_destruction(self):
        barrier = {"submode_66": 1, "damage_86": 0x2040}
        attacker = {"ptr": 0x00FF1AA4, "type_0": 0x1E}  # Valid Bomb!
        sfx, cb = [], []
        step_bomb_destructible_barrier(barrier, attacker, sfx_log=sfx, callback_log=cb)
        self.assertEqual(barrier["submode_66"], 2)
        self.assertEqual(barrier["anim_2a"], 6)
        self.assertEqual(barrier["cooldown_82"], 0xFF)
        self.assertEqual(cb, ["0x017FF2"])
        self.assertEqual(sfx, [0x900202])

    def test_bomb_barrier_submode_2_anim_completion_despawn(self):
        barrier = {"submode_66": 2, "type_0": 0x24, "flag_37": 0}
        step_bomb_destructible_barrier(barrier, attacker={}, sfx_log=[], callback_log=[])
        self.assertEqual(barrier["type_0"], 0x24)

        barrier["flag_37"] = (1 << 1)  # anim finished bit
        step_bomb_destructible_barrier(barrier, attacker={}, sfx_log=[], callback_log=[])
        self.assertEqual(barrier["type_0"], 0)

    def test_bomb_barrier_submode_3_instant_bomb_destruction(self):
        barrier = {"submode_66": 3, "damage_86": 0x2050, "type_0": 0x28}
        attacker = {"ptr": 0x00FF1AA4, "type_0": 0x1E}  # Valid Bomb!
        sfx, cb = [], []
        step_bomb_destructible_barrier(barrier, attacker, sfx_log=sfx, callback_log=cb)
        self.assertEqual(barrier["type_0"], 0)  # instantly despawned
        self.assertEqual(cb, ["0x017FF2"])
        self.assertEqual(sfx, [0x900202])


if __name__ == "__main__":
    unittest.main()
