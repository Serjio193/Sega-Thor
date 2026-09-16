"""Occurrence-scoped adapter from real GPGX ring snapshots to AUTO67 resolver input."""

from __future__ import annotations

import base64
import hashlib
import json
import struct
import time
import zlib
from collections import deque
from dataclasses import dataclass
from typing import Any

from auto67_live import Dispatcher
from auto67_predecessor import PredecessorCapture, PredecessorRecord
from auto67_snapshot_admission import (
    FrozenSnapshot,
    NativeTraceRecord,
    SnapshotPool,
)


NATIVE_RING_CAPACITY = 4096
NATIVE_RECORD = struct.Struct("<QIHH")


@dataclass(frozen=True)
class DecodedNativeSnapshot:
    snapshot_identity: str
    occurrence_identity: tuple[int, int]
    snapshot_epoch: int
    first_sequence: int
    latest_sequence: int
    count: int
    consumer_sequence: int
    consumer_pc: int
    consumer_registers: tuple[tuple[str, int], ...]
    records: tuple[NativeTraceRecord, ...]
    record_sha256: str
    read_duration_ns: int
    copy_duration_ns: int
    compression_duration_ns: int
    freeze_duration_ns: int


def _integer(value: Any, field: str) -> int:
    try:
        return int(str(value), 0) if isinstance(value, str) else int(value)
    except (TypeError, ValueError) as error:
        raise ValueError(f"invalid native snapshot {field}") from error


def _event_identity(event: dict[str, Any]) -> tuple[int, int, str]:
    epoch = _integer(event.get("epoch", 1), "event epoch")
    sequence = _integer(event.get("seq"), "event sequence")
    expected = f"epoch={epoch}:seq={sequence}"
    if event.get("occurrence_id", expected) != expected:
        raise ValueError("event occurrence identity mismatch")
    return epoch, sequence, expected


def decode_native_snapshot(event: dict[str, Any], rom: bytes) -> DecodedNativeSnapshot:
    """Validate one callback-frozen ring image without repairing its identity."""
    epoch, occurrence_sequence, occurrence_id = _event_identity(event)
    source = event.get("native_snapshot")
    if not isinstance(source, dict):
        raise ValueError("native snapshot payload missing")
    if source.get("schema") != "oasis.auto67.native-snapshot.v1":
        raise ValueError("native snapshot schema mismatch")
    if source.get("occurrence_id") != occurrence_id:
        raise ValueError("native snapshot belongs to another occurrence")

    snapshot_epoch = _integer(source.get("snapshot_epoch"), "epoch")
    first = _integer(source.get("first_sequence"), "first sequence")
    latest = _integer(source.get("latest_sequence"), "latest sequence")
    count = _integer(source.get("count"), "count")
    consumer_sequence = _integer(source.get("consumer_sequence"), "consumer sequence")
    if snapshot_epoch != epoch:
        raise ValueError("native snapshot epoch mismatch")
    if not 1 <= count <= NATIVE_RING_CAPACITY:
        raise ValueError("native snapshot count out of range")
    if count != min(latest, NATIVE_RING_CAPACITY):
        raise ValueError("native snapshot retained-count rule mismatch")
    if first != latest - count + 1 or consumer_sequence != latest:
        raise ValueError("native snapshot sequence bounds mismatch")
    expected_identity = f"NR-{epoch:04d}-{occurrence_sequence:010d}-{latest:016d}"
    if source.get("snapshot_identity") != expected_identity:
        raise ValueError("native snapshot identity mismatch")
    if _integer(source.get("record_bytes"), "record size") != NATIVE_RECORD.size:
        raise ValueError("native snapshot record size mismatch")
    if source.get("records_codec") != "deflate-base64":
        raise ValueError("native snapshot codec mismatch")

    encoded = source.get("records_b64")
    if not isinstance(encoded, str):
        raise ValueError("native snapshot records missing")
    try:
        compressed = base64.b64decode(encoded, validate=True)
        raw = zlib.decompress(compressed, -zlib.MAX_WBITS)
    except (ValueError, zlib.error) as error:
        raise ValueError("native snapshot records cannot be decoded") from error
    if len(raw) != count * NATIVE_RECORD.size:
        raise ValueError("native snapshot byte count mismatch")

    records: list[NativeTraceRecord] = []
    for offset in range(0, len(raw), NATIVE_RECORD.size):
        sequence, pc, opcode, reserved = NATIVE_RECORD.unpack_from(raw, offset)
        expected_sequence = first + len(records)
        if sequence != expected_sequence:
            raise ValueError("native snapshot sequence is not contiguous")
        if pc > 0xFFFFFF or pc & 1:
            raise ValueError("native snapshot contains invalid M68K PC")
        if reserved != 0:
            raise ValueError("native snapshot reserved field is nonzero")
        if pc + 2 <= len(rom) and opcode != int.from_bytes(rom[pc:pc + 2], "big"):
            raise ValueError(f"native opcode differs from ROM at PC 0x{pc:06X}")
        records.append(NativeTraceRecord(sequence, pc, opcode, reserved))
    if records[-1].sequence != latest:
        raise ValueError("native snapshot final sequence mismatch")

    consumer_pc = _integer(event.get("pc"), "consumer PC")
    if consumer_pc > 0xFFFFFF or consumer_pc & 1:
        raise ValueError("native snapshot consumer PC is invalid")
    register_values = event.get("native_snapshot_registers")
    if register_values is None:
        register_values = {}
    if not isinstance(register_values, dict):
        raise ValueError("native snapshot consumer registers are malformed")
    registers = tuple(sorted(
        (name, _integer(value, f"consumer {name}"))
        for name, value in register_values.items()
        if name in {"A4", "A5"} and value is not None))
    durations = tuple(_integer(source.get(name, 0), name) for name in (
        "read_duration_ns", "copy_duration_ns", "compression_duration_ns",
        "freeze_duration_ns"))
    if any(value < 0 for value in durations):
        raise ValueError("native snapshot duration is negative")
    return DecodedNativeSnapshot(
        expected_identity, (epoch, occurrence_sequence), snapshot_epoch,
        first, latest, count, consumer_sequence, consumer_pc, registers,
        tuple(records), hashlib.sha256(raw).hexdigest(), *durations)


def freeze_native_event(event: dict[str, Any], pool: SnapshotPool,
                        rom: bytes) -> FrozenSnapshot | None:
    """Copy a callback-frozen record tuple into the existing bounded 16 slots."""
    item = dict(event)
    item["native_snapshot_mode"] = True
    def publish() -> None:
        event.clear()
        event.update(item)

    if not isinstance(item.get("native_snapshot"), dict):
        pool.note_missing_native_snapshot()
        item["native_snapshot_status"] = "MISSING"
        item.pop("native_snapshot", None)
        publish()
        return None
    try:
        decoded = decode_native_snapshot(item, rom)
        frozen = pool.freeze(
            item, decoded.snapshot_epoch, decoded.first_sequence,
            decoded.latest_sequence, decoded.count,
            snapshot_identity=decoded.snapshot_identity,
            native_records=decoded.records,
            consumer_sequence=decoded.consumer_sequence,
            consumer_pc=decoded.consumer_pc,
            consumer_registers=decoded.consumer_registers,
            read_duration_ns=decoded.read_duration_ns,
            copy_duration_ns=decoded.copy_duration_ns,
            compression_duration_ns=decoded.compression_duration_ns,
            freeze_duration_ns=decoded.freeze_duration_ns)
    except ValueError as error:
        pool.note_invalid_native_snapshot()
        item["native_snapshot_status"] = "INVALID"
        item["native_snapshot_error"] = str(error)
        item.pop("native_snapshot", None)
        publish()
        return None
    item.pop("native_snapshot", None)
    if frozen is None:
        item["native_snapshot_status"] = "SNAPSHOT_POOL_FULL"
        publish()
        return None
    item["native_snapshot_status"] = "FROZEN"
    item["snapshot_identity"] = frozen.snapshot_identity
    item["snapshot_epoch"] = frozen.snapshot_epoch
    item["first_sequence"] = frozen.first_sequence
    item["latest_sequence"] = frozen.latest_sequence
    item["count"] = frozen.count
    item["native_records_sha256"] = decoded.record_sha256
    item["native_snapshot_read_duration_ns"] = frozen.read_duration_ns
    item["native_snapshot_copy_duration_ns"] = frozen.copy_duration_ns
    item["native_snapshot_freeze_duration_ns"] = frozen.freeze_duration_ns
    publish()
    return frozen


def to_predecessor_capture(snapshot: FrozenSnapshot, event: dict[str, Any],
                           requested_registers: tuple[str, ...]) -> PredecessorCapture:
    """Adapt only the exact leased immutable snapshot to the existing resolver schema."""
    epoch, sequence, occurrence_id = _event_identity(event)
    if snapshot.occurrence_identity != (epoch, sequence):
        raise ValueError("leased occurrence does not own native snapshot")
    if event.get("occurrence_id") != occurrence_id:
        raise ValueError("leased occurrence identity changed")
    if event.get("snapshot_identity") != snapshot.snapshot_identity:
        raise ValueError("leased snapshot identity changed")
    if snapshot.snapshot_epoch != epoch or event.get("snapshot_epoch") != epoch:
        raise ValueError("leased snapshot epoch mismatch")
    for name, value in (("first_sequence", snapshot.first_sequence),
                         ("latest_sequence", snapshot.latest_sequence),
                         ("count", snapshot.count)):
        if _integer(event.get(name), name) != value:
            raise ValueError(f"leased snapshot {name} mismatch")
    if (len(snapshot.native_records) != snapshot.count or
            snapshot.first_sequence != snapshot.latest_sequence - snapshot.count + 1):
        raise ValueError("frozen native sequence range is inconsistent")
    expected = snapshot.first_sequence
    for record in snapshot.native_records:
        if record.sequence != expected:
            raise ValueError("frozen native records changed after freeze")
        expected += 1
    if snapshot.consumer_sequence != snapshot.latest_sequence:
        raise ValueError("frozen native consumer sequence mismatch")

    registers = dict(snapshot.consumer_registers)
    records = tuple(PredecessorRecord(
        snapshot.snapshot_epoch, item.sequence,
        _integer(event.get("frame", 0), "event frame") if item.sequence == snapshot.consumer_sequence else None,
        item.pc, item.opcode,
        registers if item.sequence == snapshot.consumer_sequence else {})
        for item in snapshot.native_records)
    consumer_pc = snapshot.consumer_pc
    join_status = ("EXACT" if records[-1].sequence == snapshot.consumer_sequence
                   and records[-1].pc == consumer_pc else "MISSING")
    return PredecessorCapture(
        path=f"native-snapshot:{snapshot.snapshot_identity}",
        epoch=snapshot.snapshot_epoch, target_pc=consumer_pc,
        requested_registers=tuple(requested_registers),
        first_sequence=snapshot.first_sequence,
        last_sequence=snapshot.latest_sequence, complete=True,
        truncated=False, gap=False, consumer_sequence=snapshot.consumer_sequence,
        consumer_frame=_integer(event.get("frame", 0), "event frame"),
        overwrites=max(0, snapshot.latest_sequence - snapshot.count),
        records=records, format_version=2, ring_capacity=NATIVE_RING_CAPACITY,
        ring_wrapped=snapshot.latest_sequence > NATIVE_RING_CAPACITY,
        consumer_pc=consumer_pc, join_status=join_status)


class NativeSnapshotAdmission:
    """Freeze native records before ordinary Dispatcher ingestion; release on Worker IDLE."""

    def __init__(self, dispatcher: Dispatcher, pool: SnapshotPool, rom: bytes):
        self.dispatcher = dispatcher
        self.pool = pool
        self.rom = rom
        self.by_occurrence: dict[tuple[int, int], str] = {}
        self.dispatched: set[tuple[int, int]] = set()
        self.seen_transitions: set[tuple[str, str, str]] = set()
        self.transition_order: deque[tuple[str, str, str]] = deque(maxlen=4096)
        self.latest_native_sequence = 0
        self.native_ring_progress = 0
        self.worker_snapshot_receipts: deque[dict[str, Any]] = deque(maxlen=64)
        self.freeze_receipts: deque[dict[str, Any]] = deque(maxlen=64)

    def ingest(self, event: dict[str, Any]) -> None:
        item = dict(event)
        frozen = freeze_native_event(item, self.pool, self.rom)
        identity = SnapshotPool._identity(item)
        if frozen is not None:
            self.by_occurrence[identity] = frozen.snapshot_identity
            record_hash = hashlib.sha256(b"".join(
                NATIVE_RECORD.pack(record.sequence, record.pc,
                                   record.opcode, record.reserved)
                for record in frozen.native_records)).hexdigest()
            for receipt in self.freeze_receipts:
                if frozen.latest_sequence > receipt["latest_sequence"]:
                    receipt["live_ring_advanced_after_freeze"] = True
            self.freeze_receipts.append({
                "occurrence_id": item["occurrence_id"],
                "snapshot_identity": frozen.snapshot_identity,
                "first_sequence": frozen.first_sequence,
                "latest_sequence": frozen.latest_sequence,
                "count": frozen.count,
                "records_sha256_at_freeze": record_hash,
                "live_ring_advanced_after_freeze": False,
                "frozen_snapshot_read_identical_at_worker_start": None,
                "slot_released_after_worker_completion": False,
            })
            if frozen.latest_sequence > self.latest_native_sequence:
                self.native_ring_progress += int(self.latest_native_sequence > 0)
                self.latest_native_sequence = frozen.latest_sequence
        self.dispatcher.ingest(item)
        self.reconcile()

    def reconcile(self) -> None:
        state = self.dispatcher.snapshot()
        idle_occurrences: set[tuple[int, int]] = set()
        for transition in state["transition_history"]:
            key = (transition["state"], transition["occurrence_id"],
                   transition["investigation_id"] or "")
            if key in self.seen_transitions:
                continue
            if len(self.transition_order) == self.transition_order.maxlen:
                self.seen_transitions.discard(self.transition_order[0])
            self.transition_order.append(key)
            self.seen_transitions.add(key)
            identity = _parse_identity(transition["occurrence_id"])
            if identity is None or identity not in self.by_occurrence:
                continue
            snapshot_identity = self.by_occurrence[identity]
            snapshot = self.pool.get(snapshot_identity, identity)
            if transition["state"] == "LEASED" and identity not in self.dispatched:
                self.pool.mark_dispatched(snapshot.snapshot_identity)
                self.dispatched.add(identity)
            elif transition["state"] == "WORKING":
                repeated = self.pool.get(snapshot_identity, identity)
                record_hash = hashlib.sha256(b"".join(
                    NATIVE_RECORD.pack(record.sequence, record.pc,
                                       record.opcode, record.reserved)
                    for record in repeated.native_records)).hexdigest()
                receipt = next((item for item in self.freeze_receipts
                                if item["snapshot_identity"] == snapshot_identity), None)
                unchanged = bool(receipt and
                                 record_hash == receipt["records_sha256_at_freeze"])
                if repeated != snapshot or not unchanged:
                    raise ValueError("frozen native snapshot changed before Worker start")
                if receipt is not None:
                    receipt["frozen_snapshot_read_identical_at_worker_start"] = True
                self.worker_snapshot_receipts.append({
                    "worker_id": transition["worker_id"],
                    "occurrence_id": transition["occurrence_id"],
                    "snapshot_identity": snapshot.snapshot_identity,
                    "first_sequence": snapshot.first_sequence,
                    "latest_sequence": snapshot.latest_sequence,
                    "count": snapshot.count,
                    "records_sha256": hashlib.sha256(b"".join(
                        NATIVE_RECORD.pack(record.sequence, record.pc,
                                           record.opcode, record.reserved)
                        for record in snapshot.native_records)).hexdigest(),
                    "frozen_read_identical_at_worker_start": True,
                    "live_ring_advanced_after_freeze": bool(
                        receipt and receipt["live_ring_advanced_after_freeze"]),
                })
            elif transition["state"] == "IDLE":
                idle_occurrences.add(identity)
        for investigation in state["investigations"]:
            identity = _parse_identity(investigation.get("occurrence_id"))
            if identity is None or identity not in idle_occurrences:
                continue
            snapshot_identity = self.by_occurrence.pop(identity, None)
            if snapshot_identity is None:
                continue
            snapshot = self.pool.get(snapshot_identity, identity)
            self.pool.release(snapshot.snapshot_identity, identity)
            receipt = next((item for item in self.freeze_receipts
                            if item["snapshot_identity"] == snapshot_identity), None)
            if receipt is not None:
                receipt["slot_released_after_worker_completion"] = True
            self.dispatched.discard(identity)

    def snapshot(self) -> dict[str, Any]:
        self.reconcile()
        return {**self.pool.snapshot(),
                "native_ring_latest_sequence_seen": self.latest_native_sequence,
                "native_ring_advanced_after_freeze_count": self.native_ring_progress,
                "active_occurrence_slots": len(self.by_occurrence),
                "freeze_receipts": list(self.freeze_receipts),
                "worker_snapshot_receipts": list(self.worker_snapshot_receipts)}


def _parse_identity(value: Any) -> tuple[int, int] | None:
    if not isinstance(value, str) or not value.startswith("epoch="):
        return None
    try:
        epoch_part, sequence_part = value.split(":", 1)
        return int(epoch_part.split("=", 1)[1]), int(sequence_part.split("=", 1)[1])
    except (ValueError, IndexError):
        return None
