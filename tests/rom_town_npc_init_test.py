#!/usr/bin/env python3
"""
Unit tests for ROM Town NPC Spawn Initializer (20 Variants, 0x018072..0x018538).
Milestone M15.526.
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
    / "rom_town_npc_init_m15_526.json"
)


# ==============================================================================
# Emulation Routine
# ==============================================================================

def init_town_npc_spawn(
    npc: dict,
    flag_dbf_bit0: bool = False,
    flag_dbc_bit0: bool = False,
    flag_dc5_bit3: bool = False,
) -> bool:
    """
    Emulates 0x018072..0x018538 Town NPC Spawn Initializer.
    Returns True if initialized (and falls through to active tick),
    or False if +$66 == 0 (jumps directly to active tick 0x018538).
    """
    variant = npc.get("submode_66", 0)
    if variant == 0:
        return False

    # Condition A: ($FF0DBF:0 == 1) or ($FF0DBC:0 == 0)
    branch_a = flag_dbf_bit0 or (not flag_dbc_bit0)

    def set_box(xmin: int, ymin: int, xmax: int, ymax: int) -> None:
        npc["xmin_a6"] = xmin
        npc["ymin_a8"] = ymin
        npc["xmax_aa"] = xmax
        npc["ymax_ac"] = ymax

    v = variant if 1 <= variant <= 20 else 1

    if v == 1:
        npc["height_4a"] = 0x14
        npc["anim_2a"] = 6
        if branch_a:
            set_box(0x00E8, 0x01A8, 0x01A8, 0x0210)
            npc["dialog_a0"] = 0x0001
        else:
            set_box(0x0048, 0x01C0, 0x0160, 0x02C0)
            npc["dialog_a0"] = 0x0055

    elif v == 2:
        npc["anim_2a"] = 0x0A
        if branch_a:
            set_box(0x0040, 0x0188, 0x0098, 0x01A8)
            npc["dialog_a0"] = 0x0002
        else:
            set_box(0x0050, 0x0188, 0x0138, 0x01E0)
            npc["dialog_a0"] = 0x0056

    elif v == 3:
        npc["anim_2a"] = 0x0E
        if branch_a:
            set_box(0x0070, 0x0288, 0x0120, 0x02E0)
            npc["dialog_a0"] = 0x0003
        else:
            set_box(0x0020, 0x01D8, 0x0178, 0x02E0)
            npc["dialog_a0"] = 0x0057

    elif v == 4:
        npc["anim_2a"] = 0x12
        if branch_a:
            set_box(0x00C8, 0x01A8, 0x00E8, 0x0240)
            npc["dialog_a0"] = 0x0004
        else:
            set_box(0x00D0, 0x00F8, 0x0170, 0x01F0)
            npc["dialog_a0"] = 0x0058

    elif v == 5:
        npc["height_4a"] = 0x10
        npc["flag_3c"] = npc.get("flag_3c", 0) | (1 << 2)
        npc["flag_37"] = npc.get("flag_37", 0) | (1 << 6) | (1 << 7)
        npc["anim_2a"] = 0x14
        dlg = 0x0005
        if flag_dbf_bit0:
            dlg = 0x0059
            if flag_dc5_bit3:
                dlg = 0x005D
        npc["dialog_a0"] = dlg

    elif v == 6:
        npc["height_4a"] = 0x16
        npc["flag_37"] = npc.get("flag_37", 0) | (1 << 6) | (1 << 7)
        npc["anim_2a"] = 0x16
        npc["facing_16"] = 3
        npc["dialog_a0"] = 0x0006

    elif v == 7:
        npc["flag_38"] = npc.get("flag_38", 0) | (1 << 2) | (1 << 0)
        npc["flag_36"] = npc.get("flag_36", 0) | (1 << 2)
        npc["shadow_3d"] = 0
        npc["dialog_a0"] = 0
        npc["anim_2a"] = 0x1E

    elif v == 8:
        npc["height_4a"] = 8
        npc["dialog_a0"] = 0
        npc["anim_2a"] = 0x20
        if branch_a:
            set_box(0x00C8, 0x01D0, 0x01A0, 0x0228)
        else:
            set_box(0x0020, 0x00C0, 0x0170, 0x0238)

    elif v == 9:
        npc["anim_2a"] = 0x28
        npc["z_10"] = 0x0050
        npc["flag_38"] = npc.get("flag_38", 0) | (1 << 2)
        npc["flag_36"] = npc.get("flag_36", 0) | (1 << 3)
        npc["dialog_a0"] = 0

    elif v == 10:
        npc["height_4a"] = 0x14
        npc["anim_2a"] = 6
        if branch_a:
            set_box(0x00E0, 0x01D0, 0x0168, 0x02A8)
            npc["dialog_a0"] = 0x000A
        else:
            set_box(0x0010, 0x0088, 0x0130, 0x0168)
            npc["dialog_a0"] = 0x005B

    elif v == 11:
        npc["anim_2a"] = 0x0E
        if branch_a:
            set_box(0x00D0, 0x0100, 0x0138, 0x01A0)
            npc["dialog_a0"] = 0x000B
        else:
            set_box(0x00D0, 0x0088, 0x01D8, 0x01E0)
            npc["dialog_a0"] = 0x005C

    elif v == 12:
        npc["height_4a"] = 0x14
        npc["anim_2a"] = 6
        set_box(0x00B8, 0x0218, 0x0150, 0x03F0)
        npc["dialog_a0"] = 0x0029

    elif v == 13:
        npc["height_4a"] = 0x14
        npc["anim_2a"] = 6
        set_box(0x0070, 0x0320, 0x0190, 0x03F0)
        npc["dialog_a0"] = 0x0029

    elif v == 14:
        npc["anim_2a"] = 0x12
        set_box(0x0038, 0x0208, 0x0118, 0x02C8)
        npc["dialog_a0"] = 0x002D

    elif v == 15:
        npc["anim_2a"] = 0x0E
        set_box(0x00D0, 0x02E8, 0x0190, 0x03F0)
        npc["dialog_a0"] = 0x002C

    elif v == 16:
        npc["anim_2a"] = 0x0A
        set_box(0x00C8, 0x0218, 0x0138, 0x03C0)
        npc["dialog_a0"] = 0x002B

    elif v == 17:
        npc["anim_2a"] = 0x0A
        set_box(0x0118, 0x0210, 0x01B0, 0x0268)
        npc["dialog_a0"] = 0x002A

    elif v == 18:
        npc["anim_2a"] = 0x0A
        npc["facing_16"] = 2
        npc["dialog_a0"] = 0x0031
        npc["flag_37"] = npc.get("flag_37", 0) | (1 << 6)

    elif v == 19:
        npc["anim_2a"] = 0x0E
        npc["facing_16"] = 2
        npc["dialog_a0"] = 0x0032
        npc["flag_37"] = npc.get("flag_37", 0) | (1 << 6)

    elif v == 20:
        npc["height_4a"] = 0x14
        npc["anim_2a"] = 6
        npc["facing_16"] = 2
        npc["dialog_a0"] = 0x0033
        npc["flag_37"] = npc.get("flag_37", 0) | (1 << 6)

    # Common Epilogue (0x018526..0x018538)
    npc["variant_saved_9e"] = variant
    npc["submode_66"] = 0
    npc["timer_2"] = 0
    return True


# ==============================================================================
# Unit Tests
# ==============================================================================

class TestROMTownNPCInit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)

    def test_evidence_structure_and_counts(self):
        meta = self.evidence["metadata"]
        self.assertEqual(meta["milestone"], "M15.526")
        self.assertEqual(meta["range_start"], "0x018072")
        self.assertEqual(meta["range_end"], "0x018538")
        self.assertEqual(meta["range_size_bytes"], 1222)
        self.assertEqual(meta["instruction_count"], 274)
        self.assertEqual(meta["routine_count"], 1)
        self.assertEqual(meta["variant_count"], 20)

        routine = self.evidence["routines"][0]
        self.assertEqual(routine["name"], "AI_TownNPC_SpawnInitializer")
        self.assertEqual(routine["instruction_count"], 274)
        self.assertEqual(routine["size_bytes"], 1222)
        self.assertEqual(len(self.evidence["variants"]), 20)

    def test_submode_0_branches_to_active_tick(self):
        npc = {"submode_66": 0}
        did_init = init_town_npc_spawn(npc)
        self.assertFalse(did_init)
        self.assertNotIn("variant_saved_9e", npc)

    def test_variants_1_to_4_and_10_11_story_flag_branching(self):
        cases = [
            # (variant, anim, height, box_a, dlg_a, box_b, dlg_b)
            (1,  6,    0x14, (0x00E8, 0x01A8, 0x01A8, 0x0210), 0x0001, (0x0048, 0x01C0, 0x0160, 0x02C0), 0x0055),
            (2,  0x0A, None, (0x0040, 0x0188, 0x0098, 0x01A8), 0x0002, (0x0050, 0x0188, 0x0138, 0x01E0), 0x0056),
            (3,  0x0E, None, (0x0070, 0x0288, 0x0120, 0x02E0), 0x0003, (0x0020, 0x01D8, 0x0178, 0x02E0), 0x0057),
            (4,  0x12, None, (0x00C8, 0x01A8, 0x00E8, 0x0240), 0x0004, (0x00D0, 0x00F8, 0x0170, 0x01F0), 0x0058),
            (10, 6,    0x14, (0x00E0, 0x01D0, 0x0168, 0x02A8), 0x000A, (0x0010, 0x0088, 0x0130, 0x0168), 0x005B),
            (11, 0x0E, None, (0x00D0, 0x0100, 0x0138, 0x01A0), 0x000B, (0x00D0, 0x0088, 0x01D8, 0x01E0), 0x005C),
        ]

        for vid, anim, h, box_a, dlg_a, box_b, dlg_b in cases:
            # Branch A: dbf=False, dbc=False -> !dbc is True -> Branch A
            npc_a = {"submode_66": vid}
            self.assertTrue(init_town_npc_spawn(npc_a, flag_dbf_bit0=False, flag_dbc_bit0=False))
            self.assertEqual(npc_a["anim_2a"], anim)
            if h is not None:
                self.assertEqual(npc_a["height_4a"], h)
            self.assertEqual((npc_a["xmin_a6"], npc_a["ymin_a8"], npc_a["xmax_aa"], npc_a["ymax_ac"]), box_a)
            self.assertEqual(npc_a["dialog_a0"], dlg_a)
            self.assertEqual(npc_a["variant_saved_9e"], vid)
            self.assertEqual(npc_a["submode_66"], 0)

            # Branch B: dbf=False, dbc=True -> Branch B
            npc_b = {"submode_66": vid}
            self.assertTrue(init_town_npc_spawn(npc_b, flag_dbf_bit0=False, flag_dbc_bit0=True))
            self.assertEqual((npc_b["xmin_a6"], npc_b["ymin_a8"], npc_b["xmax_aa"], npc_b["ymax_ac"]), box_b)
            self.assertEqual(npc_b["dialog_a0"], dlg_b)

            # Branch A override when dbf=True even if dbc=True
            npc_ovr = {"submode_66": vid}
            self.assertTrue(init_town_npc_spawn(npc_ovr, flag_dbf_bit0=True, flag_dbc_bit0=True))
            self.assertEqual(npc_ovr["dialog_a0"], dlg_a)

    def test_variant_5_three_stage_dialog_progression(self):
        # Stage 1: dbf=False -> dialog 0x05
        npc1 = {"submode_66": 5}
        init_town_npc_spawn(npc1, flag_dbf_bit0=False, flag_dc5_bit3=False)
        self.assertEqual(npc1["height_4a"], 0x10)
        self.assertEqual(npc1["anim_2a"], 0x14)
        self.assertTrue(npc1["flag_3c"] & (1 << 2))
        self.assertEqual(npc1["flag_37"] & 0xC0, 0xC0)
        self.assertEqual(npc1["dialog_a0"], 0x0005)

        # Stage 2: dbf=True, dc5=False -> dialog 0x59
        npc2 = {"submode_66": 5}
        init_town_npc_spawn(npc2, flag_dbf_bit0=True, flag_dc5_bit3=False)
        self.assertEqual(npc2["dialog_a0"], 0x0059)

        # Stage 3: dbf=True, dc5=True -> dialog 0x5D
        npc3 = {"submode_66": 5}
        init_town_npc_spawn(npc3, flag_dbf_bit0=True, flag_dc5_bit3=True)
        self.assertEqual(npc3["dialog_a0"], 0x005D)

    def test_variants_6_7_8_9_static_and_prop_npcs(self):
        # Variant 6
        npc6 = {"submode_66": 6}
        init_town_npc_spawn(npc6)
        self.assertEqual(npc6["height_4a"], 0x16)
        self.assertEqual(npc6["anim_2a"], 0x16)
        self.assertEqual(npc6["facing_16"], 3)
        self.assertEqual(npc6["dialog_a0"], 0x0006)
        self.assertEqual(npc6["flag_37"] & 0xC0, 0xC0)

        # Variant 7
        npc7 = {"submode_66": 7}
        init_town_npc_spawn(npc7)
        self.assertEqual(npc7["anim_2a"], 0x1E)
        self.assertEqual(npc7["dialog_a0"], 0)
        self.assertEqual(npc7["shadow_3d"], 0)
        self.assertEqual(npc7["flag_38"] & 0x05, 0x05)
        self.assertTrue(npc7["flag_36"] & (1 << 2))

        # Variant 8 (Branch A & B)
        npc8a = {"submode_66": 8}
        init_town_npc_spawn(npc8a, flag_dbf_bit0=True)
        self.assertEqual(npc8a["height_4a"], 8)
        self.assertEqual(npc8a["anim_2a"], 0x20)
        self.assertEqual(npc8a["dialog_a0"], 0)
        self.assertEqual((npc8a["xmin_a6"], npc8a["ymin_a8"], npc8a["xmax_aa"], npc8a["ymax_ac"]), (0x00C8, 0x01D0, 0x01A0, 0x0228))

        npc8b = {"submode_66": 8}
        init_town_npc_spawn(npc8b, flag_dbf_bit0=False, flag_dbc_bit0=True)
        self.assertEqual((npc8b["xmin_a6"], npc8b["ymin_a8"], npc8b["xmax_aa"], npc8b["ymax_ac"]), (0x0020, 0x00C0, 0x0170, 0x0238))

        # Variant 9
        npc9 = {"submode_66": 9}
        init_town_npc_spawn(npc9)
        self.assertEqual(npc9["anim_2a"], 0x28)
        self.assertEqual(npc9["z_10"], 0x0050)
        self.assertEqual(npc9["dialog_a0"], 0)
        self.assertTrue(npc9["flag_38"] & (1 << 2))
        self.assertTrue(npc9["flag_36"] & (1 << 3))

    def test_variants_12_to_17_fixed_patrol_boxes(self):
        cases = [
            (12, 6,    0x14, (0x00B8, 0x0218, 0x0150, 0x03F0), 0x0029),
            (13, 6,    0x14, (0x0070, 0x0320, 0x0190, 0x03F0), 0x0029),
            (14, 0x12, None, (0x0038, 0x0208, 0x0118, 0x02C8), 0x002D),
            (15, 0x0E, None, (0x00D0, 0x02E8, 0x0190, 0x03F0), 0x002C),
            (16, 0x0A, None, (0x00C8, 0x0218, 0x0138, 0x03C0), 0x002B),
            (17, 0x0A, None, (0x0118, 0x0210, 0x01B0, 0x0268), 0x002A),
        ]
        for vid, anim, h, box, dlg in cases:
            npc = {"submode_66": vid}
            init_town_npc_spawn(npc)
            self.assertEqual(npc["anim_2a"], anim)
            if h is not None:
                self.assertEqual(npc["height_4a"], h)
            self.assertEqual((npc["xmin_a6"], npc["ymin_a8"], npc["xmax_aa"], npc["ymax_ac"]), box)
            self.assertEqual(npc["dialog_a0"], dlg)
            self.assertEqual(npc["variant_saved_9e"], vid)

    def test_variants_18_to_20_stationary_facing_down(self):
        cases = [
            (18, 0x0A, None, 0x0031),
            (19, 0x0E, None, 0x0032),
            (20, 6,    0x14, 0x0033),
        ]
        for vid, anim, h, dlg in cases:
            npc = {"submode_66": vid}
            init_town_npc_spawn(npc)
            self.assertEqual(npc["anim_2a"], anim)
            if h is not None:
                self.assertEqual(npc["height_4a"], h)
            self.assertEqual(npc["facing_16"], 2)
            self.assertEqual(npc["dialog_a0"], dlg)
            self.assertTrue(npc["flag_37"] & (1 << 6))
            self.assertEqual(npc["variant_saved_9e"], vid)

    def test_out_of_range_variant_falls_through_to_variant_1(self):
        npc = {"submode_66": 25, "timer_2": 99}
        init_town_npc_spawn(npc, flag_dbf_bit0=True)
        self.assertEqual(npc["anim_2a"], 6)
        self.assertEqual(npc["dialog_a0"], 0x0001)
        self.assertEqual(npc["variant_saved_9e"], 25)
        self.assertEqual(npc["submode_66"], 0)
        self.assertEqual(npc["timer_2"], 0)


if __name__ == "__main__":
    unittest.main()
