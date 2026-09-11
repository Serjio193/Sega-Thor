import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_nibble_lookup_promote", ROOT / "src/tools/re_m12_nibble_lookup_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    assert MODULE.TABLE_END - MODULE.TABLE_START == 32
    assert len(MODULE.VALUES) == 16
    entries = [{"start": 0, "end": 0x70000, "kind": "UNKNOWN", "size": 0}]
    result = MODULE.split_unknown(entries)
    owned = [entry for entry in result if entry["kind"] == "STRUCTURED_DATA_CONFIRMED"]
    assert len(owned) == 1 and owned[0]["size"] == 32
    print("M12 nibble lookup promotion helper tests passed")


if __name__ == "__main__":
    main()
