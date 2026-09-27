"""Decode supported FLOW_V1 record widths without inventing absent fields."""

from __future__ import annotations

import struct


LEGACY_RECORD = struct.Struct("<QQIIHHI")
FULL_RECORD = struct.Struct("<QQQIIIHBBHHI")
LEGACY_FORMAT = "FLOW_V1_NATIVE32_LEGACY"
FULL_FORMAT = "FLOW_V1_NATIVE48"


def decode_flow_records(data: bytes, expected_count: int
                        ) -> tuple[list[tuple[int, ...]], list[tuple[int | None, ...]],
                                   int, str]:
    """Return exact raw rows, normalized semantic rows, width and format ID."""
    if expected_count <= 0:
        raise ValueError("FLOW_V1 record count must be positive")
    if len(data) == expected_count * FULL_RECORD.size:
        raw_rows = list(FULL_RECORD.iter_unpack(data))
        return raw_rows, raw_rows, FULL_RECORD.size, FULL_FORMAT
    if len(data) == expected_count * LEGACY_RECORD.size:
        raw_rows = list(LEGACY_RECORD.iter_unpack(data))
        normalized = [(stream, instruction, None, pc, next_pc, opcode, flags,
                       0, None, None, None, auxiliary)
                      for stream, instruction, pc, next_pc, opcode, flags, auxiliary
                      in raw_rows]
        return raw_rows, normalized, LEGACY_RECORD.size, LEGACY_FORMAT
    raise ValueError("FLOW_V1 record count and supported byte width disagree")
