"""Regression checks for M12 consumer promotion range semantics."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_consumer_promote", ROOT / "src/tools/re_m12_consumer_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    entries = [{"start": 0, "end": 100, "kind": "UNKNOWN"}]
    entries = MODULE.split_unknown(entries, 10, 20, "LOCAL_ROM_DERIVED_ASSET",
                                   "TEST", "test", "test")
    assert [(x["start"], x["end"], x["kind"]) for x in entries] == [
        (0, 10, "UNKNOWN"), (10, 20, "LOCAL_ROM_DERIVED_ASSET"),
        (20, 100, "UNKNOWN")]
    try:
        MODULE.split_unknown(entries, 15, 25, "LOCAL_ROM_DERIVED_ASSET",
                             "TEST", "test", "test")
    except ValueError:
        pass
    else:
        raise AssertionError("overlap with owned range was accepted")
    print("M12 consumer promotion helpers passed")


if __name__ == "__main__":
    main()
