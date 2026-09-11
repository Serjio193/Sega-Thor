"""Regression checks for runtime-correlated graphics promotion."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_runtime_graphics_promote",
    ROOT / "src/tools/re_m12_runtime_graphics_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    entries = [{"start": 0, "end": 0x1000, "kind": "UNKNOWN", "size": 0x1000}]
    entries = MODULE.split_unknown(entries, {
        "start": 0x120, "end": 0x1A0,
        "reason": "test exact 0x3820 consumer"})
    owned = [item for item in entries if item["kind"] != "UNKNOWN"]
    assert owned[0]["kind"] == "LOCAL_ROM_DERIVED_ASSET"
    assert owned[0]["classification"] == "EXACT_3820_GRAPHICS_STREAM"
    assert owned[0]["size"] == 0x80
    assert MODULE.source_owned(entries, 0x1000)["SOURCE_OWNED_BYTES"] == 0x80
    print("M12 runtime graphics promotion helper tests passed")


if __name__ == "__main__":
    main()
