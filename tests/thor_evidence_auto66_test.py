import importlib.util
import tempfile
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "auto66_campaign", ROOT / "src/tools/thor_evidence/auto66_campaign.py")
AUTO66 = importlib.util.module_from_spec(SPEC)
assert SPEC.loader
SPEC.loader.exec_module(AUTO66)


class Auto66Test(unittest.TestCase):
    def test_planner_rejects_prior_capture_and_selects_other_scenario(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            first = root / "first.txt"
            second = root / "second.txt"
            first.write_text("scenario_id=first\nstart_state=hardware_reset\n"
                             "stop_condition=max_frames:100\ntarget_address=0x100\n", encoding="utf-8")
            second.write_text("scenario_id=second\nstart_state=hardware_reset\n"
                              "stop_condition=max_frames:200\ntarget_address=0x200\n"
                              "input frame=10 port=1 buttons=Start\n", encoding="utf-8")
            pool = [AUTO66.scenario_metadata(first), AUTO66.scenario_metadata(second)]
            selected, decisions = AUTO66.plan_next_scenario(
                pool, {pool[0]["planner_fingerprint"]}, 2)
            self.assertEqual(selected["id"], "second")
            self.assertEqual(decisions[0]["decision"], "REJECTED_EQUIVALENT")
            self.assertEqual(decisions[1]["decision"], "SELECTED")

    def test_planner_reaches_fixed_point_when_pool_is_equivalent(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "only.txt"
            path.write_text("scenario_id=only\nstart_state=hardware_reset\n"
                            "stop_condition=max_frames:100\n", encoding="utf-8")
            item = AUTO66.scenario_metadata(path)
            selected, decisions = AUTO66.plan_next_scenario(
                [item], {item["planner_fingerprint"]}, 0)
            self.assertIsNone(selected)
            self.assertEqual(decisions[0]["decision"], "REJECTED_EQUIVALENT")

    def test_scenario_fingerprint_includes_watch_and_input_file_identity(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            left = root / "left.txt"
            right = root / "right.txt"
            left.write_text("scenario_id=x\nstart_state=hardware_reset\n"
                            "stop_condition=max_frames:100\ntarget_address=0x100\n", encoding="utf-8")
            right.write_text("scenario_id=x\nstart_state=hardware_reset\n"
                             "stop_condition=max_frames:100\ntarget_address=0x101\n", encoding="utf-8")
            self.assertNotEqual(AUTO66.scenario_metadata(left)["planner_fingerprint"],
                                AUTO66.scenario_metadata(right)["planner_fingerprint"])


if __name__ == "__main__":
    unittest.main()
