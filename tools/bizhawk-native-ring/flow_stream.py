"""Bounded in-memory FLOW_V1 handoff primitives.

The runtime producer owns validation and submits immutable segment chunks.  A
single consumer drains the bounded queue, so the producer never has to build a
second complete FLOW file or retain the whole run in Python memory.
"""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
from queue import Full, Queue
from threading import Event, Lock
from typing import Iterator, Any

from live_forward_scaling_audit import RECORD


@dataclass(frozen=True)
class FlowChunk:
    """One host-audited FLOW segment and its exact record bytes."""

    segment: dict[str, Any]
    rows: tuple[tuple[int, ...], ...]
    records_blob: bytes


@dataclass(frozen=True)
class FlowHandoffStats:
    mode: str
    segments: int
    records: int
    raw_bytes: int
    raw_sha256: str
    logical_sha256: str
    disk_reads: int
    disk_writes: int
    buffer_peak_chunks: int
    buffer_peak_bytes: int
    fallback: str


def _logical_item(segment: dict[str, Any], ordinal: int, blob: bytes) -> bytes:
    value = {"ordinal": ordinal, "segment": segment,
             "records_sha256": hashlib.sha256(blob).hexdigest(),
             "record_count": len(blob) // RECORD.size}
    return (json.dumps(value, sort_keys=True, separators=(",", ":")) + "\n").encode()


class BoundedFlowHandoff:
    """Single-producer/single-consumer bounded FLOW queue.

    A full queue blocks the producer, providing explicit backpressure.  Any
    producer or consumer exception closes the handoff and is re-raised by the
    other side instead of activating a disk fallback.
    """

    def __init__(self, max_chunks: int = 32):
        if max_chunks <= 0:
            raise ValueError("max_chunks must be positive")
        self._queue: Queue[FlowChunk | None] = Queue(maxsize=max_chunks)
        self._closed = Event()
        self._failure: BaseException | None = None
        self._lock = Lock()
        self._segments = 0
        self._records = 0
        self._raw_bytes = 0
        self._raw_hash = hashlib.sha256()
        self._logical_hash = hashlib.sha256()
        self._peak_chunks = 0
        self._queued_bytes = 0
        self._peak_bytes = 0

    def submit(self, segment: dict[str, Any], rows: list[tuple[int, ...]],
               records_blob: bytes) -> None:
        with self._lock:
            if self._closed.is_set():
                raise RuntimeError("FLOW handoff is closed")
            if segment.get("valid") is not True or segment.get("ready_for_cartographer") is not True:
                raise ValueError("FLOW handoff accepts only host-audited segments")
            expected = int(segment.get("record_count", -1))
            if expected != len(rows) or len(records_blob) != expected * RECORD.size:
                raise ValueError("FLOW handoff record count mismatch")
            if any(right[0] != left[0] + 1 for left, right in zip(rows, rows[1:])):
                raise ValueError("FLOW handoff record ordering mismatch")
            ordinal = self._segments
            chunk = FlowChunk(dict(segment), tuple(rows), bytes(records_blob))
            self._segments += 1
            self._records += expected
            self._raw_bytes += len(records_blob)
            self._raw_hash.update(records_blob)
            self._logical_hash.update(_logical_item(chunk.segment, ordinal, chunk.records_blob))
        while True:
            try:
                with self._lock:
                    self._queued_bytes += len(chunk.records_blob)
                    self._peak_bytes = max(self._peak_bytes, self._queued_bytes)
                self._queue.put(chunk, timeout=0.25)
                with self._lock:
                    self._peak_chunks = max(self._peak_chunks, self._queue.qsize())
                return
            except Full:
                with self._lock:
                    self._queued_bytes -= len(chunk.records_blob)
                self._raise_failure()

    def close(self) -> None:
        with self._lock:
            if self._closed.is_set():
                return
            self._closed.set()
        while True:
            try:
                self._queue.put(None, timeout=0.25)
                return
            except Full:
                self._raise_failure()

    def fail(self, error: BaseException) -> None:
        with self._lock:
            self._failure = error
            self._closed.set()
        while True:
            try:
                self._queue.put(None, timeout=0.25)
                return
            except Full:
                try:
                    self._queue.get_nowait()
                except Exception:
                    pass

    def _raise_failure(self) -> None:
        with self._lock:
            if self._failure is not None:
                raise RuntimeError("FLOW handoff consumer failed") from self._failure

    def consume(self) -> Iterator[FlowChunk]:
        while True:
            item = self._queue.get()
            if item is None:
                self._raise_failure()
                return
            with self._lock:
                self._queued_bytes -= len(item.records_blob)
            self._raise_failure()
            yield item

    def stats(self) -> FlowHandoffStats:
        with self._lock:
            return FlowHandoffStats(
                mode="IN_MEMORY_STREAM", segments=self._segments,
                records=self._records, raw_bytes=self._raw_bytes,
                raw_sha256=self._raw_hash.hexdigest(),
                logical_sha256=self._logical_hash.hexdigest(), disk_reads=0,
                disk_writes=0, buffer_peak_chunks=self._peak_chunks,
                buffer_peak_bytes=self._peak_bytes,
                fallback="DISABLED")


__all__ = ["BoundedFlowHandoff", "FlowChunk", "FlowHandoffStats"]
