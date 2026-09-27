"""M12 W5d Functional Audio Validation and Listening Tests.

Verifies:
A. exact resource-local header rejection & descriptor semantics
B. stream start exact
C. stream end exact
D. deterministic PCM SHA
E. PCM generation does not access outside resource
F. WAV data chunk equals canonical PCM
G. sample rate evidence classification
H. semantic mutation encodes successfully
I. temp-ROM changes restricted to resource range
J. modified standalone decode predicts modified runtime DAC
K. original canonical ROM remains unchanged
"""

from __future__ import annotations

import hashlib
from pathlib import Path
import wave

import pytest

from src.tools.thor_evidence.w3_z80_evidence import (
    DOMAIN_YM2612,
    iter_records,
)
from src.tools.thor_evidence.w5_audio_decode import (
    decode_audio_resource,
    decode_format_a_stream,
)
from src.tools.thor_evidence.w5_audio_encode import (
    encode_audio_resource,
)
from src.tools.thor_evidence.w5_audio_format import (
    INITIAL_ACCUMULATOR,
    ROM_SHA,
    ROM_SIZE,
    parse_bank_descriptors,
)
from src.tools.thor_evidence.w5_audio_functional import (
    MEAN_OBSERVED_SAMPLE_RATE_HZ,
    NOMINAL_DAC_PERIOD_CYCLES,
    PREVIEW_SAMPLE_RATE_HZ,
    SOURCE_OWNED_STABLE,
    analyze_timing_evidence,
    compute_genesis_checksum,
    create_temporary_modified_rom,
    mutate_resource_1_semantic,
)

ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
FUNCTIONAL_DIR = Path("build/m12-w5-functional")
EARLY_BOOT_BIN = Path("scaling-output-w4-earlyboot/natural/count-1/live-forward-wave-records-pass1.bin")
MOD_BIN = Path("scaling-output-w5-functional/natural/count-1/live-forward-wave-records-pass1.bin")


@pytest.fixture(scope="module")
def rom_bytes() -> bytes:
    assert ROM_PATH.is_file(), f"ROM file missing: {ROM_PATH}"
    data = ROM_PATH.read_bytes()
    assert len(data) == ROM_SIZE
    assert hashlib.sha256(data).hexdigest() == ROM_SHA
    return data


def test_a_resource_local_header_rejection_and_descriptor_semantics(rom_bytes: bytes) -> None:
    """Test A: Proves the 3 bytes are NOT a resource-local header but bank descriptors."""
    # Bank 7 descriptors at 0x0B8000
    descs = parse_bank_descriptors(rom_bytes, 0x0B8000, 7)
    d1 = next(d for d in descs if d.entry_index == 7)
    d2 = next(d for d in descs if d.entry_index == 6)

    # In bank table at 0x0B8000:
    # Entry 7 raw descriptor: addr 0xD540, len 0x2228, mode 0x00
    # Entry 6 raw descriptor: addr 0xC95C, len 0x0BE4, mode 0x00
    assert d1.physical_address == 0x0BD540
    assert d1.byte_length == 8744
    assert d1.mode == 0

    assert d2.physical_address == 0x0BC95C
    assert d2.byte_length == 3044
    assert d2.mode == 0

    # The actual stream bytes at 0x0BD540 start with delta-PCM data (0x10, 0x00), NOT a header
    assert rom_bytes[d1.physical_address : d1.physical_address + 2] == b"\x10\x00"
    assert rom_bytes[d2.physical_address : d2.physical_address + 2] == b"\x10\x00"


def test_b_stream_start_exact(rom_bytes: bytes) -> None:
    """Test B: Audio stream starts exactly at byte 0 of each resource."""
    descs = parse_bank_descriptors(rom_bytes, 0x0B8000, 7)
    d1 = next(d for d in descs if d.entry_index == 7)
    d2 = next(d for d in descs if d.entry_index == 6)

    ir1 = decode_audio_resource(rom_bytes, d1, "res1")
    ir2 = decode_audio_resource(rom_bytes, d2, "res2")

    assert ir1.tokens[0].byte_index == 0
    assert ir1.tokens[0].nibble_pos == "HIGH"
    assert ir2.tokens[0].byte_index == 0
    assert ir2.tokens[0].nibble_pos == "HIGH"


def test_c_stream_end_exact(rom_bytes: bytes) -> None:
    """Test C: Stream termination is exact to descriptor byte count without over-read."""
    descs = parse_bank_descriptors(rom_bytes, 0x0B8000, 7)
    d1 = next(d for d in descs if d.entry_index == 7)
    d2 = next(d for d in descs if d.entry_index == 6)

    ir1 = decode_audio_resource(rom_bytes, d1, "res1")
    ir2 = decode_audio_resource(rom_bytes, d2, "res2")

    assert ir1.tokens[-1].byte_index == d1.byte_length - 1
    assert ir1.tokens[-1].nibble_pos == "LOW"
    assert ir2.tokens[-1].byte_index == d2.byte_length - 1
    assert ir2.tokens[-1].nibble_pos == "LOW"


def test_d_deterministic_pcm_sha(rom_bytes: bytes) -> None:
    """Test D: Standalone decoded PCM SHA is deterministic across repeated runs."""
    descs = parse_bank_descriptors(rom_bytes, 0x0B8000, 7)
    d1 = next(d for d in descs if d.entry_index == 7)
    d2 = next(d for d in descs if d.entry_index == 6)

    ir1_a = decode_audio_resource(rom_bytes, d1, "res1")
    ir1_b = decode_audio_resource(rom_bytes, d1, "res1")
    sha1_a = hashlib.sha256(bytes(ir1_a.pcm_samples)).hexdigest()
    sha1_b = hashlib.sha256(bytes(ir1_b.pcm_samples)).hexdigest()
    assert sha1_a == sha1_b == "4d6c5245ab6fa76cbea97a55d3fb468f3e94cd0fb016ae75319cc358212d9670"

    ir2_a = decode_audio_resource(rom_bytes, d2, "res2")
    ir2_b = decode_audio_resource(rom_bytes, d2, "res2")
    sha2_a = hashlib.sha256(bytes(ir2_a.pcm_samples)).hexdigest()
    sha2_b = hashlib.sha256(bytes(ir2_b.pcm_samples)).hexdigest()
    assert sha2_a == sha2_b == "9a269b714fabc729b838337fef0b28b17e9f297138f69f110e883b48973d4f0a"


def test_e_pcm_generation_does_not_access_outside_resource(rom_bytes: bytes) -> None:
    """Test E: Decoding isolated slice produces identical output to full ROM decode."""
    descs = parse_bank_descriptors(rom_bytes, 0x0B8000, 7)
    d1 = next(d for d in descs if d.entry_index == 7)

    raw_slice = rom_bytes[d1.physical_address : d1.physical_end_exclusive]
    toks, pcm = decode_format_a_stream(raw_slice, mode=d1.mode, initial_acc=INITIAL_ACCUMULATOR)

    ir1 = decode_audio_resource(rom_bytes, d1, "res1")
    assert bytes(pcm) == bytes(ir1.pcm_samples)
    assert len(toks) == len(ir1.tokens)


def test_f_wav_data_chunk_equals_canonical_pcm() -> None:
    """Test F: Exported WAV data frames equal canonical PCM bytes byte-for-byte."""
    for num in ("0001", "0002", "0001_modified"):
        pcm_path = FUNCTIONAL_DIR / f"audio_resource_{num}.pcm"
        wav_path = FUNCTIONAL_DIR / f"audio_resource_{num}.wav"
        assert pcm_path.is_file(), f"PCM missing: {pcm_path}"
        assert wav_path.is_file(), f"WAV missing: {wav_path}"

        pcm_data = pcm_path.read_bytes()
        with wave.open(str(wav_path), "rb") as w:
            assert w.getnchannels() == 1
            assert w.getsampwidth() == 1
            wav_frames = w.readframes(w.getnframes())
            assert wav_frames == pcm_data


def test_g_sample_rate_evidence_classification() -> None:
    """Test G: Analyzes timing evidence and verifies hardware status classification."""
    records = list(iter_records(EARLY_BOOT_BIN))
    info = analyze_timing_evidence(records)

    assert info["sample_rate_status"] == "VARIABLE_SOFTWARE_TIMED"
    assert info["proven_hardware_constant_rate"] == "UNKNOWN"
    assert info["nominal_dac_period_master_cycles"] == NOMINAL_DAC_PERIOD_CYCLES
    assert info["nominal_dac_sample_rate_hz"] == PREVIEW_SAMPLE_RATE_HZ
    assert info["observed_average_sample_rate_hz"] == MEAN_OBSERVED_SAMPLE_RATE_HZ


def test_h_semantic_mutation_encodes_successfully(rom_bytes: bytes) -> None:
    """Test H: Mutating semantic tokens produces predictable encoded bytes."""
    descs = parse_bank_descriptors(rom_bytes, 0x0B8000, 7)
    d1 = next(d for d in descs if d.entry_index == 7)
    ir1 = decode_audio_resource(rom_bytes, d1, "res1")

    mod_ir = mutate_resource_1_semantic(ir1)
    encoded_mod = encode_audio_resource(mod_ir)

    # Byte 6311 was 0x7D, should now be 0x11
    assert encoded_mod[6311] == 0x11
    # Byte 6313 was 0x5C, should now be 0xC8
    assert encoded_mod[6313] == 0xC8
    # Byte 6310 and 6312 unchanged
    assert encoded_mod[6310] == 0xBD
    assert encoded_mod[6312] == 0x2C


def test_i_temp_rom_changes_restricted_to_resource_range(rom_bytes: bytes) -> None:
    """Test I: Temp ROM changes are strictly restricted to Resource 1 with conserved checksum."""
    descs = parse_bank_descriptors(rom_bytes, 0x0B8000, 7)
    d1 = next(d for d in descs if d.entry_index == 7)
    ir1 = decode_audio_resource(rom_bytes, d1, "res1")

    mod_ir = mutate_resource_1_semantic(ir1)
    temp_rom, diffs = create_temporary_modified_rom(rom_bytes, mod_ir)

    assert len(diffs) == 2
    for addr in diffs:
        assert d1.physical_address <= addr < d1.physical_end_exclusive

    # 16-bit checksum strictly identical
    assert compute_genesis_checksum(temp_rom) == compute_genesis_checksum(rom_bytes) == 0x98ED


def test_j_modified_standalone_decode_predicts_modified_runtime_dac(rom_bytes: bytes) -> None:
    """Test J: Modified standalone decode predicts emulator hardware DAC writes with 0 mismatches."""
    descs = parse_bank_descriptors(rom_bytes, 0x0B8000, 7)
    d1 = next(d for d in descs if d.entry_index == 7)
    ir1 = decode_audio_resource(rom_bytes, d1, "res1")

    mod_ir = mutate_resource_1_semantic(ir1)
    temp_rom, _ = create_temporary_modified_rom(rom_bytes, mod_ir)
    mod_decoded = decode_audio_resource(temp_rom, d1, "mod")

    # Witness window at offset 12697
    predicted = list(mod_decoded.pcm_samples[12697 : 12697 + 18])

    # Read from emulator capture
    assert MOD_BIN.is_file(), f"Modified capture binary missing: {MOD_BIN}"
    records = list(iter_records(MOD_BIN))
    observed = [r.value for r in records if r.domain == DOMAIN_YM2612 and r.address == 0x4001]

    assert len(observed) == 18
    assert observed == predicted


def test_k_original_canonical_rom_remains_unchanged(rom_bytes: bytes) -> None:
    """Test K: Canonical ROM on disk remains 100% unaltered."""
    current = ROM_PATH.read_bytes()
    assert len(current) == ROM_SIZE
    assert hashlib.sha256(current).hexdigest() == ROM_SHA
    assert current == rom_bytes
