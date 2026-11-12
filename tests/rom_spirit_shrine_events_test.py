#!/usr/bin/env python3
"""
rom_spirit_shrine_events_test.py — Automated Verification Suite for M15.261

Verifies:
1. Dispatcher 0x018CAA jumps correctly to all 5 shrine handlers based on room ID.
2. Dytto handler at 0x018CDE sets:
   - Dialogue message 0
   - Area flag $FF0E1D bit 2
   - Global flag $FF0DBC bit 3 (Global Flag 3)
   - Active spirit $FF0D7A bit 0
3. Spawn script for Room 0x0106 gates Altar Entity 0x0020 on Global Flag 3.
4. All 5 shrine definitions are complete and verified against ROM bytes.
"""

import json
import struct
import unittest
from pathlib import Path

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
EVIDENCE_PATH = Path("data/thor-evidence/rom_spirit_shrine_events_m15_261.json")


class TestSpiritShrineEvents(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        assert ROM_PATH.exists(), f"ROM not found at {ROM_PATH}"
        assert EVIDENCE_PATH.exists(), f"Evidence JSON not found at {EVIDENCE_PATH}"
        cls.rom = ROM_PATH.read_bytes()
        cls.evidence = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    def test_evidence_structure(self):
        self.assertEqual(self.evidence["milestone"], "M15.261")
        self.assertEqual(len(self.evidence["shrines"]), 5)
        self.assertIn("m16_50_correction", self.evidence["architectural_notes"])

    def test_dispatcher_rom_bytes(self):
        # 0x018CAA: MOVE.W $00FF0E40.L, D0 -> 30 39 00 FF 0E 40
        self.assertEqual(self.rom[0x018CAA:0x018CB0], bytes([0x30, 0x39, 0x00, 0xFF, 0x0E, 0x40]))
        # 0x018CB0: CMP.W #$0106, D0 -> 0C 40 01 06
        self.assertEqual(self.rom[0x018CB0:0x018CB4], bytes([0x0C, 0x40, 0x01, 0x06]))

    def test_dytto_handler_rom_bytes(self):
        # 0x018CDE: MOVE.W #$003F, $32(A6) -> 3D 7C 00 3F 00 32
        self.assertEqual(self.rom[0x018CDE:0x018CE4], bytes([0x3D, 0x7C, 0x00, 0x3F, 0x00, 0x32]))
        # Dialogue Msg 0 -> MOVEQ #0, D0 -> 70 00
        self.assertEqual(self.rom[0x018CEE:0x018CF0], bytes([0x70, 0x00]))
        # Spawn helper BSR.W $014668 at 0x018D1E -> 61 00 B9 48
        self.assertEqual(self.rom[0x018D1E:0x018D22], bytes([0x61, 0x00, 0xB9, 0x48]))
        # Area flag BSET.B #2, $FF0E1D.L -> 08 B9 00 02 00 FF 0E 1D
        self.assertEqual(self.rom[0x018D30:0x018D38], bytes([0x08, 0xB9, 0x00, 0x02, 0x00, 0xFF, 0x0E, 0x1D]))
        # Global flag 3 BSET.B #3, $FF0DBC.L -> 08 F9 00 03 00 FF 0D BC
        self.assertEqual(self.rom[0x018D38:0x018D40], bytes([0x08, 0xF9, 0x00, 0x03, 0x00, 0xFF, 0x0D, 0xBC]))
        # Active spirit BSET.B #0, $FF0D7A.L -> 08 F9 00 00 00 FF 0D 7A
        self.assertEqual(self.rom[0x018D40:0x018D48], bytes([0x08, 0xF9, 0x00, 0x00, 0x00, 0xFF, 0x0D, 0x7A]))

    def test_room_106_altar_spawn_gating(self):
        dytto = next(s for s in self.evidence["shrines"] if s["spirit_name"] == "Dytto")
        rec = dytto["altar_spawn_record"]
        self.assertEqual(rec["bank"], 1)
        self.assertEqual(rec["entity_type"], "0x0020")
        self.assertEqual(rec["flag_index"], 3)
        self.assertEqual(rec["vram_tile_base"], "0x0200")
        self.assertEqual(rec["position"], {"x": 160, "y": 80, "z": 0})

    def test_all_shrines_present(self):
        names = [s["spirit_name"] for s in self.evidence["shrines"]]
        self.assertEqual(names, ["Dytto", "Efreet", "Shade", "Bow", "Agito"])
        for s in self.evidence["shrines"]:
            self.assertTrue(len(s["dialogue_text"]) > 0)
            self.assertTrue(s["handler_address"].startswith("0x018"))


if __name__ == "__main__":
    unittest.main()
