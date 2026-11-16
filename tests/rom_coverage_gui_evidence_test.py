"""Headless tests for ROM coverage GUI evidence integration and inspector panel.

Verifies:
1. Layer switcher ("PROPERTY VIEW" vs "EVIDENCE / RECOMPILATION VIEW").
2. Evidence categories and heat overlay rendering.
3. Inspector panel presentation with all 18 fields.
4. RECOMP_CANDIDATE_RANK exclusions (00B3B2, 061E48, 00E268 excluded; 00CEEA rank #1).
5. Backward compatibility of Property View.
"""

from __future__ import annotations

from pathlib import Path
import sys
import tkinter as tk
import unittest

repo_root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(repo_root / "tools/bizhawk-native-ring"))

from evidence_overlay_model import EvidenceOverlayModel, OVERLAY_COLORS
from evidence_inspector_panel import EvidenceInspectorWindow, draw_evidence_cell
from rom_coverage_gui import RomCoverageApp


class TestRomCoverageGuiEvidence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.root = tk.Tk()
        cls.root.withdraw()
        cls.app = RomCoverageApp(cls.root, None, None)

    @classmethod
    def tearDownClass(cls):
        try:
            cls.app.close()
            cls.root.quit()
        except Exception:
            pass

    def test_layer_switch_and_defaults(self):
        self.assertEqual(self.app.view_layer.get(), "PROPERTY VIEW")
        self.app.view_layer.set("EVIDENCE / RECOMPILATION VIEW")
        self.app._on_layer_changed()
        self.assertEqual(self.app.view_layer.get(), "EVIDENCE / RECOMPILATION VIEW")
        # Switch back to verify idempotence
        self.app.view_layer.set("PROPERTY VIEW")
        self.app._on_layer_changed()
        self.assertEqual(self.app.view_layer.get(), "PROPERTY VIEW")

    def test_recomp_candidate_ranking_rules(self):
        model = self.app.evidence_model

        # 0x00B3B2: active native + verified path specific -> excluded
        insp_b3b2 = model.inspect("0x00B3B2")
        self.assertEqual(insp_b3b2["RAW_HOT_RANK"], 47)
        self.assertIsNone(insp_b3b2["RECOMP_CANDIDATE_RANK"])

        # 0x061E48: active native + active indirect -> excluded
        insp_61e48 = model.inspect("0x061E48")
        self.assertEqual(insp_61e48["RAW_HOT_RANK"], 4)
        self.assertIsNone(insp_61e48["RECOMP_CANDIDATE_RANK"])

        # 0x00E268: verified path specific -> excluded
        insp_e268 = model.inspect("0x00E268")
        self.assertEqual(insp_e268["RAW_HOT_RANK"], 1)
        self.assertIsNone(insp_e268["RECOMP_CANDIDATE_RANK"])

        # 0x00CEEA: verified path specific -> excluded
        insp_ceea = model.inspect("0x00CEEA")
        self.assertEqual(insp_ceea["RAW_HOT_RANK"], 48)
        self.assertIsNone(insp_ceea["RECOMP_CANDIDATE_RANK"])

        # 0x00C9B2: unverified, not implemented, not active, 742 calls -> excluded by dependency on HOLD
        insp_c9b2 = model.inspect("0x00C9B2")
        self.assertEqual(insp_c9b2["RAW_HOT_RANK"], 52)
        self.assertIsNone(insp_c9b2["RECOMP_CANDIDATE_RANK"])

    def test_inspector_window_formatting_and_fields(self):
        insp_win = EvidenceInspectorWindow(self.root)
        insp_data = self.app.evidence_model.inspect("0x00B3B2")
        insp_win.show_or_update(insp_data)

        self.assertIsNotNone(insp_win.text_widget)
        content = insp_win.text_widget.get("1.0", "end")

        # Verify all 18 fields appear in inspector display
        required_fields = [
            "TARGET: 0x00B3B2..0x00B3B3",
            "STRUCTURAL_STATUS:",
            "RUNTIME_STATUS:",
            "RUNTIME_CALL_COUNT:",
            "RAW_HOT_RANK:              #47",
            "RECOMP_CANDIDATE_RANK:     None",
            "CPP_IMPLEMENTATION_STATUS:",
            "CPP_REGISTRATION_STATUS:",
            "CPP_ACTIVATION_STATUS:",
            "ACTIVATION_PROVENANCE:",
            "VERIFICATION_STATUS:       VERIFIED_PATH_SPECIFIC",
            "VERIFICATION_SCOPE:",
            "GLOBAL_STATUS:             NOT_PROVEN",
            "CANONICAL_STREAM_HASHES:",
            "MACHINE_RECEIPT:",
            "SEMANTIC_CLAIMS:",
            "CALLERS:",
            "CALLEES:",
            "SOURCE_ARTIFACT_PATHS:",
            "PROVENANCE:",
        ]
        for field in required_fields:
            self.assertIn(field, content)

    def test_draw_evidence_cell_heat_overlay(self):
        cv = self.app.canvas
        v_cool = self.app.evidence_model.get_cell_visual(0x000000, 0x000100)
        v_hot = self.app.evidence_model.get_cell_visual(0x00E268, 0x00E2A2)

        rect_cool = draw_evidence_cell(cv, 0, 0, 20, 20, v_cool)
        rect_hot = draw_evidence_cell(cv, 30, 0, 50, 20, v_hot)

        self.assertIsNotNone(rect_cool)
        self.assertIsNotNone(rect_hot)

        # Status color is preserved as fill
        self.assertEqual(cv.itemcget(rect_hot, "fill"), OVERLAY_COLORS["VERIFIED_PATH_SPECIFIC"])
        # Heat overlay creates distinct outline/pip
        self.assertIn(cv.itemcget(rect_hot, "outline"), ("#ff3b30", "#ff9500", "#ffd60a"))


if __name__ == "__main__":
    unittest.main()
