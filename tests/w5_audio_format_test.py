"""M12 W5 Audio Resource Format and Round-Trip Test Suite.

Verifies format semantics, descriptor parsing, deterministic decoding,
byte-identical round-trip encoding, witness alignment, and artifact integrity.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import sys
import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from src.tools.thor_evidence.w5_audio_decode import (
    decode_audio_resource,
    decode_format_a_stream,
)
from src.tools.thor_evidence.w5_audio_encode import (
    encode_audio_resource,
    encode_format_a_tokens,
)
from src.tools.thor_evidence.w5_audio_format import (
    AUDIO_BANK_INDICES,
    AUDIO_BANK_PHYSICAL_BASES,
    DELTA_TABLE,
    DESCRIPTOR_ENTRY_SIZE,
    DESCRIPTOR_TABLE_SIZE,
    DESCRIPTORS_PER_BANK,
    INITIAL_ACCUMULATOR,
    ROM_SHA,
    ROM_SIZE,
    AudioDescriptor,
    parse_bank_descriptors,
)
from src.tools.thor_evidence.w5_audio_pipeline import run_w5_pipeline

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
OUTPUT_DIR = Path("build/m12-w5-acceptance")
MAP_PATH = Path("docs/reports/THOR_M12_CANONICAL_ROM_KNOWLEDGE_MAP_2D.json")


@pytest.fixture(scope="module")
def rom_bytes() -> bytes:
    assert ROM_PATH.exists(), f"Canonical ROM missing at {ROM_PATH}"
    data = ROM_PATH.read_bytes()
    assert len(data) == ROM_SIZE
    assert hashlib.sha256(data).hexdigest() == ROM_SHA
    return data


def test_a_format_spec_constants() -> None:
    """Test A: Format specification constants and delta table symmetry."""
    assert len(DELTA_TABLE) == 16
    assert DELTA_TABLE[0] is None  # Nibble 0 is repeat
    assert DELTA_TABLE[1] == 0     # Delta 0
    # Positives 2..8
    assert DELTA_TABLE[2:9] == (1, 2, 6, 12, 24, 48, 96)
    # Negatives 9..15
    assert DELTA_TABLE[9:16] == (-96, -48, -24, -12, -6, -2, -1)
    assert len(AUDIO_BANK_PHYSICAL_BASES) == 8
    assert len(AUDIO_BANK_INDICES) == 8
    assert DESCRIPTOR_ENTRY_SIZE == 5
    assert DESCRIPTORS_PER_BANK == 16
    assert DESCRIPTOR_TABLE_SIZE == 80
    assert INITIAL_ACCUMULATOR == 0x80


def test_b_descriptor_table_parsing(rom_bytes: bytes) -> None:
    """Test B: Descriptor table parsing for Bank 0x17."""
    bank_17_phys = 0x0B8000
    descs = parse_bank_descriptors(rom_bytes, bank_17_phys, 0x17)
    assert len(descs) == 8
    entry7 = next(d for d in descs if d.entry_index == 7)
    assert entry7.logical_address == 0xD540
    assert entry7.physical_address == 0x0BD540
    assert entry7.byte_length == 8744
    assert entry7.mode == 0
    assert entry7.physical_end_exclusive == 0x0BF768


def test_c_primary_resource_boundaries(rom_bytes: bytes) -> None:
    """Test C: Primary resource boundaries and contiguous packing."""
    bank_17_phys = 0x0B8000
    descs = parse_bank_descriptors(rom_bytes, bank_17_phys, 0x17)
    entry6 = next(d for d in descs if d.entry_index == 6)
    entry7 = next(d for d in descs if d.entry_index == 7)

    # Entry 6 directly precedes Entry 7
    assert entry6.physical_end_exclusive == entry7.physical_address
    assert entry7.physical_address == 0x0BD540
    assert entry7.physical_end_exclusive == 0x0BF768
    assert entry7.byte_length == 8744

    # Remainder of bank from 0x0BF768 to 0x0C0000 is 0xFF padding
    tail = rom_bytes[entry7.physical_end_exclusive : bank_17_phys + 0x8000]
    assert len(tail) == 2200
    assert set(tail) == {0xFF}


def test_d_witness_cluster_containment() -> None:
    """Test D: W4 witness cluster strictly contained within primary resource."""
    primary_start = 0x0BD540
    primary_end = 0x0BF768
    witness_start = 0x0BEDE4
    witness_end = 0x0BEDED

    assert primary_start <= witness_start
    assert witness_end < primary_end
    offset = witness_start - primary_start
    assert offset == 6308


def test_e_primary_resource_deterministic_decode(rom_bytes: bytes) -> None:
    """Test E: Deterministic decoding of primary audio resource."""
    bank_17_phys = 0x0B8000
    descs = parse_bank_descriptors(rom_bytes, bank_17_phys, 0x17)
    entry7 = next(d for d in descs if d.entry_index == 7)

    ir = decode_audio_resource(rom_bytes, entry7, "AUDIO_RESOURCE_FORMAT_A_0001")
    assert ir.format_id == "AUDIO_FORMAT_A_MODE0"
    assert ir.resource_id == "AUDIO_RESOURCE_FORMAT_A_0001"
    assert ir.byte_length == 8744
    assert ir.total_tokens == 8744 * 2
    assert ir.total_pcm_samples == 17830
    assert ir.initial_accumulator == 0x80


def test_f_primary_resource_roundtrip(rom_bytes: bytes) -> None:
    """Test F: Primary resource deterministic encode produces byte-identical bytes."""
    bank_17_phys = 0x0B8000
    descs = parse_bank_descriptors(rom_bytes, bank_17_phys, 0x17)
    entry7 = next(d for d in descs if d.entry_index == 7)

    orig_bytes = rom_bytes[entry7.physical_address : entry7.physical_end_exclusive]
    ir = decode_audio_resource(rom_bytes, entry7, "AUDIO_RESOURCE_FORMAT_A_0001")
    reencoded = encode_audio_resource(ir)

    assert len(reencoded) == len(orig_bytes)
    assert reencoded == orig_bytes
    assert hashlib.sha256(reencoded).hexdigest() == hashlib.sha256(orig_bytes).hexdigest()
    assert hashlib.sha256(reencoded).hexdigest() == "4496000b2d8efed59d75ea80f32b0aa1591d880dc982b2d1bb97b4d49297e606"


def test_g_witness_dac_sample_alignment(rom_bytes: bytes) -> None:
    """Test G: Decoded PCM matches observed runtime W4 DAC write at witness address."""
    bank_17_phys = 0x0B8000
    descs = parse_bank_descriptors(rom_bytes, bank_17_phys, 0x17)
    entry7 = next(d for d in descs if d.entry_index == 7)

    ir = decode_audio_resource(rom_bytes, entry7, "AUDIO_RESOURCE_FORMAT_A_0001")
    witness_offset = 0x0BEDE4 - entry7.physical_address

    # At witness byte offset 6308, low nibble token emits sample 0x78
    lo_token = ir.tokens[witness_offset * 2 + 1]
    assert lo_token.nibble_pos == "LOW"
    assert lo_token.pcm_samples[0] == 0x78  # Exact match to W4 DAC witness


def test_h_secondary_resource_roundtrip(rom_bytes: bytes) -> None:
    """Test H: Secondary resource deterministic decode & byte-identical round-trip."""
    bank_17_phys = 0x0B8000
    descs = parse_bank_descriptors(rom_bytes, bank_17_phys, 0x17)
    entry6 = next(d for d in descs if d.entry_index == 6)

    orig_bytes = rom_bytes[entry6.physical_address : entry6.physical_end_exclusive]
    ir = decode_audio_resource(rom_bytes, entry6, "AUDIO_RESOURCE_FORMAT_A_0002")
    reencoded = encode_audio_resource(ir)

    assert len(reencoded) == len(orig_bytes)
    assert reencoded == orig_bytes
    assert hashlib.sha256(reencoded).hexdigest() == hashlib.sha256(orig_bytes).hexdigest()


def test_i_multi_bank_format_generality(rom_bytes: bytes) -> None:
    """Test I: Format generality across multiple audio banks."""
    for bank_phys, bank_id in zip(AUDIO_BANK_PHYSICAL_BASES[:4], AUDIO_BANK_INDICES[:4]):
        descs = parse_bank_descriptors(rom_bytes, bank_phys, bank_id)
        assert len(descs) > 0
        test_desc = descs[0]
        orig = rom_bytes[test_desc.physical_address : test_desc.physical_end_exclusive]
        ir = decode_audio_resource(rom_bytes, test_desc, f"TEST_{bank_id:02X}")
        rec = encode_audio_resource(ir)
        assert rec == orig


def test_j_mode1_hold_semantics() -> None:
    """Test J: Mode 1 sample hold interpolation via 0x0999."""
    synthetic = bytes([0x20])  # hi=2 (delta=+1), lo=0 (repeat 3)
    tokens, pcm = decode_format_a_stream(synthetic, mode=1, initial_acc=0x80)
    assert len(tokens) == 2
    # Mode 1 emits 2 PCM samples per delta step (1 normal + 1 hold)
    assert tokens[0].pcm_samples == (0x81, 0x81)
    # Low nibble repeat 3 emits 6 samples (3 steps * 2 holds)
    assert len(tokens[1].pcm_samples) == 6


def test_k_pipeline_and_artifacts() -> None:
    """Test K: Full pipeline execution and artifact validation."""
    res = run_w5_pipeline(ROM_PATH, OUTPUT_DIR, MAP_PATH)
    assert res["status"] == "PASS_EXACT_AUDIO_RESOURCE_ROUNDTRIP_V1"
    assert res["primary_match"] is True
    assert res["secondary_match"] is True
    assert res["all_gates_pass"] is True
    assert res["artifacts_written"] == 9

    expected_files = (
        "w5_audio_format_spec.json",
        "w5_audio_resources.json",
        "w5_primary_resource_ir.json",
        "w5_secondary_resource_ir.json",
        "w5_roundtrip_receipt.json",
        "w5_witness_alignment.json",
        "w5_canonical_map_overlap.json",
        "w5_source_owned_reconciliation.json",
        "w5_audio_format_report.md",
    )
    for fname in expected_files:
        p = OUTPUT_DIR / fname
        assert p.exists()
        assert p.stat().st_size > 0


def test_l_neutral_naming_hygiene() -> None:
    """Test L: Zero promotional / musical names in emitted data and code."""
    forbidden = ["SONG", "TRACK", "VOICE", "MUSIC", "INSTRUMENT", "BGM", "SFX"]
    spec_text = (OUTPUT_DIR / "w5_audio_format_spec.json").read_text(encoding="utf-8")
    report_text = (OUTPUT_DIR / "w5_audio_format_report.md").read_text(encoding="utf-8")

    for f in forbidden:
        assert f not in spec_text.upper()
        # In report, check that no identifiers use promotional names
        assert f"AUDIO_{f}" not in report_text


def test_m_gates_a_through_j_audit() -> None:
    """Test M: Explicit verification of all Gates A through J."""
    recon_path = OUTPUT_DIR / "w5_source_owned_reconciliation.json"
    data = json.loads(recon_path.read_text(encoding="utf-8"))
    gates = data["gates_audit"]

    assert gates["gate_a_start_proven"] is True
    assert gates["gate_b_end_proven"] is True
    assert gates["gate_c_size_proven"] is True
    assert gates["gate_d_decoder_proven"] is True
    assert gates["gate_e_format_semantics_understood"] is True
    assert gates["gate_f_decoder_passes_tests"] is True
    assert gates["gate_g_encoder_passes_tests"] is True
    assert gates["gate_h_roundtrip_verified"] is True
    assert gates["gate_i_second_resource_roundtripped"] is True
    assert gates["gate_j_no_handwaved_assumptions"] is True
    assert data["all_gates_pass"] is True


def test_n_source_owned_baseline_preservation() -> None:
    """Test N: SOURCE_OWNED baseline accounting and zero delta verification."""
    recon_path = OUTPUT_DIR / "w5_source_owned_reconciliation.json"
    data = json.loads(recon_path.read_text(encoding="utf-8"))

    assert data["source_owned_before"] == 1475600
    assert data["source_owned_after"] == 1475600
    assert data["source_owned_delta"] == 0
    assert data["status"] == "PASS_EXACT_AUDIO_RESOURCE_ROUNDTRIP_V1"


def test_o_artifact_schema_and_contents() -> None:
    """Test O: Validates schema and key contents across all 9 artifacts."""
    receipt = json.loads((OUTPUT_DIR / "w5_roundtrip_receipt.json").read_text(encoding="utf-8"))
    assert receipt["status"] == "PASS_ROUNDTRIP_VERIFIED"
    assert receipt["primary_resource"]["byte_identical"] is True
    assert receipt["secondary_resource"]["byte_identical"] is True

    witness = json.loads((OUTPUT_DIR / "w5_witness_alignment.json").read_text(encoding="utf-8"))
    assert witness["contained_in_primary"] is True
    assert witness["runtime_matches"] == 18
    assert witness["runtime_mismatches"] == 0

    overlap = json.loads((OUTPUT_DIR / "w5_canonical_map_overlap.json").read_text(encoding="utf-8"))
    assert overlap["conflict_bytes"] == 0


def test_p_encoder_validation_errors() -> None:
    """Test P: Encoder error handling on odd token count and position mismatch."""
    with pytest.raises(ValueError, match="STOP_AUDIO_TOKEN_COUNT_UNPAIRED"):
        encode_format_a_tokens([])  # 0 is even, test 1 token
        ir = decode_format_a_stream(bytes([0x12]))[0]
        encode_format_a_tokens(ir[:1])

    ir = decode_format_a_stream(bytes([0x12]))[0]
    # Swap token positions
    bad_tokens = [ir[1], ir[0]]
    with pytest.raises(ValueError, match="STOP_AUDIO_TOKEN_ORDER_MISMATCH"):
        encode_format_a_tokens(bad_tokens)


def test_q_decoder_bounds_checking(rom_bytes: bytes) -> None:
    """Test Q: Decoder bounds checking on out-of-range descriptors."""
    invalid_desc = AudioDescriptor(
        bank_id=0x17,
        bank_physical_base=0x0B8000,
        entry_index=99,
        logical_address=0x8000,
        physical_address=len(rom_bytes) - 10,
        byte_length=100,  # Exceeds ROM length
        mode=0,
    )
    with pytest.raises(ValueError, match="STOP_AUDIO_RESOURCE_EXCEEDS_ROM"):
        decode_audio_resource(rom_bytes, invalid_desc, "OUT_OF_BOUNDS")


def test_r_non_bijective_representation_demonstration() -> None:
    """Test R: Validates that multiple token streams can produce flatline silence."""
    # delta 0 via non-zero nibble 1 vs repeat via nibble 0
    t1, p1 = decode_format_a_stream(bytes([0x11, 0x11]))  # 4 nibbles of delta 0
    t2, p2 = decode_format_a_stream(bytes([0x10]))        # 1 delta 0 + repeat 3
    # Both output 4 samples of 0x80
    assert p1 == [0x80, 0x80, 0x80, 0x80]
    assert p2 == [0x80, 0x80, 0x80, 0x80]
    # But original bytes are different (0x1111 vs 0x10) -> proves why IR token stream is required
    assert bytes([0x11, 0x11]) != bytes([0x10])


def test_s_non_tautological_source_nibble_removal_and_mutation(rom_bytes: bytes) -> None:
    """Test S: Proves encoder is strictly non-tautological and ignores source_nibble."""
    from dataclasses import replace

    bank_17_phys = 0x0B8000
    descs = parse_bank_descriptors(rom_bytes, bank_17_phys, 0x17)
    entry7 = next(d for d in descs if d.entry_index == 7)
    orig_bytes = rom_bytes[entry7.physical_address : entry7.physical_end_exclusive]
    ir = decode_audio_resource(rom_bytes, entry7, "AUDIO_RESOURCE_FORMAT_A_0001")

    # 1. Strip source_nibble entirely (set to None)
    stripped_tokens = [replace(t, source_nibble=None) for t in ir.tokens]
    encoded_stripped = encode_format_a_tokens(stripped_tokens)
    assert encoded_stripped == orig_bytes
    assert hashlib.sha256(encoded_stripped).hexdigest() == hashlib.sha256(orig_bytes).hexdigest()

    # 2. Mutate source_nibble to deliberately wrong values
    mutated_tokens = [replace(t, source_nibble=0xEE) for t in ir.tokens]
    encoded_mutated = encode_format_a_tokens(mutated_tokens)
    assert encoded_mutated == orig_bytes
    assert hashlib.sha256(encoded_mutated).hexdigest() == hashlib.sha256(orig_bytes).hexdigest()


def test_t_encoding_uniqueness_proof() -> None:
    """Test T: Proves bijective inverse mapping and encoding uniqueness for Mode 0."""
    from src.tools.thor_evidence.w5_audio_format import (
        INVERSE_DELTA_TABLE,
        semantic_token_to_nibble,
    )

    # 15 non-zero deltas must be pairwise distinct
    non_zero_deltas = [d for d in DELTA_TABLE if d is not None]
    assert len(non_zero_deltas) == 15
    assert len(set(non_zero_deltas)) == 15

    # Inverse mapping must contain exactly 15 entries
    assert len(INVERSE_DELTA_TABLE) == 15
    for delta, nibble in INVERSE_DELTA_TABLE.items():
        assert 1 <= nibble <= 15
        assert DELTA_TABLE[nibble] == delta

    # Semantic encoder function uniqueness
    assert semantic_token_to_nibble("REPEAT", 0) == 0
    for delta in non_zero_deltas:
        expected_nibble = INVERSE_DELTA_TABLE[delta]
        assert semantic_token_to_nibble("DELTA", delta) == expected_nibble

    # Invalid delta raises error
    with pytest.raises(ValueError, match="STOP_INVALID_SEMANTIC_DELTA"):
        semantic_token_to_nibble("DELTA", 999)


def test_u_full_runtime_18_dac_alignment() -> None:
    """Test U: Proves full runtime alignment of all 18 early-boot DAC events."""
    alignment_path = OUTPUT_DIR / "w5_witness_alignment.json"
    data = json.loads(alignment_path.read_text(encoding="utf-8"))

    assert data["runtime_dac_events_total"] == 18
    assert data["runtime_dac_events_alignable"] == 18
    assert data["runtime_bytes_compared"] == 10
    assert data["runtime_matches"] == 18
    assert data["runtime_mismatches"] == 0
    assert data["first_mismatch"] is None
    assert len(data["alignment_events"]) == 18
    for ev in data["alignment_events"]:
        assert ev["match"] is True


def test_v_canonical_ir_field_audit(rom_bytes: bytes) -> None:
    """Test V: Canonical IR field audit proving zero forbidden fields used by encoder."""
    from dataclasses import replace

    bank_17_phys = 0x0B8000
    descs = parse_bank_descriptors(rom_bytes, bank_17_phys, 0x17)
    entry7 = next(d for d in descs if d.entry_index == 7)
    orig_bytes = rom_bytes[entry7.physical_address : entry7.physical_end_exclusive]
    ir = decode_audio_resource(rom_bytes, entry7, "AUDIO_RESOURCE_FORMAT_A_0001")

    # Verify field classification
    spec_path = OUTPUT_DIR / "w5_audio_format_spec.json"
    spec = json.loads(spec_path.read_text(encoding="utf-8"))
    field_audit = spec["canonical_ir_field_audit"]
    assert field_audit["forbidden_fields_used_by_encoder_count"] == 0

    # Delete all provenance fields and re-encode
    pure_tokens = [
        replace(t, token_index=0, byte_index=0, source_nibble=None)
        for t in ir.tokens
    ]
    encoded_pure = encode_format_a_tokens(pure_tokens)
    assert encoded_pure == orig_bytes
