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

    def test_overwritten_predecessor_ring_fails_closed(self):
        result = self.run_case([self.rec(1, 0x10, 0x49F9),
                                self.rec(2, 0x20, 0x3955)], consumer=2,
                               overwrites=1)
        self.assertEqual(result["steps"], [])
        self.assertEqual(result["reason"], "PREDECESSOR_RING_OVERWRITE")

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
