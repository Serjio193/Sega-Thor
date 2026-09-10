import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_screen_promote", ROOT / "src/tools/re_m12_screen_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    assert MODULE.merge_ranges([
        {"start": 20, "end": 30, "status": "ACCEPTED"},
        {"start": 25, "end": 40, "status": "ACCEPTED"},
        {"start": 50, "end": 60, "status": "REJECTED"},
    ]) == [(20, 40)]
    entries = [{"start": 0, "end": 100, "kind": "UNKNOWN"}]
    split = MODULE.split_unknown(entries, 20, 40)
    assert [(x["start"], x["end"], x["kind"]) for x in split] == [
        (0, 20, "UNKNOWN"), (20, 40, "LOCAL_ROM_DERIVED_ASSET"),
        (40, 100, "UNKNOWN")]
    descriptor = MODULE.split_descriptor([{"start": 0, "end": 100, "kind": "UNKNOWN"}], 40, 50)
    assert descriptor[1]["kind"] == "STRUCTURED_DATA_CONFIRMED"
    print("M12 screen promotion helper tests passed")


if __name__ == "__main__":
    main()
