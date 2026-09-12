import importlib.util
from pathlib import Path

from m12_local_inputs import optional_bytes


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_a372_shadow_sat_producer",
    ROOT / "src/tools/m12_a372_shadow_sat_producer.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_static_producer_grammar():
    rom = optional_bytes(ROOT / "build/reference/Beyond Oasis (USA).bin", "canonical ROM")
    if rom is None:
        return
    result = MODULE.parse_producer(rom)
    assert result["status"] == "STATIC_PRODUCER_GRAMMAR_PROVEN_SEMANTICS_NEUTRAL"
    assert result["routine"]["range"] == ["0x00A342", "0x00A438"]
    assert result["routine"]["incoming_callers"] == ["0x00A19C", "0x00A6A0"]
    assert result["routine"]["incoming_direct_branches"] == []
    assert result["root_alias_correction"]["exact_pc_relative_targets"] == [
        "0x00A438", "0x00A480"
    ]
    for root in result["roots"].values():
        assert root["record_count"] == 9
        assert root["record_width_bytes"] == 8
    assert result["d2_provenance"]["observed_value_explanation"]["record_source"] == "0x00A438"
    assert result["a5_provenance"]["first_store"] == "0x00A372"
    assert result["ff1858"]["local_role"].startswith("boolean root selector")
    assert result["semantic_result"]["frame_or_piece_grammar"] == "NOT_PROVEN"
    assert result["join"]["dma"].endswith("SAT VRAM 0x0000D000")


if __name__ == "__main__":
    test_static_producer_grammar()
    print("M12 A372 shadow-SAT producer tests passed")
