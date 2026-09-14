"""Versioned, bounded decoder for AUTO67 frozen capsule files."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re
import struct


LEGACY_MAGIC = b"O67C"
V2_MAGIC = b"O67V"
V2_VERSION = 2
LEGACY_HEADER_BYTES = 20
V2_HEADER_BYTES = 24
LOGICAL_HEADER_BYTES = 64
LEGACY_RECORD_BYTES = 16
V2_RECORD_BYTES = 20
MAX_CAPSULE_RECORDS = 8192
KIND_NAMES = {1: "BUS_WRITE_PC", 2: "BUS_EXEC_PC"}
CAPSULE_NAME = re.compile(r"^capsule-(\d{2})-([A-Za-z0-9_-]+)\.bin$")


class CapsuleFormatError(ValueError):
    """Capsule evidence is absent, stale, malformed, or unsupported."""


@dataclass(frozen=True)
class DecodedObservation:
    sequence: int
    frame: int
    address: int
    pc: int
    kind: str | None
    kind_code: int | None


@dataclass(frozen=True)
class DecodedCapsule:
    path: str
    format_version: int
    capsule_id: int
    lease_id: str | None
    start_frame: int
    logical_bytes_used: int
    physical_bytes: int
    event_count: int
    observations: tuple[DecodedObservation, ...]


def _validate_name(path: Path, capsule_id: int, expected_lease_id: str | None) -> str | None:
    match = CAPSULE_NAME.match(path.name)
    if match is None:
        raise CapsuleFormatError("capsule filename is not versioned/lease-addressed")
    if int(match.group(1)) != capsule_id:
        raise CapsuleFormatError("capsule id does not match filename")
    lease_id = match.group(2)
    if expected_lease_id is not None and lease_id != expected_lease_id:
        raise CapsuleFormatError("stale capsule lease")
    return lease_id


def _validate_count(data: bytes, header_bytes: int, record_bytes: int, count: int) -> None:
    if count < 0 or count > MAX_CAPSULE_RECORDS:
        raise CapsuleFormatError("capsule record count is out of bounds")
    expected = header_bytes + count * record_bytes
    if len(data) != expected:
        raise CapsuleFormatError(
            f"capsule physical length mismatch: expected {expected}, got {len(data)}")


def _decode_legacy(data: bytes, path: Path, expected_capsule_id: int | None,
                   expected_lease_id: str | None) -> DecodedCapsule:
    if len(data) < LEGACY_HEADER_BYTES:
        raise CapsuleFormatError("legacy capsule header is truncated")
    capsule_id, start_frame, logical_bytes, count = struct.unpack_from("<IIII", data, 4)
    if expected_capsule_id is not None and capsule_id != expected_capsule_id:
        raise CapsuleFormatError("legacy capsule id mismatch")
    lease_id = _validate_name(path, capsule_id, expected_lease_id)
    _validate_count(data, LEGACY_HEADER_BYTES, LEGACY_RECORD_BYTES, count)
    expected_logical = LOGICAL_HEADER_BYTES + count * LEGACY_RECORD_BYTES
    if logical_bytes != expected_logical:
        raise CapsuleFormatError("legacy logical bytes_used mismatch")
    records = tuple(
        DecodedObservation(*struct.unpack_from("<IIII", data, 20 + index * 16), None, None)
        for index in range(count))
    return DecodedCapsule(str(path), 1, capsule_id, lease_id, start_frame,
                          logical_bytes, len(data), count, records)


def _decode_v2(data: bytes, path: Path, expected_capsule_id: int | None,
               expected_lease_id: str | None) -> DecodedCapsule:
    if len(data) < V2_HEADER_BYTES:
        raise CapsuleFormatError("O67V header is truncated")
    version, capsule_id, start_frame, logical_bytes, count = struct.unpack_from(
        "<IIIII", data, 4)
    if version != V2_VERSION:
        raise CapsuleFormatError(f"unsupported capsule version {version}")
    if expected_capsule_id is not None and capsule_id != expected_capsule_id:
        raise CapsuleFormatError("capsule id mismatch")
    lease_id = _validate_name(path, capsule_id, expected_lease_id)
    _validate_count(data, V2_HEADER_BYTES, V2_RECORD_BYTES, count)
    expected_logical = LOGICAL_HEADER_BYTES + count * V2_RECORD_BYTES
    if logical_bytes != expected_logical:
        raise CapsuleFormatError("v2 logical bytes_used mismatch")
    records = []
    for index in range(count):
        sequence, frame, address, pc, kind_code = struct.unpack_from(
            "<IIIII", data, V2_HEADER_BYTES + index * V2_RECORD_BYTES)
        records.append(DecodedObservation(sequence, frame, address, pc,
                                           KIND_NAMES.get(kind_code), kind_code))
    return DecodedCapsule(str(path), version, capsule_id, lease_id, start_frame,
                          logical_bytes, len(data), count, tuple(records))


def decode_capsule(path: Path, expected_capsule_id: int | None = None,
                   expected_lease_id: str | None = None) -> DecodedCapsule:
    """Decode one capsule without accepting stale or malformed evidence."""
    path = Path(path).resolve()
    try:
        data = path.read_bytes()
    except OSError as error:
        raise CapsuleFormatError(f"capsule cannot be read: {error}") from error
    if data[:4] == LEGACY_MAGIC:
        return _decode_legacy(data, path, expected_capsule_id, expected_lease_id)
    if data[:4] == V2_MAGIC:
        return _decode_v2(data, path, expected_capsule_id, expected_lease_id)
    raise CapsuleFormatError("unknown capsule magic")
