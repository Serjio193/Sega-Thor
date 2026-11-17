#!/usr/bin/env python3
"""Unit tests for ROM shadow & special effects SAT composition pass (0x00A6A0..0x00A7E6)."""

import unittest
from dataclasses import dataclass
from typing import List, Tuple


@dataclass
class ShadowEntity:
    slot_id: int
    entity_type: int
    y: int
    flags_3a: int = 0
    flags_36: int = 0


def filter_and_sort_shadows(entities: List[ShadowEntity]) -> List[Tuple[int, str]]:
    """Simulates 0x00A6C6..0x00A738 shadow filtering and descending-Y depth sorting."""
    assert len(entities) == 21

    # Pass 1: Build inactive mask in d7 (marks inactive type or missing shadow bit 2)
    d7 = 0
    for i in range(21):
        ent = entities[i]
        if ent.entity_type <= 0 or not (ent.flags_3a & 0x04):
            d7 |= (1 << i)

    render_order = []

    # Pass 2: Selection sort active shadows by descending Y
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


class RomShadowSatPassTest(unittest.TestCase):
    def test_entities_without_shadow_flag_are_skipped(self):
        pool = [ShadowEntity(i, 1, 100, flags_3a=0) for i in range(21)]
        order = filter_and_sort_shadows(pool)
        self.assertEqual(len(order), 0)

    def test_entities_with_shadow_flag_are_included(self):
        pool = [ShadowEntity(i, 1, 100, flags_3a=0) for i in range(21)]
        pool[4] = ShadowEntity(4, 1, 150, flags_3a=0x04)
        order = filter_and_sort_shadows(pool)
        self.assertEqual(order, [(4, "normal_sat")])

    def test_shadow_descending_y_sorting(self):
        pool = [ShadowEntity(i, 1, 0, flags_3a=0) for i in range(21)]
        pool[1] = ShadowEntity(1, 1, 100, flags_3a=0x04)
        pool[6] = ShadowEntity(6, 1, 250, flags_3a=0x04)
        pool[12] = ShadowEntity(12, 1, 180, flags_3a=0x04)

        order = filter_and_sort_shadows(pool)
        self.assertEqual([slot for slot, _ in order], [6, 12, 1])

    def test_custom_sat_target_for_shadow(self):
        pool = [ShadowEntity(i, 1, 0, flags_3a=0) for i in range(21)]
        pool[5] = ShadowEntity(5, 1, 100, flags_3a=0x04, flags_36=1)
        order = filter_and_sort_shadows(pool)
        self.assertEqual(order, [(5, "custom_sat")])


if __name__ == "__main__":
    unittest.main()
