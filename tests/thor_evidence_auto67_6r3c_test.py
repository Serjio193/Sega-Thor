import unittest
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))

from auto67_predecessor import register_writer_candidate_report  # noqa: E402


class Auto676R3CTest(unittest.TestCase):
    def test_static_writer_candidates_are_deduplicated_and_include_canary(self):
        rom = bytearray(0x3000)
        rom[0x2234:0x223A] = bytes.fromhex("4BF900FF134C")
        rom[0x27BE:0x27C4] = bytes.fromhex("49F900C00004")
        report = register_writer_candidate_report(bytes(rom))
        candidates = report["candidates"]

        self.assertEqual(report["unique_count"], len(candidates))
        self.assertEqual(report["duplicate_pcs"], 0)
        self.assertEqual(candidates[0x2234] & 2, 2)
        self.assertEqual(candidates[0x27BE] & 1, 1)
        self.assertTrue(report["distribution"])

    def test_runtime_source_has_no_canary_producer_configuration(self):
        source = (ROOT / "src/tools/thor_evidence/capture/predecessor_burst.lua").read_text(
            encoding="utf-8"
        )
        self.assertNotIn("0x002234", source)
        self.assertNotIn("0x0027BE", source)
        self.assertIn("read_targets", source)
        self.assertIn("target_path", source)

    def test_real_generic_idle_run_keeps_full_source_count_truthful(self):
        path = ROOT / "build/auto67-6r3c-targeted-idle-500.json"
        if not path.exists():
            self.skipTest("real generic hook artifact is not present")
        data = __import__("json").loads(path.read_text(encoding="utf-8"))
        config = data["prehistory_config"]
        prehistory = data["lua"]["prehistory"]
        self.assertEqual(config["writer_candidate_count"], 30042)
        self.assertEqual(config["writer_installed_count"], 500)
        self.assertEqual(prehistory["source_candidate_count"], 30042)
        self.assertEqual(prehistory["installed_hook_count"], 500)
        self.assertEqual(prehistory["hook_install_errors"], 0)
        self.assertEqual(data["raw_event_backlog"], 0)


if __name__ == "__main__":
    unittest.main()
