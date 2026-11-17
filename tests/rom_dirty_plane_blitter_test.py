#!/usr/bin/env python3
"""
rom_dirty_plane_blitter_test.py — Unit tests for ROM Dynamic Partial Tilemap
Plane VRAM Dirty Block Blitter, Viewport Bounds Clipper & Toroidal Stride
Streamer (0x00DC26..0x00DDA2).
"""

import json
import os
import unittest

EVIDENCE_PATH = os.path.join(
    os.path.dirname(__file__), "..", "data", "thor-evidence", "rom_dirty_plane_blitter_m15_444.json"
)


class TestRomDirtyPlaneBlitter(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with open(EVIDENCE_PATH, "r", encoding="utf-8") as f:
            cls.evidence = json.load(f)
        cls.insns_by_addr = {
            insn["address"]: insn for insn in cls.evidence["instructions"]
        }

    def test_01_json_evidence_integrity(self):
        self.assertEqual(self.evidence["start_address"], "0x00DC26")
        self.assertEqual(self.evidence["end_address"], "0x00DDA2")
        self.assertEqual(self.evidence["byte_count"], 380)
        self.assertEqual(self.evidence["instruction_count"], 127)
        self.assertIn("vdp_sync_dirty_plane_rect_clipped", self.evidence["subroutines"])
        self.assertIn("vdp_sync_dirty_plane_rect_unclipped", self.evidence["subroutines"])

    def test_02_clipped_mode_viewport_clipping_logic(self):
        # Disassembly structure checks
        self.assertEqual(self.insns_by_addr["0x00DC26"]["mnemonic"], "btst.b")
        self.assertIn("$27(a3)", self.insns_by_addr["0x00DC26"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DC2C"]["mnemonic"], "bne.w")
        self.assertIn("$dd14", self.insns_by_addr["0x00DC2C"]["op_str"])

        # Horizontal clip against screen width (41 cols buffer: move.w #$29, d0)
        self.assertEqual(self.insns_by_addr["0x00DC46"]["mnemonic"], "move.w")
        self.assertIn("#$29, d0", self.insns_by_addr["0x00DC46"]["op_str"])

        # Vertical clip against screen height (29 rows buffer: move.w #$1d, d0)
        self.assertEqual(self.insns_by_addr["0x00DC72"]["mnemonic"], "move.w")
        self.assertIn("#$1d, d0", self.insns_by_addr["0x00DC72"]["op_str"])

        # Python simulation of clipping
        def clip_rect(cam_x, cam_y, target_x, target_y, w, h, stride_shift):
            rel_x = target_x - cam_x
            if rel_x < 0:
                w += rel_x
                if w < 0:
                    return None  # Culled
                src_skip_bytes = (-rel_x) * 2
                rel_x = 0
            else:
                src_skip_bytes = 0

            max_w = 41 - rel_x
            if max_w < 0:
                return None
            w = min(w, max_w)

            rel_y = target_y - cam_y
            if rel_y < 0:
                h += rel_y
                if h < 0:
                    return None
                src_skip_bytes += (-rel_y) * (1 << stride_shift)
                rel_y = 0

            max_h = 29 - rel_y
            if max_h < 0:
                return None
            h = min(h, max_h)

            return rel_x, rel_y, w, h, src_skip_bytes

        # Test case 1: Fully within viewport
        res = clip_rect(10, 10, 15, 12, 4, 3, 7)
        self.assertEqual(res, (5, 2, 4, 3, 0))

        # Test case 2: Clipped left and top
        res = clip_rect(10, 10, 8, 7, 5, 6, 7)
        # rel_x was -2 -> w = 5 - 2 = 3, rel_x = 0, skip_x = 4 bytes
        # rel_y was -3 -> h = 6 - 3 = 3, rel_y = 0, skip_y = 3 * 128 = 384 bytes
        self.assertEqual(res, (0, 0, 3, 3, 4 + 384))

        # Test case 3: Fully offscreen left
        res = clip_rect(10, 10, 2, 10, 4, 4, 7)
        self.assertIsNone(res)

    def test_03_toroidal_wrapping_math(self):
        # Toroidal wrap masks: #$3f for horizontal (64 tiles), #$1f for vertical (32 tiles)
        self.assertIn("#$3f", self.insns_by_addr["0x00DC90"]["op_str"])
        self.assertIn("#$1f", self.insns_by_addr["0x00DC96"]["op_str"])

        # Check compare instructions during inner loop: wrap at 64 (0x40)
        self.assertEqual(self.insns_by_addr["0x00DCD8"]["mnemonic"], "cmpi.w")
        self.assertIn("#$40", self.insns_by_addr["0x00DCD8"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DCE0"]["mnemonic"], "subi.w")
        self.assertIn("#$40", self.insns_by_addr["0x00DCE0"]["op_str"])

        # Check vertical wrap at 32 (0x20)
        self.assertEqual(self.insns_by_addr["0x00DCFE"]["mnemonic"], "cmpi.w")
        self.assertIn("#$20", self.insns_by_addr["0x00DCFE"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DD06"]["mnemonic"], "subi.w")
        self.assertIn("#$20", self.insns_by_addr["0x00DD06"]["op_str"])

        # Python verification of toroidal coordinates
        def calc_vram_addr(vram_base, col, row):
            col_wrapped = col % 64
            row_wrapped = row % 32
            # d0 = (row << 7) + (col << 1) + vram_base
            return vram_base + (row_wrapped << 7) + (col_wrapped << 1)

        self.assertEqual(calc_vram_addr(0xC000, 0, 0), 0xC000)
        self.assertEqual(calc_vram_addr(0xC000, 63, 0), 0xC000 + 126)
        self.assertEqual(calc_vram_addr(0xC000, 64, 0), 0xC000)  # Wrapped col
        self.assertEqual(calc_vram_addr(0xC000, 0, 31), 0xC000 + (31 * 128))
        self.assertEqual(calc_vram_addr(0xC000, 0, 32), 0xC000)  # Wrapped row

    def test_04_unclipped_toroidal_path(self):
        # Entry at 0x00DD14
        self.assertEqual(self.insns_by_addr["0x00DD14"]["mnemonic"], "sub.w")
        self.assertIn("$8(a3), d1", self.insns_by_addr["0x00DD14"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DD18"]["mnemonic"], "sub.w")
        self.assertIn("$a(a3), d2", self.insns_by_addr["0x00DD18"]["op_str"])

        # Modulo masks
        self.assertIn("#$3f", self.insns_by_addr["0x00DD1C"]["op_str"])
        self.assertIn("#$1f", self.insns_by_addr["0x00DD20"]["op_str"])

        # Inner loop streaming instruction
        self.assertEqual(self.insns_by_addr["0x00DD62"]["mnemonic"], "move.w")
        self.assertIn("(a1)+, (a2)", self.insns_by_addr["0x00DD62"]["op_str"])

        # Wrap at 64
        self.assertEqual(self.insns_by_addr["0x00DD66"]["mnemonic"], "cmpi.w")
        self.assertIn("#$40", self.insns_by_addr["0x00DD66"]["op_str"])

        # Return instruction
        self.assertEqual(self.insns_by_addr["0x00DDA0"]["mnemonic"], "rts")

    def test_05_instruction_level_parity(self):
        # Interrupt mask handling: disable before write, restore after
        self.assertEqual(self.insns_by_addr["0x00DCBE"]["mnemonic"], "ori.w")
        self.assertIn("#$700, sr", self.insns_by_addr["0x00DCBE"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DD0E"]["mnemonic"], "andi.w")
        self.assertIn("#$f9ff, sr", self.insns_by_addr["0x00DD0E"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DD12"]["mnemonic"], "rts")

        # Unclipped path interrupt mask handling
        self.assertEqual(self.insns_by_addr["0x00DD4C"]["mnemonic"], "ori.w")
        self.assertIn("#$700, sr", self.insns_by_addr["0x00DD4C"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DD9C"]["mnemonic"], "andi.w")
        self.assertIn("#$f9ff, sr", self.insns_by_addr["0x00DD9C"]["op_str"])
        self.assertEqual(self.insns_by_addr["0x00DDA0"]["mnemonic"], "rts")


if __name__ == "__main__":
    unittest.main()
