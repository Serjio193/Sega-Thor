import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_event_dispatch_promote", ROOT / "src/tools/re_m12_event_dispatch_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    assert MODULE.TABLE_END - MODULE.TABLE_START == 76
    assert MODULE.ENTRY_COUNT == 38
    assert MODULE.DEFAULT_TARGET == 0x532A
    entries = [{"start": 0, "end": 0x10000, "kind": "UNKNOWN", "size": 0}]
    result = MODULE.split_unknown(entries, MODULE.TABLE_START, MODULE.TABLE_END)
    owned = [entry for entry in result if entry["kind"] == "STRUCTURED_DATA_CONFIRMED"]
    assert len(owned) == 1 and owned[0]["size"] == 76
    print("M12 event dispatch promotion helper tests passed")


if __name__ == "__main__":
    main()
