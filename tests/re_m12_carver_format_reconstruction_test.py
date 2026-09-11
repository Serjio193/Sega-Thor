"""Synthetic contracts for the whole-ROM format/container Carver pass."""

import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import tempfile


MODULE_PATH = Path(__file__).parents[1] / "src/tools/m12_carver_format_reconstruction.py"
sys.path.insert(0, str(MODULE_PATH.parent))
SPEC = importlib.util.spec_from_file_location("m12_carver_format_reconstruction", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)
from m12_carver_bo_decoder import _decode_graphics


def test_fingerprint_is_bounded_and_deterministic():
    rom = bytes.fromhex("0001000200030000ffff")
    first = MODULE.fingerprint(rom, 0, len(rom))
    second = MODULE.fingerprint(rom, 0, len(rom))
    assert first == second
    assert first["size"] == 10
    assert first["sha256"] == hashlib.sha256(rom).hexdigest()
    assert "data" not in first


def test_structural_candidates_are_non_owning():
    data = (3).to_bytes(2, "big") + b"abcd" * 3
    shape = MODULE.fingerprint(data, 0, len(data))
    candidates = MODULE.boundary_candidates(data, 0, len(data), shape)
    assert any(item["kind"] == "count_times_stride" for item in candidates)
    assert all(item["exact_boundary"] for item in candidates
               if item["kind"] == "count_times_stride")


def test_existing_bo_command_grammar_returns_exact_consumption():
    stream = bytes.fromhex("0400014100")
    hit = _decode_graphics(stream, 0, len(stream))
    assert hit["mode"] == "command"
    assert hit["end"] == len(stream)
    assert hit["consumed"] == len(stream)
    assert hit["output_size"] == 1


def test_run_preserves_manifest_ownership_and_writes_hashes():
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
        report = MODULE.run(manifest_path, rom, root / "out")
        assert report["final"]["source_owned_bytes"] == 0
        assert report["final"]["source_owned_unchanged"]
        assert report["hashes"]["interval_db_sha256"]
        assert (root / "out" / "format_reconstruction_report.json").exists()


if __name__ == "__main__":
    test_fingerprint_is_bounded_and_deterministic()
    test_structural_candidates_are_non_owning()
    test_existing_bo_command_grammar_returns_exact_consumption()
    test_run_preserves_manifest_ownership_and_writes_hashes()
    print("M12 Carver format reconstruction tests passed")
