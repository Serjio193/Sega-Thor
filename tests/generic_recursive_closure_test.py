"""Regression tests for generic RAM -> ROM selector -> SAT closure."""

from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))
from generic_recursive_closure import RecursiveClosure  # noqa: E402


class GenericClosureTests(unittest.TestCase):
    def test_flow_records_add_only_rom_verified_observed_cfg_edges(self):
        rom = bytes.fromhex("4E75 4E71 4E71")
        corpus = {"records": [
            {"kind": "instruction", "pc": 0, "opcode": 0x4E75, "address": 2},
            {"kind": "instruction", "pc": 0, "opcode": 0x4E75, "address": 2},
            {"kind": "instruction", "pc": 2, "opcode": 0x4E71, "address": 4},
            {"kind": "instruction", "pc": 4, "opcode": 0x4E75, "address": 6},
            {"kind": "bus", "pc": 0, "opcode": 1, "address": 0xFF0000},
        ]}
        result = RecursiveClosure(corpus, rom).run()
        edges = result["graph"]["edges"]
        self.assertEqual(result["cfg_edges"], 2)
        self.assertEqual(result["total_facts"], 2)
        self.assertEqual(result["flow_metrics"]["instruction_records"], 4)
        self.assertEqual(result["flow_metrics"]["bus_records"], 1)
        self.assertEqual(result["flow_metrics"]["rejected_instruction_identities"], 1)
        self.assertEqual(sum(edge["kind"] == "OBSERVED_NEXT_PC" for edge in edges), 2)
        self.assertEqual(result["gap_ranking"][0]["blocker_class"],
                         "MISSING_REGISTER_SNAPSHOT_OR_DECODED_OPERANDS")

    def test_selector_chain_is_recovered_without_pc_rules(self):
        corpus = {
            "source_owned_before": 1487672,
            "rom_ranges": [{"start": 0x1742E2, "end": 0x1742E4, "exact": True,
                             "canonical_bytes": "0076", "source_bytes": "0076"}],
            "instructions": [
                {"pc": 0x3B376, "op": "LEA", "dst": "A6", "src": 0xFFAFCE},
                {"pc": 0x3B428, "op": "MOVE", "dst": "D0", "src": {"kind": "memory", "base": "A6", "displacement": 8, "width": 2}},
                {"pc": 0x3B90A, "op": "LEA", "dst": "A1", "src": 0x3BE82},
                {"pc": 0x3B910, "op": "LEA", "dst": "A3", "src": 0x3B90A},
                {"pc": 0x3B914, "op": "MOVE", "dst": "A4", "src": {"kind": "memory", "base": "A3", "width": 4}},
                {"pc": 0x3B918, "op": "MOVE", "dst": "A5", "src": {"kind": "memory", "base": "A4", "width": 4}},
                {"pc": 0x3B982, "op": "MOVE", "dst": "A2", "src": {"kind": "memory", "base": "A5", "index": "D0", "scale": 2, "width": 4}},
                {"pc": 0x3B990, "op": "MOVE", "dst": "D1", "src": {"kind": "memory", "base": "A2", "width": 2}},
                {"pc": 0x3B9A0, "op": "LEA", "dst": "A0", "src": 0x174358},
                {"pc": 0x3B448, "op": "JSR", "src": 0xB730},
                {"pc": 0xB73C, "op": "MOVE", "dst": "A0", "src": 0x17435A},
            ],
            "memory": [
                {"kind": "ram", "address": 0xFFAFD6, "width": 2, "value": 3},
                {"kind": "rom", "address": 0x3B90A, "width": 4, "value": 0x3B982},
                {"kind": "rom", "address": 0x3B982, "width": 4, "value": 0x1742DC},
                {"kind": "rom", "address": 0x3BE82, "width": 2, "value": 3},
                {"kind": "rom", "address": 0x1742E2, "width": 4, "value": 0x1742E2,
                 "base": 0x1742DC, "stride": 2, "index": 3, "exact": True, "pc": 0x3B982},
                {"kind": "rom", "address": 0x1742E2, "width": 2, "value": 0x76,
                 "base": 0x1742DC, "stride": 2, "index": 3, "exact": True, "pc": 0x3B990},
            ],
        }
        result = RecursiveClosure(corpus).run()
        regs = result["graph"]["register_values"]
        self.assertEqual(regs["A6"]["value"], 0xFFAFCE)
        self.assertEqual(regs["D0"]["value"], 3)
        self.assertEqual(regs["A4"]["value"], 0x3B982)
        self.assertEqual(regs["A5"]["value"], 0x1742DC)
        self.assertEqual(regs["A2"]["value"], 0x1742E2)
        self.assertEqual(regs["D1"]["value"], 0x76)
        self.assertEqual(result["calls"][0]["caller"], "0x0003B448")
        self.assertEqual(result["calls"][0]["callee"], "0x0000B730")
        self.assertTrue(any(edge["kind"] == "INDEXES" for edge in result["graph"]["edges"]))
        self.assertEqual(result["source_owned_before"], 1487672)
        self.assertEqual(result["source_owned_after"], 1487674)
        self.assertEqual(result["map_operations"][0]["op"], "PROMOTE_SOURCE_OWNED")
        self.assertTrue(result["map_operations"][0]["byte_roundtrip"])

    def test_missing_evidence_is_queued_and_does_not_stop_other_facts(self):
        result = RecursiveClosure({"instructions": [
            {"pc": 1, "op": "LEA", "dst": "A0", "src": 0x100},
            {"pc": 2, "op": "MOVE", "dst": "D0", "src": {"kind": "memory", "base": "A0", "width": 2}},
            {"pc": 3, "op": "LEA", "dst": "A1", "src": 0x200},
        ]}).run()
        self.assertEqual(result["graph"]["register_values"]["A1"]["status"], "EXACT")
        self.assertTrue(result["queue"])

    def test_fixpoint_retries_and_clears_resolved_out_of_order_facts(self):
        result = RecursiveClosure({"instructions": [
            {"pc": 1, "op": "MOVE", "dst": "D1", "src": "D0"},
            {"pc": 2, "op": "MOVEQ", "dst": "D0", "src": 7},
        ]}).run()
        self.assertEqual(len(result["iterations"]), 3)
        self.assertEqual(result["unresolved"], [])
        self.assertEqual(result["queue"], [])
        self.assertEqual(result["graph"]["register_values"]["D1"]["value"], 7)


if __name__ == "__main__":
    unittest.main(verbosity=2)
