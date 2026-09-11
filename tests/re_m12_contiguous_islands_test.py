"""Regression checks for the exact contiguous-island transaction shape."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_contiguous_islands", ROOT / "src/tools/re_m12_contiguous_islands_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    assert len(MODULE.SLICES) == 15
    assert sum(end - start for start, end in MODULE.SLICES) == 1158
    entries = [{"start": 0, "end": 0x70000, "kind": "UNKNOWN", "size": 0x70000}]
    for start, end in MODULE.SLICES:
        entries = MODULE.split_unknown(entries, start, end)
    owned = [item for item in entries if item["kind"] == "CODE_VERIFIED"]
    assert sum(item["size"] for item in owned) == 1158
    assert all(item["classification"] == "STATIC_DIRECT_CALLER_RTS_ISLAND" for item in owned)
    print("M12 contiguous-island helper tests passed")


if __name__ == "__main__":
    main()
