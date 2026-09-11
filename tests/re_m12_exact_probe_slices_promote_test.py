"""Regression checks for bounded exact probe-slice promotion."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_exact_probe_slices_promote",
    ROOT / "src/tools/re_m12_exact_probe_slices_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    entries = [{"start": 0, "end": 0x1000, "kind": "UNKNOWN", "size": 0x1000}]
    entries = MODULE.split_unknown(entries, 0x120, 0x1A0)
    owned = [item for item in entries if item["kind"] != "UNKNOWN"]
    assert owned[0]["kind"] == "CODE_VERIFIED"
    assert owned[0]["classification"] == "RUNTIME_CORRELATED_EXACT_PROBE_SLICE"
    assert owned[0]["size"] == 0x80
    metrics = MODULE.source_owned(entries, 0x1000)
    assert metrics["SOURCE_OWNED_BYTES"] == 0x80
    print("M12 exact probe-slice promotion helper tests passed")


if __name__ == "__main__":
    main()
