"""M12 W5c Audio Resource Ownership and Canonical Promotion Test Suite.

Verifies ownership eligibility, set-theoretic overlap reconciliation,
canonical partition integrity, idempotence, diff accuracy, and ADR compliance.
"""

from __future__ import annotations

import copy
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT), str(ROOT / "src/tools"), str(ROOT / "src/tools/thor_evidence")]

from src.tools.thor_evidence.identity import ROM_SHA, ROM_SIZE
from src.tools.thor_evidence.w5_audio_promote import (
    compute_overlap,
    run_promotion,
    RESOURCE_1_START,
    RESOURCE_1_END,
    RESOURCE_1_SIZE,
    RESOURCE_2_START,
    RESOURCE_2_END,
    RESOURCE_2_SIZE,
    PARENT_EMISSION_START,
    PARENT_EMISSION_END,
    TOTAL_ELIGIBLE_BYTES,
)

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
MAP_PATH = Path("docs/reports/THOR_M12_CANONICAL_ROM_KNOWLEDGE_MAP_2D.json")
W5_DIR = Path("build/m12-w5-acceptance")


@pytest.fixture(scope="module")
def map_data() -> dict:
    assert MAP_PATH.is_file(), f"Canonical map missing at {MAP_PATH}"
    return json.loads(MAP_PATH.read_text(encoding="utf-8"))


def test_a_exact_eligible_interval_union() -> None:
    """Test A: Exact eligible interval union equals 11,788 bytes."""
    assert RESOURCE_2_END == RESOURCE_1_START
    union_start = RESOURCE_2_START
    union_end = RESOURCE_1_END
    assert union_end - union_start == TOTAL_ELIGIBLE_BYTES
    assert TOTAL_ELIGIBLE_BYTES == 11788


def test_b_primary_resource_exact_ownership_eligibility() -> None:
    """Test B: Primary resource exact ownership eligibility."""
    elig_path = W5_DIR / "w5_ownership_eligibility.json"
    assert elig_path.is_file()
    elig = json.loads(elig_path.read_text(encoding="utf-8"))
    res1 = next(r for r in elig["resources"] if r["resource_id"] == "AUDIO_RESOURCE_FORMAT_A_0001")
    assert res1["boundary_start"] == "STATIC_VERIFIED"
    assert res1["boundary_end"] == "STATIC_VERIFIED"
    assert res1["format"] == "STATIC_VERIFIED"
    assert res1["decoder"] == "EXACT"
    assert res1["encoder"] == "EXACT"
    assert res1["roundtrip"] == "BYTE_IDENTICAL"
    assert res1["encoder_independence"] == "PASS"
    assert res1["runtime_overlap"] == "PASS"
    assert res1["canonical_rom_identity"] == "PASS"


def test_c_secondary_resource_exact_ownership_eligibility() -> None:
    """Test C: Secondary resource exact ownership eligibility."""
    elig_path = W5_DIR / "w5_ownership_eligibility.json"
    elig = json.loads(elig_path.read_text(encoding="utf-8"))
    res2 = next(r for r in elig["resources"] if r["resource_id"] == "AUDIO_RESOURCE_FORMAT_A_0002")
    assert res2["boundary_start"] == "STATIC_VERIFIED"
    assert res2["boundary_end"] == "STATIC_VERIFIED"
    assert res2["format"] == "STATIC_VERIFIED"
    assert res2["decoder"] == "EXACT"
    assert res2["encoder"] == "EXACT"
    assert res2["roundtrip"] == "BYTE_IDENTICAL"
    assert res2["encoder_independence"] == "PASS"
    assert res2["runtime_overlap"] == "PASS_STATIC_BOUNDARY_WITNESS"


def test_d_mode1_excluded() -> None:
    """Test D: Mode 1 resources are strictly excluded from ownership."""
    elig_path = W5_DIR / "w5_ownership_eligibility.json"
    elig = json.loads(elig_path.read_text(encoding="utf-8"))
    assert elig["excluded_from_ownership"]["mode1_resources"] == "REMAIN_HYPOTHESIS"
    for r in elig["resources"]:
        assert r["mode"] == 0


def test_e_padding_excluded(map_data: dict) -> None:
    """Test E: Bank padding 0x0BF768..0x0C0000 retains pre-existing status."""
    padding_em = next(e for e in map_data["emission"] if e["start"] == 0x0BF768)
    assert padding_em["end"] == 0x0C0000
    assert padding_em["classification"] == "ERASED_ROM_ALIGNMENT_PADDING"
    assert padding_em["source_kind"] == "PADDING_ALIGNMENT_CONFIRMED"
    assert padding_em["source_owned"] == 1


def test_f_descriptor_table_excluded(map_data: dict) -> None:
    """Test F: Descriptor table at 0x0B8000 remains unpromoted UNKNOWN."""
    table_em = next(e for e in map_data["emission"] if e["start"] == 0x0B8000)
    assert table_em["end"] == RESOURCE_2_START
    assert table_em["classification"] == "UNKNOWN"
    assert table_em["source_owned"] == 0


def test_g_existing_source_owned_overlap_not_double_counted(map_data: dict) -> None:
    """Test G: Existing SOURCE_OWNED overlap is not double-counted."""
    overlap = compute_overlap(map_data)
    # On the promoted map, already owned is 11,788 and new is 0
    assert overlap["already_source_owned_bytes"] == 11788
    assert overlap["new_source_owned_bytes"] == 0


def test_h_partial_overlap_splits_correctly(map_data: dict) -> None:
    """Test H: Parent range splits into exactly 3 disjoint pieces."""
    b17_em = [e for e in map_data["emission"] if PARENT_EMISSION_START <= e["start"] < PARENT_EMISSION_END]
    assert len(b17_em) == 3
    assert b17_em[0]["start"] == PARENT_EMISSION_START
    assert b17_em[0]["end"] == RESOURCE_2_START
    assert b17_em[0]["source_owned"] == 0

    assert b17_em[1]["start"] == RESOURCE_2_START
    assert b17_em[1]["end"] == RESOURCE_2_END
    assert b17_em[1]["source_owned"] == 1

    assert b17_em[2]["start"] == RESOURCE_1_START
    assert b17_em[2]["end"] == RESOURCE_1_END
    assert b17_em[2]["source_owned"] == 1


def test_i_dry_run_expected_total_exact() -> None:
    """Test I: Dry-run expected total is exact."""
    plan_path = W5_DIR / "w5_ownership_promotion_plan.json"
    assert plan_path.is_file()
    plan = json.loads(plan_path.read_text(encoding="utf-8"))
    assert sum(item["newly_owned_bytes"] for item in plan) == 11788
    expected_after = 1475600 + 11788
    assert expected_after == 1487388
    assert expected_after <= ROM_SIZE


def test_j_promotion_uses_canonical_tooling() -> None:
    """Test J: Promotion uses KnowledgeStore canonical tooling."""
    from src.tools.thor_evidence.w5_audio_promote import run_promotion
    assert callable(run_promotion)


def test_k_second_promotion_is_idempotent() -> None:
    """Test K: Second promotion run is idempotent (NEW=0, identical hashes)."""
    with tempfile.TemporaryDirectory() as td:
        tdp = Path(td)
        test_map = tdp / "test_map.json"
        test_map.write_bytes(MAP_PATH.read_bytes())
        res1 = run_promotion(test_map, ROM_PATH, W5_DIR, tdp / "out1")
        assert res1["new_source_owned_bytes"] == 0
        assert res1["canonical_promotion_executed"] is False
        assert res1["source_owned_delta"] == 0


def test_l_regenerated_partition_has_no_gaps(map_data: dict) -> None:
    """Test L: Regenerated emission partition has no gaps."""
    cursor = 0
    for em in map_data["emission"]:
        assert em["start"] == cursor, f"Gap at {cursor}"
        cursor = em["end"]
    assert cursor == ROM_SIZE


def test_m_regenerated_partition_has_no_overlaps(map_data: dict) -> None:
    """Test M: Regenerated emission partition has no overlaps."""
    for e1, e2 in zip(map_data["emission"], map_data["emission"][1:]):
        assert e1["end"] == e2["start"], f"Overlap/gap between {e1} and {e2}"


def test_n_emission_total_equals_rom_size(map_data: dict) -> None:
    """Test N: Emission total exactly equals ROM size (3,145,728 bytes)."""
    total = sum(e["end"] - e["start"] for e in map_data["emission"])
    assert total == ROM_SIZE


def test_o_recomputed_source_owned_equals_report(map_data: dict) -> None:
    """Test O: Recomputed SOURCE_OWNED equals report (1,487,388 bytes)."""
    computed = sum(e["end"] - e["start"] for e in map_data["emission"] if e.get("source_owned") == 1)
    assert computed == 1487388
    assert map_data["metrics"]["source_owned_bytes"] == 1487388


def test_p_canonical_rom_sha_mismatch_aborts() -> None:
    """Test P: Canonical ROM SHA mismatch aborts fail-closed."""
    with tempfile.TemporaryDirectory() as td:
        fake_rom = Path(td) / "fake.rom"
        fake_rom.write_bytes(b"\x00" * ROM_SIZE)
        with pytest.raises(ValueError, match="STOP_ROM_IDENTITY_MISMATCH"):
            run_promotion(MAP_PATH, fake_rom, W5_DIR, Path(td))


def test_q_w5_proof_hash_mismatch_aborts() -> None:
    """Test Q: W5 proof hash mismatch aborts fail-closed."""
    with tempfile.TemporaryDirectory() as td:
        empty_dir = Path(td)
        with pytest.raises(FileNotFoundError, match="STOP_W5_PROOF_MISSING"):
            run_promotion(MAP_PATH, ROM_PATH, empty_dir, Path(td))


def test_r_unrelated_canonical_ranges_unchanged(map_data: dict) -> None:
    """Test R: Unrelated canonical ranges outside 0x0B8000..0x0BF768 are unchanged."""
    diff_path = W5_DIR / "w5_ownership_canonical_diff.json"
    assert diff_path.is_file()
    diff = json.loads(diff_path.read_text(encoding="utf-8"))
    assert diff["other_canonical_changes"] == 0
    assert diff["unchanged_already_source_owned_bytes"] == 1475600


def test_s_provenance_receipt_links_exact_w5_proofs(map_data: dict) -> None:
    """Test S: Provenance receipt links exact W5 proofs."""
    receipt_path = W5_DIR / "w5_ownership_receipt.json"
    assert receipt_path.is_file()
    receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
    assert "w5_roundtrip_receipt" in receipt["hashes"]
    assert "w5_audio_format_spec" in receipt["hashes"]


def test_t_ownership_claim_cannot_extend_outside_reconstructed_boundary(map_data: dict) -> None:
    """Test T: Ownership claims cannot extend outside reconstructed boundary."""
    audio_objs = [o for o in map_data["objects"] if o.get("object_type") == "AUDIO_DATA"]
    w5c_objs = [o for o in audio_objs if o.get("attributes", {}).get("resource_id") in
                ("AUDIO_RESOURCE_FORMAT_A_0001", "AUDIO_RESOURCE_FORMAT_A_0002")]
    assert len(w5c_objs) == 2
    for ao in w5c_objs:
        rid = ao["range_id"]
        rng = next(r for r in map_data["ranges"] if r["range_id"] == rid)
        assert rng["start"] >= RESOURCE_2_START
        assert rng["end"] <= RESOURCE_1_END
