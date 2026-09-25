from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))
sys.path.insert(0, str(ROOT / "src" / "tools"))

from rom_knowledge_map import KnowledgeStore, object_id, range_id
from rom_knowledge_static_entry import ROM_SHA, _admit
import re_m12_auto_promote as stage7


class StaticEntryAdmissionTest(unittest.TestCase):
    def test_exact_fallthrough_admits_entry_without_changing_stage7_selector(self):
        rom_sha = ROM_SHA
        with tempfile.TemporaryDirectory() as temporary:
            store = KnowledgeStore(Path(temporary) / "graph.sqlite", rom_sha, 8)
            source_id = object_id(rom_sha, 0, 4, "M68K_INSTRUCTION")
            target_id = object_id(rom_sha, 4, 8, "M68K_INSTRUCTION")
            store.insert_rows("rom_range", [
                {"range_id": range_id(rom_sha, 0, 4), "rom_sha256": rom_sha, "start": 0, "end": 4},
                {"range_id": range_id(rom_sha, 4, 8), "rom_sha256": rom_sha, "start": 4, "end": 8},
            ])
            store.insert_rows("rom_object", [
                {"object_id": source_id, "range_id": range_id(rom_sha, 0, 4),
                 "object_type": "M68K_INSTRUCTION", "attributes_json": "{}"},
                {"object_id": target_id, "range_id": range_id(rom_sha, 4, 8),
                 "object_type": "M68K_INSTRUCTION", "attributes_json": "{}"},
            ])
            component = {"start": 4, "end": 8}
            candidate = {"kind": "VERIFIED_FALLTHROUGH", "target_is_entry": "true",
                "caller_pc": "0", "caller_end": "4", "target_pc": "4",
                "source_start": "0", "source_end": "4"}
            source = {"object_id": source_id, "start": 0, "end": 4,
                      "bytes": b"\x4e\x71\x4e\x71",
                      "attributes": {"opcode": 0x4E71}}
            target = {"object_id": target_id, "start": 4}
            admitted = _admit(store.db, component, candidate, source, target,
                rom_sha, "b" * 64)
            store.db.commit()
            self.assertEqual(admitted["target_pc"], 4)
            self.assertEqual(store.db.execute("SELECT status FROM claim WHERE claim_type='STATIC_VERIFIED_ENTRY'").fetchone()[0], "STATIC_VERIFIED")
            self.assertEqual(store.db.execute("SELECT status FROM relation WHERE relation_type='STATIC_ENTRY_FALLTHROUGH'").fetchone()[0], "STATIC_VERIFIED")
            self.assertEqual(store.db.execute("SELECT COUNT(*) FROM evidence_ref").fetchone()[0], 2)
            self.assertEqual(store.db.execute("SELECT COUNT(*) FROM derivation").fetchone()[0], 2)
            self.assertEqual(store.db.execute("SELECT COUNT(*) FROM relation WHERE relation_type IN ('DIRECT_CALL_TARGET','STATIC_CALLER')").fetchone()[0], 0)
            self.assertEqual(stage7.selected([{"start": 4, "end": 8,
                "exact": True, "called_by": 0}], {"records": []})[0], [])
            store.db.close()

    def test_middle_of_component_is_not_admitted_as_entry(self):
        with self.assertRaisesRegex(ValueError, "STOP_STATIC_ENTRY_CANDIDATE_KIND_NOT_ADMISSIBLE"):
            _admit(None, {"start": 4, "end": 8},
                {"kind": "VERIFIED_FALLTHROUGH", "target_is_entry": "false"},
                {}, {}, ROM_SHA, "b" * 64)


if __name__ == "__main__":
    unittest.main()
