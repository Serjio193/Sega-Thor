import struct
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools/thor_evidence"))

from auto67_predecessor import decode, resolve  # noqa: E402
from auto67_materializer import required_registers  # noqa: E402


def write_capture(path: Path, records, *, target=0x20, epoch=0,
                  consumer=3, complete=True, truncated=False, gap=False,
                  register_mask=1, overwrites=0):
    values = [1, epoch, target, register_mask, records[0][1] if records else 0,
              records[-1][1] if records else 0, len(records), int(complete),
              int(truncated), int(gap), consumer if consumer is not None else 0xFFFFFFFF,
              100 if consumer is not None else 0xFFFFFFFF, overwrites]
    payload = bytearray(b"O67P")
    payload.extend(struct.pack("<13I", *values))
    for record in records:
        payload.extend(struct.pack("<7I", *record))
    path.write_bytes(payload)


def write_v2_capture(path: Path, records, *, target=0x20, epoch=0,
                     consumer=3, consumer_pc=0x20, register_mask=1,
                     ring_capacity=4096, ring_wrapped=False, overwrites=0,
                     join_status=True):
    values = [2, epoch, target, register_mask, records[0][1] if records else 0,
              records[-1][1] if records else 0, len(records), 1, 0, 0,
              consumer if consumer is not None else 0xFFFFFFFF,
              100 if consumer is not None else 0xFFFFFFFF, overwrites,
              ring_capacity, int(ring_wrapped), consumer_pc,
              int(join_status)]
    payload = bytearray(b"O67P")
    payload.extend(struct.pack("<17I", *values))
    for record in records:
        payload.extend(struct.pack("<7I", *record))
    path.write_bytes(payload)


def rom_with_producers():
    rom = bytearray(0x100)
    for pc, opcode in ((0x10, 0x49F9), (0x12, 0x49F9), (0x14, 0x4BF9)):
        rom[pc:pc + 2] = opcode.to_bytes(2, "big")
        rom[pc + 2:pc + 6] = (0x00FF0000).to_bytes(4, "big")
    rom[0x20:0x22] = bytes.fromhex("3955")
    return bytes(rom)


def rom_with_canary():
    rom = bytearray(0x100)
    rom[0x20:0x22] = bytes.fromhex("3955")
    return bytes(rom)


class Auto676PredecessorTest(unittest.TestCase):
    def run_case(self, records, **kwargs):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "pred.bin"
            write_capture(path, records, **kwargs)
            return resolve(decode(path), rom_with_producers(), ["A4"])

    def rec(self, seq, pc, opcode, a4=0x00FF0000, epoch=0):
        return (epoch, seq, 100, pc, opcode, a4, 0)

    def test_two_writes_latest_reaching_write_wins(self):
        result = self.run_case([self.rec(1, 0x10, 0x49F9),
                                self.rec(2, 0x12, 0x49F9),
                                self.rec(3, 0x20, 0x3955)])
        self.assertEqual(result["steps"][0]["producer_pc"], "0x000012")

    def test_write_after_consumer_cannot_connect(self):
        result = self.run_case([self.rec(1, 0x20, 0x3955),
                                self.rec(2, 0x10, 0x49F9)], consumer=1)
        self.assertEqual(result["steps"], [])

    def test_sequence_gap_fails_closed(self):
        result = self.run_case([self.rec(1, 0x10, 0x49F9),
                                self.rec(3, 0x20, 0x3955)])
        self.assertEqual(result["steps"], [])

    def test_truncated_predecessor_ring_fails_closed(self):
        result = self.run_case([self.rec(1, 0x10, 0x49F9),
                                self.rec(2, 0x20, 0x3955)], truncated=True,
                               consumer=2)
        self.assertEqual(result["steps"], [])

    def test_overwrite_outside_required_interval_is_allowed(self):
        result = self.run_case([self.rec(1, 0x10, 0x49F9),
                                self.rec(2, 0x20, 0x3955)], consumer=2,
                               overwrites=1)
        self.assertEqual(len(result["steps"]), 1)

    def test_overwrite_inside_required_interval_fails_closed(self):
        result = self.run_case([self.rec(1, 0x10, 0x49F9),
                                self.rec(3, 0x20, 0x3955)], consumer=3,
                               overwrites=1)
        self.assertEqual(result["steps"], [])

    def test_v2_exact_join_and_ring_metadata_decode(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "pred-v2.bin"
            records = [self.rec(10, 0x10, 0x49F9), self.rec(11, 0x20, 0x3955)]
            write_v2_capture(path, records, consumer=11, ring_wrapped=True,
                             overwrites=7)
            capture = decode(path)
            self.assertEqual(capture.format_version, 2)
            self.assertEqual(capture.join_status, "EXACT")
            self.assertEqual(capture.ring_capacity, 4096)
            result = resolve(capture, rom_with_producers(), ["A4"])
            self.assertEqual(len(result["steps"]), 1)
            self.assertTrue(result["steps"][0]["evidence"]["ring_wrapped"])

    def test_v2_missing_join_fails_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "pred-v2.bin"
            records = [self.rec(1, 0x10, 0x49F9), self.rec(2, 0x20, 0x3955)]
            write_v2_capture(path, records, consumer=2, join_status=False)
            result = resolve(decode(path), rom_with_producers(), ["A4"])
            self.assertEqual(result["steps"], [])
            self.assertEqual(result["reason"], "CONSUMER_JOIN_NOT_EXACT")

    def test_epoch_mismatch_fails_closed(self):
        result = self.run_case([self.rec(1, 0x10, 0x49F9, epoch=0),
                                self.rec(2, 0x20, 0x3955, epoch=1)], epoch=1,
                               consumer=2)
        self.assertEqual(result["steps"], [])

    def test_same_value_without_static_producer_is_not_causal(self):
        result = self.run_case([self.rec(1, 0x20, 0x3955)], consumer=1)
        self.assertEqual(result["steps"], [])

    def test_unsupported_register_writer_fails_closed(self):
        result = self.run_case([self.rec(1, 0x10, 0x4E54),
                                self.rec(2, 0x20, 0x3955)], consumer=2)
        self.assertEqual(result["steps"], [])

    def test_complete_supported_interval_emits_one_step(self):
        result = self.run_case([self.rec(1, 0x10, 0x49F9),
                                self.rec(2, 0x20, 0x3955)], consumer=2)
        self.assertEqual(len(result["steps"]), 1)
        self.assertTrue(result["steps"][0]["evidence"]["complete_interval"])

    def test_zero_bus_value_decodes_producer_from_runtime_pc(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "pred-v2.bin"
            records = [self.rec(1, 0x10, 0), self.rec(2, 0x20, 0)]
            write_v2_capture(path, records, consumer=2, consumer_pc=0x20)
            result = resolve(decode(path), rom_with_producers(), ["A4"])
        self.assertEqual(len(result["steps"]), 1)
        self.assertEqual(result["steps"][0]["producer_opcode"], "0x49F9")
        self.assertEqual(result["steps"][0]["producer_opcode_source"],
                         "STATIC_ROM_PC")

    def test_generic_intervening_pea_and_indexed_lea_are_not_false_blocks(self):
        rom = bytearray(0x100)
        rom[0x10:0x12] = bytes.fromhex("4DFA")
        rom[0x12:0x14] = bytes.fromhex("0004")
        rom[0x20:0x22] = bytes.fromhex("3955")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "pred.bin"
            records = [self.rec(1, 0x10, 0), self.rec(2, 0x20, 0)]
            write_capture(path, records, consumer=2)
            result = resolve(decode(path), bytes(rom), ["A4"])
            self.assertEqual(result["steps"], [])

    def test_same_pc_different_occurrence_uses_exact_sequence(self):
        result = self.run_case([self.rec(1, 0x20, 0x3955),
                                self.rec(2, 0x10, 0x49F9),
                                self.rec(3, 0x20, 0x3955)], consumer=3)
        self.assertEqual(result["steps"][0]["consumer_occurrence"]["sequence"], 3)

    def test_a5_is_supported_by_the_same_generic_resolver(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "pred.bin"
            records = [self.rec(1, 0x14, 0x4BF9), self.rec(2, 0x20, 0x3955)]
            write_capture(path, records, consumer=2, register_mask=2)
            capture = decode(path)
            result = resolve(capture, rom_with_producers(), ["A5"])
            self.assertEqual(result["steps"][0]["register"], "A5")

    def test_memory_source_register_is_requested_for_targeted_capture(self):
        seed = {"kind": "BUS_WRITE_PC", "pc": "0x0020", "address": "0xC00004"}
        self.assertEqual(required_registers(seed, rom_with_canary()), ["A5", "A4"])


if __name__ == "__main__":
    unittest.main()
