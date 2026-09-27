"""Bounded subprocess runner for map-driven Stage 7/8 tools."""

from __future__ import annotations

from collections import deque
from pathlib import Path
import subprocess
import threading
import time
from typing import Callable


DEFAULT_TIMEOUT_SECONDS = 120.0
STAGE7_TIMEOUT_CODE = "STOP_STAGE7_SUBPROCESS_TIMEOUT"
STAGE8_TIMEOUT_CODE = "STOP_STAGE8_SUBPROCESS_TIMEOUT"
_TAIL_LINES = 40


def _drain(stream, tail: deque[str], errors: list[BaseException]) -> None:
    try:
        for line in iter(stream.readline, ""):
            tail.append(line.rstrip("\r\n"))
    except BaseException as error:  # fail closed if a pipe reader breaks
        errors.append(error)
    finally:
        stream.close()


def _stop(process: subprocess.Popen[str]) -> None:
    if process.poll() is not None:
        return
    try:
        process.terminate()
    except ProcessLookupError:
        return
    try:
        process.wait(timeout=2.0)
    except subprocess.TimeoutExpired:
        try:
            process.kill()
        except ProcessLookupError:
            return
        process.wait(timeout=2.0)


def _wait_for_exit(process: subprocess.Popen[str], completed: threading.Event,
                   wake: threading.Event, errors: list[BaseException]) -> None:
    """Block on the OS process wait and publish completion exactly once."""
    try:
        process.wait()
    except BaseException as error:  # pragma: no cover - defensive/mock path
        errors.append(error)
    finally:
        completed.set()
        wake.set()


def _heartbeat_until_exit(completed: threading.Event, wake: threading.Event,
                          heartbeat: Callable[[], None],
                          errors: list[BaseException]) -> None:
    """Keep progress alive without polling the child process."""
    while not completed.wait(0.25):
        try:
            heartbeat()
        except BaseException as error:  # preserve main-thread callback failure
            errors.append(error)
            wake.set()
            return


def run(command: list[Path | str], *, cwd: Path | None = None,
        heartbeat: Callable[[], None] | None = None,
        timeout_seconds: float = DEFAULT_TIMEOUT_SECONDS,
        label: str = "stage7 subprocess",
        timeout_code: str = STAGE7_TIMEOUT_CODE) -> subprocess.CompletedProcess[str]:
    """Run a tool with live output tails and a factual fail-closed timeout."""
    process = subprocess.Popen([str(item) for item in command], cwd=cwd,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               text=True)
    stdout_tail: deque[str] = deque(maxlen=_TAIL_LINES)
    stderr_tail: deque[str] = deque(maxlen=_TAIL_LINES)
    reader_errors: list[BaseException] = []
    threads = [threading.Thread(target=_drain,
                                args=(process.stdout, stdout_tail, reader_errors),
                                daemon=True),
               threading.Thread(target=_drain,
                                args=(process.stderr, stderr_tail, reader_errors),
                                daemon=True)]
    for thread in threads:
        thread.start()
    completed = threading.Event()
    wake = threading.Event()
    wait_errors: list[BaseException] = []
    heartbeat_errors: list[BaseException] = []
    waiter = threading.Thread(target=_wait_for_exit,
                              args=(process, completed, wake, wait_errors),
                              name="stage7-process-wait", daemon=True)
    waiter.start()
    heartbeat_thread = None
    if heartbeat is not None:
        heartbeat_thread = threading.Thread(
            target=_heartbeat_until_exit,
            args=(completed, wake, heartbeat, heartbeat_errors),
            name="stage7-process-heartbeat", daemon=True)
        heartbeat_thread.start()
    started = time.monotonic()
    timed_out = False
    try:
        remaining = timeout_seconds - (time.monotonic() - started)
        if remaining <= 0 or not wake.wait(remaining):
            if process.poll() is None:
                _stop(process)
                timed_out = not wait_errors and not heartbeat_errors
            completed.wait(2.5)
        waiter.join(timeout=2.0)
        for thread in threads:
            thread.join(timeout=1.0)
        if timed_out:
            elapsed = time.monotonic() - started
            raise ValueError(
                f"{timeout_code}: "
                f"label={label};pid={process.pid};elapsed_seconds={elapsed:.3f};"
                f"stdout_tail={list(stdout_tail)!r};stderr_tail={list(stderr_tail)!r}")
        if wait_errors:
            raise wait_errors[0]
        if heartbeat_errors:
            raise heartbeat_errors[0]
        if reader_errors:
            raise reader_errors[0]
        return subprocess.CompletedProcess(command, process.returncode,
                                           "\n".join(stdout_tail),
                                           "\n".join(stderr_tail))
    finally:
        if process.poll() is None:
            _stop(process)
        completed.wait(2.5)
        waiter.join(timeout=2.0)
        for thread in threads:
            thread.join(timeout=1.0)
        if heartbeat_thread is not None:
            heartbeat_thread.join(timeout=1.0)


__all__ = ["DEFAULT_TIMEOUT_SECONDS", "STAGE7_TIMEOUT_CODE",
           "STAGE8_TIMEOUT_CODE", "run"]
