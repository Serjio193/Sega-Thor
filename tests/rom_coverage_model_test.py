"""Focused checks for read-only ROM coverage visualization math and decoding."""

import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/bizhawk-native-ring"))
import rom_coverage_model as model


def checkpoint_bytes(masks: list[int], contract: str = model.CONTRACT_SHA256) -> bytes:
    """Create a small valid checkpoint using the production binary contract."""
    strings = (
        "a" * 64, "thor.rom-properties.v1",
        contract,
        "c" * 64, "run_test", "PARTIAL",
    )
    rom_size = len(masks)
    body = bytearray(b"OASROMP1")
    body += struct.pack("<I", 1)

    def add_text(text: str) -> None:
        data = text.encode()
        body.extend(struct.pack("<I", len(data)))
        body.extend(data)

    add_text(strings[0])
    body.extend(struct.pack("<Q", rom_size))
    add_text(strings[1])
    add_text(strings[2])
    add_text(strings[3])
    add_text(strings[4])
    body.extend(struct.pack("<QQ", 1, 511))
    add_text(strings[5])
    body.extend(struct.pack("<Q", rom_size))
    body.extend(b"".join(struct.pack("<H", value) for value in masks))
    return bytes(body) + hashlib.sha256(body).hexdigest().encode()


class RomCoverageModelTests(unittest.TestCase):
    def test_checkpoint_accepts_high_property_bit(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "graphics.checkpoint"
            path.write_bytes(checkpoint_bytes([0x100]))
            checkpoint = model.load_checkpoint(path)
            self.assertEqual(checkpoint.masks, struct.pack("<H", 0x100))
            self.assertEqual(model.classification_coverage(checkpoint.masks), (1, 0, 1))

    def test_compressed_graphics_property_is_classified_and_filterable(self):
        summary = model.summarize_cell(struct.pack("<HH", 0x100, 0x100), 0, 2)
        self.assertEqual(summary["state"], "COMPRESSED_GRAPHICS_SOURCE")
        self.assertEqual(summary["classified"], 2)
        self.assertTrue(model.matches_filter(summary, "Graphics"))

    def test_checkpoint_identity_checksum_and_bitmap(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "run.checkpoint"
            path.write_bytes(checkpoint_bytes([0, 4, 20, 0]))
            checkpoint = model.load_checkpoint(path)
            self.assertEqual(checkpoint.rom_size, 4)
            self.assertEqual(checkpoint.identity["run_id"], "run_test")
            self.assertEqual(model.coverage(checkpoint.masks), (2, 4, 50.0))
            path.write_bytes(path.read_bytes()[:-1] + b"0")
            with self.assertRaisesRegex(ValueError, "checksum"):
                model.load_checkpoint(path)

    def test_overlay_receipt_requires_exact_checkpoint_identity(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            checkpoint = root / "run.checkpoint"
            raw = checkpoint_bytes([4, 0])
            checkpoint.write_bytes(raw)
            receipt = root / "overlay.json"
            receipt.write_text(json.dumps({
                "status": "PASS_SEAL_REPLAY",
                "checkpoint_sha256": hashlib.sha256(raw).hexdigest(),
                "rom_sha256": "a" * 64,
                "rom_size": 2,
                "classifier_schema": "thor.rom-properties.v1",
                "proof_contract_sha256": model.CONTRACT_SHA256,
                "runtime_build_id": "c" * 64,
                "run_id": "run_test",
                "capabilities": 511,
                "validation_state": "PARTIAL",
            }), encoding="utf-8")
            self.assertEqual(model.load_overlay_checkpoint(receipt, checkpoint).rom_size, 2)
            receipt.write_text(receipt.read_text(encoding="utf-8").replace("run_test", "other"), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "identities disagree"):
                model.load_overlay_checkpoint(receipt, checkpoint)

    def test_accepted_overlay_can_load_its_exact_prior_contract(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            prior_contract = "a" * 64
            checkpoint = root / "run.rom-properties.v1"
            raw = checkpoint_bytes([4, 0], prior_contract)
            checkpoint.write_bytes(raw)
            receipt = root / "overlay.json"
            receipt.write_text(json.dumps({
                "status": "PASS_SEAL_REPLAY",
                "checkpoint_sha256": hashlib.sha256(raw).hexdigest(),
                "rom_sha256": "a" * 64,
                "rom_size": 2,
                "classifier_schema": "thor.rom-properties.v1",
                "proof_contract_sha256": prior_contract,
                "runtime_build_id": "c" * 64,
                "run_id": "run_test",
                "capabilities": 511,
                "validation_state": "PARTIAL",
            }), encoding="utf-8")
            self.assertEqual(model.load_overlay_checkpoint(receipt, checkpoint).rom_size, 2)
            with self.assertRaisesRegex(ValueError, "proof contract"):
                model.load_checkpoint(checkpoint)

    def test_cell_partition_has_exact_ordered_coverage_for_tail(self):
        size, width = 3075, 1024
        ranges = [model.cell_range(size, index, width) for index in range(4)]
        self.assertEqual(ranges, [(0, 1024), (1024, 2048), (2048, 3072), (3072, 3075)])
        self.assertEqual([a for a, _ in ranges], [0, 1024, 2048, 3072])
        self.assertEqual(ranges[-1][1], size)
        with self.assertRaises(IndexError):
            model.cell_range(size, 4, width)

    def test_coverage_union_and_mixed_property_overlap(self):
        bitmap = b"".join(struct.pack("<H", value) for value in (0, 4, 48, 16, 0))
        summary = model.summarize_cell(bitmap, 0, 5)
        self.assertEqual(summary["covered"], 3)
        self.assertEqual(summary["unknown"], 2)
        self.assertEqual(summary["counts"]["M68K_DATA_READ"], 1)
        self.assertEqual(summary["counts"]["VDP_VRAM_SOURCE"], 2)
        self.assertEqual(summary["counts"]["VDP_CRAM_SOURCE"], 1)
        self.assertEqual(summary["state"], "MIXED")
        self.assertEqual(model.coverage(bitmap), (3, 5, 60.0))
        self.assertEqual(model.classification_coverage(bitmap), (2, 1, 5))

    def test_single_property_with_unknown_tail_keeps_dominant_color(self):
        bitmap = b"".join(struct.pack("<H", value) for value in (4, 4, 0, 0))
        summary = model.summarize_cell(bitmap, 0, 4)
        self.assertEqual(summary["state"], "OBSERVED_UNCLASSIFIED")
        self.assertEqual(summary["visual_state"], "M68K_DATA_READ")
        self.assertEqual(summary["observed_unclassified"], 2)
        self.assertEqual(summary["classified"], 0)
        self.assertEqual(summary["percent"], 50.0)

    def test_observation_overlap_does_not_hide_single_proven_class(self):
        bitmap = struct.pack("<HHH", 4, 20, 0)
        summary = model.summarize_cell(bitmap, 0, 3)
        self.assertEqual(summary["state"], "VDP_VRAM_SOURCE")
        self.assertEqual(summary["classified"], 1)
        self.assertEqual(summary["observed_unclassified"], 1)
        self.assertFalse(model.matches_filter(summary, "Observed only"))
        self.assertTrue(model.matches_filter(summary, "Classified only"))
        observed = model.summarize_cell(struct.pack("<HH", 4, 0), 0, 2)
        self.assertTrue(model.matches_filter(observed, "Observed only"))

    def test_z80_read_state_keeps_distinct_observation_color(self):
        summary = model.summarize_cell(struct.pack("<H", 8), 0, 1)
        self.assertEqual(summary["state"], "OBSERVED_UNCLASSIFIED")
        self.assertEqual(summary["visual_state"], "Z80_DATA_READ")

    def test_two_observation_types_show_mixed_without_becoming_classified(self):
        summary = model.summarize_cell(struct.pack("<HH", 4, 8), 0, 2)
        self.assertEqual(summary["state"], "OBSERVED_UNCLASSIFIED")
        self.assertEqual(summary["visual_state"], "MIXED")
        self.assertEqual(summary["classified"], 0)
        self.assertTrue(model.matches_filter(summary, "Mixed only"))

    def test_new_coverage_is_union_delta_not_sum_of_property_bits(self):
        current = b"".join(struct.pack("<H", value) for value in (20, 0, 4, 0))
        canonical = b"".join(struct.pack("<H", value) for value in (4, 0, 0, 0))
        self.assertEqual(model.newly_covered(current, canonical), 1)
        self.assertEqual(model.newly_covered_cells(current, canonical, 2), 1)

    def test_new_classified_delta_ignores_overlap_and_read_only_bytes(self):
        current = struct.pack("<HHHH", 5, 20, 4, 0)
        canonical = struct.pack("<HHHH", 4, 4, 0, 0)
        self.assertEqual(model.newly_classified(current, canonical), 2)

    def test_filters(self):
        bitmap = struct.pack("<HH", 0, 4)
        unknown = model.summarize_cell(bitmap, 0, 1)
        m68k = model.summarize_cell(bitmap, 1, 2)
        self.assertTrue(model.matches_filter(unknown, "Unknown only"))
        self.assertFalse(model.matches_filter(unknown, "M68K"))
        self.assertTrue(model.matches_filter(m68k, "M68K"))


if __name__ == "__main__":
    unittest.main()
