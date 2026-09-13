import copy
import json
import sys
from pathlib import Path

ROOT = Path(__file__).parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))

from thor_evidence.dense_gate import coverage_certificate


FIXTURE = ROOT / "tests/fixtures/thor_evidence_v1_gate_coverage/negative_cases.json"


def mutate(base, mutations):
    result = copy.deepcopy(base)
    for mutation in mutations:
        op = mutation["op"]
        if op == "remove":
            result["instructions"].pop(mutation["index"])
        elif op == "swap":
            left, right = mutation["left"], mutation["right"]
            result["instructions"][left], result["instructions"][right] = result["instructions"][right], result["instructions"][left]
        elif op == "classify":
            result["instructions"][mutation["index"]]["classification"] = mutation["value"]
        elif op == "interrupt":
            result["interruption"]["status"] = mutation["value"]
        elif op == "handler_incomplete":
            result["interruption"] = {"status": "INTERRUPTION_INCLUDED", "handlers_complete": False}
        else:
            raise AssertionError(f"unknown fixture mutation: {op}")
    return result


def test_dense_base_keeps_partial_and_same_value_writers():
    fixture = json.loads(FIXTURE.read_text(encoding="utf-8"))
    result = coverage_certificate(fixture["base"])
    assert result["status"] == "PROVEN"
    assert result["WRITE_TARGET_OVERLAP"] == 2


def test_adversarial_dense_fixtures_refuse_proven():
    fixture = json.loads(FIXTURE.read_text(encoding="utf-8"))
    failures = []
    for case in fixture["cases"]:
        result = coverage_certificate(mutate(fixture["base"], case["mutations"]))
        if result["status"] == "PROVEN":
            failures.append(case["id"])
    assert not failures, failures


if __name__ == "__main__":
    test_dense_base_keeps_partial_and_same_value_writers()
    test_adversarial_dense_fixtures_refuse_proven()
    print("PASS dense base and adversarial coverage fixtures")
