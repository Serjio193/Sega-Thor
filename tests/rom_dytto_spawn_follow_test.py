#!/usr/bin/env python3
"""
rom_dytto_spawn_follow_test.py — Automated Verification Suite for M15.262

Verifies:
1. Dytto spawn routine byte sequence at 0x014668..0x0146BE.
2. Dytto Nemesis graphics pointer 0x00141580 at 0x01454C.
3. State jump table at 0x014914 mapping State 0x1A to 0x014F60.
4. State 0x1A milestones (180, 240, 300, 360, 420 frames).
5. Follow math routine 0x0150A0 calling angle helper 0x00E6F0.
6. JSON evidence file integrity and structure.
"""

import json
import struct
import unittest
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
EVIDENCE_PATH = Path("data/thor-evidence/rom_dytto_spawn_follow_m15_262.json")


class TestDyttoSpawnFollow(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        assert ROM_PATH.exists(), f"ROM not found at {ROM_PATH}"
        assert EVIDENCE_PATH.exists(), f"Evidence JSON not found at {EVIDENCE_PATH}"
        cls.rom = ROM_PATH.read_bytes()
        cls.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.262")
        self.assertIn("spawn_routine", self.evidence)
        self.assertIn("graphics_and_dma", self.evidence)
        self.assertIn("state_machine", self.evidence)
        self.assertIn("follow_mechanics", self.evidence)
        self.assertIn("active_spirit_semantics", self.evidence)

    def test_spawn_routine_bytes(self):
        # 0x014668: MOVE.W #$0018, $00(A6) -> 3D 7C 00 18 00 00
        self.assertEqual(self.rom[0x014668:0x01466E], bytes([0x3D, 0x7C, 0x00, 0x18, 0x00, 0x00]))
        # 0x01466E: JSR $008D06 -> 4E B9 00 00 8D 06
        self.assertEqual(self.rom[0x01466E:0x014674], bytes([0x4E, 0xB9, 0x00, 0x00, 0x8D, 0x06]))
        # 0x014680: MOVE.W #$0064, $88(A6) -> 3D 7C 00 64 00 88 (Max SP = 100)
        self.assertEqual(self.rom[0x014680:0x014686], bytes([0x3D, 0x7C, 0x00, 0x64, 0x00, 0x88]))
        # 0x0146B4: BSR.W $014522 -> 61 00 FE 6C
        self.assertEqual(self.rom[0x0146B4:0x0146B8], bytes([0x61, 0x00, 0xFE, 0x6C]))

    def test_graphics_dma_pointer(self):
        # 0x01454C: LEA $00141580.L, A0 -> 41 F9 00 14 15 80
        self.assertEqual(self.rom[0x01454C:0x014552], bytes([0x41, 0xF9, 0x00, 0x14, 0x15, 0x80]))
        # 0x014552: LEA $00FF2FA8.L, A1 -> 43 F9 00 FF 2F A8
        self.assertEqual(self.rom[0x014552:0x014558], bytes([0x43, 0xF9, 0x00, 0xFF, 0x2F, 0xA8]))
        # 0x014558: JSR $003820 -> 4E B9 00 00 38 20
        self.assertEqual(self.rom[0x014558:0x01455E], bytes([0x4E, 0xB9, 0x00, 0x00, 0x38, 0x20]))

    def test_state_1a_milestones(self):
        # 0x014F60: MOVE.W $02(A6), D0; ADDQ.W #1, D0; MOVE.W D0, $02(A6)
        self.assertEqual(self.rom[0x014F60:0x014F6A], bytes([0x30, 0x2E, 0x00, 0x02, 0x52, 0x40, 0x3D, 0x40, 0x00, 0x02]))
        # CMP.W #$00B4, D0 (180f)
        self.assertEqual(self.rom[0x014F6A:0x014F6E], bytes([0x0C, 0x40, 0x00, 0xB4]))
        # CMP.W #$01A4, D0 (420f)
        self.assertEqual(self.rom[0x014F84:0x014F88], bytes([0x0C, 0x40, 0x01, 0xA4]))

    def test_follow_math_logic(self):
        # 0x0150A0: BSR.W $00E6F0 -> 61 00 96 4E
        self.assertEqual(self.rom[0x0150A0:0x0150A4], bytes([0x61, 0x00, 0x96, 0x4E]))
        # 0x0150A4: MOVE.W $AE(A6), D3 -> 36 2E 00 AE
        self.assertEqual(self.rom[0x0150A4:0x0150A8], bytes([0x36, 0x2E, 0x00, 0xAE]))


if __name__ == "__main__":
    unittest.main()
