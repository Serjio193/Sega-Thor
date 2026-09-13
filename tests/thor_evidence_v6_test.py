"""V6 scenario isolation and differential manifest tests."""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))

from thor_evidence.differential import DifferentialSet


ROM = "e" * 64


def test_differential_is_observation_only():
    diff = DifferentialSet(ROM)
    neutral = diff.add_scenario("neutral-1", "env-a", "a" * 64)
    repeat = diff.add_scenario("neutral-2", "env-a", "b" * 64,
                               repeat_of=neutral.id)
    alternate = diff.add_scenario("alternate", "env-b", "c" * 64,
                                  arm="MOTIVATED_ALTERNATE", motivation="right-arm")
    for scenario in (neutral, repeat, alternate):
        diff.add_event(scenario.id, "event-" + scenario.id, 10, "WRITE")
    shared = {"kind": "RAM_VALUE", "location": "FF13CC", "value": "80",
              "status": "OBSERVED", "event_ref": "event-neutral-1"}
    diff.add_fact(neutral.id, shared)
    diff.add_fact(repeat.id, {**shared, "event_ref": "event-neutral-2"})
    diff.add_fact(alternate.id, {**shared, "event_ref": "event-alternate"})
    manifest = diff.compare()
    assert len(manifest["common"]) == 1
    assert manifest["common"][0]["observation_only"]
    assert manifest["common"][0]["causal"] is False
    assert manifest["cross_scenario_edges"] == []
    assert manifest["causal_claims"] == []
    assert diff.export()["ownership"]["source_owned_delta"] == 0


def test_graph_rejects_cross_scenario_and_bad_arms():
    diff = DifferentialSet(ROM)
    first = diff.add_scenario("one", "env", "1" * 64)
    second = diff.add_scenario("two", "env", "2" * 64)
    diff.add_event(first.id, "e1", 0, "EXEC")
    diff.add_event(second.id, "e2", 0, "EXEC")
    try:
        diff.add_edge(first.id, "e1", "e2", "TEMPORAL")
    except ValueError:
        pass
    else:
        raise AssertionError("cross-scenario edge must be rejected")
    try:
        diff.add_scenario("bad", "env", "3" * 64, arm="MOTIVATED_ALTERNATE")
    except ValueError:
        pass
    else:
        raise AssertionError("alternate without motivation must be rejected")


def main():
    test_differential_is_observation_only()
    test_graph_rejects_cross_scenario_and_bad_arms()
    print("PASS thor evidence v6")


if __name__ == "__main__":
    main()
