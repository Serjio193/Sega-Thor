"""A-O deterministic tests for S7 Genesis sprite scanline evaluation."""

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
raster_spec = importlib.util.spec_from_file_location(
    "m12_sprite_scanline_raster", ROOT / "src/tools/m12_sprite_scanline_raster.py")
raster = importlib.util.module_from_spec(raster_spec)
assert raster_spec.loader
raster_spec.loader.exec_module(raster)


def _mode(h40=True):
    return {"schema": "synthetic.mode.v1", "canonical_rom_sha256": raster.EXPECTED_ROM,
            "state_writes_emitted": False,
            "vdp_controls": [
                {"frame": 0, "register": 1, "data": 0x34},
                {"frame": 0, "register": 12, "data": 0x81 if h40 else 0x80},
            ]}


def _frame(count=1, width_cells=1, x_start=128, y=128, links=None,
           visible=True, x_values=None, flips=None):
    links = links or {index: index + 1 for index in range(count - 1)}
    links[count - 1] = 0
    pieces = []
    points = []
    for index in range(count):
        x = x_values[index] if x_values else x_start + index * width_cells * 8
        flip_h, flip_v = (flips[index] if flips else (False, False))
        pieces.append({"traversal_index": index, "sat_entry": index,
                       "decoded_attributes": {"x": x, "y": y, "width_cells": width_cells,
                                              "height_cells": 1, "link": links[index],
                                              "flip_h": flip_h, "flip_v": flip_v,
                                              "tile_index": 1, "palette_line": 0,
                                              "priority": True},
                       "dimensions_pixels": {"width": width_cells * 8, "height": 8},
                       "screen_position": {"x": x, "y": y},
                       "tiles": [], "cram": {}, "sat": {}, "shadow_sat": {},
                       "evidence_references": {"observation_id": f"synthetic:7:{index}"}})
        if visible:
            for local_x in (0, width_cells * 8 - 1):
                points.append((index, x, y, local_x))
    left = min(piece["decoded_attributes"]["x"] for piece in pieces)
    right = max(piece["decoded_attributes"]["x"] + piece["dimensions_pixels"]["width"] for piece in pieces)
    top, bottom = y, y + 8
    matrix = [[None for _ in range(right - left)] for _ in range(bottom - top)]
    refs = [[None for _ in row] for row in matrix]
    provenance = []
    for index, x, point_y, local_x in points:
        column = x - left + local_x
        row = point_y - top
        ref = len(provenance)
        provenance.append({"witness_id": f"synthetic:7:{index}", "sat_entry": index,
                           "piece_local_x": local_x, "piece_local_y": 0,
                           "screen_x": x + local_x, "screen_y": point_y,
                           "tile_index": 1, "tile_pixel_index": 1,
                           "palette_line": 0, "cram_index": 1, "priority": True,
                           "rgb": [255, 255, 255]})
        matrix[row][column] = 1
        refs[row][column] = ref
    return {"schema": raster.FRAME_SCHEMA, "status": "PASS_ONE_PROVEN_HARDWARE_SPRITE_FRAME_V1",
            "run_id": "synthetic", "frame": 7, "canonical_rom_sha256": raster.EXPECTED_ROM,
            "artifact_sha256": "s6-synthetic", "sat_traversal_order": list(range(count)),
            "pieces": pieces, "bounding_box": {"left": left, "top": top, "right": right,
                                                  "bottom": bottom, "width": right - left,
                                                  "height": bottom - top},
            "sprite_only_pixel_matrix": matrix, "pixel_provenance_index_matrix": refs,
            "pixel_provenance": provenance}


def _build(frame, h40=True):
    return raster.build_raster_artifact(frame, _mode(h40))


def test_a_below_sprite_count_limit_accepts_all():
    result = _build(_frame(count=2))
    row = result["scanlines"][0]
    assert row["hardware_sprite_budget_used"] == 2
    assert not row["overflow_flags"]["sprite_count_overflow"]


def test_b_exact_sprite_count_limit_is_deterministic():
    result = _build(_frame(count=20, width_cells=1))
    row = result["scanlines"][0]
    assert row["hardware_sprite_budget_used"] == 20
    assert len(row["accepted_sprites"]) == 20


def test_c_count_limit_suppresses_later_sprite():
    result = _build(_frame(count=21, width_cells=1))
    row = result["scanlines"][0]
    assert row["overflow_flags"]["sprite_count_overflow"]
    assert row["rejected_suppressed_sprites"][-1]["sat_entry"] == 20
    assert row["rejected_suppressed_sprites"][-1]["reason"] == "SPRITE_COUNT_LIMIT"


def test_d_below_pixel_budget_accepts_all():
    result = _build(_frame(count=10, width_cells=2))
    row = result["scanlines"][0]
    assert row["hardware_pixel_budget_used"] == 160
    assert not row["overflow_flags"]["sprite_pixel_overflow"]


def test_e_exact_pixel_budget_accepts_all():
    result = _build(_frame(count=20, width_cells=2))
    row = result["scanlines"][0]
    assert row["hardware_pixel_budget_used"] == 320
    assert len(row["accepted_sprites"]) == 20


def test_f_pixel_budget_exceeded_partially_accepts_final_sprite():
    result = _build(_frame(count=14, width_cells=3))
    row = result["scanlines"][0]
    assert row["overflow_flags"]["sprite_pixel_overflow"]
    assert row["accepted_sprites"][-1]["evaluation_width_pixels"] == 8
    assert row["accepted_sprites"][-1]["pixel_budget_partial"] is True


def test_g_sat_order_is_used_instead_of_x_order():
    result = _build(_frame(count=2, x_values=[200, 128]))
    assert result["sat_traversal_order"] == [0, 1]
    assert result["scanlines"][0]["evaluation_order"] == [0, 1]


def test_h_transparent_sprite_still_consumes_evaluation_budget():
    result = _build(_frame(count=1, visible=False))
    row = result["scanlines"][0]
    assert row["hardware_pixel_budget_used"] == 8
    assert result["surviving_pixel_count"] == 0


def test_i_partially_offscreen_sprite_is_counted_and_clipped_only_in_screen_raster():
    result = _build(_frame(count=1, x_values=[120]))
    row = result["scanlines"][0]
    assert row["hardware_pixel_budget_used"] == 8
    assert result["surviving_pixel_count"] == 2
    assert result["screen_surviving_pixel_count"] == 0


def test_j_fully_offscreen_sprite_remains_hardware_relevant():
    result = _build(_frame(count=1, x_values=[0]))
    assert result["scanlines"][0]["hardware_sprite_budget_used"] == 1
    assert result["screen_surviving_pixel_count"] == 0


def test_k_x_zero_masks_subsequent_sprite_and_records_trigger():
    result = _build(_frame(count=3, x_values=[136, 0, 144]))
    row = result["scanlines"][0]
    assert row["mask_source_sat_entry"] == 1
    assert row["rejected_suppressed_sprites"][-1]["reason"] == "X0_MASKED_BY_SAT_ENTRY"
    assert row["rejected_suppressed_sprites"][-1]["mask_source_sat_entry"] == 1


def test_l_hflip_vflip_do_not_change_evaluation_identity():
    result = _build(_frame(count=2, flips=[(True, True), (False, True)]))
    assert result["scanlines"][0]["evaluation_order"] == [0, 1]
    assert [item["sat_entry"] for item in result["scanlines"][0]["accepted_sprites"]] == [0, 1]


def test_m_no_limit_preserves_s6_logical_pixels():
    source = _frame(count=2)
    result = _build(source)
    assert result["logical_s6_pixel_matrix"] == result["hardware_filtered_pixel_matrix"]
    assert result["removed_pixel_count"] == 0


def test_n_artifact_generation_is_deterministic_and_truth_is_explicit():
    source = _frame(count=2)
    first, second = _build(source), _build(source)
    assert json.dumps(first, sort_keys=True) == json.dumps(second, sort_keys=True)
    assert first["truth_classification"]["overflow_behavior_runtime_witness"] == "NOT_OBSERVED"
    assert first["mode"]["horizontal_mode"] == "H40"


def test_o_h32_mode_derives_smaller_limits():
    result = _build(_frame(count=17), h40=False)
    row = result["scanlines"][0]
    assert result["mode"]["horizontal_mode"] == "H32"
    assert row["sprite_budget_limit"] == 16
    assert row["pixel_budget_limit"] == 256
    assert row["rejected_suppressed_sprites"][-1]["reason"] == "SPRITE_COUNT_LIMIT"


def test_p_missing_mode_register_fails_closed():
    with pytest.raises(reconstruction.ReconstructionError, match="STOP_SPRITE_MODE_REGISTER_12_MISSING"):
        raster.build_raster_artifact(_frame(), {"canonical_rom_sha256": raster.EXPECTED_ROM,
                                                "state_writes_emitted": False,
                                                "vdp_controls": [{"frame": 0, "register": 1, "data": 0x34}]})


if __name__ == "__main__":
    for name, function in sorted(globals().items()):
        if name.startswith("test_"):
            function()
    print("m12 sprite scanline raster tests passed")
