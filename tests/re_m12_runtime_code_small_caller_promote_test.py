"""Regression checks for AUTO43 small caller-backed routine boundaries."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_runtime_code_small_caller_promote",
    ROOT / "src/tools/re_m12_runtime_code_small_caller_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    entries = [{"start": 0, "end": 0x10000, "kind": "UNKNOWN", "size": 0x10000}]
    owned = [e for e in MODULE.split_unknown(entries) if e["kind"] == "CODE_VERIFIED"]
    assert (owned[0]["start"], owned[0]["end"], owned[0]["size"]) == (MODULE.START, MODULE.END, 116)
    print("M12 small caller-backed runtime-code promotion helper tests passed")


if __name__ == "__main__":
    main()
