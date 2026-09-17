import hashlib
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from argparse import Namespace


sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools" / "bizhawk-native-ring"))
from live_forward_scaling_audit import overlap_peak, validate_segment
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
            rows = [struct.pack("<QQIIHHI", index + 1, index + 1,
                0x100 + index * 8, 0x108 + index * 8, 0x6602, 27, 0)
                for index in range(20)]
            path.write_bytes(b"".join(rows))
            meta = [1000001, 1, 1, 3, 1, 21, 1, 21, 0, 20, 0, 1,
                20, 65536, 20, 928, 20, 640, 1, 0]
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

            mutated = Path(directory) / "records-reread.bin"
            changed_rows = list(rows)
            changed_rows[-1] = struct.pack("<QQIIHHI", 20, 20,
                0x198, 0x1a2, 0x6602, 27, 0)
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


if __name__ == "__main__":
    unittest.main()
