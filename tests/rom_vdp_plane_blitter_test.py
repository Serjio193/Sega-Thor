"""Tests for M15.311 Full-Screen Wrap-Around VDP Plane Blitter."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_vdp_plane_blitter import (
    DEFAULT_ROM_PATH,
    extract_vdp_plane_blitter,
    select_blit_dimensions,
)


class RomVdpPlaneBlitterTest(unittest.TestCase):
    """Verify VDP plane blitter dimensions and full-plane selection rule."""

    def test_blitter_dimensions(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_vdp_plane_blitter(ROOT / DEFAULT_ROM_PATH)
        pb = data["vdp_plane_blitter"]
        self.assertEqual((pb["viewport_cols"], pb["viewport_rows"]), (41, 29))
        self.assertEqual((pb["full_plane_cols"], pb["full_plane_rows"]), (64, 32))
        self.assertEqual(select_blit_dimensions(64, 32, False), (64, 32))
        self.assertEqual(select_blit_dimensions(96, 32, False), (41, 29))
        self.assertEqual(select_blit_dimensions(96, 64, True), (64, 32))

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_vdp_plane_blitter_m15_311.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.311")
        self.assertEqual(parsed["vdp_plane_blitter"]["entry_addr"], "0x00D800")


if __name__ == "__main__":
    unittest.main()
