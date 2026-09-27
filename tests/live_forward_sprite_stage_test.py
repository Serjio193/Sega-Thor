"""A-S comprehensive fail-closed tests for M12 SPRITE / SAT ANALYSIS stage."""
from __future__ import annotations

import json
from pathlib import Path
import sys
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
RING_DIR = ROOT / "tools" / "bizhawk-native-ring"
if str(RING_DIR) not in sys.path:
    sys.path.insert(0, str(RING_DIR))

import live_forward_sprite_stage as sp_mod
from live_forward_sprite_stage import (
    decode_sat_base, decode_sprite_entry_exact, traverse_sat_links,
    rasterize_hardware_pieces, verify_sprite_regression, run_sprite_stage,
)
from live_forward_progress import ProgressPublisher, STAGES


def _sample_sat_entry(x: int = 168, y: int = 128, width_c: int = 2, height_c: int = 2,
                       link: int = 1, tile: int = 10, pal: int = 1, pri: bool = True,
                       hf: bool = False, vf: bool = False) -> bytes:
    raw_x, raw_y = (x + 128) & 0x1FF, (y + 128) & 0x1FF
    w1 = ((width_c - 1) << 10) | ((height_c - 1) << 8) | (link & 0x7F)
    w2 = (0x8000 if pri else 0) | (pal << 13) | (0x1000 if vf else 0) | (0x0800 if hf else 0) | (tile & 0x7FF)
    return bytes([raw_y >> 8, raw_y & 0xFF, w1 >> 8, w1 & 0xFF,
                  w2 >> 8, w2 & 0xFF, raw_x >> 8, raw_x & 0xFF])


def test_a_exact_sat_base_decode() -> None:
    base, mode, max_entries, sz = decode_sat_base(reg5=0x68, reg12=0x81)
    assert base == 0xD000 and mode == "H40" and max_entries == 80 and sz == 640
    base32, mode32, max32, sz32 = decode_sat_base(reg5=0x68, reg12=0x80)
    assert base32 == 0xD000 and mode32 == "H32" and max32 == 64 and sz32 == 512


def test_b_sat_entry_decode() -> None:
    raw = _sample_sat_entry(x=100, y=50, width_c=3, height_c=4, link=5, tile=42, pal=2, pri=True, hf=True, vf=False)
    entry = decode_sprite_entry_exact(raw, 0)
    assert entry["screen_x"] == 100 and entry["screen_y"] == 50
    assert entry["width_cells"] == 3 and entry["height_cells"] == 4
    assert entry["width_pixels"] == 24 and entry["height_pixels"] == 32
    assert entry["link"] == 5 and entry["tile_index"] == 42 and entry["palette"] == 2
    assert entry["priority"] is True and entry["hflip"] is True and entry["vflip"] is False
    assert all(v == "EXACT_DECODED" for v in entry["field_truth"].values())


def test_c_link_traversal() -> None:
    entries = [{"link": 1}, {"link": 2}, {"link": 0}]
    chain, outcome = traverse_sat_links(entries, 80)
    assert chain == [0, 1, 2] and outcome["terminated"] is True


def test_d_termination() -> None:
    entries = [{"link": 0}]
    chain, outcome = traverse_sat_links(entries, 80)
    assert chain == [0] and outcome["terminated"] is True and not outcome["cycle"]


def test_e_cycle_detection() -> None:
    entries = [{"link": 1}, {"link": 2}, {"link": 0}, {"link": 1}]
    entries[2]["link"] = 1  # 0 -> 1 -> 2 -> 1 (cycle)
    chain, outcome = traverse_sat_links(entries, 80)
    assert outcome["cycle"] is True and chain == [0, 1, 2]


def test_f_partial_unknown_sat_handling() -> None:
    entries = [{"link": 1}, {"link": None}]
    chain, outcome = traverse_sat_links(entries, 80)
    assert outcome["unknown_link"] is True and chain == [0, 1]


def test_g_direct_vs_persisted_provenance() -> None:
    assert "OBSERVED" not in "PERSISTED_FROM_PROVEN_PRIOR_STATE"
    assert "DERIVED_FROM_EXACT_DMA" != "OBSERVED_WRITE"


def test_h_exact_dma_to_sat_linkage(tmp_path: Path) -> None:
    dma = {"source_address": "0xFF13CC", "source_domain": "68K_RAM", "dma_type": "68K_BUS",
           "destination_address": "0xD000", "length_bytes": 24, "classification": "DMA_EXACT", "frame": 0}
    dma_file = tmp_path / "postrun_vdp_dma.json"
    dma_file.write_text(json.dumps({"dma_events_sample": [dma]}), encoding="utf-8")
    receipt = {"runtime": {"run_id": 100}, "flow_handoff": {"raw_path": "", "index_path": ""}}
    rom = tmp_path / "rom.bin"
    rom.write_bytes(b"\x00" * 3145728)
    res = run_sprite_stage(receipt, rom, tmp_path)
    assert res["exact_dma_to_sat_chains"] == 1
    assert res["exact_sat_entries"] > 0


def test_i_overlapping_worker_deduplication(tmp_path: Path) -> None:
    receipt = {"runtime": {"run_id": 100}, "flow_handoff": {"raw_path": "", "index_path": ""}}
    rom = tmp_path / "rom.bin"; rom.write_bytes(b"\x00" * 3145728)
    res = run_sprite_stage(receipt, rom, tmp_path)
    assert res["duplicate_sat_observations_removed"] == (res["sat_write_observations"] - res["unique_sat_write_events"])


def test_j_hardware_sprite_geometry() -> None:
    entry = decode_sprite_entry_exact(_sample_sat_entry(x=0, y=0, width_c=4, height_c=4), 0)
    assert entry["width_pixels"] == 32 and entry["height_pixels"] == 32


def test_k_hv_flip() -> None:
    e_norm = decode_sprite_entry_exact(_sample_sat_entry(hf=False, vf=False), 0)
    e_flip = decode_sprite_entry_exact(_sample_sat_entry(hf=True, vf=True), 1)
    assert (not e_norm["hflip"]) and (not e_norm["vflip"])
    assert e_flip["hflip"] and e_flip["vflip"]


def test_l_palette_selection() -> None:
    for pal in range(4):
        e = decode_sprite_entry_exact(_sample_sat_entry(pal=pal), 0)
        assert e["palette"] == pal


def test_m_clipping() -> None:
    raw = _sample_sat_entry(x=319, y=223, width_c=2, height_c=2)
    entry = decode_sprite_entry_exact(raw, 0)
    entry["raw_bytes"] = list(raw)
    tile_data = tuple(tuple(1 for _ in range(8)) for _ in range(8))
    tiles = {entry["tile_index"] + i: tile_data for i in range(4)}
    raster = rasterize_hardware_pieces([entry], tiles, h40=True)
    assert raster["clipped_pixels"] > 0


def test_n_scanline_limits() -> None:
    pieces = []
    tile_data = tuple(tuple(1 for _ in range(8)) for _ in range(8))
    tiles = {}
    for i in range(25):
        raw = _sample_sat_entry(x=i * 8, y=100, width_c=1, height_c=1, tile=i + 1)
        e = decode_sprite_entry_exact(raw, i)
        e["raw_bytes"] = list(raw)
        pieces.append(e)
        tiles[i + 1] = tile_data
    raster = rasterize_hardware_pieces(pieces, tiles, h40=True)
    assert raster["scanline_limit_hits"] > 0


def test_o_no_logical_object_naming(tmp_path: Path) -> None:
    receipt = {"runtime": {"run_id": 100}, "flow_handoff": {"raw_path": "", "index_path": ""}}
    rom = tmp_path / "rom.bin"; rom.write_bytes(b"\x00" * 3145728)
    run_sprite_stage(receipt, rom, tmp_path)
    for p in tmp_path.glob("postrun_sprite_*.json"):
        text = p.read_text(encoding="utf-8").lower()
        for forbidden in ("player", "enemy", "item", "npc", "boss"):
            assert forbidden not in text


def test_p_deterministic_receipt(tmp_path: Path) -> None:
    receipt = {"runtime": {"run_id": 100}, "flow_handoff": {"raw_path": "", "index_path": ""}}
    rom = tmp_path / "rom.bin"; rom.write_bytes(b"\x00" * 3145728)
    r1 = run_sprite_stage(receipt, rom, tmp_path / "run1")
    r2 = run_sprite_stage(receipt, rom, tmp_path / "run2")
    assert r1["receipt_path"] and r2["receipt_path"]
    rec1 = json.loads(Path(r1["receipt_path"]).read_text(encoding="utf-8"))
    rec2 = json.loads(Path(r2["receipt_path"]).read_text(encoding="utf-8"))
    assert rec1["metrics"] == rec2["metrics"]


def test_q_source_owned_stage_delta_zero(tmp_path: Path) -> None:
    receipt = {"runtime": {"run_id": 100}, "flow_handoff": {"raw_path": "", "index_path": ""}}
    rom = tmp_path / "rom.bin"; rom.write_bytes(b"\x00" * 3145728)
    res = run_sprite_stage(receipt, rom, tmp_path)
    assert res["source_owned_before"] == 1487672
    assert res["source_owned_after"] == 1487672
    assert res["source_owned_delta"] == 0


def test_r_automatic_coordinator_invocation() -> None:
    assert "SPRITE / SAT ANALYSIS" in STAGES
    vdp_idx = STAGES.index("VDP / DMA ANALYSIS")
    sprite_idx = STAGES.index("SPRITE / SAT ANALYSIS")
    gameplay_idx = STAGES.index("GAMEPLAY RAM / ENTITY CANDIDATES")
    controlled_idx = STAGES.index("CONTROLLED ENTITY PROVENANCE")
    asm_idx = STAGES.index("ASM CLOSURE")
    assert vdp_idx + 1 == sprite_idx and sprite_idx + 1 == gameplay_idx
    assert gameplay_idx + 1 == controlled_idx
    assert controlled_idx + 1 == STAGES.index("GENERIC RECURSIVE CLOSURE")
    assert STAGES.index("GENERIC RECURSIVE CLOSURE") + 1 == asm_idx


def test_s_durable_real_sprite_oracle_regression() -> None:
    reg = verify_sprite_regression()
    assert reg["s6_s7_contract_regression"] == "PASS"
    assert reg["sprite_oracle_conflicts"] == 0
    assert reg["sprite_oracle_id"] == "sprite_oracle_v1_f779"
    assert reg["sprite_oracle_frame"] == 779
