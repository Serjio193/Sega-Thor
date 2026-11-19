#!/usr/bin/env python3
"""
Unit tests for ROM Bow Spirit (0x1E) 4-Node Vine Tether Physics,
Round-Robin Pool 2 Enemy Target Scanner & Nearest Manhattan Finder (0x014242..0x014522).
Milestone M15.501.
"""

import json
import math
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_bow_tether_target_m15_501.json"
)

POOL2_BASE = 0xFF1CD8
POOL2_END = 0xFF2954
SLOT_SIZE = 0xBC  # 188 bytes
SLOT_COUNT = (POOL2_END - POOL2_BASE) // SLOT_SIZE  # 17 slots


def step_node_timers(timers):
    """Emulate 0x01425C..0x014270: 4 timers decremented if > 0."""
    return [max(0, t - 1) if t > 0 else t for t in timers]


def distance_constraint_00f160(anchor, candidate, radius):
    """
    Emulate ROM 0x00F160: 3D sphere ray-interception constraint.
    anchor: (d1, d2, d3)
    candidate: (d4, d5, d6)
    radius: d0
    Returns: (carry_set, new_candidate)
      carry_set=False: dist <= radius (satisfied, candidate unchanged).
      carry_set=True:  dist > radius (clamped to radius sphere surface).
    """
    dx = candidate[0] - anchor[0]
    dy = candidate[1] - anchor[1]
    dz = candidate[2] - anchor[2]
    dist_sq = dx * dx + dy * dy + dz * dz
    dist = int(math.isqrt(dist_sq))

    if dist <= radius:
        return False, list(candidate)

    # Clamping: candidate = anchor + (delta * radius) // dist
    new_x = anchor[0] + (dx * radius) // dist
    new_y = anchor[1] + (dy * radius) // dist
    new_z = anchor[2] + (dz * radius) // dist
    return True, [new_x, new_y, new_z]


def solve_vine_tether_physics(ali_pos, bow_pos, nodes, max_radius):
    """
    Emulate 0x014272..0x0143C6:
    2-pass 3D distance constraint solver across 4 nodes.
    Forward pass: Bow -> node 3 -> node 2 -> node 1 -> node 0.
    Backward pass: Ali -> node 0 -> node 1 -> node 2 -> node 3 -> Bow.
    Early-exits on Carry Clear (distance constraint satisfied without clamping).
    """
    current_nodes = [list(n) for n in nodes]
    current_bow = list(bow_pos)

    # --- Forward Pass (Bow -> 3 -> 2 -> 1 -> 0) ---
    anchor = current_bow
    for i in reversed(range(4)):
        clamped, updated = distance_constraint_00f160(anchor, current_nodes[i], max_radius)
        if not clamped:
            return False, current_bow, current_nodes
        current_nodes[i] = updated
        anchor = current_nodes[i]

    # --- Backward Pass (Ali -> 0 -> 1 -> 2 -> 3 -> Bow) ---
    anchor = ali_pos
    # First link: Ali to Node 0 with radius (max_radius - 2)
    clamped, updated = distance_constraint_00f160(anchor, current_nodes[0], max_radius - 2)
    if not clamped:
        return False, current_bow, current_nodes
    current_nodes[0] = updated
    anchor = current_nodes[0]

    # Links 0->1, 1->2, 2->3 with radius max_radius
    for i in range(1, 4):
        clamped, updated = distance_constraint_00f160(anchor, current_nodes[i], max_radius)
        if not clamped:
            return False, current_bow, current_nodes
        current_nodes[i] = updated
        anchor = current_nodes[i]

    # Final link: Node 3 to Bow
    clamped, updated_bow = distance_constraint_00f160(anchor, current_bow, max_radius)
    if not clamped:
        return False, current_bow, current_nodes

    current_bow = updated_bow
    return True, current_bow, current_nodes


def advance_round_robin_scanner(current_offset, pool_entities, ali_pos):
    """
    Emulate 0x0143C8..0x014494:
    Round-robin slot scan with wrapping and validation.
    Returns: (success, next_offset, lunge_data_or_None)
    """
    next_offset = current_offset + SLOT_SIZE
    slot_addr = POOL2_BASE + next_offset
    if slot_addr < POOL2_BASE or slot_addr >= POOL2_END:
        next_offset = 0

    slot_idx = next_offset // SLOT_SIZE
    entity = pool_entities[slot_idx]

    # Filter 1: active status
    if entity.get("status", 0) <= 0:
        return False, next_offset, None

    # Filter 2: flags at +$38 bit 3
    if (entity.get("flags_38", 0) & 0x08) != 0:
        return False, next_offset, None

    # Filter 3: type $66 bypasses +$3a bit 0 check
    status = entity.get("status", 0)
    if status != 0x66:
        if (entity.get("flags_3a", 0) & 0x01) != 0:
            return False, next_offset, None

    # Filter 4: Euclidean distance to Ali <= 0x36 (54 px)
    enemy_x = entity.get("x", 0)
    enemy_y = entity.get("y", 0)
    enemy_z = entity.get("z", 0) + (entity.get("height", 0) >> 2)

    dx = enemy_x - ali_pos[0]
    dy = enemy_y - ali_pos[1]
    dz = enemy_z - ali_pos[2]
    dist = int(math.isqrt(dx * dx + dy * dy + dz * dz))

    if dist > 0x36:
        return False, next_offset, None

    lunge = {
        "target_idx": slot_idx,
        "speed": 0x60,
        "action_anim": 0x22,
        "action_timer": 12,
        "substate": 3
    }
    return True, next_offset, lunge


def find_nearest_pool2_enemy(pool_entities, ali_pos):
    """
    Emulate 0x014494..0x014522:
    Nearest Pool 2 enemy Manhattan distance finder.
    Returns: (found, closest_idx, min_dist)
    """
    best_dist = 0xFFFF
    best_idx = None

    for idx, entity in enumerate(pool_entities):
        if entity.get("status", 0) <= 0:
            continue
        if (entity.get("flags_38", 0) & 0x08) != 0:
            continue
        if entity.get("status", 0) != 0x66 and (entity.get("flags_3a", 0) & 0x01) != 0:
            continue

        manhattan = (
            abs(entity.get("x", 0) - ali_pos[0]) +
            abs(entity.get("y", 0) - ali_pos[1]) +
            abs(entity.get("z", 0) - ali_pos[2])
        )

        if manhattan < best_dist:
            best_dist = manhattan
            best_idx = idx

    if best_idx is None or best_dist == 0xFFFF:
        return False, None, 0xFFFF

    return True, best_idx, best_dist


class TestROMBowTetherTarget(unittest.TestCase):
    def setUp(self):
        self.assertTrue(os.path.exists(EVIDENCE_PATH), f"Missing evidence: {EVIDENCE_PATH}")
        with open(EVIDENCE_PATH, "r") as f:
            self.evidence = json.load(f)

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.501")
        self.assertEqual(self.evidence["range"]["start"], "0x014242")
        self.assertEqual(self.evidence["range"]["end"], "0x014522")
        self.assertEqual(self.evidence["range"]["size_bytes"], 736)
        self.assertEqual(len(self.evidence["routines"]), 3)
        self.assertEqual(self.evidence["instruction_count"], 220)

    def test_timer_decrement(self):
        timers = [5, 0, 1, 10]
        step1 = step_node_timers(timers)
        self.assertEqual(step1, [4, 0, 0, 9])
        step2 = step_node_timers(step1)
        self.assertEqual(step2, [3, 0, 0, 8])

    def test_distance_constraint_00f160(self):
        anchor = [100, 100, 100]
        # Candidate inside radius 20 (dx=10, dist=10 <= 20) -> no clamping
        clamped, updated = distance_constraint_00f160(anchor, [110, 100, 100], 20)
        self.assertFalse(clamped)
        self.assertEqual(updated, [110, 100, 100])

        # Candidate outside radius 10 (dx=30, dist=30 > 10) -> clamped to radius 10
        clamped, updated = distance_constraint_00f160(anchor, [130, 100, 100], 10)
        self.assertTrue(clamped)
        self.assertEqual(updated, [110, 100, 100])

    def test_vine_tether_physics_passes(self):
        ali_pos = [100, 100, 0]
        # Nodes placed at distances of 15 px each, max radius 10 -> triggers clamping
        nodes = [
            [115, 100, 0],
            [130, 100, 0],
            [145, 100, 0],
            [160, 100, 0]
        ]
        bow_pos = [175, 100, 0]

        clamped, new_bow, new_nodes = solve_vine_tether_physics(ali_pos, bow_pos, nodes, 10)
        self.assertTrue(clamped)
        # First node constrained to ali (radius 10-2 = 8) -> x=108
        self.assertEqual(new_nodes[0][0], 108)
        # Subsequent nodes spaced by radius 10
        self.assertEqual(new_nodes[1][0], 118)
        self.assertEqual(new_nodes[2][0], 128)
        self.assertEqual(new_nodes[3][0], 138)
        self.assertEqual(new_bow[0], 148)

    def test_round_robin_pool2_scanner(self):
        ali_pos = [200, 200, 0]
        # 17 empty slots
        pool = [{"status": 0} for _ in range(SLOT_COUNT)]
        # Slot 2: active enemy, within 30 px
        pool[2] = {"status": 0x10, "flags_38": 0, "flags_3a": 0, "x": 210, "y": 210, "z": 0, "height": 0}
        # Slot 4: active enemy, but too far (> 0x36 = 54 px)
        pool[4] = {"status": 0x10, "flags_38": 0, "flags_3a": 0, "x": 300, "y": 300, "z": 0, "height": 0}

        # Step 0 -> tests slot 1 (empty)
        success, offset, _ = advance_round_robin_scanner(0, pool, ali_pos)
        self.assertFalse(success)
        self.assertEqual(offset, SLOT_SIZE)

        # Step 1 -> tests slot 2 (target match!)
        success, offset, lunge = advance_round_robin_scanner(offset, pool, ali_pos)
        self.assertTrue(success)
        self.assertEqual(offset, SLOT_SIZE * 2)
        self.assertEqual(lunge["target_idx"], 2)
        self.assertEqual(lunge["action_anim"], 0x22)
        self.assertEqual(lunge["action_timer"], 12)
        self.assertEqual(lunge["substate"], 3)

        # Test wrap-around: from slot 16 offset, should wrap to 0
        _, wrap_offset, _ = advance_round_robin_scanner((SLOT_COUNT - 1) * SLOT_SIZE, pool, ali_pos)
        self.assertEqual(wrap_offset, 0)

    def test_nearest_pool2_enemy_manhattan(self):
        ali_pos = [100, 100, 10]
        pool = [{"status": 0} for _ in range(SLOT_COUNT)]

        # No enemies: returns not found
        found, idx, dist = find_nearest_pool2_enemy(pool, ali_pos)
        self.assertFalse(found)
        self.assertEqual(dist, 0xFFFF)

        # Add two enemies: slot 3 at dist 40, slot 7 at dist 15
        pool[3] = {"status": 0x12, "flags_38": 0, "flags_3a": 0, "x": 120, "y": 110, "z": 20}  # |20|+|10|+|10| = 40
        pool[7] = {"status": 0x15, "flags_38": 0, "flags_3a": 0, "x": 105, "y": 105, "z": 15}  # |5|+|5|+|5| = 15

        found, idx, dist = find_nearest_pool2_enemy(pool, ali_pos)
        self.assertTrue(found)
        self.assertEqual(idx, 7)
        self.assertEqual(dist, 15)

        # Immune enemy (flag 38 bit 3 set) should be skipped
        pool[7]["flags_38"] = 0x08
        found, idx, dist = find_nearest_pool2_enemy(pool, ali_pos)
        self.assertTrue(found)
        self.assertEqual(idx, 3)
        self.assertEqual(dist, 40)


if __name__ == "__main__":
    unittest.main()
