import importlib.util
import json
import tempfile
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


def load(name, relative):
    spec = importlib.util.spec_from_file_location(name, ROOT / relative)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


CHAIN = load("auto65_chain", "src/tools/thor_evidence/auto65_chain.py")
CAMPAIGN = load("auto65_campaign", "src/tools/thor_evidence/auto65_campaign.py")


def candidate(suffix, value=1):
    scenario = {"id": "s", "state": "hardware_reset", "family": "x"}
    nodes = [{"kind": "ROOT", "id": "RESET:hardware_reset"},
             {"kind": "WRITER", "id": "PC:0x0000026C"},
             {"kind": "RAM", "id": "RAM:" + suffix}]
    edges = [{"source": nodes[0]["id"], "target": nodes[1]["id"], "kind": "RESET_REACH"},
             {"source": nodes[1]["id"], "target": nodes[2]["id"], "kind": "WRITE", "value": value}]
    return CHAIN._candidate(scenario, nodes, edges, {"address": suffix}, "test")


class Auto65Test(unittest.TestCase):
    def test_incremental_prefix_reuse_and_replay(self):
        first = candidate("RAM:one")
        second = candidate("RAM:two")
        self.assertEqual(CHAIN.classify(first, []), "NEW_WRITER")
        self.assertEqual(CHAIN.classify(second, [first]), "NEW_BRANCH")
        self.assertEqual(CHAIN.classify(first, [first]), "KNOWN_NEW_INSTANCE")

    def test_conflict_preserves_lineage(self):
        self.assertTrue(CHAIN.conflict(candidate("RAM:one", 2), [candidate("RAM:one", 1)]))

    def test_capture_fingerprint_changes_with_watch_or_scenario(self):
        scenario = {"identity": "a", "start_state": "hardware_reset", "inputs": []}
        item = candidate("RAM:one")
        left = CAMPAIGN.capture_fingerprint(scenario, [item], [])
        right = CAMPAIGN.capture_fingerprint({**scenario, "identity": "b"}, [item], [])
        self.assertNotEqual(left, right)

    def test_scenario_selector_requires_fixed_point_and_rejects_quicksave(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            old = root / "old.txt"
            new = root / "new.txt"
            old.write_text("scenario_id=QuickSave1\n", encoding="utf-8")
            new.write_text("scenario_id=natural\nstart_state=hardware_reset\nstop_condition=max_frames:10\ninput frame=1 port=1 buttons=Start\n", encoding="utf-8")
            graph = {"nodes": {"a": {"status": "BLOCKED"}}}
            self.assertEqual(CAMPAIGN.select_scenario([old, new], graph)["id"], "natural")
            with self.assertRaises(ValueError):
                CAMPAIGN.select_scenario([new], {"nodes": {"a": {"status": "OPEN"}}})


if __name__ == "__main__":
    unittest.main()
