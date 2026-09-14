import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))
sys.path.insert(0, str(ROOT / "src/tools"))

from auto67_capsule_codec import (  # noqa: E402
    CapsuleFormatError,
    decode_capsule,
)
from auto67_materializer import materialize  # noqa: E402
from auto67_persistence import materialized_descriptor  # noqa: E402
from thor_evidence.store import Store  # noqa: E402
from auto67_capsule import CapsulePool  # noqa: E402


def write_v2(path: Path, records, capsule_id=0, start_frame=100):
    logical = 64 + len(records) * 20
    payload = bytearray(b"O67V")
    payload.extend(struct.pack("<IIIII", 2, capsule_id, start_frame,
                               logical, len(records)))
    for record in records:
        payload.extend(struct.pack("<IIIII", *record))
    path.write_bytes(payload)


def write_legacy(path: Path, records, capsule_id=0, start_frame=100):
    logical = 64 + len(records) * 16
    payload = bytearray(b"O67C")
    payload.extend(struct.pack("<IIII", capsule_id, start_frame,
                               logical, len(records)))
    for record in records:
        payload.extend(struct.pack("<IIII", *record))
    path.write_bytes(payload)


class Auto674CapsuleTest(unittest.TestCase):
    def test_stale_free_status_cannot_erase_active_lease(self):
        pool = CapsulePool(max_live=1)
        try:
            claimed = pool.claim(0, "INV-1", {"frame": 10, "pc": "0x27EC"})
            self.assertIsNotNone(claimed)
            assert claimed is not None
            pool.sync([{"capsule_id": 0, "state": "FREE", "lease_id": None}])
            self.assertEqual(pool.snapshot()["items"][0]["lease_id"], claimed.lease_id)
            self.assertEqual(pool.snapshot()["items"][0]["state"], "CAPTURING")
        finally:
            pool.stop()

    def test_v2_decodes_and_stale_or_truncated_capsules_fail_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capsule-00-L00000001.bin"
            write_v2(path, [(1, 101, 0xC00004, 0x27EC, 1),
                            (2, 102, 0xC00006, 0x2800, 1)])
            decoded = decode_capsule(path, expected_capsule_id=0,
                                     expected_lease_id="L00000001")
            self.assertEqual(decoded.format_version, 2)
            self.assertEqual(decoded.event_count, 2)
            self.assertEqual(decoded.observations[1].kind, "BUS_WRITE_PC")

            with self.assertRaises(CapsuleFormatError):
                decode_capsule(path, expected_capsule_id=0,
                               expected_lease_id="L00000002")
            path.write_bytes(path.read_bytes()[:-1])
            with self.assertRaises(CapsuleFormatError):
                decode_capsule(path)

    def test_legacy_o67c_remains_readable(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capsule-00-L00000001.bin"
            write_legacy(path, [(1, 101, 0xC00004, 0x27EC)])
            decoded = decode_capsule(path, expected_capsule_id=0,
                                     expected_lease_id="L00000001")
            self.assertEqual(decoded.format_version, 1)
            self.assertEqual(decoded.event_count, 1)
            self.assertIsNone(decoded.observations[0].kind)

    def test_materialization_uses_real_records_and_not_nearby_exec(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "capsule-00-L00000001.bin"
            write_v2(path, [(1, 600, 0xC00004, 0x27EC, 1),
                            (2, 601, 0xC00006, 0x2800, 1)])
            capsule = decode_capsule(path, 0, "L00000001")
            seed = {"kind": "BUS_WRITE_PC", "pc": "0x0027EC",
                    "address": "0xC00004"}
            result = materialize(seed, capsule)
            self.assertEqual(len(result["runtime_observations"]), 2)
            self.assertEqual(result["causal_facts"][0]["writer_pc"], "0x002800")
            self.assertEqual(result["causal_facts"][0]["evidence"],
                             "RUNTIME_CAPSULE")

            write_v2(path, [(1, 600, 0xC00004, 0x27EC, 1),
                            (2, 601, 0xC00006, 0x2800, 2)])
            exec_only = materialize(seed, decode_capsule(path, 0, "L00000001"))
            self.assertEqual(exec_only["causal_facts"], [])

    def test_materialized_identity_excludes_provenance_but_keeps_causal_fact(self):
        materialized = {
            "seed": {"kind": "BUS_WRITE_PC", "pc": "0x0027EC",
                     "address": "0xC00004"},
            "runtime_observations": [{"kind": "BUS_WRITE_PC",
                                       "pc": "0x002800", "address": "0xC00006"}],
            "causal_facts": [{"kind": "BUS_WRITE_PC", "writer_pc": "0x002800",
                               "address": "0xC00006", "evidence": "RUNTIME_CAPSULE"}],
            "unresolved_frontier": {"status": "UNKNOWN", "missing": ["STATIC_DECODE"],
                                     "evidence": "RUNTIME_CAPSULE"},
            "capsule_format_version": 2,
            "capsule_record_count": 2,
        }
        first = materialized_descriptor(
            {"kind": "BUS_WRITE_PC", "pc": "0x0027EC", "address": "0xC00004",
             "seq": 1, "frame": 600, "epoch": 1}, materialized,
            "BOUNDED_UNRESOLVED", 600, 1, "L1", "INV-A")
        replay = materialized_descriptor(
            {"kind": "BUS_WRITE_PC", "pc": "0x0027EC", "address": "0xC00004",
             "seq": 999, "frame": 9000, "epoch": 9}, materialized,
            "PROVEN", 9000, 9, "L9", "INV-Z")
        self.assertEqual(first["chain_hash"], replay["chain_hash"])
        altered = json.loads(json.dumps(materialized))
        altered["causal_facts"][0]["writer_pc"] = "0x002801"
        changed = materialized_descriptor(
            {"kind": "BUS_WRITE_PC", "pc": "0x0027EC", "address": "0xC00004"},
            altered, "BOUNDED_UNRESOLVED", 600, 1, "L1", "INV-A")
        self.assertNotEqual(first["chain_hash"], changed["chain_hash"])

    def test_record_class_migration_and_materialized_row(self):
        with tempfile.TemporaryDirectory() as directory:
            database = Path(directory) / "chains.sqlite"
            store = Store(database)
            try:
                store.begin_live_session("S1", "AUTO67-live", "0", "{}")
                item = materialized_descriptor(
                    {"kind": "BUS_WRITE_PC", "pc": "0x0027EC",
                     "address": "0xC00004"},
                    {"seed": {}, "runtime_observations": [], "causal_facts": [],
                     "unresolved_frontier": {"status": "UNKNOWN", "missing": []}},
                    "BOUNDED_UNRESOLVED", 600, 0, "L1", "INV-1")
                store.record_live_chain("S1", item["chain_hash"],
                                        item["canonical_payload"], item["status"],
                                        item["frame"], item["provenance"],
                                        item["record_class"])
                row = store.connection.execute(
                    "SELECT record_class FROM live_chain WHERE chain_hash=?",
                    (item["chain_hash"],)).fetchone()
                self.assertEqual(row["record_class"], "MATERIALIZED_CHAIN")
            finally:
                store.close()


if __name__ == "__main__":
    unittest.main()
