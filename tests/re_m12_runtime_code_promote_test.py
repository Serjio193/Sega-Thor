"""Regression checks for AUTO40 runtime-code promotion boundaries."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_runtime_code_promote", ROOT / "src/tools/re_m12_runtime_code_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    entries = [{"start": 0, "end": 0xC000, "kind": "UNKNOWN", "size": 0xC000}]
    promoted = MODULE.split_unknown(entries)
    owned = [item for item in promoted if item["kind"] == "CODE_VERIFIED"]
    assert owned[0]["start"] == MODULE.START
    assert owned[0]["end"] == MODULE.END
    assert owned[0]["size"] == 0xB8
    assert MODULE.source_owned(promoted, 0x1000)["SOURCE_OWNED_BYTES"] == 0xB8
    print("M12 runtime-code promotion helper tests passed")


if __name__ == "__main__":
    main()
