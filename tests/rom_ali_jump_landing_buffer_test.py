#!/usr/bin/env python3
"""Unit tests for M15.372 ROM Ali jump arc, landing & post-landing command buffer."""

import unittest
from pathlib import Path

from tools.extract_rom_ali_jump_landing_buffer import COMMAND_MOTION_PRIORITY, extract_evidence


def step_jump_arc(z: int, vz: int) -> tuple[int, int, bool, bool]:
    """Mirror 0x005E06..0x005E80 -> (new_z, new_vz, apex_hit, landed)."""
    vz -= 0x0000C000
    apex = False
    if vz <= 0:
        apex = True
        if vz == 0:
            vz -= 1
    z += vz
    if z <= 0:
        return 0, 0, apex, True
    return z, vz, apex, False


def select_post_landing_action(attack_pressed: bool, motion_detected: str | None) -> str:
    """Mirror 0x005F14..0x005F6E command-motion priority cascade."""
    if not attack_pressed:
        return "idle_walk"
    for m in COMMAND_MOTION_PRIORITY:
        if motion_detected == m["name"]:
            return m["target"]
    return "idle_walk"


class AliJumpLandingBufferTest(unittest.TestCase):
    def test_rom_motion_priority_table(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(len(ev["post_landing_buffer"]["command_motions"]), 5)
        self.assertEqual(ev["post_landing_buffer"]["command_motions"][0]["name"], "chained_fbf_360")
        self.assertEqual(ev["post_landing_buffer"]["command_motions"][1]["name"], "spin_360")
        self.assertEqual(ev["post_landing_buffer"]["command_motions"][2]["name"], "fbf_dash")
        self.assertEqual(ev["post_landing_buffer"]["command_motions"][3]["name"], "rapid_mash")
        self.assertEqual(ev["post_landing_buffer"]["command_motions"][4]["name"], "triple_tap")

    def test_jump_arc_and_landing_buffer(self) -> None:
        z, vz = 0, 0x50000
        z, vz, apex, landed = step_jump_arc(z, vz)
        self.assertEqual(z, 0x44000)
        self.assertFalse(apex)
        self.assertFalse(landed)

        self.assertEqual(select_post_landing_action(True, "spin_360"), "0x00770C")
        self.assertEqual(select_post_landing_action(True, "chained_fbf_360"), "0x00776C")
        self.assertEqual(select_post_landing_action(False, "spin_360"), "idle_walk")
        self.assertEqual(select_post_landing_action(True, None), "idle_walk")


if __name__ == "__main__":
    unittest.main()
