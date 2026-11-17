#!/usr/bin/env python3
"""
rom_ali_dpad_slide_recognizer_test.py — M15.392: Unit tests for ROM Ali 8-Direction D-Pad & Slide Recognizer
Tests:
  1. test_dpad_8way_velocity_resolver
  2. test_slide_motion_rotational_sequence_success
  3. test_slide_motion_rejection
"""

import unittest

DIRECTION_LUT = [0xFF, 0, 4, 0xFF, 6, 7, 5, 0xFF, 2, 1, 3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]


def resolve_dpad_8way(dpad_bits):
    # 0x0085E2..0x0086B2
    val = dpad_bits & 0x0F
    if val == 0 or val in (3, 7, 0x0B, 0x0C, 0x0F):
        return {"vx": 0, "vy": 0, "moving": False}
    elif val == 1:  # Up
        return {"vx": 0, "vy": -0x30000, "facing": 0, "moving": True}
    elif val == 2 or val == 0x0E:  # Down
        return {"vx": 0, "vy": 0x30000, "facing": 2, "moving": True}
    elif val == 4:  # Left
        return {"vx": -0x36000, "vy": 0, "facing": 3, "moving": True}
    elif val == 8:  # Right
        return {"vx": 0x36000, "vy": 0, "facing": 1, "moving": True}
    elif val == 0x0A:  # Down-Right
        return {"vx": 0x2A000, "vy": 0x25800, "facing": 2, "moving": True}
    elif val == 9 or val == 0x0D:  # Up-Right
        return {"vx": 0x2A000, "vy": -0x25800, "facing": 0, "moving": True}
    elif val == 6:  # Down-Left
        return {"vx": -0x2A000, "vy": 0x25800, "facing": 2, "moving": True}
    elif val == 5:  # Up-Left
        return {"vx": -0x2A000, "vy": -0x25800, "facing": 0, "moving": True}
    return {"vx": 0, "vy": 0, "moving": False}


def recognize_slide_motion(ring_words, head_offset, frame_count):
    # 0x0086B4..0x008766: Rotational sequence scanner
    if frame_count == 0:
        return False
    frames_to_check = min(30, frame_count)
    head = head_offset
    d1 = 0  # stage
    d2 = 0  # previous direction
    d3 = 0  # step direction (+2 or -2)

    for _ in range(frames_to_check):
        w = ring_words[(head & 0x7E) // 2] & 0x0F
        d0 = DIRECTION_LUT[w]
        if d0 != 0xFF and (d0 & 1) == 0:
            if d1 == 0:
                d2 = d0
                d1 = 1
            elif d1 == 1:
                # Check clockwise (+2) or counter-clockwise (-2)
                step_cw = (d2 + 2) & 7
                step_ccw = (d2 - 2) & 7
                if d0 == step_cw:
                    d3 = 2
                    d2 = (d2 + 2) & 7
                    d1 = 2
                elif d0 == step_ccw:
                    d3 = -2
                    d2 = (d2 - 2) & 7
                    d1 = 2
            elif d1 == 2:
                next_step = (d2 + d3) & 7
                if d0 == next_step:
                    d2 = (d2 + d3) & 7
                    d1 = 3
            elif d1 == 3:
                next_step = (d2 + d3) & 7
                if d0 == next_step:
                    # Match!
                    return True
        head = (head + 2) & 0x7E
    return False


class TestRomAliDpadSlideRecognizer(unittest.TestCase):
    def test_dpad_8way_velocity_resolver(self):
        # Neutral
        neut = resolve_dpad_8way(0)
        self.assertFalse(neut["moving"])
        self.assertEqual(neut["vx"], 0)
        self.assertEqual(neut["vy"], 0)

        # Cardinal: Up, Down, Left, Right
        self.assertEqual(resolve_dpad_8way(1), {"vx": 0, "vy": -0x30000, "facing": 0, "moving": True})
        self.assertEqual(resolve_dpad_8way(2), {"vx": 0, "vy": 0x30000, "facing": 2, "moving": True})
        self.assertEqual(resolve_dpad_8way(4), {"vx": -0x36000, "vy": 0, "facing": 3, "moving": True})
        self.assertEqual(resolve_dpad_8way(8), {"vx": 0x36000, "vy": 0, "facing": 1, "moving": True})

        # Diagonals
        self.assertEqual(resolve_dpad_8way(0x0A), {"vx": 0x2A000, "vy": 0x25800, "facing": 2, "moving": True})
        self.assertEqual(resolve_dpad_8way(9), {"vx": 0x2A000, "vy": -0x25800, "facing": 0, "moving": True})
        self.assertEqual(resolve_dpad_8way(6), {"vx": -0x2A000, "vy": 0x25800, "facing": 2, "moving": True})
        self.assertEqual(resolve_dpad_8way(5), {"vx": -0x2A000, "vy": -0x25800, "facing": 0, "moving": True})

    def test_slide_motion_rotational_sequence_success(self):
        # 4 quarter-turns: Up (1 -> code 0) -> Right (8 -> code 2) -> Down (2 -> code 4) -> Left (4 -> code 6)
        ring = [0] * 64
        ring[0] = 1  # Up (0)
        ring[1] = 8  # Right (2)
        ring[2] = 2  # Down (4)
        ring[3] = 4  # Left (6)
        matched_cw = recognize_slide_motion(ring, head_offset=0, frame_count=10)
        self.assertTrue(matched_cw)

        # Counter-clockwise: Up (0) -> Left (6) -> Down (4) -> Right (2)
        ring_ccw = [0] * 64
        ring_ccw[0] = 1  # Up (0)
        ring_ccw[1] = 4  # Left (6)
        ring_ccw[2] = 2  # Down (4)
        ring_ccw[3] = 8  # Right (2)
        matched_ccw = recognize_slide_motion(ring_ccw, head_offset=0, frame_count=10)
        self.assertTrue(matched_ccw)

    def test_slide_motion_rejection(self):
        # Incomplete 2-step rotation
        ring_short = [0] * 64
        ring_short[0] = 1
        ring_short[1] = 8
        self.assertFalse(recognize_slide_motion(ring_short, head_offset=0, frame_count=5))

        # Reversal without rotation
        ring_rev = [0] * 64
        ring_rev[0] = 1
        ring_rev[1] = 2
        self.assertFalse(recognize_slide_motion(ring_rev, head_offset=0, frame_count=5))


if __name__ == "__main__":
    unittest.main()
