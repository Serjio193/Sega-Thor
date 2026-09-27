"""A-N fail-closed tests for the S6 composed hardware-sprite frame."""

import hashlib
import importlib.util
import json
import sys
from pathlib import Path

import pytest


ROOT = Path(__file__).resolve().parents[1]
reconstruction_spec = importlib.util.spec_from_file_location(
    "m12_sprite_reconstruction", ROOT / "src/tools/m12_sprite_reconstruction.py")
reconstruction = importlib.util.module_from_spec(reconstruction_spec)
sys.modules["m12_sprite_reconstruction"] = reconstruction
assert reconstruction_spec.loader
reconstruction_spec.loader.exec_module(reconstruction)
catalog_spec = importlib.util.spec_from_file_location(
    "m12_sprite_piece_catalog", ROOT / "src/tools/m12_sprite_piece_catalog.py")
catalog = importlib.util.module_from_spec(catalog_spec)
assert catalog_spec.loader
catalog_spec.loader.exec_module(catalog)
frame_spec = importlib.util.spec_from_file_location(
    "m12_sprite_frame_artifact", ROOT / "src/tools/m12_sprite_frame_artifact.py")
frame = importlib.util.module_from_spec(frame_spec)
assert frame_spec.loader
frame_spec.loader.exec_module(frame)


def _sat(width=1, height=1, tile=1, palette=0, x=48, y=32, link=0):
    word1 = (width - 1) << 10 | (height - 1) << 8 | link
    attr = 0x8000 | palette << 13 | tile
    return [y >> 8, y & 255, word1 >> 8, word1 & 255,
            attr >> 8, attr & 255, x >> 8, x & 255]


def _observation(frame_number=7, sat_entry=0, width=1, height=1, x=48, y=32,
                 link=0, with_publication=True, tile_bytes=None):
    sat = _sat(width, height, tile=1, x=x, y=y, link=link)
    tile_bytes = tile_bytes or [0x11] * 32
    tiles = [{"tile_index": index, "vram_address": index * 32,
              "bytes": list(tile_bytes)}
             for index in range(1, 1 + width * height)]
    observation = {"frame": frame_number, "sat_entry": sat_entry,
                   "sat_address": f"0x{0xD000 + sat_entry * 8:X}", "sat_bytes": sat,
                   "shadow_address": "0xFF13CC", "shadow_bytes": sat, "tiles": tiles,
                   "cram_base_address": 0, "cram_bytes": [0] * 32}
    if with_publication:
        publication = {
            "frame": frame_number, "sequence": 3, "pc": "0x2700",
            "source": "0xFF13CC", "destination": f"0x{0xD000 + sat_entry * 8:X}",
            "length_words": 4, "source_bytes": sat,
        }
        observation["dma" if sat_entry == 0 else "publication"] = publication
    return observation


def _catalog(observations):
    capture = {"schema": catalog.CAPTURE_SCHEMA, "run_id": "test-run",
               "canonical_rom_sha256": catalog.EXPECTED_ROM, "state_writes_emitted": False,
               "frames_executed": 20, "catalog_observations": observations,
               "sat_mutation_observation_complete": True,
               "shadow_mutation_observation_complete": True,
               "sat_mutation_events": [], "shadow_mutation_events": [], "sat_base_events": []}
    return catalog.build_catalog(capture, hashlib.sha256(b"capture").hexdigest())


def _two_piece_catalog():
    return _catalog([
        _observation(sat_entry=0, x=48, y=32, link=1),
        _observation(sat_entry=1, x=52, y=32, link=0),
    ])


def test_a_sat_link_order_is_preserved():
    result = frame.build_frame_artifact(_two_piece_catalog(), frame=7)
    assert result["sat_traversal_order"] == [0, 1]
    assert [piece["sat_entry"] for piece in result["pieces"]] == [0, 1]


def test_b_all_active_entries_are_required():
    result = frame.build_frame_artifact(_two_piece_catalog(), frame=7)
    assert result["sat"]["all_active_entries_reachable"] is True
    assert result["sat"]["active_entry_count"] == 2


def test_c_exact_sat_tile_and_cram_provenance_is_retained():
    result = frame.build_frame_artifact(_two_piece_catalog(), frame=7)
    piece = result["pieces"][0]
    assert len(piece["sat"]["raw_bytes"]) == 8
    assert piece["tiles"][0]["raw_bytes"] == [0x11] * 32
    assert len(piece["cram"]["raw_bytes"]) == 32
    assert piece["evidence_references"]["observation_id"] == "test-run:7:0"


def test_d_genesis_piece_dimensions_and_bbox_are_exact():
    result = frame.build_frame_artifact(_catalog([_observation(width=2, height=3)]), frame=7)
    assert result["pieces"][0]["dimensions_pixels"] == {"width": 16, "height": 24}
    assert result["bounding_box"]["width"] == 16
    assert result["bounding_box"]["height"] == 24


def test_e_transparent_pixels_are_preserved():
    result = frame.build_frame_artifact(_catalog([_observation(tile_bytes=[0] * 32)]), frame=7)
    assert result["visible_pixel_count"] == 0
    assert all(all(row) for row in result["transparency_mask"])


def test_f_pixel_provenance_resolves_tile_and_witness():
    result = frame.build_frame_artifact(_catalog([_observation()]), frame=7)
    ref = result["pixel_provenance"][0]
    assert ref["witness_id"] == "test-run:7:0"
    assert ref["tile_index"] == 1
    assert ref["tile_pixel_index"] == 1
    assert result["pixel_provenance_index_matrix"][0][0] == 0


def test_g_earlier_sat_piece_wins_visible_overlap():
    result = frame.build_frame_artifact(_two_piece_catalog(), frame=7)
    assert result["overlap_pixel_count"] == 32
    assert set(value for row in result["winning_sat_entry_matrix"] for value in row) == {0, 1}


def test_h_offscreen_coordinates_are_not_silently_clipped():
    result = frame.build_frame_artifact(_catalog([_observation(x=0x1F0, y=0x1F0)]), frame=7)
    assert result["bounding_box"]["left"] == 0x1F0
    assert result["bounding_box"]["top"] == 0x1F0
    assert result["limitations"]["coordinate_clipping_applied"] is False


def test_i_persisted_publication_is_accepted_with_provenance():
    result = frame.build_frame_artifact(_catalog([
        _observation(frame_number=7), _observation(frame_number=8, with_publication=False)
    ]), frame=8)
    assert result["pieces"][0]["publication_provenance"] == "PERSISTED_FROM_PUBLICATION"
    assert result["pieces"][0]["persistence"]["relation"] == "PROVEN_PERSISTED_SAT_STATE"


def test_j_unproven_entry_stops_composition():
    value = _two_piece_catalog()
    value["entries"][0]["publication_provenance"] = "UNPROVEN"
    with pytest.raises(reconstruction.ReconstructionError, match="STOP_SPRITE_FRAME_NOT_FULLY_PROVEN"):
        frame.build_frame_artifact(value, frame=7)


def test_k_missing_link_target_stops_fail_closed():
    value = _catalog([_observation(link=2)])
    with pytest.raises(reconstruction.ReconstructionError, match="STOP_SPRITE_FRAME_SAT_CHAIN_INVALID"):
        frame.build_frame_artifact(value, frame=7)


def test_l_unreachable_active_entry_stops_fail_closed():
    value = _two_piece_catalog()
    value["entries"][0]["decoded_attributes"]["link"] = 0
    with pytest.raises(reconstruction.ReconstructionError, match="STOP_SPRITE_FRAME_SAT_CHAIN_INVALID"):
        frame.build_frame_artifact(value, frame=7)


def test_m_output_is_deterministic():
    first = frame.build_frame_artifact(_two_piece_catalog(), frame=7)
    second = frame.build_frame_artifact(_two_piece_catalog(), frame=7)
    assert json.dumps(first, sort_keys=True) == json.dumps(second, sort_keys=True)
    assert first["artifact_sha256"] == second["artifact_sha256"]


def test_n_limitations_do_not_claim_full_screen_rendering():
    result = frame.build_frame_artifact(_two_piece_catalog(), frame=7)
    limitations = result["limitations"]
    assert limitations["producer_to_dma_causality_proven"] is False
    assert limitations["sprite_vs_background_priority_modelled"] is False
    assert limitations["background_layers_modelled"] is False
    assert limitations["scanline_limit_modelled"] is False


if __name__ == "__main__":
    for name, function in sorted(globals().items()):
        if name.startswith("test_"):
            function()
    print("m12 sprite frame artifact tests passed")
