"""S5 SAT persistence and ordered publication tests."""

import copy
import hashlib
import importlib.util
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
fixture_spec = importlib.util.spec_from_file_location(
    "m12_sprite_piece_catalog_test", ROOT / "tests/m12_sprite_piece_catalog_test.py")
fixture = importlib.util.module_from_spec(fixture_spec)
assert fixture_spec.loader
fixture_spec.loader.exec_module(fixture)
catalog = fixture.catalog


def _capture(observations, **overrides):
    value = fixture._capture(observations)
    value.update({"sat_mutation_observation_complete": True,
                  "shadow_mutation_observation_complete": True,
                  "sat_mutation_events": [], "shadow_mutation_events": [],
                  "sat_base_events": [], **overrides})
    return value


def _build(observations, **overrides):
    value = _capture(observations, **overrides)
    return catalog.build_catalog(value, hashlib.sha256(b"capture").hexdigest())


def _later(frame=8):
    value = fixture._observation(frame=frame, with_publication=False)
    value["shadow_bytes"] = value["sat_bytes"]
    value["causality"] = {"status": "UNPROVEN"}
    return value


def test_a_direct_publication_is_distinct():
    result = _build([fixture._observation()])
    entry = result["entries"][0]
    assert entry["publication_provenance"] == "DIRECT_PUBLICATION"


def test_b_unchanged_sat_persists_with_complete_watch():
    result = _build([fixture._observation(frame=7), _later(8)])
    entry = result["entries"][1]
    assert entry["classification"] == "PROVEN"
    assert entry["publication_provenance"] == "PERSISTED_FROM_PUBLICATION"
    assert entry["persistence"]["relation"] == "PROVEN_PERSISTED_SAT_STATE"


def test_c_unrelated_entry_write_does_not_reset_selected_entry():
    result = _build([fixture._observation(frame=7), _later(8)],
                    sat_mutation_events=[{"frame": 8, "sequence": 4,
                                          "destination_start": 0xD008,
                                          "destination_end": 0xD010}])
    assert result["entries"][1]["publication_provenance"] == "PERSISTED_FROM_PUBLICATION"


def test_d_overlapping_sat_write_invalidates_persistence():
    result = _build([fixture._observation(frame=7), _later(8)],
                    sat_mutation_events=[{"frame": 8, "sequence": 4,
                                          "destination_start": 0xD000,
                                          "destination_end": 0xD008}])
    assert result["entries"][1]["classification"] == "INCOMPLETE"


def test_e_sat_base_change_rejects_persistence():
    result = _build([fixture._observation(frame=7), _later(8)],
                    sat_base_events=[{"frame": 8, "sequence": 4, "sat_base": 0xC000}])
    assert result["entries"][1]["classification"] == "INCOMPLETE"


def test_f_unknown_mutation_coverage_rejects_persistence():
    result = _build([fixture._observation(frame=7), _later(8)],
                    sat_mutation_observation_complete=False)
    assert result["entries"][1]["classification"] == "INCOMPLETE"


def test_g_shadow_persistence_is_explicit():
    result = _build([fixture._observation(frame=7), _later(8)])
    assert result["entries"][1]["shadow_persistence"]["relation"] == "PROVEN_PERSISTED_SHADOW_SAT_STATE"


def test_h_ordered_producer_dma_relation_is_retained():
    observation = fixture._observation()
    observation["causality"] = {"status": "ORDERED_PRODUCER_WRITE_TO_DMA",
                                 "producer_event_id": "7:2", "dma_event_id": "7:3",
                                 "source_offset": 0, "destination_offset": 0,
                                 "ordering_relation": "producer_write_sequence_lt_dma_sequence"}
    result = _build([observation])
    assert result["entries"][0]["causality"]["producer_event_id"] == "7:2"


def test_i_unordered_relation_is_not_invented():
    result = _build([fixture._observation()])
    assert result["entries"][0]["causality"]["status"] == "UNPROVEN"


def test_j_wrong_later_vram_bytes_is_conflict():
    later = _later(8)
    later["sat_bytes"] = fixture._sat(tile=2)
    result = _build([fixture._observation(frame=7), later])
    assert result["entries"][1]["classification"] == "CONFLICT"


def test_k_s4_direct_entries_remain_direct():
    result = _build([fixture._observation(frame=7), _later(8)])
    assert result["proven_count"] == 2
    assert result["direct_publication_count"] == 1
    assert result["persisted_publication_count"] == 1


def test_l_persistence_artifact_is_deterministic():
    observations = [fixture._observation(frame=7), _later(8)]
    first = _build(observations)
    second = _build(copy.deepcopy(observations))
    assert first == second


if __name__ == "__main__":
    for name, function in sorted(globals().items()):
        if name.startswith("test_"):
            function()
    print("m12 sprite persistence tests passed")
