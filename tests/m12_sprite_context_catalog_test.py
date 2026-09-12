import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_sprite_context_catalog", ROOT / "src/tools/m12_sprite_context_catalog.py"
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def event(frame, pc, a0):
    registers = {"a": [a0, "0x00FF13CC"], "d": ["0x0"] * 8, "sr": "0x0"}
    return {"frame": frame, "pc": pc, "registers": registers, "source_bytes": None}


def test_context_summary_requires_ordered_complete_edge():
    events = [event(12, pc, "0x00171864") for pc in MODULE.REQUIRED_SEQUENCE]
    context = {
        "schema": "oasis.m68k.m12-sprite-context.v1",
        "canonical_rom_sha256": MODULE.ROM_SHA256,
        "state_writes_emitted": False,
        "executions": events,
    }
    result = MODULE.summarize_context(context)
    assert result["complete_sequence_frame_count"] == 1
    assert result["observed_source_starts"][0]["address"] == "0x00171864"


def test_context_summary_rejects_unordered_edge():
    events = [event(12, pc, "0x00171864") for pc in reversed(MODULE.REQUIRED_SEQUENCE)]
    context = {
        "schema": "oasis.m68k.m12-sprite-context.v1",
        "canonical_rom_sha256": MODULE.ROM_SHA256,
        "state_writes_emitted": False,
        "executions": events,
    }
    try:
        MODULE.summarize_context(context)
    except ValueError as error:
        assert "not ordered" in str(error)
    else:
        raise AssertionError("unordered live edge was accepted")


def test_targeted_reads_close_table_pointer_and_source_edges():
    context = {
        "schema": "oasis.m68k.m12-sprite-context.v1",
        "canonical_rom_sha256": MODULE.ROM_SHA256,
        "state_writes_emitted": False,
        "executions": [
            event(12, pc, "0x00171864") for pc in MODULE.REQUIRED_SEQUENCE
        ],
    }
    targeted = {
        "schema": "oasis.m68k.m12-targeted-rom-reads.v1",
        "canonical_rom_sha256": MODULE.ROM_SHA256,
        "state_writes_emitted": False,
        "frames_executed": 730,
        "read_events": [
            {"address": "0x00FFAFAE", "value": "0x00000000", "pc": "0x0003B420"},
            {"address": "0x0003B8EA", "value": "0x0003B95C", "pc": "0x0003B426"},
            {"address": "0x0003B95C", "value": "0x00171832", "pc": "0x0003B428"},
            {"address": "0x00171864", "value": "0x00000000", "pc": "0x0000B73E"},
        ],
    }
    result = MODULE.summarize_targeted_reads(context, targeted)
    assert result["selector_to_table_fields"][0]["record_index"] == 0
    assert result["observed_table_fields"][0]["record_index"] == 0
    assert result["observed_pointer_reads"][0]["values"] == ["0x00171832"]
    assert result["pointer_read"]["events"] == 1
    assert result["source_start_reads"][0]["address"] == "0x00171864"
    assert result["source_start_read_count"] == 1


if __name__ == "__main__":
    test_context_summary_requires_ordered_complete_edge()
    test_context_summary_rejects_unordered_edge()
    test_targeted_reads_close_table_pointer_and_source_edges()
    print("M12 sprite context catalog tests passed")
