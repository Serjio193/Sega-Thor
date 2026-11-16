import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


MODULE_PATH = Path(__file__).parents[1] / "src/tools/thor_evidence/rom_read_consumer_index.py"
SPEC = importlib.util.spec_from_file_location("rom_read_consumer_index", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(MODULE)


def bus_read(sequence, cpu, reader_pc, logical, value, width, domain, physical):
    return MODULE.RECORD.pack(sequence, sequence, sequence * 10, reader_pc,
                              logical, value, 0x8800, cpu, width, domain, 0,
                              physical)


class RomReadConsumerIndexTest(unittest.TestCase):
    def test_import_maps_physical_spans_and_deduplicates_overlap(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            raw = root / "flow-v1-records.bin"
            index = root / "flow-v1-segments.jsonl"
            receipt = root / "receipt.json"
            output = root / "out"
            first = bus_read(10, 0, 0x2888, 0x11FD4C, 0xABCD, 16, 0, 0)
            second = bus_read(11, 1, 0x410, 0x8000, 0xEF, 8, 8, 0x1234)
            raw.write_bytes(first + second + first + second)
            chunks = [first, second, first, second]
            with index.open("w", encoding="utf-8") as stream:
                for number, chunk in enumerate(chunks):
                    digest = hashlib.sha256(chunk).hexdigest()
                    segment = {"record_count": 1, "records_sha256": digest,
                               "run_id": 77, "epoch": 2, "entry_frame": 0}
                    stream.write(json.dumps({"raw_offset": number * MODULE.RECORD.size,
                                             "raw_length": len(chunk),
                                             "raw_sha256": digest,
                                             "segment": segment}) + "\n")
            raw_hash = hashlib.sha256(raw.read_bytes()).hexdigest()
            index_hash = hashlib.sha256(index.read_bytes()).hexdigest()
            receipt.write_text(json.dumps({
                "status": "PASS_FLOW_V1_EXACT_ROM_RANGE_LINKAGE",
                "rom_projection": {"rom_sha256": MODULE.ROM_SHA256},
                "runtime": {"run_id": 77},
                "flow_handoff": {"raw_path": str(raw), "index_path": str(index),
                                 "raw_sha256": raw_hash, "index_sha256": index_hash,
                                 "raw_bytes": raw.stat().st_size, "segments": 4},
            }), encoding="utf-8")

            summary = MODULE.import_receipts([receipt], output)
            events = [json.loads(line) for line in
                      (output / "rom-read-events.jsonl").read_text().splitlines()]
            self.assertEqual(summary["event_count"], 2)
            self.assertTrue(summary["physical_address_present"])
            self.assertTrue(summary["reader_pc_present"])
            self.assertTrue(summary["access_width_present"])
            self.assertIsNone(events[0]["caller_pc"])
            self.assertEqual(events[0]["source_segment_hash_count"], 1)
            self.assertEqual(len(events[0]["source_segment_hashes_sha256"]), 64)
            self.assertEqual((events[0]["physical_rom_start"],
                              events[0]["physical_rom_end"]), (0x11FD4C, 0x11FD4E))
            self.assertEqual((events[1]["physical_rom_start"],
                              events[1]["physical_rom_end"]), (0x1234, 0x1235))
            self.assertEqual(summary["target_intersections"]["4635581e9d3deee0"]
                             ["exact_intersection_bytes"], 2)
            self.assertTrue((output / "import-provenance.json").is_file())

            diagnostic = json.loads(receipt.read_text(encoding="utf-8"))
            diagnostic["status"] = "STOPPED_FRAME_LIMIT"
            diagnostic["runtime"] = {"run_id": 77, "rom_sha256": MODULE.ROM_SHA256}
            diagnostic.pop("rom_projection")
            receipt.write_text(json.dumps(diagnostic), encoding="utf-8")
            diagnostic_summary = MODULE.import_receipts([receipt], root / "diagnostic")
            source = diagnostic_summary["source_artifacts"][0]
            self.assertEqual(source["identity_basis"], "RUNTIME_ROM_SHA256")
            self.assertEqual(source["receipt_status"], "STOPPED_FRAME_LIMIT")


if __name__ == "__main__":
    unittest.main()
