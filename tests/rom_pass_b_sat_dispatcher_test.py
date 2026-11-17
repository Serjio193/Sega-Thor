#!/usr/bin/env python3
"""Unit tests for ROM Pass B master SAT dispatcher & cache mask reuse (0x00A524..0x00A69A)."""

import unittest
from dataclasses import dataclass
from typing import List, Tuple


@dataclass
class SatPassEntity:
    slot_id: int
    entity_type: int
    y: int
    flags_36: int = 0


def run_pass_b_group_2(entities: List[SatPassEntity], cached_mask: int) -> List[Tuple[int, str]]:
    """Simulates 0x00A550..0x00A5A4 using cached active mask d7."""
    assert len(entities) == 21
    d7 = cached_mask
    render_order = []

    while True:
        d5 = -128
        d4_ent = None
        d3_slot = -1

        for i in range(20, -1, -1):
            if d7 & (1 << i):
                continue
            ent = entities[i]
            if ent.y > d5:
                d5 = ent.y
                d4_ent = ent
                d3_slot = i

        if d4_ent is None:
            break

        d7 |= (1 << d3_slot)
        target = "custom_sat" if (d4_ent.flags_36 & 0x01) else "normal_sat"
        render_order.append((d4_ent.slot_id, target))

    return render_order


class RomPassBSatDispatcherTest(unittest.TestCase):
    def test_cached_mask_filters_pre_marked_slots(self):
        pool = [SatPassEntity(i, 1, 100 + i * 5) for i in range(21)]
        # Suppose cached mask marks slots 0..9 as inactive/done
        cached_mask = (1 << 10) - 1
        order = run_pass_b_group_2(pool, cached_mask)
        # Only slots 10..20 should be dispatched
        dispatched_slots = {slot for slot, _ in order}
        self.assertEqual(dispatched_slots, set(range(10, 21)))

    def test_pass_b_descending_y_sorting(self):
        pool = [SatPassEntity(i, 1, 0) for i in range(21)]
        pool[3] = SatPassEntity(3, 1, 120)
        pool[8] = SatPassEntity(8, 1, 240)
        pool[15] = SatPassEntity(15, 1, 180)

        # All other slots inactive
        cached_mask = 0x1FFFFF & ~((1 << 3) | (1 << 8) | (1 << 15))
        order = run_pass_b_group_2(pool, cached_mask)
        self.assertEqual([slot for slot, _ in order], [8, 15, 3])

    def test_custom_vs_normal_sat_target(self):
        pool = [SatPassEntity(i, 1, 0) for i in range(21)]
        pool[1] = SatPassEntity(1, 1, 200, flags_36=1)  # custom
        pool[2] = SatPassEntity(2, 1, 100, flags_36=0)  # normal

        cached_mask = 0x1FFFFF & ~((1 << 1) | (1 << 2))
        order = run_pass_b_group_2(pool, cached_mask)
        self.assertEqual(order, [(1, "custom_sat"), (2, "normal_sat")])

    def test_empty_active_pool_returns_immediately(self):
        pool = [SatPassEntity(i, 0, 0) for i in range(21)]
        cached_mask = 0x1FFFFF  # all inactive
        order = run_pass_b_group_2(pool, cached_mask)
        self.assertEqual(order, [])


if __name__ == "__main__":
    unittest.main()
