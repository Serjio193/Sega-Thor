"""Regression checks for the exact AUTO38 menu record-stream contract."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_menu_record_stream", ROOT / "src/tools/re_m12_menu_record_stream_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    assert MODULE.TABLE_END - MODULE.TABLE_START == 8
    assert MODULE.RANGE_END - MODULE.TABLE_START == 238
    assert MODULE.TABLE_OFFSETS == (8, 0xD4, 0xDA, 0xE0)
    assert MODULE.EXPECTED_STREAM_COUNTS == (0x21, 0, 0, 0)
    assert len(MODULE.CONSUMER_CONTRACTS) == 5
    entries = [{"start": 0, "end": MODULE.RANGE_END + 1,
                "kind": "UNKNOWN", "size": MODULE.RANGE_END + 1}]
    promoted = MODULE.split_unknown(entries)
    assert promoted[1]["start"] == MODULE.TABLE_START
    assert promoted[1]["size"] == 238
    assert promoted[1]["classification"] == (
        "MENU_OFFSET_TABLE_AND_COUNT_BOUNDED_RECORD_STREAMS")
    print("M12 menu record-stream promotion helper tests passed")


if __name__ == "__main__":
    main()
