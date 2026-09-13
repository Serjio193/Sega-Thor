import importlib.util
import json
import tempfile
from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


FOLLOWUP = load(
    "live_discovery_followup",
    ROOT / "src/tools/thor_evidence/live_discovery_followup.py")
ANALYZE = load(
    "live_discovery_followup_analyze",
    ROOT / "src/tools/thor_evidence/live_discovery_followup_analyze.py")


class FollowupTest(unittest.TestCase):
    def test_rank_prefers_af22_stride_contract(self):
        addresses = [0x167DD8 + 6 * index for index in range(18)]
        report = {
            "investigations_created": [
                {"id": "af22", "pc": 0xAF22, "rom_ram_address": address,
                 "span": [0x167DD8, 0x167E48],
                 "current_classification": "NEW_ROM_ACTIVITY"}
                for address in addresses
            ] + [{"id": "other", "pc": 0x288A, "rom_ram_address": 0xC0000,
                  "span": [0xC0000, 0xC1000],
                  "current_classification": "NEW_ROM_ACTIVITY"}],
            "source_owned_change": 0,
        }
        ranked = FOLLOWUP.rank_investigations(report, [])
        self.assertEqual(ranked[0]["pc"], 0xAF22)
        self.assertEqual(ranked[0]["dominant_stride"], 6)

    def test_causal_edge_requires_source_value_and_consumer(self):
        dependency = {"consumer_pc": 0xAF20, "definition_pc": 0xAF06}
        events = [
            {"kind": "EXEC_FOCUS", "seq": 10, "frame": 2120,
             "data": {"instruction_pc": 0xAF06}},
            {"kind": "RAM_SOURCE_READ", "seq": 11, "frame": 2120,
             "data": {"address": 0x1234, "source_base": 0x1234,
                      "value": 0x0016, "last_exec_pc": 0xAF06}},
            {"kind": "RAM_SOURCE_READ", "seq": 12, "frame": 2120,
             "data": {"address": 0x1236, "source_base": 0x1234,
                      "value": 0x7DD8, "last_exec_pc": 0xAF06}},
            {"kind": "ROM_READ_FOCUS", "seq": 14, "frame": 2120,
             "data": {"address": 0x167DD8, "last_exec_pc": 0xAF20,
                      "registers": {"A0": 0x167DD8}}},
        ]
        result = ANALYZE.prove_causal_edges(events, dependency)
        self.assertEqual(result["status"], "PROVEN")
        self.assertEqual(result["proven"][0]["source_value"], 0x167DD8)


if __name__ == "__main__":
    unittest.main()
