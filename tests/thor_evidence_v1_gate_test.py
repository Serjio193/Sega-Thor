"""Fail-closed local FF13CC V1-gate certificates and adversarial fixtures."""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))

from thor_evidence.v1_gate import (A372, A374, ROM_SHA, TARGET, coverage_certificate,
                                   interruption_certificate, validate_pairing,
                                   validate_static, validate_versions, value_versions)


def static_fixture():
    return {"status": "PROVEN", "rom_sha256": ROM_SHA,
            "instruction": {"address": "0x00A372", "bytes": "2AC2",
                             "mnemonic": "MOVE.L", "form": "MOVE.L D2,(A5)+",
                             "width_bytes": 4, "effective_address": "A5 postincrement",
                             "length_bytes": 2, "next_pc": "0x00A374"},
            "checked_static": {"producer_tool": "fixture", "producer_sha256": "a" * 64,
                                "decoder_source": "fixture", "decoder_sha256": "b" * 64,
                                "contract": "fixture"}}


def pairing_fixture(**changes):
    value = {"status": "PROVEN", "exec_pc": "0x00A372", "callback_pc": "0x00A374",
             "post_pc": "0x00A374", "destination": "0xFF13CC", "exec_seq": 10,
             "write_seq": 11, "post_seq": 12, "value": "00880901",
             "pre": {"A5": "0xFF13CC"}, "post": {"A5": "0xFF13D0"},
             "raw_witnesses": {"header_schema": "thor.evidence.raw.v0.1", "events": [10, 11, 12]}}
    value.update(changes)
    return value


def test_static_certificate():
    assert validate_static(static_fixture())
    broken = static_fixture(); broken["instruction"]["width_bytes"] = 2
    assert not validate_static(broken)


def test_pairing_rejects_pc_heuristics_and_bad_order():
    static = static_fixture()
    assert validate_pairing(pairing_fixture(), static)
    assert not validate_pairing(pairing_fixture(callback_pc="0x00A376"), static)
    assert not validate_pairing(pairing_fixture(write_seq=12, post_seq=11), static)
    assert not validate_pairing(pairing_fixture(destination="0x00FF13D0"), static)


def test_negative_cases_are_not_proven():
    cases = json.loads((ROOT / "tests/fixtures/thor_evidence_v1_gate/negative_cases.json").read_text())
    assert cases["epochs"] == [1, 2]
    for case in cases["cases"][:5]:
        if "coverage" in case:
            assert coverage_certificate(case["coverage"])["status"] == "BLOCKED"
        elif "pairing" in case:
            assert not validate_pairing(case["pairing"], static_fixture())
        else:
            assert interruption_certificate(case["interruption"])["status"] == "BLOCKED"


def test_overlap_and_same_value_are_retained_as_writers():
    cases = json.loads((ROOT / "tests/fixtures/thor_evidence_v1_gate/negative_cases.json").read_text())["cases"]
    for case in cases[-2:]:
        result = coverage_certificate(case["coverage"])
        assert result["status"] == "PROVEN"
        assert result["classifications"][0]["classification"] == "WRITE_TARGET_OVERLAP"


def test_interruption_requires_explicit_boundary():
    assert interruption_certificate({"status": "NONE", "control_flow_continuous": True})["status"] == "PROVEN"
    assert interruption_certificate({"status": "INCLUDED", "handlers_complete": True})["status"] == "PROVEN"
    assert interruption_certificate({"status": "UNKNOWN"})["status"] == "BLOCKED"


def test_four_byte_versions_are_distinct_and_local():
    pairing = pairing_fixture()
    pairing.update(trace="c" * 64, epoch=1, execution_instance="epoch-1-seq-10")
    versions = value_versions(pairing)
    assert validate_versions(versions, pairing)
    assert [item["byte_offset"] for item in versions] == [0, 1, 2, 3]
    assert not validate_versions(versions[:3], pairing)


def main():
    tests = (test_static_certificate, test_pairing_rejects_pc_heuristics_and_bad_order,
             test_negative_cases_are_not_proven, test_overlap_and_same_value_are_retained_as_writers,
             test_interruption_requires_explicit_boundary, test_four_byte_versions_are_distinct_and_local)
    for test in tests:
        test(); print("PASS", test.__name__)


if __name__ == "__main__":
    raise SystemExit(main())
