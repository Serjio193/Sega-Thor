import hashlib
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from argparse import Namespace


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools" / "bizhawk-native-ring"))
from live_forward_scaling_audit import (EVENT_BANK_REGISTER_CHANGE,
    EVENT_BUS_READ, EVENT_BUS_WRITE,
    EVENT_FRAME_BOUNDARY, EVENT_SUBTYPE_SHIFT, FLAG_EVENT, overlap_peak,
    validate_segment)
from live_forward_scaling_runtime import TailLines, resolve_runtime_paths


class ScalingAuditTests(unittest.TestCase):
    def test_final_log_drain_captures_result_written_at_process_exit(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "runtime.txt"
            path.write_bytes(b"PLAN=1,2\r\n")
            tail = TailLines(path)
            self.assertEqual(tail.poll(), {"PLAN": "1,2"})
            with path.open("ab") as target:
                target.write(b"SEG_000001_000000=segment\r\nRESULT=PASS\r")

            final = tail.poll(final=True)

            self.assertEqual(final["SEG_000001_000000"], "segment")
            self.assertEqual(final["RESULT"], "PASS")
            self.assertEqual(tail.values["RESULT"], "PASS")

    def test_runtime_paths_are_absolute_before_install_cwd_launch(self):
        args = Namespace(install=Path("install"), rom=Path("game.bin"),
            script=Path("worker.lua"), output_dir=Path("evidence"))
        resolve_runtime_paths(args)
        self.assertTrue(all(getattr(args, name).is_absolute()
            for name in ("install", "rom", "script", "output_dir")))

    def test_valid_segment_and_fresh_generation_gate(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "records.bin"
            # <QQQIIIHBBHHI format: stream, instr, mtime, pc, addr, val, flags, cpu, len, domain, res, aux
            rows = [struct.pack("<QQQIIIHBBHHI", index + 1, index + 1, 0,
                0x100 + index * 8, 0x108 + index * 8, 0x6602, 27, 0, 0, 0, 0, 0)
                for index in range(20)]
            path.write_bytes(b"".join(rows))
            meta = [1000001, 1, 1, 3, 1, 21, 1, 21, 0, 20, 0, 1,
                20, 65536, 20, 1248, 20, 960, 1, 0] # 288 + 960 = 1248
            entry, exit_state = [0] * 20, [0] * 20
            entry[16], exit_state[16] = 0x100, 0x1a0
            meta_text = ",".join(map(str, meta))
            entry_text, exit_text = ",".join(map(str, entry)), ",".join(map(str, exit_state))
            wire = f"1|{meta_text}|{entry_text}|{exit_text}|0|{meta_text}|{entry_text}|{exit_text}|0|0.25|0.30|100|101"
            segment = validate_segment("SEG_000001_000000", wire, path, 1, 20,
                65536, 288, 1, [0], [0], [0], path)
            self.assertEqual(segment["generation"], 1)
            self.assertEqual(segment["records_sha256"], hashlib.sha256(path.read_bytes()).hexdigest())
            self.assertEqual(segment["valid"], True)
            self.assertEqual(segment["entry_registers"]["PC"], 0x100)
            self.assertEqual(segment["exit_registers"]["PC"], 0x1A0)
            self.assertIn("D0", segment["entry_registers"])
            self.assertIn("A7", segment["exit_registers"])

            mutated = Path(directory) / "records-reread.bin"
            changed_rows = list(rows)
            changed_rows[-1] = struct.pack("<QQQIIIHBBHHI", 20, 20, 0,
                0x198, 0x1a2, 0x6602, 27, 0, 0, 0, 0, 0)
            mutated.write_bytes(b"".join(changed_rows))
            with self.assertRaisesRegex(ValueError, "binary immutable result changed"):
                validate_segment("SEG_MUTATED", wire, path, 1, 20,
                    65536, 288, 1, [0], [0], [0], mutated)

            meta[1] = 2
            bad_meta_text = ",".join(map(str, meta))
            bad_wire = wire.replace(meta_text, bad_meta_text)
            with self.assertRaisesRegex(ValueError, "capture_id/generation"):
                validate_segment("SEG_BAD", bad_wire, path, 1, 20, 65536,
                    288, 1, [0], [0], [0], path)

    def test_measures_overlapping_stream_windows(self):
        windows = [
            {"entry_stream_sequence": 1, "exit_stream_sequence": 11},
            {"entry_stream_sequence": 3, "exit_stream_sequence": 13},
            {"entry_stream_sequence": 11, "exit_stream_sequence": 20},
        ]
        self.assertEqual(overlap_peak(windows), 2)

    def test_sideband_records_are_retained_but_do_not_change_flow_projection(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "records.bin"
            event = lambda subtype: FLAG_EVENT | (subtype << EVENT_SUBTYPE_SHIFT)
            rows = [struct.pack("<QQQIIIHBBHHI", 1, 1, 0, 0x100, 0x00F00010,
                0xCDEF, event(EVENT_BUS_READ), 0, 0, 0, 0, 0x89AB | (3 << 16)),
                struct.pack("<QQQIIIHBBHHI", 2, 1, 0, 0x100, 0x108, 0x6602, 27, 0, 0, 0, 0, 0)]
            for sequence in range(2, 20):
                stream = sequence + 1
                pc = 0x100 + sequence * 8
                rows.append(struct.pack("<QQQIIIHBBHHI", stream, sequence, 0,
                    pc, pc + 8, 0x6602, 27, 0, 0, 0, 0, 0))
            rows.extend([
                struct.pack("<QQQIIIHBBHHI", 21, 20, 0, 0x198, 0x00FF0010,
                    0x005A, event(EVENT_BUS_WRITE), 0, 0, 0, 0, 1 << 16),
                struct.pack("<QQQIIIHBBHHI", 22, 20, 0, 0x198, 0x1A0, 0x6602, 27, 0, 0, 0, 0, 0),
                struct.pack("<QQQIIIHBBHHI", 23, 1, 0, 0x200, 0x201, 0x07, 3, 1, 0, 0, 0, 0),
                struct.pack("<QQQIIIHBBHHI", 24, 1, 0, 0x200, 0, 0,
                    event(EVENT_BANK_REGISTER_CHANGE), 1, 0, 0, 0, 1 << 16),
                struct.pack("<QQQIIIHBBHHI", 25, 0, 0, 1, 0, 0,
                    event(EVENT_FRAME_BOUNDARY), 0, 0, 0, 0, 0)])
            path.write_bytes(b"".join(rows))
            meta = [1000001, 1, 1, 3, 1, 26, 1, 21, 0, 20, 0, 1,
                20, 65536, 20, 1488, 25, 1200, 1, 0] # 288 + 1200 = 1488
            entry, exit_state = [0] * 20, [0] * 20
            entry[16], exit_state[16] = 0x100, 0x1A0
            meta_text = ",".join(map(str, meta))
            entry_text = ",".join(map(str, entry))
            exit_text = ",".join(map(str, exit_state))
            wire = f"1|{meta_text}|{entry_text}|{exit_text}|0|{meta_text}|{entry_text}|{exit_text}|0|0.25|0.30|100|101"
            segment = validate_segment("SEG_SIDEBAND", wire, path, 1, 20,
                65536, 288, 1, [0], [0], [0], path)
            self.assertEqual(segment["bus_read_count"], 1)
            self.assertEqual(segment["bus_write_count"], 1)
            self.assertEqual(segment["bank_register_change_count"], 1)
            self.assertEqual(segment["frame_boundary_count"], 1)

            bad_rows = list(rows)
            bank = list(struct.unpack("<QQQIIIHBBHHI", bad_rows[23]))
            bank[7] = 0
            bad_rows[23] = struct.pack("<QQQIIIHBBHHI", *bank)
            bad_path = Path(directory) / "bad-bank-identity.bin"
            bad_path.write_bytes(b"".join(bad_rows))
            with self.assertRaisesRegex(ValueError,
                    "bank-register change lacks preceding Z80 instruction identity"):
                validate_segment("SEG_BAD_BANK_IDENTITY", wire, bad_path, 1, 20,
                    65536, 288, 1, [0], [0], [0], bad_path)

            bad_frame_rows = list(rows)
            boundary = list(struct.unpack("<QQQIIIHBBHHI", bad_frame_rows[24]))
            boundary[1] = 1
            bad_frame_rows[24] = struct.pack("<QQQIIIHBBHHI", *boundary)
            bad_frame_path = Path(directory) / "bad-frame-identity.bin"
            bad_frame_path.write_bytes(b"".join(bad_frame_rows))
            with self.assertRaisesRegex(ValueError,
                    "frame boundary has instruction identity"):
                validate_segment("SEG_BAD_FRAME_IDENTITY", wire, bad_frame_path,
                    1, 20, 65536, 288, 1, [0], [0], [0], bad_frame_path)

    def test_multi_round_host_export_preservation_policy(self):
        """Regression test for host exporter bug: round N+1 must not silently erase round N."""
        with tempfile.TemporaryDirectory() as directory:
            out_file = Path(directory) / "records.bin"

            def simulate_export(path: Path, round_idx: int, worker_idx: int, data: bytes, policy: str):
                if policy == "BUGGY":
                    append = (worker_idx != 0)
                elif policy == "FIXED":
                    append = (round_idx > 1) or (worker_idx != 0)
                else:
                    raise ValueError(policy)
                mode = "ab" if append else "wb"
                with path.open(mode) as f:
                    f.write(data)

            # 1. Under BUGGY policy: Round 1 writes data, Round 2 Worker 0 truncates Round 1
            simulate_export(out_file, round_idx=1, worker_idx=0, data=b"ROUND1_W0_", policy="BUGGY")
            simulate_export(out_file, round_idx=1, worker_idx=1, data=b"ROUND1_W1_", policy="BUGGY")
            self.assertEqual(out_file.read_bytes(), b"ROUND1_W0_ROUND1_W1_")

            # Round 2 Worker 0 executes with worker ~= 0 -> False -> truncates
            simulate_export(out_file, round_idx=2, worker_idx=0, data=b"ROUND2_W0_", policy="BUGGY")
            self.assertEqual(out_file.read_bytes(), b"ROUND2_W0_")  # Round 1 was silently erased

            # 2. Under FIXED policy: Round 1 creates, Round 2 Worker 0 appends ((round > 1) is True)
            out_file.unlink()
            simulate_export(out_file, round_idx=1, worker_idx=0, data=b"ROUND1_W0_", policy="FIXED")
            simulate_export(out_file, round_idx=1, worker_idx=1, data=b"ROUND1_W1_", policy="FIXED")
            self.assertEqual(out_file.read_bytes(), b"ROUND1_W0_ROUND1_W1_")

            # Round 2 appends and preserves Round 1
            simulate_export(out_file, round_idx=2, worker_idx=0, data=b"ROUND2_W0_", policy="FIXED")
            simulate_export(out_file, round_idx=2, worker_idx=1, data=b"ROUND2_W1_", policy="FIXED")
            self.assertEqual(out_file.read_bytes(), b"ROUND1_W0_ROUND1_W1_ROUND2_W0_ROUND2_W1_")


if __name__ == "__main__":
    unittest.main()
