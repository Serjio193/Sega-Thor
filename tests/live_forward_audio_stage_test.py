"""Comprehensive verification suite for M12 Post-Run Audio Analysis Stage V1."""

from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))
sys.path.insert(0, str(ROOT / "src" / "tools"))
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))

from live_forward_audio_stage import (  # noqa: E402
    cluster_candidate_reads,
    run_audio_stage,
    sha256_file,
    verify_accepted_resources,
)
from live_forward_postrun_receipts import emit_postrun_receipts  # noqa: E402
from live_forward_progress import STAGES  # noqa: E402
from thor_evidence.w5_audio_format import parse_bank_descriptors  # noqa: E402
from thor_evidence.w5_audio_decode import decode_audio_resource  # noqa: E402
from thor_evidence.w5_audio_encode import encode_audio_resource  # noqa: E402


class LiveForwardAudioStageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.rom_path = ROOT / "local-roms" / "Beyond Oasis (USA).md"
        cls.campaign_dir = (
            ROOT / "build" / "thor-evidence" / "live-worker-control" / "campaign-desktop-20260921-224537-038"
        )
        cls.receipt_path = cls.campaign_dir / "live-worker-interactive-receipt.json"
        cls.post_run_dir = cls.campaign_dir / "post-run-analysis"

        cls._tmpdir = tempfile.TemporaryDirectory()
        cls.out_dir = Path(cls._tmpdir.name)

        if cls.receipt_path.is_file() and cls.rom_path.is_file():
            receipt = json.loads(cls.receipt_path.read_text(encoding="utf-8"))
            cls.stage_res = run_audio_stage(receipt, cls.rom_path, cls.out_dir, total_segments=2304)
        else:
            cls.stage_res = None

    @classmethod
    def tearDownClass(cls) -> None:
        cls._tmpdir.cleanup()

    def _require_real_evidence(self) -> dict:
        if self.stage_res is None:
            self.skipTest("Real evidence archive or ROM missing")
        return self.stage_res

    def test_a_audio_stage_executes_cleanly_against_preserved_records(self) -> None:
        res = self._require_real_evidence()
        self.assertEqual(res["status"], "PASS")
        self.assertEqual(res["state"], "PASS")
        self.assertEqual(res["total_records_scanned"], 8270988)
        self.assertEqual(res["segments_processed"], 2304)
        self.assertEqual(res["audio_sink_events"], 24794)
        self.assertEqual(res["dac_writes"], 11756)
        self.assertEqual(res["ym2612_writes"], 13038)
        self.assertEqual(res["psg_writes"], 0)
        self.assertEqual(res["banked_rom_reads"], 6982)

    def test_b_resource_1_recognized_and_verified(self) -> None:
        res = self._require_real_evidence()
        self.assertEqual(res["resource_1_reads"], 2324)
        self.assertEqual(res["resource_2_reads"], 0)
        analysis = json.loads((self.out_dir / "postrun_audio_analysis.json").read_text(encoding="utf-8"))
        verified = {r["resource_id"]: r for r in analysis["accepted_resources_verified"]}
        self.assertIn("RESOURCE_1", verified)
        r1 = verified["RESOURCE_1"]
        self.assertEqual(r1["physical_address"], "0x0BD540")
        self.assertEqual(r1["physical_end_exclusive"], "0x0BF768")
        self.assertEqual(r1["byte_length"], 8744)
        self.assertTrue(r1["roundtrip_exact"])

    def test_c_w5_roundtrip_decode_encode_byte_exact(self) -> None:
        if not self.rom_path.is_file():
            self.skipTest("ROM missing")
        rom = self.rom_path.read_bytes()
        verified = verify_accepted_resources(rom)
        self.assertEqual(len(verified), 2)
        for entry in verified:
            self.assertTrue(entry["roundtrip_exact"])
            self.assertFalse(entry["source_nibble_read_by_encoder"])
            self.assertGreater(entry["decoded_pcm_samples"], 0)

    def test_d_strict_causal_chain_level_3_provenance_tag(self) -> None:
        res = self._require_real_evidence()
        self.assertEqual(res["strict_causal_chains"], 13541)
        prov = json.loads((self.out_dir / "postrun_audio_provenance.json").read_text(encoding="utf-8"))
        self.assertEqual(prov["strict_causal_witnesses_count"], 13541)
        self.assertGreater(len(prov["strict_causal_witnesses"]), 0)
        witness = prov["strict_causal_witnesses"][0]
        self.assertEqual(witness["level"], "LEVEL_3_STRICT")
        self.assertEqual(witness["relation_kind"], "STRICT_CAUSAL_CHAIN")
        self.assertIn("Strict dataflow provenance established", witness["rationale"])

    def test_e_candidate_ranges_discovered_and_grouped(self) -> None:
        res = self._require_real_evidence()
        self.assertEqual(res["candidate_ranges_count"], 3)
        cands = json.loads((self.out_dir / "postrun_audio_candidates.json").read_text(encoding="utf-8"))
        self.assertEqual(cands["candidate_ranges_count"], 3)
        ranges = cands["candidate_ranges"]
        self.assertEqual(ranges[0]["range_start"], "0x0BB4B7")
        self.assertEqual(ranges[0]["range_end"], "0x0BB4BD")
        self.assertEqual(ranges[0]["read_count"], 1606)
        self.assertEqual(ranges[0]["truth_class"], "HYPOTHESIS")
        self.assertEqual(ranges[1]["range_start"], "0x0BB682")
        self.assertEqual(ranges[1]["range_end"], "0x0BB688")
        self.assertEqual(ranges[1]["read_count"], 1388)
        self.assertEqual(ranges[2]["range_start"], "0x0BBB58")
        self.assertEqual(ranges[2]["range_end"], "0x0BBB5E")
        self.assertEqual(ranges[2]["read_count"], 1664)

    def test_f_source_owned_remains_exact_no_delta(self) -> None:
        res = self._require_real_evidence()
        self.assertEqual(res["source_owned_before"], 1487388)
        self.assertEqual(res["source_owned_after"], 1487388)
        self.assertEqual(res["source_owned_delta"], 0)

    def test_g_zero_fallback_pass_defaults_in_audio_receipt(self) -> None:
        self._require_real_evidence()
        audio_rec = json.loads((self.out_dir / "postrun_audio_receipt.json").read_text(encoding="utf-8"))
        self.assertEqual(audio_rec["status"], "PASS")
        self.assertEqual(audio_rec["state"], "PASS")
        self.assertIn("input_hashes", audio_rec)
        self.assertIn("output_hashes", audio_rec)
        self.assertEqual(len(audio_rec["output_hashes"]["postrun_audio_analysis_sha256"]), 64)
        self.assertEqual(len(audio_rec["output_hashes"]["postrun_audio_candidates_sha256"]), 64)
        self.assertEqual(len(audio_rec["output_hashes"]["postrun_audio_provenance_sha256"]), 64)

    def test_h_receipts_generator_incorporates_audio_receipt(self) -> None:
        if not (self.post_run_dir / "report.json").is_file():
            self.skipTest("Real run report.json missing")
        report = json.loads((self.post_run_dir / "report.json").read_text(encoding="utf-8"))
        status_path = self.post_run_dir / "status.json"
        with tempfile.TemporaryDirectory() as tmpdir:
            out_dir = Path(tmpdir)
            (out_dir / "report.json").write_text(json.dumps(report), encoding="utf-8")
            shas = emit_postrun_receipts(out_dir, self.receipt_path, report, status_path)
            self.assertIn("postrun_audio_receipt.json", shas)
            self.assertGreaterEqual(len(shas), 7)
            rec_path = out_dir / "postrun_audio_receipt.json"
            self.assertTrue(rec_path.is_file())
            self.assertEqual(sha256_file(rec_path), shas["postrun_audio_receipt.json"])

    def test_i_pipeline_runner_includes_audio_analysis_progression(self) -> None:
        self.assertIn("AUDIO ANALYSIS", STAGES)
        idx_cp = STAGES.index("CONTROL PROVENANCE")
        idx_audio = STAGES.index("AUDIO ANALYSIS")
        self.assertEqual(idx_audio, idx_cp + 1)
        if "VDP / DMA ANALYSIS" in STAGES:
            idx_vdp = STAGES.index("VDP / DMA ANALYSIS")
            self.assertEqual(idx_vdp, idx_audio + 1)
            if "SPRITE / SAT ANALYSIS" in STAGES:
                idx_sprite = STAGES.index("SPRITE / SAT ANALYSIS")
                idx_gameplay = STAGES.index("GAMEPLAY RAM / ENTITY CANDIDATES")
                idx_controlled = STAGES.index("CONTROLLED ENTITY PROVENANCE")
                idx_asm = STAGES.index("ASM CLOSURE")
                self.assertEqual(idx_sprite, idx_vdp + 1)
                self.assertEqual(idx_gameplay, idx_sprite + 1)
                self.assertEqual(idx_controlled, idx_gameplay + 1)
                self.assertEqual(STAGES.index("GENERIC RECURSIVE CLOSURE"), idx_controlled + 1)
                self.assertEqual(idx_asm, STAGES.index("GENERIC RECURSIVE CLOSURE") + 1)
            else:
                idx_asm = STAGES.index("ASM CLOSURE")
                self.assertEqual(idx_asm, idx_vdp + 1)
        else:
            idx_asm = STAGES.index("ASM CLOSURE")
            self.assertEqual(idx_asm, idx_audio + 1)

    def test_j_all_generated_artifacts_validate_against_schemas(self) -> None:
        self._require_real_evidence()
        artifacts = {
            "postrun_audio_analysis.json": "oasis.m12.postrun-audio-analysis.v1",
            "postrun_audio_candidates.json": "oasis.m12.postrun-audio-candidates.v1",
            "postrun_audio_provenance.json": "oasis.m12.postrun-audio-provenance.v1",
            "postrun_audio_receipt.json": "oasis.m12.postrun-audio-receipt.v1",
        }
        for fname, schema_name in artifacts.items():
            fpath = self.out_dir / fname
            self.assertTrue(fpath.is_file(), f"Missing artifact: {fname}")
            doc = json.loads(fpath.read_text(encoding="utf-8"))
            self.assertEqual(doc.get("schema"), schema_name, f"Schema mismatch for {fname}")

    def test_k_all_modified_and_new_python_files_under_500_lines(self) -> None:
        checked_files = [
            ROOT / "tools" / "bizhawk-native-ring" / "live_forward_audio_stage.py",
            ROOT / "tools" / "bizhawk-native-ring" / "live_forward_complete_pipeline.py",
            ROOT / "tools" / "bizhawk-native-ring" / "live_forward_postrun_receipts.py",
            ROOT / "tools" / "bizhawk-native-ring" / "live_forward_postrun_window.py",
            ROOT / "tools" / "bizhawk-native-ring" / "live_forward_progress.py",
            ROOT / "tests" / "live_forward_audio_stage_test.py",
            ROOT / "tests" / "live_forward_postrun_receipts_test.py",
        ]
        for fpath in checked_files:
            self.assertTrue(fpath.is_file(), f"File does not exist: {fpath}")
            line_count = len(fpath.read_text(encoding="utf-8").splitlines())
            self.assertLessEqual(
                line_count, 500,
                f"File {fpath.name} exceeds 500 lines: {line_count} lines"
            )


if __name__ == "__main__":
    unittest.main()
