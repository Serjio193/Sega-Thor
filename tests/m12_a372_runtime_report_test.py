import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_a372_runtime_report", ROOT / "src/tools/m12_a372_runtime_report.py"
)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


def test_payload_free_runtime_summary():
    capture = {
        "schema": "oasis.m68k.m12-a372-runtime.v1",
        "canonical_rom_sha256": MODULE.ROM_SHA256,
        "expected_rom_sha256": MODULE.ROM_SHA256,
        "emulator": "bizhawk",
        "version": "2.11.1",
        "frames_executed": 1800,
        "hook_counts": {"A196": 1, "A342": 2, "A372": 2},
        "root_counts": {"0x0000A438": 1, "0x0000A480": 1},
        "events": [
            {"kind": "A342", "root": "0x0000A438"},
            {"kind": "A342", "root": "0x0000A480"},
            {"kind": "A372", "root": "0x0000A480"},
        ],
        "source_writes": [{"address": "0x00FF13CC"}],
        "write_cap": 4096,
        "state_writes_emitted": False,
    }
    result = MODULE.summarize(capture)
    assert result["status"] == "RUNTIME_A372_REGISTER_ROOT_CAPTURE_OBSERVED"
    assert result["root_counts_from_events"]["0x0000A438"] == 1
    assert result["ownership"]["rom_bytes_added"] == 0


if __name__ == "__main__":
    test_payload_free_runtime_summary()
