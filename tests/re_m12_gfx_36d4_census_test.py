import hashlib
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_gfx_36d4_census", ROOT / "src/tools/re_m12_gfx_36d4_census.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_exact_call_and_caller_setup():
    rom = bytearray(b"\0" * 0x40)
    site = 0x20
    rom[site - 10:site - 6] = bytes.fromhex("303c0020")
    rom[site - 6:site] = MODULE.CALLER_SETUP_SUFFIX
    rom[site:site + 6] = MODULE.CALL_BYTES
    assert MODULE.direct_call_sites(bytes(rom)) == [site]
    setup = MODULE.parse_caller_setup(bytes(rom), site)
    assert setup["d0"] == 0x20
    assert setup["a5_literal"] == "0x60000003"


def test_bsr_target_and_body_classification():
    rom = bytearray(b"\0" * 0x4000)
    body = MODULE.BODY_START
    rom[body:body + 4] = bytes.fromhex("48e707e2")
    rom[0x36E2:0x36E2 + 6] = bytes.fromhex("43f900ff2fa8")
    rom[0x36EA:0x36EE] = bytes.fromhex("61000134")
    rom[0x3720:0x3724] = bytes.fromhex("6100f59a")
    rom[MODULE.BODY_END - 2:MODULE.BODY_END] = bytes.fromhex("4e75")
    assert MODULE._bsr_target(bytes(rom), 0x36EA) == MODULE.DECOMPRESSOR
    assert MODULE._bsr_target(bytes(rom), 0x3720) == MODULE.POSTPROCESSOR
    result = MODULE.analyze_body(bytes(rom))
    assert result["source_classification"] == "CALLER_A0_NOT_ROM_PROVEN"
    assert result["ram_destination"] == "0xFF2FA8"


def test_canonical_guard_rejects_wrong_rom():
    rom = bytes(0x100)
    MODULE.ROM_SHA256 = hashlib.sha256(rom).hexdigest()
    try:
        MODULE.build_report(rom)
    except ValueError as error:
        assert "36D4 direct-call set mismatch" in str(error)
    else:
        raise AssertionError("missing exact direct-call guard")


if __name__ == "__main__":
    test_exact_call_and_caller_setup()
    test_bsr_target_and_body_classification()
    test_canonical_guard_rejects_wrong_rom()
