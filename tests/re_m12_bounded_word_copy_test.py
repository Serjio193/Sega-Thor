"""Regression checks for bounded word-copy table contracts."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_bounded_word_copy", ROOT / "src/tools/re_m12_bounded_word_copy_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x300000)
    rom[MODULE.ROUTINE:MODULE.ROUTINE + len(MODULE.ROUTINE_BYTES)] = MODULE.ROUTINE_BYTES
    for start, end, caller, expected in MODULE.TABLES:
        rom[start] = 0
        rom[start + 1] = ((end - start - 2) // 2) - 1
        rom[caller:caller + len(expected)] = expected
    report = MODULE.parse_contract(bytes(rom), verify_identity=False)
    assert [item["word_count"] for item in report["tables"]] == [32, 13]
    entries = [{"start": 0, "end": len(rom), "kind": "UNKNOWN", "size": len(rom)}]
    for start, end, _, _ in MODULE.TABLES:
        entries = MODULE.split_unknown(entries, start, end)
    assert sum(item["size"] for item in entries if item["kind"] == "STRUCTURED_DATA_CONFIRMED") == 0x5E
    print("M12 bounded word-copy helper tests passed")


if __name__ == "__main__":
    main()
