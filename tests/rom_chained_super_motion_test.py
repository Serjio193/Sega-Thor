#!/usr/bin/env python3
"""Unit tests for M15.358 ROM chained F-B-F + 360-spin super-motion recognizer."""

import unittest
from pathlib import Path

from tools.extract_rom_chained_super_motion import extract_evidence
from tools.extract_rom_command_motions import extract_evidence as extract_cmd_evidence


def detect_chained_super_motion(
    samples: list[int],
    facing: int,
    fbf_patterns: list[dict],
    spin_lut: list[int],
) -> bool:
    """Mirror 0x008814..0x00890E: F-B-F followed seamlessly by 360 spin."""
    if not samples:
        return False
    perp = (1, 2) if (facing & 1) else (4, 8)
    pat = fbf_patterns[facing]
    seq = pat["sequence"]
    masks = pat["masks"]
    pat_idx = 0
    idx = 0

    while idx < len(samples):
        hi = (samples[idx] >> 8) & 0xFF
        if hi == perp[0] or hi == perp[1]:
            return False
        lo = samples[idx] & 0xFF
        if (lo & masks[pat_idx]) == seq[pat_idx]:
            pat_idx += 1
            if seq[pat_idx] == 0:
                break
        idx += 1
    else:
        return False

    stage = 0
    expected = 0
    step_dir = 0
    while idx < len(samples):
        d = spin_lut[samples[idx] & 0x0F]
        if d >= 0 and (d & 1) == 0:
            if stage == 0:
                expected = d
                stage = 1
            elif stage == 1:
                cw = (expected + 2) & 7
                ccw = (expected - 2) & 7
                if d == cw:
                    step_dir = 2
                    stage = 2
                    expected = (cw + 2) & 7
                elif d == ccw:
                    step_dir = -2
                    stage = 2
                    expected = (ccw - 2) & 7
            elif stage == 2:
                if d == expected:
                    stage = 3
                    expected = (expected + step_dir) & 7
            else:
                if d == expected:
                    return True
        idx += 1
    return False


class ChainedSuperMotionTest(unittest.TestCase):
    def test_rom_opcodes(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertTrue(ev["ring_buffer"]["uses_full_ring_count"])
        self.assertTrue(ev["phase_2_spin_360"]["continues_remaining_count"])

    def test_chained_fbf_then_spin_and_perp_abort(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        cmd_ev = extract_cmd_evidence(rom_bytes)
        pats = cmd_ev["fbf_dash_recognizer"]["patterns"]
        lut = cmd_ev["spin_360_recognizer"]["pad_to_dir8_lut"]

        # Facing Right (1): Right(8) -> Left(4) -> Right(8), then Down(2) -> Left(4) -> Up(1)
        seq = [8, 4, 8, 2, 4, 1]
        self.assertTrue(detect_chained_super_motion(seq, 1, pats, lut))
        # Perpendicular Up edge (0x0100) aborts Phase 1
        self.assertFalse(detect_chained_super_motion([8, 0x0104, 8, 2, 4, 1], 1, pats, lut))


if __name__ == "__main__":
    unittest.main()
