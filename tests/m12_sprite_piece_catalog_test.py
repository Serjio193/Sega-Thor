"""A-L fail-closed tests for the S3 hardware sprite catalog."""

from copy import deepcopy
import hashlib
import importlib.util
import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "m12_sprite_reconstruction", ROOT / "src/tools/m12_sprite_reconstruction.py")
reconstruction = importlib.util.module_from_spec(spec)
sys.modules["m12_sprite_reconstruction"] = reconstruction
assert spec.loader
spec.loader.exec_module(reconstruction)
catalog_spec = importlib.util.spec_from_file_location(
    "m12_sprite_piece_catalog", ROOT / "src/tools/m12_sprite_piece_catalog.py")
catalog = importlib.util.module_from_spec(catalog_spec)
assert catalog_spec.loader
catalog_spec.loader.exec_module(catalog)


def _sat(width=1, height=1, tile=1, palette=0):
    word1 = (width - 1) << 10 | (height - 1) << 8
    attr = 0x8000 | palette << 13 | tile
    return [0, 0x20, word1 >> 8, word1 & 255,
            attr >> 8, attr & 255, 0, 0x30]


def _tile(seed):
    return [(seed + index) & 255 for index in range(32)]


def _observation(frame=7, sat_entry=0, width=1, height=1, with_publication=True):
    sat = _sat(width, height)
    tiles = [{"tile_index": index, "vram_address": index * 32,
              "bytes": _tile(index)}
             for index in range(1, 1 + width * height)]
    observation = {"frame": frame, "sat_entry": sat_entry,
                   "sat_address": "0xD000", "sat_bytes": sat,
                   "shadow_address": "0xFF13CC", "tiles": tiles,
                   "cram_base_address": 0, "cram_bytes": [0] * 32}
    if with_publication:
        observation.update({
            "shadow_bytes": sat,
            "dma": {"frame": frame, "sequence": 3, "pc": "0x2700",
                    "source": "0xFF13CC", "destination": "0xD000",
                    "length_words": 4, "source_bytes": sat},
        })
    return observation


def _capture(observations):
    return {"schema": catalog.CAPTURE_SCHEMA, "run_id": "test-run",
            "canonical_rom_sha256": catalog.EXPECTED_ROM,
            "state_writes_emitted": False, "frames_executed": 20,
            "catalog_observations": observations}


def _build(observations):
    return catalog.build_catalog(_capture(observations), hashlib.sha256(b"capture").hexdigest())


def test_a_one_complete_s2_witness_is_proven():
    assert _build([_observation()])["proven_count"] == 1


def test_b_missing_vram_tile_is_incomplete():
    observation = _observation()
    observation["tiles"] = []
    assert _build([observation])["incomplete_count"] == 1


def test_c_missing_cram_is_incomplete():
    observation = _observation()
    observation["cram_bytes"] = None
    assert _build([observation])["incomplete_count"] == 1


def test_d_sat_mismatch_is_conflict():
    observation = _observation()
    observation["sat_bytes"] = _sat(tile=2)
    assert _build([observation])["conflict_count"] == 1


def test_e_wrong_frame_cram_is_rejected():
    observation = _observation()
    observation["cram_frame"] = 8
    result = _build([observation])
    assert result["entries"][0]["classification"] == "CONFLICT"


def test_f_wrong_tile_hash_is_rejected():
    observation = _observation()
    observation["tiles"][0]["sha256"] = "0" * 64
    assert _build([observation])["conflict_count"] == 1


def test_g_complete_multitile_piece_is_proven():
    result = _build([_observation(width=2, height=3)])
    assert result["proven_count"] == 1
    assert len(result["entries"][0]["tiles"]) == 6


def test_h_missing_one_multitile_tile_is_incomplete():
    observation = _observation(width=2, height=3)
    observation["tiles"] = observation["tiles"][:-1]
    assert _build([observation])["incomplete_count"] == 1


def test_i_repeated_piece_keeps_observation_count():
    result = _build([_observation(frame=7), _observation(frame=8)])
    assert result["entries_total"] == 2
    assert result["semantic_fingerprints"][0]["observation_count"] == 2


def test_j_semantic_fingerprint_is_deterministic():
    first = _build([_observation()])
    second = _build([_observation()])
    assert first["entries"][0]["semantic_fingerprint"] == second["entries"][0]["semantic_fingerprint"]


def test_k_classification_counts_are_exact():
    incomplete = _observation(frame=8, with_publication=False)
    result = _build([_observation(), incomplete])
    assert (result["proven_count"], result["incomplete_count"],
            result["observed_linkage_only_count"], result["conflict_count"]) == (1, 1, 0, 0)


def test_l_catalog_is_deterministic():
    observations = [_observation(frame=8), _observation(frame=7)]
    first = json.dumps(_build(observations), sort_keys=True)
    second = json.dumps(_build(deepcopy(observations)), sort_keys=True)
    assert first == second


if __name__ == "__main__":
    for name, function in sorted(globals().items()):
        if name.startswith("test_"):
            function()
    print("m12 sprite piece catalog tests passed")
