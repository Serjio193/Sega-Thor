"""Synthetic contracts for whole-ROM static consumer recovery."""

import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import tempfile


MODULE_PATH = Path(__file__).parents[1] / "src/tools/m12_carver_static_recovery.py"
sys.path.insert(0, str(MODULE_PATH.parent))
SPEC = importlib.util.spec_from_file_location("m12_carver_static_recovery", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_static_schema_and_contracts():
    assert MODULE.static_schema("oasis.m68k.re-slice.v1")
    assert not MODULE.static_schema("oasis.m68k.m12-carver.global-sweep.v1")
    assert MODULE.exact_contract({"boundary_status": "BOUNDARY_EXACT"}) is False
    assert MODULE.exact_contract({"boundary_status": "CLOSED"}) is True


def test_reference_is_typed_and_non_owning():
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        rom_bytes = bytes(0x300000)
        rom = root / "rom.bin"
        rom.write_bytes(rom_bytes)
        manifest = {"schema": "oasis.full-rom-split.v1", "start": 0,
                    "end": 0x300000, "rom_size": 0x300000,
                    "rom_sha256": hashlib.sha256(rom_bytes).hexdigest(),
                    "metrics": {"SOURCE_OWNED_BYTES": 0},
                    "entries": [{"start": 0, "end": 0x300000, "kind": "UNKNOWN",
                                 "classification": "UNKNOWN", "confidence": "UNKNOWN"}]}
        manifest_path = root / "manifest.json"
        manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
        evidence = root / "slice.json"
        evidence.write_text(json.dumps({"schema": "oasis.m68k.re-slice.v1",
                                        "entry_point": "0x10",
                                        "instructions": [{"address": "0x10",
                                                           "memory_references": [{"address": "0x40"}]}]}),
                            encoding="utf-8")
        result = MODULE.run(manifest_path, rom, root, root / "out")
        assert result["reference_census"]["by_mechanism"]["CODE_TO_ROM_RANGE"] == 1
        assert result["final"]["source_owned_bytes"] == 0
        assert result["hashes"]["interval_db_sha256"]


if __name__ == "__main__":
    test_static_schema_and_contracts()
    test_reference_is_typed_and_non_owning()
    print("M12 Carver static recovery tests passed")
