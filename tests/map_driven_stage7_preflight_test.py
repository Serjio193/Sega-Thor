"""Synthetic Stage 7 emission and generation-lineage preflight checks."""

from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / "src" / "tools"), str(ROOT / "src" / "tools" / "thor_evidence")]

from map_driven_stage7_preflight import Stage7PreflightError, resolve_manifest  # noqa: E402


ROM_SHA = "a" * 64


def _generation(root: Path, manifest_entries: list[dict]) -> Path:
    generation = root / "generations" / "gen-current"
    materialized = root / "materialized"
    materialized.mkdir(parents=True)
    (materialized / "manifest.json").write_text(json.dumps({
        "schema": "oasis.full-rom-split.v1", "rom_sha256": ROM_SHA,
        "rom_size": 10, "entries": manifest_entries}), encoding="utf-8")
    generation.mkdir(parents=True)
    (generation / "generation-metadata.json").write_text(json.dumps({
        "generation_id": "gen-current", "stage7": {"materialized": str(materialized)}}),
        encoding="utf-8")
    return generation


class Stage7PreflightTests(unittest.TestCase):
    def test_matching_manifest_and_emission_partition_pass(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            entries = [{"start": 0, "end": 4}, {"start": 4, "end": 8},
                       {"start": 8, "end": 10}]
            generation = _generation(root, entries)
            emissions = [(0, 4, "ASM"), (4, 8, "DATA"), (8, 10, "INCBIN")]
            _, _, diagnostic = resolve_manifest(generation, ROM_SHA, 10, emissions, 3, "map")
            self.assertEqual(diagnostic["status"], "PASS")
            self.assertEqual(diagnostic["canonical_map"]["sum_emission_categories"], 10)
            self.assertEqual(diagnostic["canonical_map"]["SOURCE_OWNED"], 3)

    def test_mismatched_ranges_stop_and_report_authorities(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            generation = _generation(root, [{"start": 0, "end": 5}, {"start": 5, "end": 10}])
            emissions = [(0, 4, "ASM"), (4, 8, "DATA"), (8, 10, "INCBIN")]
            with self.assertRaises(Stage7PreflightError) as caught:
                resolve_manifest(generation, ROM_SHA, 10, emissions, 3, "map")
            error = caught.exception
            self.assertEqual(str(error), "STOP_STAGE7_EMISSION_PARTITION_MISMATCH")
            self.assertEqual(error.diagnostic["authorities"][1]["status"], "PASS")
            self.assertTrue(error.diagnostic["mismatch_fields"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
