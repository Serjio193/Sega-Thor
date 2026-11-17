#!/usr/bin/env python3
"""Unit tests for M15.363 ROM entity animation keyframe stepper & velocity damper."""

import unittest
from pathlib import Path

from tools.extract_rom_entity_anim_stepper import extract_evidence


def step_entity_anim(
    anim_id: int,
    cached_id: int,
    cursor: int,
    delay: int,
    flags_37: int,
    keyframes: list[tuple[int, int]],
) -> tuple[int, int, int, int, int]:
    """Mirror 0x008FA4..0x009040 -> (frame_id, cached_id, cursor, delay, flags_37)."""
    flags_37 &= ~0x02
    if anim_id != cached_id:
        cached_id = anim_id
        cursor = -4
        delay = 0
        flags_37 &= ~0x04

    delay -= 1
    if delay >= 0:
        if delay == 0 and (flags_37 & 0x04):
            flags_37 |= 0x02
        return keyframes[max(0, cursor // 4)][0], cached_id, cursor, delay, flags_37

    cursor += 4
    flags_37 &= ~0x08
    if flags_37 & 0x04:
        flags_37 &= ~0x04
        cursor = 0
    frame_id, raw_dur = keyframes[cursor // 4]
    if raw_dur & 0x8000:
        flags_37 |= 0x04
        raw_dur &= 0x7FFF
    dur = raw_dur >> 1
    if dur != 0:
        dur -= 1
    if dur == 0 and (flags_37 & 0x04):
        flags_37 |= 0x02
    return frame_id, cached_id, cursor, dur, flags_37


class EntityAnimStepperTest(unittest.TestCase):
    def test_rom_opcodes(self) -> None:
        rom_bytes = Path("local-roms/Beyond Oasis (USA).md").read_bytes()
        ev = extract_evidence(rom_bytes)
        self.assertEqual(ev["animation_stepper"]["initial_cursor"], -4)
        self.assertEqual(ev["animation_stepper"]["duration_end_bit"], 15)

    def test_keyframe_advance_and_loop(self) -> None:
        kfs = [(10, 4), (20, 0x8004)]
        f, cid, cur, dly, fl = step_entity_anim(2, -1, 0, 0, 0, kfs)
        self.assertEqual((f, cid, cur, dly, fl & 6), (10, 2, 0, 1, 0))
        f, cid, cur, dly, fl = step_entity_anim(2, cid, cur, dly, fl, kfs)
        self.assertEqual((f, cur, dly, fl & 6), (10, 0, 0, 0))
        f, cid, cur, dly, fl = step_entity_anim(2, cid, cur, dly, fl, kfs)
        self.assertEqual((f, cur, dly, fl & 6), (20, 4, 1, 4))
        f, cid, cur, dly, fl = step_entity_anim(2, cid, cur, dly, fl, kfs)
        self.assertEqual((f, cur, dly, fl & 6), (20, 4, 0, 6))
        f, cid, cur, dly, fl = step_entity_anim(2, cid, cur, dly, fl, kfs)
        self.assertEqual((f, cur, dly, fl & 6), (10, 0, 1, 0))


if __name__ == "__main__":
    unittest.main()
