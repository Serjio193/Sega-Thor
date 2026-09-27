"""Tests for authoritative deterministic postrun receipt generation in repo."""

from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))

from live_forward_postrun_receipts import (emit_postrun_receipts,
                                          emit_asm_closure_receipt,
                                          sha256_file)  # noqa: E402


class PostrunReceiptsTests(unittest.TestCase):
    def setUp(self):
        self.vdp_run_dir = ROOT / "build" / "thor-evidence" / "live-worker-control" / "campaign-desktop-vdp-acceptance-exact-width-proof"
        self.real_run_dir = ROOT / "build" / "thor-evidence" / "live-worker-control" / "campaign-desktop-20260921-224537-038"
        self.active_run_dir = self.vdp_run_dir if (self.vdp_run_dir / "post-run-analysis" / "report.json").is_file() else self.real_run_dir
        self.post_run = self.active_run_dir / "post-run-analysis"
        self.receipt_path = self.active_run_dir / "live-worker-interactive-receipt.json"

    def test_emit_postrun_receipts_with_nonzero_stage7_promotion(self):
        if not (self.post_run / "report.json").is_file():
            self.skipTest("Real run post-run artifacts not present")
        report = json.loads((self.post_run / "report.json").read_text(encoding="utf-8"))
        status_path = self.post_run / "status.json"

        # Verify input promoted_bytes = 284
        s7_in = report["stage_results"]["ASM CLOSURE"]
        self.assertEqual(int(s7_in["promoted_bytes"]), 284)

        with tempfile.TemporaryDirectory() as tmpdir:
            out_dir = Path(tmpdir)
            (out_dir / "report.json").write_text(json.dumps(report), encoding="utf-8")

            shas = emit_postrun_receipts(out_dir, self.receipt_path, report, status_path)

            expected_files = [
                "postrun_map_refresh_receipt.json",
                "postrun_control_provenance_receipt.json",
                "postrun_audio_receipt.json",
                "postrun_asm_closure_receipt.json",
                "postrun_full_rom_audit.json",
                "postrun_cleanup_receipt.json",
                "postrun_final_receipt.json"
            ]
            for fname in expected_files:
                self.assertIn(fname, shas)
                fpath = out_dir / fname
                self.assertTrue(fpath.is_file(), f"Missing receipt: {fname}")
                self.assertEqual(sha256_file(fpath), shas[fname])

            # Verify Stage 7 derived non-zero promotion semantics
            r3 = json.loads((out_dir / "postrun_asm_closure_receipt.json").read_text(encoding="utf-8"))
            self.assertEqual(r3["promoted_bytes"], 284)
            self.assertEqual(r3["promoted_range_count"], 16)
            self.assertEqual(r3["source_owned_before"], 1487388)
            self.assertEqual(r3["source_owned_after"], 1487672)
            self.assertEqual(r3["source_owned_delta"], 284)
            self.assertEqual(r3["closed_ranges"], 16)
            self.assertEqual(r3["status"], "PASS_POSTRUN_MAP_DRIVEN_ASM_CLOSURE_V1")

            # Verify final receipt pipeline deltas
            r6 = json.loads((out_dir / "postrun_final_receipt.json").read_text(encoding="utf-8"))
            self.assertEqual(r6["status"], "PASS_MODERN_POSTRUN_PIPELINE_V1")
            self.assertEqual(r6["overall_state"], "ANALYSIS COMPLETE ✓")
            self.assertEqual(r6["vdp_source_owned_delta"], 0)
            self.assertEqual(r6["asm_source_owned_delta"], 284)
            self.assertEqual(r6["pipeline_source_owned_before"], 1487388)
            self.assertEqual(r6["pipeline_source_owned_after"], 1487672)
            self.assertEqual(r6["pipeline_source_owned_delta"], 284)
            self.assertEqual(r6["source_owned_before"], 1487388)
            self.assertEqual(r6["source_owned_after"], 1487672)
            self.assertEqual(r6["source_owned_delta"], 284)

    def test_emit_postrun_receipts_with_zero_stage7_promotion(self):
        if not (self.post_run / "report.json").is_file():
            self.skipTest("Real run post-run artifacts not present")
        report = json.loads((self.post_run / "report.json").read_text(encoding="utf-8"))
        status_path = self.post_run / "status.json"

        # Mutate to synthetic zero-promotion scenario
        s7_in = dict(report["stage_results"]["ASM CLOSURE"])
        s7_in["promoted_bytes"] = 0
        s7_in["promoted_ranges"] = []
        s7_in["source_owned_after"] = 1487388
        s7_in["source_owned_delta"] = 0
        s7_in["closed_count"] = 0
        s7_in["status"] = "PASS_STAGE7_EVALUATED_FAIL_CLOSED_NO_PROMOTION"
        s7_in["output_paths"] = {}
        report["stage_results"]["ASM CLOSURE"] = s7_in

        with tempfile.TemporaryDirectory() as tmpdir:
            out_dir = Path(tmpdir)
            (out_dir / "report.json").write_text(json.dumps(report), encoding="utf-8")

            shas = emit_postrun_receipts(out_dir, self.receipt_path, report, status_path)
            r3 = json.loads((out_dir / "postrun_asm_closure_receipt.json").read_text(encoding="utf-8"))
            self.assertEqual(r3["promoted_bytes"], 0)
            self.assertEqual(r3["promoted_range_count"], 0)
            self.assertEqual(r3["source_owned_before"], 1487388)
            self.assertEqual(r3["source_owned_after"], 1487388)
            self.assertEqual(r3["source_owned_delta"], 0)
            self.assertEqual(r3["closed_ranges"], 0)
            self.assertEqual(r3["status"], "PASS_STAGE7_EVALUATED_FAIL_CLOSED_NO_PROMOTION")

            r6 = json.loads((out_dir / "postrun_final_receipt.json").read_text(encoding="utf-8"))
            self.assertEqual(r6["vdp_source_owned_delta"], 0)
            self.assertEqual(r6["asm_source_owned_delta"], 0)
            self.assertEqual(r6["pipeline_source_owned_delta"], 0)
            self.assertEqual(r6["source_owned_delta"], 0)

    def test_missing_status_doc_fails_closed(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            out_dir = Path(tmpdir)
            with self.assertRaises(ValueError) as ctx:
                emit_postrun_receipts(out_dir, "nonexistent.json", {"run_id": 123}, out_dir / "missing_status.json")
            self.assertIn("STOP_STATUS_DOC_MISSING", str(ctx.exception))


if __name__ == "__main__":
    unittest.main()
