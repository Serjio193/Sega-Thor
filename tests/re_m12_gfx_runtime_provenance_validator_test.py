import importlib.util
import json
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))
SPEC = importlib.util.spec_from_file_location(
    "re_m12_gfx_runtime_provenance", ROOT / "src/tools/re_m12_gfx_runtime_provenance.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    rom = bytes(0x100)
    payload = {
        "schema": "oasis.m68k.m12-gfx-runtime-provenance.v1",
        "canonical_rom_sha256": MODULE.ROM_SHA256,
        "target": MODULE.TARGET,
        "writes_emitted": False,
        "captures": [{"caller": "0x0003B236", "frame": 1,
                      "registers": {"a": ["0x100000", "0x00FF316C"]}}],
        "frames_executed": 1,
    }
    try:
        MODULE.validate_capture(payload, rom, [])
    except ValueError as error:
        assert "not ROM" in str(error)
    else:
        raise AssertionError("non-ROM runtime source must fail closed")
    assert json.loads(json.dumps(payload))["writes_emitted"] is False
    print("re_m12_gfx_runtime_provenance_validator_test: pass")


if __name__ == "__main__":
    main()
