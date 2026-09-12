import hashlib
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_screen_descriptor_candidate_promote",
    ROOT / "src/tools/re_m12_screen_descriptor_candidate_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_contract_rejects_wrong_stream_boundary():
    rom = bytearray(b"\0" * 0x300000)
    rom[MODULE.RECORD_START + 4:MODULE.RECORD_START + 8] = MODULE.STREAM[0].to_bytes(4, "big")
    rom[MODULE.RECORD_START + 8:MODULE.RECORD_START + 12] = bytes((78, 79, 80, 81))
    rom[MODULE.CALL_SITE:MODULE.CALL_SITE + 6] = MODULE.CALL_BYTES
    MODULE.ROM_SHA256 = hashlib.sha256(bytes(rom)).hexdigest()
    census = {"records": [{"start": MODULE.STREAM[0], "end": MODULE.STREAM[1] + 1,
                            "decompressed_bytes": MODULE.STREAM[2]}]}
    try:
        MODULE.verify_contract(bytes(rom), census)
    except ValueError as error:
        assert "boundary" in str(error)
    else:
        raise AssertionError("wrong stream boundary was accepted")


if __name__ == "__main__":
    test_contract_rejects_wrong_stream_boundary()
