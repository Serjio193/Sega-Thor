#!/usr/bin/env python3
"""Unit tests for M15.371 ROM Ali airborne knockdown physics & landing recovery."""

import unittest
from pathlib import Path

from tools.extract_rom_ali_knockdown_physics import extract_evidence


def step_knockdown_airborne(
    z: int,
    vz: int,
    vx: int,
    vy: int,
    x_wall: bool,
    y_wall: bool,
) -> tuple[int, int, int, int, bool]:
    """Mirror 0x005CF8..0x005D5A: returns (z, vz, vx, vy, landed)."""
    if x_wall:
        vx = 0
    if y_wall:
        vy = 0
    vz -= 0x0000C000
    if vz == 0:
        vz -= 1
    z += vz
    if z < 0:
        return 0, vz, vx, vy, True
    return z, vz, vx, vy, False


def resolve_landing_recovery(
    is_dead: bool,
    anim_id: int,
    heavy_stun: bool,
) -> tuple[int, int, int]:
    """Mirror 0x005D5C..0x005DC6 -> (state, anim, recovery_timer)."""
    if is_dead:
        anim = 0x68 if anim_id == 0x26 else 0x6C
        return 0x5E, anim, 0
    timer = 20 if heavy_stun else 10
    if anim_id != 0x28:
        timer += 10
    return 0x1C, anim_id + 2, timer


class AliKnockdownPhysicsTest(unittest.TestCase):
    def test_rom_constants(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(ev["prone_height"], 16)
        self.assertEqual(ev["wall_impact"]["sfx_id"], "0x0080035D")
        self.assertEqual(ev["airborne_gravity"]["gravity_step"], -0xC000)
        self.assertEqual(ev["death_transition"]["death_state"], "0x5E")

    def test_airborne_step_and_landing_recovery(self) -> None:
        z, vz, vx, vy, landed = step_knockdown_airborne(0x20000, 0x10000, 100, 200, True, False)
        self.assertFalse(landed)
        self.assertEqual(vx, 0)
        self.assertEqual(vy, 200)
        self.assertEqual(vz, 0x4000)
        self.assertEqual(z, 0x24000)

        # Ground impact
        z, vz, vx, vy, landed = step_knockdown_airborne(0x2000, -0x5000, 100, 200, False, False)
        self.assertTrue(landed)
        self.assertEqual(z, 0)

        # Recovery timers
        self.assertEqual(resolve_landing_recovery(False, 0x28, False), (0x1C, 0x2A, 10))
        self.assertEqual(resolve_landing_recovery(False, 0x26, True), (0x1C, 0x28, 30))
        self.assertEqual(resolve_landing_recovery(True, 0x26, False), (0x5E, 0x68, 0))


if __name__ == "__main__":
    unittest.main()
