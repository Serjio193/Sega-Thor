#!/usr/bin/env python3
"""
rom_ali_somersault_dash_leap_test.py — M15.393: Unit tests for ROM Ali Somersault & Dash Leap Recognizers
Tests:
  1. test_somersault_motion_all_facings
  2. test_somersault_motion_rejection
  3. test_compound_dash_leap_recognizer
"""

import unittest

SOMERSAULT_TABLE = {
    0: [1, 2, 1],  # Up -> Down -> Up
    1: [8, 4, 8],  # Right -> Left -> Right
    2: [2, 1, 2],  # Down -> Up -> Down
    3: [4, 8, 4],  # Left -> Right -> Left
}

DIRECTION_LUT = [0xFF, 0, 4, 0xFF, 6, 7, 5, 0xFF, 2, 1, 3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]


def recognize_somersault(ring_words, head_offset, frame_count, facing):
    # 0x008778..0x0087F2
    expected_steps = SOMERSAULT_TABLE.get(facing % 4, [0, 0, 0])
    step = 0
    head = head_offset
    frames = min(20, frame_count)
    for _ in range(frames):
        w = ring_words[(head & 0x7E) // 2] & 0x0F
        if w == expected_steps[step]:
            step += 1
            if step == len(expected_steps):
                return True
        head = (head + 2) & 0x7E
    return False


def recognize_compound_dash_leap(ring_words, head_offset, frame_count, facing):
    # 0x008814..0x00890E: Lead-in then rotational sequence
    expected_lead = SOMERSAULT_TABLE.get(facing % 4, [0, 0, 0])
    step = 0
    head = head_offset
    frames = min(30, frame_count)
    lead_in_done = False
    rot_stage = 0
    rot_prev = 0
    rot_dir = 0

    for _ in range(frames):
        w = ring_words[(head & 0x7E) // 2] & 0x0F
        if not lead_in_done:
            if w == expected_lead[step]:
                step += 1
                if step == len(expected_lead):
                    lead_in_done = True
        else:
            # Rotational phase (0x00887E)
            d0 = DIRECTION_LUT[w]
            if d0 != 0xFF and (d0 & 1) == 0:
                if rot_stage == 0:
                    rot_prev = d0
                    rot_stage = 1
                elif rot_stage == 1:
                    step_cw = (rot_prev + 2) & 7
                    step_ccw = (rot_prev - 2) & 7
                    if d0 == step_cw:
                        rot_dir = 2
                        rot_prev = (rot_prev + 2) & 7
                        rot_stage = 2
                    elif d0 == step_ccw:
                        rot_dir = -2
                        rot_prev = (rot_prev - 2) & 7
                        rot_stage = 2
                elif rot_stage == 2:
                    if d0 == (rot_prev + rot_dir) & 7:
                        rot_prev = (rot_prev + rot_dir) & 7
                        rot_stage = 3
                elif rot_stage == 3:
                    if d0 == (rot_prev + rot_dir) & 7:
                        return True
        head = (head + 2) & 0x7E
    return False


class TestRomAliSomersaultDashLeap(unittest.TestCase):
    def test_somersault_motion_all_facings(self):
        # Facing 0: Up (1) -> Down (2) -> Up (1)
        ring0 = [0] * 64
        ring0[0], ring0[1], ring0[2] = 1, 2, 1
        self.assertTrue(recognize_somersault(ring0, 0, 5, facing=0))

        # Facing 1: Right (8) -> Left (4) -> Right (8)
        ring1 = [0] * 64
        ring1[0], ring1[1], ring1[2] = 8, 4, 8
        self.assertTrue(recognize_somersault(ring1, 0, 5, facing=1))

        # Facing 2: Down (2) -> Up (1) -> Down (2)
        ring2 = [0] * 64
        ring2[0], ring2[1], ring2[2] = 2, 1, 2
        self.assertTrue(recognize_somersault(ring2, 0, 5, facing=2))

        # Facing 3: Left (4) -> Right (8) -> Left (4)
        ring3 = [0] * 64
        ring3[0], ring3[1], ring3[2] = 4, 8, 4
        self.assertTrue(recognize_somersault(ring3, 0, 5, facing=3))

    def test_somersault_motion_rejection(self):
        # Incomplete sequence
        ring = [0] * 64
        ring[0], ring[1] = 8, 4
        self.assertFalse(recognize_somersault(ring, 0, 5, facing=1))

        # Wrong direction
        ring_wrong = [0] * 64
        ring_wrong[0], ring_wrong[1], ring_wrong[2] = 8, 1, 8
        self.assertFalse(recognize_somersault(ring_wrong, 0, 5, facing=1))

    def test_compound_dash_leap_recognizer(self):
        # Lead-in (Right -> Left -> Right) followed by clockwise rotation (Up -> Right -> Down -> Left)
        ring = [0] * 64
        ring[0], ring[1], ring[2] = 8, 4, 8  # Lead-in
        ring[3] = 1  # Up (0)
        ring[4] = 8  # Right (2)
        ring[5] = 2  # Down (4)
        ring[6] = 4  # Left (6)
        self.assertTrue(recognize_compound_dash_leap(ring, 0, 10, facing=1))

        # Failed rotation after lead-in
        ring_fail = [0] * 64
        ring_fail[0], ring_fail[1], ring_fail[2] = 8, 4, 8
        ring_fail[3] = 1
        ring_fail[4] = 2
        self.assertFalse(recognize_compound_dash_leap(ring_fail, 0, 10, facing=1))


if __name__ == "__main__":
    unittest.main()
