"""R4 MASTER V2-only startup and continuation tests."""
from __future__ import annotations

import json
import hashlib
import os
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))
import master_startup_authority as authority  # noqa: E402
from master_v2_shadow import decode_master_v2  # noqa: E402


MASTER = ROOT / "build/thor-evidence/master-v2-shadow-r3/run-1789872670/master-v2-r3.bin"
ROM = Path(r"C:/Github/gpgx-test-roms/Beyond Oasis (USA).md")


class MasterStartupAuthorityTests(unittest.TestCase):
    def setUp(self) -> None:
        if not MASTER.is_file() or not ROM.is_file():
            self.skipTest("accepted R3 MASTER/ROM evidence is unavailable")
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.master = self.root / "master.bin"
        self.rom = self.root / "rom.md"
        os.link(MASTER, self.master)
        shutil.copyfile(ROM, self.rom)
        self.pointer = self.root / "current.json"
        template = json.loads((ROOT / "build/thor-evidence/master-v2/current.json").read_text(encoding="utf-8"))
        template["path"] = self.master.name
        template["sha256"] = hashlib.sha256(self.master.read_bytes()).hexdigest()
        template["bytes"] = self.master.stat().st_size
        decoded = decode_master_v2(self.master)
        template["logical_sha256"] = decoded["overall_sha256"]
        template["sections"] = {item["name"]: item["sha256"] for item in decoded["sections"]}
        self.pointer.write_text(json.dumps(template), encoding="utf-8")

    def tearDown(self) -> None:
        self.temp.cleanup()

    def test_a_b_k_valid_master_without_legacy_reads(self) -> None:
        state = authority.load_startup(self.root, self.rom, self.pointer)
        self.assertEqual(state.generation_id, "master-v2-88dba3b088db3ebb")
        self.assertEqual(state.metrics.legacy_bytes_read, 0)
        self.assertEqual(state.canonical.source_owned_bytes, 1_475_642)
        self.assertEqual(sum(state.emission_bytes.values()), authority.ROM_SIZE)
        self.assertEqual(state.outcomes.require_stage("STAGE_8")["status"],
                         "PASS_POSTRUN_FULL_ROM_AUDIT_V1")

    def test_c_d_e_f_g_i_h_invalid_identity_stops(self) -> None:
        data = json.loads(self.pointer.read_text(encoding="utf-8"))
        for field, value in (("sha256", "0" * 64), ("logical_sha256", "1" * 64),
                             ("rom_sha256", "2" * 64)):
            bad = dict(data); bad[field] = value
            self.pointer.write_text(json.dumps(bad), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, authority.STOP_INTEGRITY):
                authority.load_startup(self.root, self.rom, self.pointer)
        bad = dict(data); bad["sections"] = dict(data["sections"])
        bad["sections"]["canonical_knowledge"] = "3" * 64
        self.pointer.write_text(json.dumps(bad), encoding="utf-8")
        with self.assertRaisesRegex(ValueError, authority.STOP_INTEGRITY):
            authority.load_startup(self.root, self.rom, self.pointer)
        self.pointer.unlink()
        with self.assertRaisesRegex(ValueError, authority.STOP_UNAVAILABLE):
            authority.load_startup(self.root, self.rom, self.pointer)

    def test_j_n_shadow_boundary_and_n_plus_one_candidate(self) -> None:
        state = authority.load_startup(self.root, self.rom, self.pointer)
        candidate = state.prepare_candidate({"run_id": 9001, "segments": 4})
        self.assertEqual(candidate["status"], "CANDIDATE_READY")
        self.assertEqual(candidate["parent_generation"], state.generation_id)
        self.assertNotEqual(candidate["generation_id"], state.generation_id)
        self.assertEqual(candidate["source_owned_before"], candidate["source_owned_after"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
