"""Pure tests for the independent M12-GFX-1 parser and classifier."""

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).parents[1] / "src" / "tools"))

from m12_gfx_ancient import DecodeError, classify, decode, deterministic_decode


def test_raw_and_repeat() -> None:
    raw = bytes((6, 0, 3, ord("A"), ord("B"), ord("C"), 0))
    result = deterministic_decode(raw, 0)
    assert result.end == len(raw)
    assert result.output == b"ABC"
    repeat = bytes((4, 0, 0x41, ord("Z"), 0))
    assert decode(repeat, 0).output == b"Z" * 5


def test_back_reference_and_extended_repeat() -> None:
    source = bytes((10, 0, 3, ord("A"), ord("B"), ord("C"),
                    0x80, 3, 0x62, 0x61, 0))
    result = decode(source, 0)
    assert result.output == b"ABCABCABCA"
    extended = bytes((0x18, 0, 0x20, 20)) + b"Q" * 20 + bytes((0,))
    assert decode(extended, 0).output == b"Q" * 20
    extended_rle = bytes((5, 0, 0x50, 0x10, ord("R"), 0))
    assert decode(extended_rle, 0).output == b"R" * 20


def test_bit_stream_and_malformed_inputs() -> None:
    source = bytes((0, 0, 0, 6, ord("X"), 0, 0))
    assert decode(source, 0).output == b"X"
    for malformed in (b"", b"\x01\x00\x03", b"\xFF\xFF\x03\x00"):
        try:
            decode(malformed, 0)
        except DecodeError:
            pass
        else:
            raise AssertionError("malformed stream was accepted")


def test_classification_is_metadata_only() -> None:
    features = classify(bytes(32) + bytes(range(32)))
    assert features["output_size_mod_32"] == 0
    assert features["classification"] == "GFX_TILE_CANDIDATE"
    assert 0 <= features["transparent_zero_density"] <= 1


if __name__ == "__main__":
    test_raw_and_repeat()
    test_back_reference_and_extended_repeat()
    test_bit_stream_and_malformed_inputs()
    test_classification_is_metadata_only()
    print("m12_gfx_sweep_test: pass")
