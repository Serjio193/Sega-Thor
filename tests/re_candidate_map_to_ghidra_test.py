import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_candidate_map_to_ghidra", ROOT / "src/tools/re_candidate_map_to_ghidra.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    source = {
        "ghidra": {"program": "test.bin", "language": "68000:BE:32:default"},
        "candidates": [
            {"entry": "0x10", "ghidra_function": True,
             "ghidra_range": {"start": "0x10", "end": "0x20"}},
            {"entry": "0x30", "ghidra_function": False,
             "ghidra_range": {"start": "0x30", "end": "0x40"}},
        ],
    }
    result = MODULE.rebuild(source)
    assert len(result["functions"]) == 1
    assert result["functions"][0]["range"] == "0x10..0x20"
    print("candidate-map Ghidra reconstruction helper tests passed")


if __name__ == "__main__":
    main()
