import copy
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))

from master_v2_contribution_boundary import (STOP_CONFLICT, apply_contribution,
                                             contribution_hash, forensic_replay,
                                             mark_absorbed_exact, verify_only)


def contribution(run_id=7, count=5):
    return {"run_id": run_id, "rom": {"sha256": "rom", "size": 3},
            "source_evidence": {"raw_sha256": "raw", "segments": 1},
            "segments": 1, "records": count,
            "instruction_occurrences": {"0:1": count},
            "relation_occurrences": {"0:EXECUTED_NEXT:2": count},
            "terminal_occurrences": {"0:2": 1},
            "new_instruction_identities": ["0:1"],
            "new_relation_identities": ["0:EXECUTED_NEXT:2"],
            "provenance": {}, "canonical": {}, "source_owned_delta": 0,
            "asm": {"promoted_bytes": 0}, "parent_master_generation": "N"}


class ContributionBoundaryTests(unittest.TestCase):
    def test_a_new_run_applies_once(self):
        state = {}
        result = apply_contribution(state, contribution())
        self.assertEqual(result["status"], "APPLIED_ONCE")
        self.assertEqual(state["aggregates"]["instruction_occurrences"]["0:1"], 5)

    def test_b_identical_replay_is_no_delta(self):
        state = {}; item = contribution(); apply_contribution(state, item)
        before = copy.deepcopy(state)
        self.assertEqual(verify_only(state, item)["status"], "COMMITTED_EXACT")
        self.assertEqual(apply_contribution(state, item)["status"], "NO_DELTA_ALREADY_APPLIED")
        self.assertEqual(state, before)

    def test_c_different_contribution_conflicts(self):
        state = {}; apply_contribution(state, contribution())
        with self.assertRaisesRegex(ValueError, STOP_CONFLICT):
            apply_contribution(state, contribution(count=6))

    def test_d_existing_fact_occurrence_still_increments(self):
        state = {}; apply_contribution(state, contribution(7, 5))
        apply_contribution(state, contribution(8, 3))
        self.assertEqual(state["aggregates"]["instruction_occurrences"]["0:1"], 8)

    def test_e_second_replay_does_not_increment(self):
        state = {}; item = contribution(); apply_contribution(state, item)
        apply_contribution(state, item)
        self.assertEqual(state["aggregates"]["relation_occurrences"]["0:EXECUTED_NEXT:2"], 5)

    def test_f_forensic_replay_writes_no_state(self):
        state = {"run_contributions": [], "aggregates": {}}
        before = copy.deepcopy(state)
        result = forensic_replay(state, lambda: contribution())
        self.assertEqual(result["status"], "FORENSIC_ONLY")
        self.assertEqual(state, before)

    def test_g_parent_delta_can_be_compared_to_child(self):
        parent = {}; item = contribution(); child = copy.deepcopy(parent)
        apply_contribution(child, item)
        self.assertEqual(child["aggregates"]["instruction_occurrences"], {"0:1": 5})
        self.assertEqual(contribution_hash(item), child["run_contributions"][0]["contribution_hash"])

    def test_h_lineage_without_hash_is_not_committed(self):
        self.assertEqual(verify_only({"run_contributions": []}, contribution())["status"],
                         "NOT_COMMITTED")

    def test_i_invalid_run_is_not_accepted(self):
        with self.assertRaisesRegex(ValueError, "INVALID_RUN_ID"):
            apply_contribution({}, contribution(0))

    def test_j_absorption_requires_stage_5_to_8(self):
        item = contribution()
        with self.assertRaisesRegex(ValueError, "ABSORPTION_PROOF_INCOMPLETE"):
            mark_absorbed_exact(item, {"STAGE_5": "PASS", "STAGE_6": "PASS"})
        self.assertEqual(mark_absorbed_exact(item, {"STAGE_5": "PASS", "STAGE_6": "NO_DELTA",
                                                    "STAGE_7": "NO_DELTA", "STAGE_8": "PASS"})[
                                                    "commit_state"], "ABSORBED_EXACT")

    def test_k_ambiguous_state_is_not_deletable_by_boundary(self):
        state = {"run_contributions": [{"run_id": 7, "contribution_hash": "other"}]}
        with self.assertRaisesRegex(ValueError, STOP_CONFLICT):
            verify_only(state, contribution())

    def test_l_boundary_has_no_legacy_write_operation(self):
        state = {}; apply_contribution(state, contribution())
        self.assertNotIn("legacy_generation", state)

    def test_m_source_owned_delta_is_preserved(self):
        state = {}; item = contribution(); apply_contribution(state, item)
        self.assertEqual(item["source_owned_delta"], 0)

    def test_n_hash_is_deterministic(self):
        self.assertEqual(contribution_hash(contribution()), contribution_hash(contribution()))


if __name__ == "__main__":
    unittest.main()
