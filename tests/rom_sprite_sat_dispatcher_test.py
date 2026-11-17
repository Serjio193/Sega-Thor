#!/usr/bin/env python3
"""Unit tests for ROM master sprite composition, Y-depth sorter & SAT dispatcher (0x00A196..0x00A336)."""

import unittest
from dataclasses import dataclass
from typing import List, Tuple


@dataclass
class RenderEntity:
    slot_id: int
    entity_type: int
    y: int
    flags_36: int = 0
    special_ptr: int = 0


def sort_group_2_depth(entities: List[RenderEntity]) -> List[Tuple[int, str]]:
    """Simulates the 0x00A1D6..0x00A244 selection-sort depth sorter."""
    assert len(entities) == 21

    # Pass 1: Build inactive mask in d7
    d7 = 0
    for i in range(21):
        if entities[i].entity_type == 0:
            d7 |= (1 << i)

    render_order = []

    # Pass 2: Iteratively find entity with largest Y among unrendered active slots
    while True:
        d5 = -128  # sign-extended 0xFF
        d4_ent = None
        d3_slot = -1

        for i in range(20, -1, -1):
            if d7 & (1 << i):
                continue
            ent = entities[i]
            # cmp.w $c(a6), d5; bge.w skip -> if d5 < ent.y: update
            if ent.y > d5:
                d5 = ent.y
                d4_ent = ent
                d3_slot = i

        if d4_ent is None:
            break

        # Mark slot processed
        d7 |= (1 << d3_slot)

        # Dispatch target
        target = "custom_sat" if (d4_ent.flags_36 & 0x01) else "normal_sat"
        render_order.append((d4_ent.slot_id, target))

    return render_order


class RomSpriteSatDispatcherTest(unittest.TestCase):
    def test_inactive_slots_are_skipped(self):
        pool = [RenderEntity(i, 0, 100) for i in range(21)]
        order = sort_group_2_depth(pool)
        self.assertEqual(len(order), 0)

    def test_single_active_entity_dispatched(self):
        pool = [RenderEntity(i, 0, 100) for i in range(21)]
        pool[5] = RenderEntity(5, 1, 150, flags_36=0)
        order = sort_group_2_depth(pool)
        self.assertEqual(order, [(5, "normal_sat")])

    def test_descending_y_sorting_order(self):
        pool = [RenderEntity(i, 0, 0) for i in range(21)]
        pool[2] = RenderEntity(2, 1, 100)  # background
        pool[7] = RenderEntity(7, 1, 300)  # foreground (highest Y)
        pool[14] = RenderEntity(14, 1, 200) # midground

        order = sort_group_2_depth(pool)
        self.assertEqual([slot for slot, _ in order], [7, 14, 2])

    def test_custom_sat_render_target_dispatch(self):
        pool = [RenderEntity(i, 0, 0) for i in range(21)]
        pool[1] = RenderEntity(1, 1, 50, flags_36=1)  # custom SAT
        pool[2] = RenderEntity(2, 1, 150, flags_36=0) # normal SAT

        order = sort_group_2_depth(pool)
        self.assertEqual(order, [(2, "normal_sat"), (1, "custom_sat")])

    def test_full_21_slot_sorting(self):
        pool = [RenderEntity(i, 1, (i * 17) % 250) for i in range(21)]
        order = sort_group_2_depth(pool)
        self.assertEqual(len(order), 21)
        # Verify descending order of Y
        y_values = [pool[slot].y for slot, _ in order]
        self.assertEqual(y_values, sorted(y_values, reverse=True))


if __name__ == "__main__":
    unittest.main()
