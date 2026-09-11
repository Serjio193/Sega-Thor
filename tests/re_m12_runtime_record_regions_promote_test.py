"""Regression checks for AUTO44 record-region tiling."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_runtime_record_regions_promote", ROOT / "src/tools/re_m12_runtime_record_regions_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    entries = [{"start": 0, "end": 0x300000, "kind": "UNKNOWN", "size": 0x300000}]
    promoted = MODULE.split_unknown(entries, 0x100, 0x120, MODULE.AF16)
    owned = [e for e in promoted if e["kind"] == "STRUCTURED_DATA_CONFIRMED"]
    assert owned[0]["size"] == 0x20
    assert MODULE.source_owned(promoted, 0x300000)["SOURCE_OWNED_BYTES"] == 0x20
    assert len(MODULE.RANGES) == 51
    assert len(MODULE.NEW_RANGES) == 27
    print("M12 runtime record-region promotion helper tests passed")


if __name__ == "__main__":
    main()
