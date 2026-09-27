"""Comprehensive verification suite for M12 Post-Run VDP / DMA Analysis Stage V1."""

from __future__ import annotations

import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))
sys.path.insert(0, str(ROOT / "src" / "tools"))
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))

from live_forward_vdp_stage import (  # noqa: E402
    SOURCE_OWNED_CANONICAL,
    VdpProtocolDecoder,
    cluster_candidate_vdp_ranges,
    decompose_vdp_bus_event,
    run_vdp_stage,
    sha256_file,
    verify_s8_regression,
)
from live_forward_postrun_receipts import emit_postrun_receipts  # noqa: E402
from live_forward_progress import STAGES  # noqa: E402


class LiveForwardVdpStageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.rom_path = ROOT / "local-roms" / "Beyond Oasis (USA).md"
        cls.campaign_dir = (
            ROOT / "build" / "thor-evidence" / "live-worker-control" / "campaign-desktop-20260921-224537-038"
        )
        cls.receipt_path = cls.campaign_dir / "live-worker-interactive-receipt.json"
        cls._tmpdir = tempfile.TemporaryDirectory()
        cls.out_dir = Path(cls._tmpdir.name)

        if cls.receipt_path.is_file() and cls.rom_path.is_file():
            receipt = json.loads(cls.receipt_path.read_text(encoding="utf-8"))
            cls.stage_res = run_vdp_stage(receipt, cls.rom_path, cls.out_dir, total_segments=2304)
        else:
            cls.stage_res = None

    @classmethod
    def tearDownClass(cls) -> None:
        cls._tmpdir.cleanup()

    def _require_real(self) -> dict:
        if self.stage_res is None:
            self.skipTest("Real evidence archive or ROM missing")
        return self.stage_res

    def test_a_w3_v2_vdp_records_accepted(self) -> None:
        res = self._require_real()
        self.assertEqual(res["status"], "PASS")
        self.assertEqual(res["state"], "PASS")
        self.assertEqual(res["segments_processed"], 2304)
        self.assertEqual(res["vdp_control_writes"], 7468)
        self.assertEqual(res["vdp_data_writes"], 3581)

    def test_b_complete_two_word_vdp_commands_decoded(self) -> None:
        decoder = VdpProtocolDecoder()
        decoder.consume_control_word(0x4000, pc=0x100, frame=1, time=10, seq=1)
        cmd = decoder.consume_control_word(0x0003, pc=0x102, frame=1, time=10, seq=2)
        self.assertIsNotNone(cmd)
        self.assertEqual(cmd["target"], "VRAM")
        self.assertEqual(cmd["address"], 0xC000)
        self.assertEqual(cmd["direction"], "WRITE")
        self.assertFalse(cmd["dma_requested"])
        self.assertEqual(len(decoder.complete_commands), 1)

    def test_c_incomplete_command_remains_incomplete(self) -> None:
        decoder = VdpProtocolDecoder()
        decoder.consume_control_word(0x4000, pc=0x100, frame=1, time=10, seq=1)
        decoder.reset_segment()
        self.assertEqual(len(decoder.complete_commands), 0)
        self.assertEqual(len(decoder.incomplete_commands), 1)
        self.assertEqual(decoder.incomplete_commands[0]["word"], 0x4000)

    def test_d_register_writes_decoded(self) -> None:
        decoder = VdpProtocolDecoder()
        decoder.consume_control_word(0x8174, pc=0x200, frame=2, time=20, seq=5)
        decoder.consume_control_word(0x8004, pc=0x202, frame=2, time=21, seq=6)
        self.assertEqual(decoder.active_registers.get(1), 0x74)
        self.assertEqual(decoder.active_registers.get(0), 0x04)
        self.assertEqual(decoder.register_write_counts[1], 1)
        self.assertEqual(decoder.register_write_counts[0], 1)

    def test_e_vram_cram_vsram_destinations_distinguished(self) -> None:
        decoder = VdpProtocolDecoder()
        decoder.consume_control_word(0x4000, pc=0x10, frame=1, time=1, seq=1)
        c_vram = decoder.consume_control_word(0x0000, pc=0x12, frame=1, time=1, seq=2)
        self.assertEqual(c_vram["target"], "VRAM")

        decoder.consume_control_word(0xC000, pc=0x14, frame=1, time=2, seq=3)
        c_cram = decoder.consume_control_word(0x0000, pc=0x16, frame=1, time=2, seq=4)
        self.assertEqual(c_cram["target"], "CRAM")

        decoder.consume_control_word(0x4000, pc=0x18, frame=1, time=3, seq=5)
        c_vsram = decoder.consume_control_word(0x0010, pc=0x1A, frame=1, time=3, seq=6)
        self.assertEqual(c_vsram["target"], "VSRAM")

    def test_f_dma_source_destination_length_exact_where_proven(self) -> None:
        decoder = VdpProtocolDecoder()
        for r, v in ((19, 0x10), (20, 0x02), (21, 0x00), (22, 0x89), (23, 0x7F)):
            decoder.consume_control_word(0x8000 | (r << 8) | v, pc=0x300, frame=3, time=50, seq=r)
        decoder.consume_control_word(0x5000, pc=0x302, frame=3, time=51, seq=100)
        cmd = decoder.consume_control_word(0x0083, pc=0x304, frame=3, time=51, seq=101)
        self.assertTrue(cmd["dma_requested"])
        dma = cmd["dma"]
        self.assertEqual(dma["classification"], "DMA_EXACT")
        self.assertEqual(dma["dma_type"], "68K_BUS")
        self.assertEqual(dma["source_domain"], "68K_RAM")
        self.assertEqual(dma["source_address"], "0xFF1200")
        self.assertEqual(dma["destination_domain"], "VRAM")
        self.assertEqual(dma["destination_address"], "0xD000")
        self.assertEqual(dma["length_words"], 0x0210)
        self.assertEqual(dma["length_bytes"], 0x0420)

    def test_g_non_exact_dma_remains_partial_incomplete(self) -> None:
        decoder = VdpProtocolDecoder()
        decoder.consume_control_word(0x8000 | (19 << 8) | 0x20, pc=0x400, frame=4, time=60, seq=1)
        decoder.consume_control_word(0x8000 | (20 << 8) | 0x00, pc=0x402, frame=4, time=60, seq=2)
        decoder.consume_control_word(0x5000, pc=0x404, frame=4, time=61, seq=3)
        cmd_partial = decoder.consume_control_word(0x0083, pc=0x406, frame=4, time=61, seq=4)
        self.assertEqual(cmd_partial["dma"]["classification"], "DMA_PARTIAL")

        decoder.reset_segment()
        decoder.consume_control_word(0x5000, pc=0x410, frame=5, time=70, seq=10)
        cmd_inc = decoder.consume_control_word(0x0083, pc=0x412, frame=5, time=70, seq=11)
        self.assertEqual(cmd_inc["dma"]["classification"], "DMA_INCOMPLETE")

    def test_h_equality_only_causality_rejected(self) -> None:
        res = self._require_real()
        self.assertGreater(res["temporal_only_vdp_relations"], 0)
        self.assertEqual(res["exact_rom_to_vdp_chains"], 0)
        self.assertEqual(res["exact_ram_to_vdp_chains"], 2753)

    def test_i_frame_identity_enforced(self) -> None:
        decoder = VdpProtocolDecoder()
        decoder.consume_control_word(0x4000, pc=0x500, frame=10, time=100, seq=1)
        decoder.reset_segment()
        cmd = decoder.consume_control_word(0x0003, pc=0x502, frame=11, time=105, seq=2)
        self.assertIsNone(cmd)
        self.assertEqual(len(decoder.complete_commands), 0)
        self.assertEqual(len(decoder.incomplete_commands), 1)

    def test_j_s8_frame_779_regression_has_zero_conflicts(self) -> None:
        s8 = verify_s8_regression()
        self.assertEqual(s8["status"], "PASS")
        self.assertEqual(s8["conflicts"], 0)

    def test_k_unknown_rom_source_remains_candidate(self) -> None:
        raw_accesses = {
            0x120000: {"count": 10, "frames": {100}, "pcs": {"0x0027BE"}, "domains": {"VRAM"}},
            0x120020: {"count": 5, "frames": {100}, "pcs": {"0x0027BE"}, "domains": {"VRAM"}},
        }
        candidates = cluster_candidate_vdp_ranges(raw_accesses)
        self.assertEqual(len(candidates), 1)
        c = candidates[0]
        self.assertEqual(c["physical_start"], "0x120000")
        self.assertEqual(c["physical_end"], "0x120020")
        self.assertEqual(c["classification"], "VDP_RESOURCE_CANDIDATE")

    def test_l_source_owned_unchanged(self) -> None:
        res = self._require_real()
        self.assertEqual(res["source_owned_before"], SOURCE_OWNED_CANONICAL)
        self.assertEqual(res["source_owned_after"], SOURCE_OWNED_CANONICAL)
        self.assertEqual(res["source_owned_delta"], 0)

    def test_m_idempotent_deterministic_receipt(self) -> None:
        self._require_real()
        receipt_file = self.out_dir / "postrun_vdp_receipt.json"
        self.assertTrue(receipt_file.is_file())
        receipt_data = json.loads(receipt_file.read_text(encoding="utf-8"))
        self.assertEqual(receipt_data["schema"], "oasis.m12.postrun-vdp-receipt.v1")
        self.assertEqual(receipt_data["status"], "PASS")
        self.assertEqual(receipt_data["state"], "PASS")
        self.assertEqual(receipt_data["source_owned_delta"], 0)
        self.assertIn("postrun_vdp_analysis_sha256", receipt_data["output_hashes"])
        self.assertIn("postrun_vdp_dma_sha256", receipt_data["output_hashes"])
        self.assertIn("postrun_vdp_registers_sha256", receipt_data["output_hashes"])
        self.assertIn("postrun_vdp_provenance_sha256", receipt_data["output_hashes"])
        self.assertIn("postrun_vdp_candidates_sha256", receipt_data["output_hashes"])

    def test_n_pipeline_invokes_stage_automatically(self) -> None:
        self.assertIn("VDP / DMA ANALYSIS", STAGES)
        audio_idx = STAGES.index("AUDIO ANALYSIS")
        vdp_idx = STAGES.index("VDP / DMA ANALYSIS")
        self.assertEqual(vdp_idx, audio_idx + 1)
        if "SPRITE / SAT ANALYSIS" in STAGES:
            sprite_idx = STAGES.index("SPRITE / SAT ANALYSIS")
            gameplay_idx = STAGES.index("GAMEPLAY RAM / ENTITY CANDIDATES")
            controlled_idx = STAGES.index("CONTROLLED ENTITY PROVENANCE")
            asm_idx = STAGES.index("ASM CLOSURE")
            self.assertEqual(sprite_idx, vdp_idx + 1)
            self.assertEqual(gameplay_idx, sprite_idx + 1)
            self.assertEqual(controlled_idx, gameplay_idx + 1)
            self.assertEqual(STAGES.index("GENERIC RECURSIVE CLOSURE"), controlled_idx + 1)
            self.assertEqual(asm_idx, STAGES.index("GENERIC RECURSIVE CLOSURE") + 1)
        else:
            asm_idx = STAGES.index("ASM CLOSURE")
            self.assertEqual(asm_idx, vdp_idx + 1)

    def test_o_durable_real_vdp_oracle_v2_validation(self) -> None:
        oracle_path = ROOT / "build" / "thor-evidence" / "oracles" / "vdp" / "vdp_oracle_v2_f3" / "raw_capture.json"
        if not oracle_path.is_file():
            self.skipTest("Durable VDP Oracle V2 raw capture not present")
        res = verify_s8_regression(oracle_path)
        self.assertEqual(res["status"], "PASS")
        self.assertEqual(res["conflicts"], 0)
        self.assertEqual(res["commands_decoded"], 9)
        self.assertEqual(res["dma_decoded"], 3)
        receipt_path = ROOT / "docs" / "reports" / "THOR_M12_VDP_ORACLE_V2_RECEIPT.json"
        self.assertTrue(receipt_path.is_file())
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
        self.assertEqual(receipt["oracle_id"], "vdp_oracle_v2_f3")
        self.assertEqual(receipt["conflict_count"], 0)
        self.assertEqual(receipt["source_owned_delta"], 0)

    def test_p_exact_width_decomposition_contract(self) -> None:
        # 1. 32-bit value whose upper word is zero -> MUST be 2 words (0x0000, 0x1234)
        ev_32_zero_hi = {"value": 0x00001234, "width": 32}
        words_32 = decompose_vdp_bus_event(ev_32_zero_hi)
        self.assertEqual(words_32, [0x0000, 0x1234])
        self.assertEqual(len(words_32), 2)

        # 2. 16-bit value with magnitude 0xFFFF -> MUST be 1 word (0xFFFF)
        ev_16_max = {"value": 0xFFFF, "width": 16}
        words_16 = decompose_vdp_bus_event(ev_16_max)
        self.assertEqual(words_16, [0xFFFF])
        self.assertEqual(len(words_16), 1)

        # 3. width == UNKNOWN / invalid / missing -> STOP / unresolved oracle event
        for bad_width in ("UNKNOWN", None, 0, 24, "32"):
            with self.assertRaises(ValueError) as ctx:
                decompose_vdp_bus_event({"value": 0x1234, "width": bad_width})
            self.assertIn("STOP_UNRESOLVED_ORACLE_EVENT_WIDTH", str(ctx.exception))


if __name__ == "__main__":
    unittest.main()
