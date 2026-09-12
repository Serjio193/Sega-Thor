"""Regression tests for the bounded M12-GFX-2 caller closure helpers."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_gfx_caller_closure", ROOT / "src/tools/m12_gfx_caller_closure.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_sequential_expansion_uses_exact_half_open_parser_boundaries():
    records = {
        0x100: {"start": 0x100, "end": 0x110, "compressed_bytes": 0x10,
                "decompressed_bytes": 32, "output_sha256": "a", "mode": "command",
                "deterministic": True},
        0x110: {"start": 0x110, "end": 0x130, "compressed_bytes": 0x20,
                "decompressed_bytes": 48, "output_sha256": "b", "mode": "bit",
                "deterministic": True},
    }
    result = MODULE.expand_chain(records, "test", 0x100, [0x20, 0x26], "0x00FF2FA8")
    assert [(item["start"], item["end"]) for item in result] == [(0x100, 0x110), (0x110, 0x130)]
    assert result[1]["caller_sites"] == [0x26]


def test_split_unknown_preserves_neighbors_and_typed_ownership():
    entries = [{"start": 0, "end": 0x100, "kind": "UNKNOWN", "size": 0x100}]
    result = MODULE.split_unknown(entries, 0x20, 0x40, "test caller")
    assert [(item["start"], item["end"], item["kind"]) for item in result] == [
        (0, 0x20, "UNKNOWN"), (0x20, 0x40, "LOCAL_ROM_DERIVED_ASSET"),
        (0x40, 0x100, "UNKNOWN")]
    assert result[1]["classification"] == "ANCIENT_COMPRESSED_RESOURCE"


def test_all_static_callers_are_explicitly_represented():
    assert len(MODULE.CALLERS) == 52
    assert len(set(MODULE.CALLERS)) == 52
    assert set(MODULE.CALLERS) == set(MODULE.ROUTINES)
    assert set(MODULE.DYNAMIC).issubset(set(MODULE.CALLERS))
    assert MODULE.DYNAMIC[0x00D54A][2] == "INHERITED_A1_FIELD_NOT_ROM_PROVEN"
    assert MODULE.DYNAMIC[0x00D650][2] == "FIRST_STREAM_AND_CONTINUATION_NOT_ROM_PROVEN"
    assert MODULE.DYNAMIC[0x02DB52][2] == "D406_POST_SOURCE_NOT_ROM_PROVEN"
    assert MODULE.DYNAMIC[0x02F6A0][2] == "D406_POST_STATE_NOT_ROM_PROVEN"
    assert MODULE.DYNAMIC[0x03B236][2] == "A5_FIELD_NOT_ROM_PROVEN"
    assert MODULE.DYNAMIC[0x03B28A][2] == "A3_ARGUMENT_NOT_ROM_PROVEN"
    assert MODULE.DYNAMIC[0x03B2FE][2] == "A4_ARGUMENT_NOT_ROM_PROVEN"


if __name__ == "__main__":
    test_sequential_expansion_uses_exact_half_open_parser_boundaries()
    test_split_unknown_preserves_neighbors_and_typed_ownership()
    test_all_static_callers_are_explicitly_represented()
    print("m12 gfx caller closure tests passed")
