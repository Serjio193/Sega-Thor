"""MASTER V2 canonical read-layer contract."""
from __future__ import annotations

import json
from pathlib import Path
import sqlite3
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))
import master_canonical_view as view  # noqa: E402
import master_v2_shadow as shadow  # noqa: E402


def section(tables: dict[str, list[list[object]]]) -> bytes:
    return json.dumps({"tables": tables}, separators=(",", ":")).encode()


class CanonicalViewTests(unittest.TestCase):
    def setUp(self) -> None:
        self.meta = {"schema": shadow.SCHEMA, "generation_id": "master-v2-test",
                     "parent_generation": "master-parent", "parent_master_sha256": "m",
                     "canonical_master_sha256": "cm", "canonical_knowledge_sha256": "ck",
                     "rom": {"sha256": shadow.ROM_SHA, "size": shadow.ROM_SIZE},
                     "absorbed_run_ids": [7], "source_owned_delta": 0}
        map_tables = {name: [] for name in view.MASTER_SCHEMA}
        map_tables["map_meta"] = [["source_owned_bytes", "4"]]
        knowledge_tables = {name: [] for name in view.KNOWLEDGE_SCHEMA}
        knowledge_tables["emission"] = [[0, 4, "ASM", "X", "Y", 1, "", ""]]
        knowledge_tables["rom_range"] = [["r", shadow.ROM_SHA, 0, 4]]
        knowledge_tables["rom_object"] = [["o", "r", "M68K_INSTRUCTION", "{}"]]
        knowledge_tables["claim"] = [["c", "o", "EXECUTED_FROM_ROM", "true", "OBSERVED"]]
        knowledge_tables["relation"] = [["n", "EXECUTED_NEXT", "o", "o", None, "OBSERVED", "{}"],
                                          ["p", "OBSERVED_NEXT_PC", "o", None, 4, "OBSERVED", "{}"]]
        self.payloads = {"meta": json.dumps(self.meta).encode(),
                         "canonical_map_master": section(map_tables),
                         "canonical_knowledge": section(knowledge_tables)}
        self.temp = tempfile.TemporaryDirectory(); self.path = Path(self.temp.name) / "shadow.bin"
        self.reader_patch = mock.patch.object(view, "read_master_v2_section",
                                               side_effect=lambda path, name: self.payloads[name])
        self.reader_patch.start()

    def tearDown(self) -> None:
        self.reader_patch.stop()
        self.temp.cleanup()

    def make_view(self) -> view.MasterCanonicalView:
        return view.MasterCanonicalView(self.path)

    def test_api_reads_v2_canonical_state(self) -> None:
        canonical = self.make_view()
        self.assertEqual(canonical.generation_id, "master-v2-test")
        self.assertEqual(canonical.source_owned_bytes, 4)
        self.assertEqual(list(canonical.emission_partition)[0]["emission_type"], "ASM")
        self.assertEqual(list(canonical.instruction_map)[0]["object_id"], "o")
        self.assertEqual(len(list(canonical.executed_instruction_facts)), 1)
        self.assertEqual(len(list(canonical.executed_next)), 1)
        self.assertEqual(len(list(canonical.observed_next_pc)), 1)
        self.assertEqual(canonical.canonical_hashes["canonical_master_sha256"], "cm")

    def test_materialization_uses_view_and_preserves_semantics(self) -> None:
        canonical = self.make_view()
        generation = canonical.materialize(Path(self.temp.name) / "scratch")
        db = sqlite3.connect(generation / "knowledge.sqlite")
        self.assertEqual(db.execute("SELECT SUM(end-start) FROM emission WHERE source_owned=1").fetchone()[0], 4)
        db.close()

    def test_legacy_shadow_pointer_is_verification_only(self) -> None:
        canonical = self.make_view(); legacy = Path(self.temp.name) / "legacy"; legacy.mkdir()
        (legacy / "current.json").write_text(json.dumps({"master_sha256": "cm", "knowledge_sha256": "ck"}))
        canonical.verify_legacy_shadow(legacy)
        (legacy / "current.json").write_text(json.dumps({"master_sha256": "changed", "knowledge_sha256": "ck"}))
        with self.assertRaisesRegex(ValueError, "SHADOW_MISMATCH"):
            canonical.verify_legacy_shadow(legacy)


if __name__ == "__main__":
    unittest.main(verbosity=2)
