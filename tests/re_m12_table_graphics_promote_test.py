import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_table_graphics_promote",
    ROOT / "src" / "tools" / "re_m12_table_graphics_promote.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_split_unknown_promotes_only_exact_stream():
    entries = [{"start": 0, "end": 100, "kind": "UNKNOWN", "size": 100}]
    result = MODULE.split_unknown(entries, 20, 80)
    assert [(item["start"], item["end"], item["kind"]) for item in result] == [
        (0, 20, "UNKNOWN"), (20, 80, "LOCAL_ROM_DERIVED_ASSET"),
        (80, 100, "UNKNOWN")]


def test_split_unknown_rejects_overlap():
    entries = [{"start": 0, "end": 50, "kind": "DATA_KNOWN", "size": 50},
               {"start": 50, "end": 100, "kind": "UNKNOWN", "size": 50}]
    try:
        MODULE.split_unknown(entries, 40, 60)
    except ValueError as error:
        assert "overlaps non-UNKNOWN" in str(error)
    else:
        raise AssertionError("expected overlap rejection")
