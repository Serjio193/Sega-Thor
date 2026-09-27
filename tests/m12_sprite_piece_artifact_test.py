"""A-K contract tests for the one-piece hardware witness artifact."""

from copy import deepcopy
import hashlib
import importlib.util
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
reconstruction_spec = importlib.util.spec_from_file_location(
    "m12_sprite_reconstruction", ROOT / "src/tools/m12_sprite_reconstruction.py")
reconstruction = importlib.util.module_from_spec(reconstruction_spec)
sys.modules["m12_sprite_reconstruction"] = reconstruction
assert reconstruction_spec.loader
reconstruction_spec.loader.exec_module(reconstruction)
artifact_spec = importlib.util.spec_from_file_location(
    "m12_sprite_piece_artifact", ROOT / "src/tools/m12_sprite_piece_artifact.py")
artifact = importlib.util.module_from_spec(artifact_spec)
assert artifact_spec.loader
artifact_spec.loader.exec_module(artifact)


def _capture() -> dict:
    sat = [0x00, 0x20, 0x00, 0x00, 0x80, 0x01, 0x00, 0x30]
    tile = [0x12, 0x30, 0x00, 0x00] * 8
    cram = [0] * 32
    cram[2:4] = [0x0E, 0xEE]
    return {
        "schema": artifact.CAPTURE_SCHEMA,
        "emulator": "synthetic",
        "canonical_rom_sha256": artifact.EXPECTED_ROM,
        "scenario_path": "synthetic",
        "state_writes_emitted": False,
        "witnesses": [{
            "frame": 7, "sat_index": 0, "sat_address": "0xD000",
            "shadow_address": "0xFF13CC", "sat_bytes": sat,
            "shadow_bytes": sat, "tile_bytes": tile, "cram_bytes": cram,
            "dma": {"frame": 7, "sequence": 3, "pc": "0x2700",
                    "source": "0xFF13CC", "destination": "0xD000",
                    "length_words": 4, "source_bytes": sat},
            "producer_writes": [{"frame": 7, "sequence": 2,
                                 "address": "0xFF13CC", "value": "0x20"}],
            "producer_executions": [{"frame": 7, "sequence": 1,
                                     "pc": "0xB752", "address": "0xFF13CC",
                                     "bytes": sat}],
        }],
    }


def _build(capture=None):
    capture = capture or _capture()
    return artifact.build_artifact(capture, hashlib.sha256(b"capture").hexdigest())


def _reject(change) -> None:
    value = _capture()
    change(value)
    try:
        _build(value)
    except reconstruction.ReconstructionError:
        return
    raise AssertionError("invalid hardware witness was accepted")


def test_a_exact_sat_decode() -> None:
    assert _build()["sat"]["decoded"]["tile_index"] == 1


def test_b_tile_address_is_genesis_index_times_32() -> None:
    assert _build()["tile"]["vram_address"] == 32


def test_c_corrected_packed_nibble_decoder() -> None:
    pixels = _build()["pixels"]
    assert pixels[0][0]["pixel_index"] == 1
    assert pixels[0][1]["pixel_index"] == 2


def test_d_palette_line_maps_to_cram_index() -> None:
    assert _build()["pixels"][0][0]["cram_index"] == 1


def test_e_zero_nibble_is_transparent() -> None:
    assert _build()["pixels"][0][3] is None


def test_f_priority_is_retained_without_background_claim() -> None:
    result = _build()
    assert result["sat"]["decoded"]["priority"]
    assert not result["reconstruction_metadata"]["sprite_vs_background_priority_modelled"]


def test_g_shadow_and_vram_sat_are_exact() -> None:
    result = _build()
    assert result["shadow_sat"]["raw_bytes"] == result["sat"]["raw_bytes"]


def test_h_wrong_frame_is_rejected() -> None:
    _reject(lambda value: value["witnesses"][0]["dma"].update(frame=8))


def test_i_wrong_tile_bytes_are_rejected() -> None:
    _reject(lambda value: value["witnesses"][0].update(tile_bytes=[0] * 31))


def test_j_wrong_cram_bytes_are_rejected() -> None:
    _reject(lambda value: value["witnesses"][0].update(cram_bytes=[0] * 31))


def test_k_missing_producer_link_is_rejected() -> None:
    _reject(lambda value: value["witnesses"][0].update(producer_writes=[]))


if __name__ == "__main__":
    for name, function in sorted(globals().items()):
        if name.startswith("test_"):
            function()
    print("m12 sprite piece artifact tests passed")
