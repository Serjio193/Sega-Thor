import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_targeted_dynamic_sat_capture", ROOT / "src/tools/m12_targeted_dynamic_sat_capture.py"
)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


def test_entry_diff_correlates_shadow_and_writer():
    snapshots = [
        {"frame": 0, "sat": [0] * 16, "shadow": [0] * 16},
        {"frame": 1, "sat": [0] * 16, "shadow": [0, 0, 0, 1] + [0] * 12},
        {"frame": 2, "sat": [0, 0, 0, 1] + [0] * 12, "shadow": [0, 0, 0, 1] + [0] * 12},
    ]
    changes = MODULE.changed_entries(snapshots)
    assert changes[0]["entry"] == 0
    report = MODULE.correlate(changes[0], snapshots, [{"frame": 1, "pc": "0x00A372", "address": "0xFF13CF"}])
    assert report["shadow_offsets"] == [3]
    assert report["writer_window_pcs"] == ["0x00A372"]
    assert report["shadow_entry_equals_sat_entry"] is True


def test_no_change_fails_closed():
    document = {"snapshots": [{"frame": 0, "sat": [0] * 8, "shadow": [0] * 8}], "shadow_writes": []}
    assert MODULE.analyze(document)["status"] == "STOP"


if __name__ == "__main__":
    test_entry_diff_correlates_shadow_and_writer()
    test_no_change_fails_closed()
    print("M12 targeted dynamic SAT capture tests passed")
