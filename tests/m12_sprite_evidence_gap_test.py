"""S4 evidence-gap taxonomy and report tests."""

import copy
import hashlib
import importlib.util
import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
fixture_spec = importlib.util.spec_from_file_location(
    "m12_sprite_piece_catalog_test", ROOT / "tests/m12_sprite_piece_catalog_test.py")
fixture = importlib.util.module_from_spec(fixture_spec)
assert fixture_spec.loader
fixture_spec.loader.exec_module(fixture)
catalog = fixture.catalog
gap_spec = importlib.util.spec_from_file_location(
    "m12_sprite_evidence_gap", ROOT / "src/tools/m12_sprite_evidence_gap.py")
gap = importlib.util.module_from_spec(gap_spec)
assert gap_spec.loader
gap_spec.loader.exec_module(gap)


def _build(observations):
    return catalog.build_catalog(fixture._capture(observations), hashlib.sha256(b"capture").hexdigest())


def test_a_every_incomplete_has_exact_reason():
    result = _build([fixture._observation(with_publication=False)])
    assert result["entries"][0]["classification"] == "INCOMPLETE"
    assert result["entries"][0]["gap_reasons"]


def test_b_every_observed_linkage_only_has_reason():
    observation = fixture._observation()
    observation["linkage_only"] = True
    result = _build([observation])
    assert result["entries"][0]["classification"] == "OBSERVED_LINKAGE_ONLY"
    assert "AMBIGUOUS_TEMPORAL_LINK" in result["entries"][0]["gap_reasons"]


def test_c_wrong_frame_tile_data_is_conflict():
    observation = fixture._observation()
    observation["tiles"][0]["frame"] = observation["frame"] + 1
    result = _build([observation])
    assert result["entries"][0]["classification"] == "CONFLICT"
    assert "WRONG_FRAME_TILE" in result["entries"][0]["gap_reasons"]


def test_d_wrong_frame_cram_is_conflict():
    observation = fixture._observation()
    observation["cram_frame"] = observation["frame"] + 1
    result = _build([observation])
    assert result["entries"][0]["classification"] == "CONFLICT"
    assert "WRONG_FRAME_CRAM" in result["entries"][0]["gap_reasons"]


def test_e_missing_multitile_tile_is_precise():
    observation = fixture._observation(width=2, height=3)
    observation["tiles"] = observation["tiles"][:-1]
    result = _build([observation])
    assert "PARTIAL_MULTI_TILE_CAPTURE" in result["entries"][0]["gap_reasons"]


def test_f_complete_multitile_evidence_can_be_proven():
    result = _build([fixture._observation(width=2, height=3)])
    assert result["proven_count"] == 1


def test_g_value_linkage_does_not_imply_causality():
    observation = fixture._observation()
    observation["linkage_only"] = True
    result = _build([observation])
    assert result["entries"][0]["classification"] == "OBSERVED_LINKAGE_ONLY"


def test_h_reason_counts_are_deterministic():
    observations = [fixture._observation(frame=7, with_publication=False),
                    fixture._observation(frame=8, with_publication=False)]
    first = gap.build_report(_build(observations))
    second = gap.build_report(_build(copy.deepcopy(observations)))
    assert first["reason_counts"] == second["reason_counts"]


def test_i_gap_report_is_deterministic():
    observations = [fixture._observation(frame=8), fixture._observation(frame=7)]
    first = json.dumps(gap.build_report(_build(observations)), sort_keys=True)
    second = json.dumps(gap.build_report(_build(copy.deepcopy(observations))), sort_keys=True)
    assert first == second


def test_j_s1_s2_s3_contracts_remain_available():
    result = _build([fixture._observation()])
    assert result["entries"][0]["decoded_attributes"]["tile_index"] == 1
    assert result["entries"][0]["priority_metadata"]["scanline_limit_modelled"] is False


def test_k_conflict_remains_fail_closed():
    observation = fixture._observation()
    observation["sat_bytes"] = fixture._sat(tile=2)
    result = _build([observation])
    assert result["conflict_count"] == 1


def test_l_frame_714_shape_contract_is_unchanged():
    result = _build([fixture._observation()])
    entry = result["entries"][0]
    assert len(entry["decoded_pixels"]) == 8
    assert len(entry["decoded_pixels"][0]) == 8
    assert entry["tiles"][0]["vram_address"] == 32
    assert entry["frame_coherence"]["capture_phase"] == "frame_end"


if __name__ == "__main__":
    for name, function in sorted(globals().items()):
        if name.startswith("test_"):
            function()
    print("m12 sprite evidence gap tests passed")
