"""S8 A-U tests for exact logical Genesis VDP composition."""

import importlib.util
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("m12_sprite_reconstruction", ROOT / "src/tools/m12_sprite_reconstruction.py")
reconstruction = importlib.util.module_from_spec(spec)
sys.modules["m12_sprite_reconstruction"] = reconstruction
assert spec.loader
spec.loader.exec_module(reconstruction)
vdp_spec = importlib.util.spec_from_file_location("m12_vdp_frame_artifact", ROOT / "src/tools/m12_vdp_frame_artifact.py")
vdp = importlib.util.module_from_spec(vdp_spec)
assert vdp_spec.loader
vdp_spec.loader.exec_module(vdp)


def _word(data, address, value):
    data[address:address + 2] = bytes((value >> 8, value & 255))


def _capture():
    vram = bytearray(0x10000)
    vram[32:36] = bytes((0x10, 0, 0, 0))
    _word(vram, 0, 1)
    _word(vram, 0x2000, 1 | 0x8000)
    _word(vram, 0x4000, 1 | (1 << 13))
    _word(vram, 0x8000, 0)
    _word(vram, 0x8002, 0)
    regs = [1, 2, 3, 4, 5, 7, 10, 11, 12, 13, 16, 17, 18]
    values = {1: 0x40, 2: 0, 3: 0x10, 4: 1, 5: 0x68, 7: 2,
              10: 0, 11: 0, 12: 1, 13: 0x20, 16: 0, 17: 0x81, 18: 0x80}
    events = [{"frame": 7, "sequence": i, "register": reg, "value": values[reg]}
              for i, reg in enumerate(regs)]
    return {"schema": vdp.CAPTURE_SCHEMA, "run_id": "synthetic", "frame": 7,
            "vram_frame": 7, "cram_frame": 7, "vsram_frame": 7,
            "canonical_rom_sha256": vdp.EXPECTED_ROM, "state_writes_emitted": False,
            "register_events": events, "vram": list(vram), "cram": [0] * 0x80,
            "vsram": [0] * 0x50}


def _sprites():
    return {"schema": vdp.RASTER_SCHEMA, "status": vdp.S7_STATUS, "frame": 7,
            "artifact_sha256": "s7", "pixel_provenance": []}


def _s6():
    return {"frame": 7, "pixel_provenance": []}


@pytest.fixture(scope="module")
def artifact():
    return vdp.build_artifact(_capture(), _sprites(), _s6())


def test_a_plane_a_basic_cell_decode(artifact):
    assert artifact["planes"]["PLANE_A"]["visible_cells"][0]["tile_index"] == 1


def test_b_plane_b_basic_cell_decode(artifact):
    assert artifact["planes"]["PLANE_B"]["table_base"] == 0x2000


def test_c_window_replaces_plane_a_in_region(artifact):
    assert artifact["composition"][0]["window_region"] is False
    assert artifact["composition"][319]["window_region"] is True


def test_d_horizontal_scroll_wrap():
    capture = _capture()
    capture["vram"][0x8000:0x8002] = [0x03, 0xFF]
    regs = vdp._registers(capture, 7)
    vdp._CURRENT_VSRAM = bytes(capture["vsram"])
    pixel = vdp._plane_pixel(bytes(capture["vram"]), bytes(capture["cram"]), regs,
                             "PLANE_A", 0, 0, (32, 32), True)
    assert pixel["tile_local_x"] == 7


def test_e_vertical_scroll_wrap():
    capture = _capture()
    capture["vsram"][0:2] = [0x03, 0xFF]
    regs = vdp._registers(capture, 7)
    vdp._CURRENT_VSRAM = bytes(capture["vsram"])
    pixel = vdp._plane_pixel(bytes(capture["vram"]), bytes(capture["cram"]), regs,
                             "PLANE_A", 0, 0, (32, 32), True)
    assert pixel["tile_local_y"] == 7


def test_f_plane_size_addressing():
    assert vdp._plane_size(0x00) == (32, 32)
    assert vdp._plane_size(0x11) == (64, 64)
    assert vdp._plane_size(0x33) == (128, 128)
    with pytest.raises(reconstruction.ReconstructionError, match="PLANE_SIZE_UNSUPPORTED"):
        vdp._plane_size(0x22)


def test_g_tile_hflip():
    assert reconstruction.decode_tile_attributes(0x0800).flip_h


def test_h_tile_vflip():
    assert reconstruction.decode_tile_attributes(0x1000).flip_v


def test_i_palette_selection():
    assert reconstruction.decode_tile_attributes(0x6000).palette == 3


def test_j_transparent_plane_pixel(artifact):
    assert artifact["composition"][1]["winner"]["layer"] == "BACKDROP"


def test_k_backdrop_selection(artifact):
    assert artifact["composition"][1]["winner"]["cram_index"] == 2


def test_l_low_priority_sprite_vs_plane():
    assert vdp._priority_rank({"layer": "SPRITE", "priority": False}) > vdp._priority_rank({"layer": "PLANE_A", "priority": False})


def test_m_high_priority_sprite_vs_plane():
    assert vdp._priority_rank({"layer": "SPRITE", "priority": True}) > vdp._priority_rank({"layer": "PLANE_A", "priority": True})


def test_n_plane_a_vs_plane_b_priority():
    assert vdp._priority_rank({"layer": "PLANE_A", "priority": False}) > vdp._priority_rank({"layer": "PLANE_B", "priority": False})


def test_o_window_priority_behavior():
    assert vdp._priority_rank({"layer": "WINDOW", "priority": True}) > vdp._priority_rank({"layer": "PLANE_A", "priority": True})


def test_p_exact_per_pixel_provenance(artifact):
    winner = artifact["composition"][0]["winner"]
    assert {"layer", "cram_index", "rgb"}.issubset(winner)


def test_q_wrong_frame_vram_rejected():
    capture = _capture()
    capture["vram_frame"] = 8
    with pytest.raises(reconstruction.ReconstructionError, match="FRAME_MISMATCH"):
        vdp.build_artifact(capture, _sprites(), _s6())


def test_r_wrong_cram_rejected():
    capture = _capture()
    capture["cram_frame"] = 8
    with pytest.raises(reconstruction.ReconstructionError, match="CRAM_FRAME_MISMATCH"):
        vdp.build_artifact(capture, _sprites(), _s6())


def test_s_missing_scroll_evidence_stops():
    capture = _capture()
    capture["vram"] = capture["vram"][:0x8000]
    with pytest.raises(reconstruction.ReconstructionError, match="VRAM_MISSING"):
        vdp.build_artifact(capture, _sprites(), _s6())


def test_t_deterministic_artifact():
    assert vdp.build_artifact(_capture(), _sprites(), _s6()) == vdp.build_artifact(_capture(), _sprites(), _s6())


def test_u_s1_to_s7_reference_contract(artifact):
    assert artifact["sprites"]["s7_schema"] == vdp.RASTER_SCHEMA
