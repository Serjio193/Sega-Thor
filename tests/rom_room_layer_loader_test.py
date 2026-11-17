"""Tests for M15.309 Room Tilemap & Collision Layer Descriptor Builder."""

from __future__ import annotations

import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from extract_rom_room_layer_loader import (
    DEFAULT_ROM_PATH,
    compute_row_shift_and_layout,
    extract_room_layer_loader,
)


class RomRoomLayerLoaderTest(unittest.TestCase):
    """Verify room layer header unpacking, row shifts, and VRAM bases."""

    def test_room_layer_header_and_constants(self) -> None:
        if not (ROOT / DEFAULT_ROM_PATH).exists():
            self.skipTest("Local ROM not present")
        data = extract_room_layer_loader(ROOT / DEFAULT_ROM_PATH)
        rl = data["room_layer_loader"]
        self.assertEqual(rl["tileset_dma_words"], 0x2000)
        self.assertEqual(rl["plane_a_vram"], "0xC000")
        self.assertEqual(rl["plane_b_vram"], "0xE000")
        layout = compute_row_shift_and_layout(bytes([0x82, 0x01, 0x01, 0x01]))
        self.assertEqual(layout["plane_a"]["tiles_w"], 64)
        self.assertEqual(layout["plane_a"]["tiles_h"], 32)
        self.assertEqual(layout["plane_a"]["row_shift"], 7)
        self.assertEqual(layout["plane_a"]["tilemap_bytes"], 4096)
        self.assertEqual(layout["plane_a"]["collision_bytes"], 2048)
        self.assertEqual(layout["plane_b"]["tilemap_offset"], 4 + 4096 + 2048)

    def test_json_artifact(self) -> None:
        artifact = ROOT / "data/thor-evidence/rom_room_layer_loader_m15_309.json"
        if not artifact.exists():
            self.skipTest("Evidence JSON not yet generated")
        parsed = json.loads(artifact.read_text(encoding="utf-8"))
        self.assertEqual(parsed["milestone"], "M15.309")
        self.assertEqual(parsed["room_layer_loader"]["entry_addr"], "0x00D500")


if __name__ == "__main__":
    unittest.main()
