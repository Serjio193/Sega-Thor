"""Regression checks for AUTO42 caller-backed routine boundaries."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_runtime_code_caller_promote", ROOT / "src/tools/re_m12_runtime_code_caller_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    entries = [{"start": 0, "end": 0x40000, "kind": "UNKNOWN", "size": 0x40000}]
    promoted = MODULE.split_unknown(entries)
    owned = [item for item in promoted if item["kind"] == "CODE_VERIFIED"]
    assert owned[0]["start"] == MODULE.START
    assert owned[0]["end"] == MODULE.END
    assert owned[0]["size"] == 302
    assert MODULE.source_owned(promoted, 0x40000)["SOURCE_OWNED_BYTES"] == 302
    print("M12 caller-backed runtime-code promotion helper tests passed")


if __name__ == "__main__":
    main()
