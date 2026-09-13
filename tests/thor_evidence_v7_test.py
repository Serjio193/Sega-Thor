"""V7 bounded scheduling, ranking and two-pass exhaustion tests."""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))

from thor_evidence.frontier import FrontierScheduler


def test_rank_and_two_pass_policy():
    scheduler = FrontierScheduler(rom_size=0x1000, max_request_span=0x100)
    low = scheduler.add("LOW", 0x300, 0x320, information_gain=2, confidence=20,
                        cost=1, risk=1, evidence_classes=("STATIC", "RUNTIME"))
    high = scheduler.add("HIGH", 0x100, 0x110, information_gain=9, confidence=60,
                         cost=2, risk=4, evidence_classes=("RUNTIME",))
    assert scheduler.next_request()["frontier_id"] == high
    scheduler.record(high, progress=False)
    assert scheduler.next_request()["frontier_id"] == high
    scheduler.record(high, progress=False)
    assert high not in {item["frontier_id"] for item in scheduler.rank()}
    assert scheduler.next_request()["frontier_id"] == low
    assert scheduler.export()["whole_rom_trace"] is False


def test_bounds_and_fixed_point():
    scheduler = FrontierScheduler(rom_size=0x1000, max_request_span=0x40)
    try:
        scheduler.add("WHOLE", 0, 0x1000)
    except ValueError:
        pass
    else:
        raise AssertionError("whole-ROM frontier must be rejected")
    frontier = scheduler.add("CONFLICT", 0x20, 0x30, status="CONFLICT")
    scheduler.record(frontier, status="CONFLICT", progress=True)
    assert not scheduler.fixed_point()
    assert scheduler.export()["ownership"]["source_owned_delta"] == 0


def main():
    test_rank_and_two_pass_policy()
    test_bounds_and_fixed_point()
    print("PASS thor evidence v7")


if __name__ == "__main__":
    main()
