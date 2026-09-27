"""Focused safety and timing regressions for the Stage 7 subprocess runner."""

from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import threading
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src" / "tools" / "thor_evidence"))

from stage7_subprocess import _drain, _stop, run  # noqa: E402


def child(*statements: str) -> list[str]:
    return [sys.executable, "-c", ";".join(statements)]


class Stage7SubprocessTests(unittest.TestCase):
    def test_immediate_and_short_success_preserve_result(self) -> None:
        immediate = run(child("print('out')", "print('err', file=__import__('sys').stderr)"),
                         timeout_seconds=2.0)
        self.assertEqual(immediate.returncode, 0)
        self.assertEqual(immediate.stdout, "out")
        self.assertEqual(immediate.stderr, "err")
        started = time.monotonic()
        short = run(child("import time", "time.sleep(.05)", "print('done')"),
                    timeout_seconds=2.0)
        self.assertEqual(short.stdout, "done")
        self.assertLess(time.monotonic() - started, 0.8)

    def test_large_stdout_and_stderr_do_not_deadlock(self) -> None:
        result = run(child(
            "import sys",
            "sys.stdout.write('o'*200000)",
            "sys.stderr.write('e'*200000)"), timeout_seconds=5.0)
        self.assertEqual(result.returncode, 0)
        self.assertEqual(len(result.stdout), 200000)
        self.assertEqual(len(result.stderr), 200000)

    def test_output_is_drained_after_child_exit(self) -> None:
        result = run(child("import sys", "sys.stdout.write('tail\\n')",
                           "sys.stderr.write('error-tail\\n')"), timeout_seconds=2.0)
        self.assertEqual(result.stdout, "tail")
        self.assertEqual(result.stderr, "error-tail")

    def test_nonzero_and_crash_results_are_returned(self) -> None:
        nonzero = run(child("print('failed')", "__import__('sys').exit(7)"),
                      timeout_seconds=2.0)
        self.assertEqual(nonzero.returncode, 7)
        crashed = run(child("__import__('os')._exit(9)"), timeout_seconds=2.0)
        self.assertEqual(crashed.returncode, 9)

    def test_timeout_is_fail_closed_and_leaves_no_runner_threads(self) -> None:
        with self.assertRaisesRegex(ValueError, "STOP_STAGE7_SUBPROCESS_TIMEOUT"):
            run(child("import time", "time.sleep(2)"), timeout_seconds=.1,
                label="focused timeout")
        self.assertFalse(any(thread.name.startswith("stage7-process-")
                             for thread in threading.enumerate()))

    def test_heartbeat_runs_without_delaying_process_completion(self) -> None:
        beats: list[float] = []
        result = run(child("import time", "time.sleep(.6)", "print('done')"),
                     timeout_seconds=2.0, heartbeat=lambda: beats.append(time.monotonic()))
        self.assertEqual(result.stdout, "done")
        self.assertGreaterEqual(len(beats), 1)

    def test_heartbeat_failure_stops_child_and_propagates(self) -> None:
        def broken_heartbeat() -> None:
            raise RuntimeError("synthetic heartbeat failure")

        with self.assertRaisesRegex(RuntimeError, "synthetic heartbeat failure"):
            run(child("import time", "time.sleep(2)"), timeout_seconds=5.0,
                heartbeat=broken_heartbeat)
        self.assertFalse(any(thread.name.startswith("stage7-process-")
                             for thread in threading.enumerate()))

    def test_terminate_timeout_falls_back_to_kill(self) -> None:
        class Stubborn:
            pid = 123

            def __init__(self) -> None:
                self.returncode = None
                self.calls: list[str] = []

            def poll(self):
                return self.returncode

            def terminate(self):
                self.calls.append("terminate")

            def kill(self):
                self.calls.append("kill")
                self.returncode = -9

            def wait(self, timeout=None):
                self.calls.append("wait")
                if "kill" not in self.calls:
                    raise subprocess.TimeoutExpired("stub", timeout)
                return self.returncode

        process = Stubborn()
        _stop(process)  # type: ignore[arg-type]
        self.assertEqual(process.calls, ["terminate", "wait", "kill", "wait"])

    def test_reader_exception_is_captured_and_stream_is_closed(self) -> None:
        class BrokenStream:
            def __init__(self) -> None:
                self.closed_by_runner = False

            def readline(self):
                raise OSError("synthetic pipe failure")

            def close(self):
                self.closed_by_runner = True

        stream = BrokenStream()
        errors: list[BaseException] = []
        _drain(stream, [], errors)  # type: ignore[arg-type]
        self.assertTrue(stream.closed_by_runner)
        self.assertEqual(len(errors), 1)
        self.assertIsInstance(errors[0], OSError)


if __name__ == "__main__":
    unittest.main(verbosity=2)
