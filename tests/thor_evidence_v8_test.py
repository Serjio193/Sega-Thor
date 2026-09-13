"""V8 held-out frontier evaluation tests."""
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))

from thor_evidence.heldout import HeldOutEvaluation
from thor_evidence.identity import ROM_SHA


def runtime_report():
    return {"schema": "oasis.m68k.natural-reach.v1", "scenario_id": "heldout-runtime",
            "backend": "bounded-runtime-discovery", "rom_sha256": ROM_SHA,
            "frames_executed": 4, "target_addresses": ["0x03BDD8"],
            "target_reached": True, "target_frame": 3, "target_sequence": 8}


def test_runtime_contract_and_unknown_join():
    evaluation = HeldOutEvaluation()
    result = evaluation.runtime_discovery(runtime_report())
    assert result["status"] == "OBSERVED"
    try:
        evaluation.runtime_discovery({**runtime_report(), "rom_sha256": "0" * 64})
    except ValueError:
        pass
    else:
        raise AssertionError("wrong ROM must be rejected")
    try:
        evaluation.runtime_discovery({"rom_sha256": ROM_SHA})
    except ValueError:
        pass
    else:
        raise AssertionError("incomplete runtime receipt must be rejected")


def test_real_local_frontier_if_available():
    rom_path = ROOT / "build/reference/Beyond Oasis (USA).bin"
    report_path = ROOT / "build/reference/natural-report.json"
    if not rom_path.exists() or not report_path.exists():
        return
    import json
    result = HeldOutEvaluation().evaluate(rom_path.read_bytes(), json.loads(report_path.read_text()))
    assert result["static"]["frontier_id"] == result["frontier"]["id"]
    assert result["join"]["status"] == "UNKNOWN"
    assert result["ownership"]["source_owned_delta"] == 0


def main():
    test_runtime_contract_and_unknown_join()
    test_real_local_frontier_if_available()
    print("PASS thor evidence v8")


if __name__ == "__main__":
    main()
