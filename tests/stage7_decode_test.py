"""Determinism and failure contracts for bounded Stage 7 decode batching."""

from __future__ import annotations

from pathlib import Path
import sys
import time
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))
import stage7_decode  # noqa: E402


class Stage7DecodeTests(unittest.TestCase):
    def test_results_are_keyed_by_stable_candidate_ordinal(self) -> None:
        jobs = [(7, ROOT / "build" / "candidate-7", {"intervals": [[7, 8]]}),
                (2, ROOT / "build" / "candidate-2", {"intervals": [[2, 3]]})]

        def fake_decode(_tool, _rom, candidate, output):
            time.sleep(0.01 if candidate["intervals"][0][0] == 7 else 0.0)
            return {"start": candidate["intervals"][0][0]}, output / "candidate.asm"

        with mock.patch.object(stage7_decode, "decode_candidate", side_effect=fake_decode):
            result = stage7_decode.decode_candidates(jobs, Path("tool"), Path("rom"),
                                                     max_workers=2)
        self.assertEqual(list(result), [2, 7])
        self.assertEqual(result[7][0]["start"], 7)
        self.assertEqual(result[2][0]["start"], 2)

    def test_decode_error_is_returned_for_fail_closed_coordinator(self) -> None:
        jobs = [(0, Path("candidate"), {"intervals": [[0, 1]]})]
        with mock.patch.object(stage7_decode, "decode_candidate",
                               side_effect=ValueError("decode failed")):
            result = stage7_decode.decode_candidates(jobs, Path("tool"), Path("rom"))
        self.assertIsNone(result[0][0])
        self.assertIsInstance(result[0][2], ValueError)

    def test_measurement_counts_duplicate_requests_without_deduplicating(self) -> None:
        jobs = [(0, Path("candidate-0"), {"intervals": [[0, 4]]}),
                (1, Path("candidate-1"), {"intervals": [[0, 4]]}),
                (2, Path("candidate-2"), {"intervals": [[4, 8]]})]
        metrics = {}

        def fake_decode(_tool, _rom, candidate, output):
            return {"start": candidate["intervals"][0][0]}, output / "candidate.asm"

        with mock.patch.object(stage7_decode, "decode_candidate", side_effect=fake_decode) as decoder:
            result = stage7_decode.decode_candidates(jobs, Path("tool"), Path("rom"),
                                                     metrics=metrics, max_workers=2)
        self.assertEqual(decoder.call_count, 3)
        self.assertEqual(metrics["decode_request_total"], 3)
        self.assertEqual(metrics["decode_unique_keys"], 2)
        self.assertEqual(metrics["decode_duplicate_requests"], 1)
        self.assertEqual(metrics["avoidable_invocations"], 1)
        self.assertEqual(result[0][0], result[1][0])

    def test_decode_key_distinguishes_ranges_and_decoder_identity(self) -> None:
        left = {"intervals": [[0, 4]]}
        right = {"intervals": [[0, 6]]}
        self.assertNotEqual(stage7_decode.decode_request_key(Path("tool"), Path("rom"), left),
                            stage7_decode.decode_request_key(Path("tool"), Path("rom"), right))

    def test_metric_summary_counts_repeated_keys_across_iterations(self) -> None:
        summary = stage7_decode.summarize_decode_metrics([
            {"decode_request_total": 2, "external_decoder_invocations": 2,
             "_key_digests": ["a", "b"], "batch_wall_seconds": 1.0},
            {"decode_request_total": 2, "external_decoder_invocations": 2,
             "_key_digests": ["b", "c"], "batch_wall_seconds": 1.5},
        ])
        self.assertEqual(summary["decode_request_total"], 4)
        self.assertEqual(summary["decode_unique_keys"], 3)
        self.assertEqual(summary["decode_duplicate_requests"], 1)
        self.assertEqual(summary["batch_wall_seconds"], 2.5)


if __name__ == "__main__":
    unittest.main(verbosity=2)
