"""Regression checks for the exact 0x003260 static routine contract."""
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_static_003260", ROOT / "src/tools/re_m12_static_003260_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytearray(0x300000)
    rom[MODULE.CALLER:MODULE.CALLER + len(MODULE.CALLER_BYTES)] = MODULE.CALLER_BYTES
    contract = MODULE.parse_contract(bytes(rom), verify_identity=False)
    assert contract["routine_end"] - contract["routine_start"] == 0x88
    entries = [{"start": 0, "end": len(rom), "kind": "UNKNOWN", "size": len(rom)}]
    promoted = MODULE.split_unknown(entries)
    owned = next(item for item in promoted if item["kind"] == "CODE_VERIFIED")
    assert (owned["start"], owned["end"]) == (MODULE.ROUTINE_START, MODULE.ROUTINE_END)
    assert owned["classification"] == "STATIC_EXACT_BOUNDED_ROUTINE"
    print("M12 static 0x003260 helper tests passed")


if __name__ == "__main__":
    main()
