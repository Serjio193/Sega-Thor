"""Independent FLOW_V1 byte and identity checks for scaling receipts."""

from __future__ import annotations

import hashlib
from pathlib import Path
import struct


RECORD = struct.Struct("<QQIIHHI")
REASON_DEPTH, REASON_MEMORY, REASON_RETENTION = 1, 2, 3
FLAG_INSTRUCTION, FLAG_COMPLETE, FLAG_FAULTED = 1, 2, 4
FLAG_EXCEPTION_EVENT, FLAG_CPU_STOP_EVENT = 256, 1024


def integer_list(value: str, width: int, label: str) -> list[int]:
    result = [int(item, 10) for item in value.split(",")]
    if len(result) != width:
        raise ValueError(f"{label}: expected {width} values, got {len(result)}")
    return result


def read_record_slice(path: Path, offset: int, count: int, worker: int) -> bytes:
    with path.open("rb") as source:
        source.seek(offset * RECORD.size)
        data = source.read(count * RECORD.size)
    if len(data) != count * RECORD.size:
        raise ValueError(f"Worker {worker}: binary FLOW_V1 segment is truncated")
    return data


def validate_segment(key: str, value: str, record_path: Path, worker_count: int,
                     expected_depth: int, memory_bytes: int, result_header_bytes: int,
                     run_id: int, previous_exit: list[int], global_entry: list[int],
                     cycle_counts: list[int], second_record_path: Path) -> dict[str, object]:
    pieces = value.split("|")
    if len(pieces) != 13:
        raise ValueError(f"{key}: malformed segment announcement")
    (cycle_text, meta_text, entry_text, exit_text, offset_text, meta2_text,
     entry2_text, exit2_text, offset2_text, export1_text, export2_text,
     immutable_before_text, immutable_after_text) = pieces
    cycle, offset, offset2 = int(cycle_text), int(offset_text), int(offset2_text)
    export_ms1, export_ms2 = float(export1_text), float(export2_text)
    meta = integer_list(meta_text, 20, f"{key} metadata")
    entry = integer_list(entry_text, 20, f"{key} ENTRY")
    exit_state = integer_list(exit_text, 20, f"{key} EXIT")
    meta2 = integer_list(meta2_text, 20, f"{key} reread metadata")
    entry2 = integer_list(entry2_text, 20, f"{key} reread ENTRY")
    exit2 = integer_list(exit2_text, 20, f"{key} reread EXIT")
    immutable_delta = int(immutable_after_text) - int(immutable_before_text)
    if meta != meta2 or entry != entry2 or exit_state != exit2 or immutable_delta <= 0:
        raise ValueError(f"{key}: result changed during CPU execution after completion")
    (capture, generation, run, epoch, first_stream, end_stream, first_instruction,
     end_instruction, first_flow, end_flow, worker, reason, configured_depth,
     configured_memory, consumed_depth, consumed_memory, record_count, record_bytes,
     valid, copy_ns) = meta
    if not 1 <= cycle <= 100 or not 0 <= worker < worker_count:
        raise ValueError(f"{key}: worker or cycle is outside the requested range")
    expected_capture = run_id * 1_000_000 + (cycle - 1) * worker_count + worker + 1
    if capture != expected_capture or generation != cycle or run != run_id or epoch <= 0:
        raise ValueError(f"{key}: capture_id/generation/run/epoch mismatch")
    if configured_depth != expected_depth or configured_memory != memory_bytes:
        raise ValueError(f"{key}: configured resource bounds mismatch")
    if not valid or record_count < 1 or record_count > 4096 or record_bytes != record_count * RECORD.size:
        raise ValueError(f"{key}: native result is invalid or outside fixed record bounds")
    if consumed_memory != result_header_bytes + record_bytes or consumed_memory > memory_bytes:
        raise ValueError(f"{key}: bounded memory accounting mismatch")
    if end_stream - first_stream != record_count:
        raise ValueError(f"{key}: stream range does not match record count")
    if reason == REASON_DEPTH and consumed_depth != configured_depth:
        raise ValueError(f"{key}: depth termination was not exact")
    if reason not in (REASON_DEPTH, REASON_MEMORY, REASON_RETENTION):
        raise ValueError(f"{key}: unsupported termination reason {reason}")
    capacity = (memory_bytes - result_header_bytes) // RECORD.size
    if reason == REASON_MEMORY and record_count + 2 <= capacity:
        raise ValueError(f"{key}: memory termination occurred before result capacity")
    if reason == REASON_RETENTION:
        raise ValueError(f"{key}: ring-retention termination invalidates the cycle")

    data = read_record_slice(record_path, offset, record_count, worker)
    reread_data = read_record_slice(second_record_path, offset2, record_count, worker)
    if data != reread_data:
        raise ValueError(f"{key}: binary immutable result changed after CPU execution")
    rows = list(RECORD.iter_unpack(data))
    if rows[0][0] != first_stream or rows[-1][0] + 1 != end_stream:
        raise ValueError(f"{key}: binary stream endpoints differ from FLOW_V1 metadata")
    if any(right[0] != left[0] + 1 for left, right in zip(rows, rows[1:])):
        raise ValueError(f"{key}: stream records are not contiguous")
    instructions = [row for row in rows if row[5] & FLAG_INSTRUCTION]
    if end_instruction - first_instruction != len(instructions):
        raise ValueError(f"{key}: instruction range does not reconcile with records")
    if any(row[1] != first_instruction + index for index, row in enumerate(instructions)):
        raise ValueError(f"{key}: instruction sequence contains a gap")
    control_rows = sum(bool(row[5] & 8) for row in rows)
    if end_flow - first_flow != consumed_depth or control_rows != consumed_depth:
        raise ValueError(f"{key}: control-flow depth accounting mismatch")
    if any(not row[5] & (FLAG_INSTRUCTION | FLAG_EXCEPTION_EVENT | FLAG_CPU_STOP_EVENT)
           for row in rows):
        raise ValueError(f"{key}: untyped native record")
    if any(row[5] & FLAG_FAULTED for row in rows):
        raise ValueError(f"{key}: faulted instruction appears in a valid result")
    if any(row[5] & FLAG_INSTRUCTION and not row[5] & FLAG_COMPLETE for row in rows):
        raise ValueError(f"{key}: incomplete instruction appears in a valid result")
    if reason == REASON_DEPTH:
        first_instruction_row = next((row for row in rows if row[5] & FLAG_INSTRUCTION), None)
        if first_instruction_row is None or entry[16] != first_instruction_row[2]:
            raise ValueError(f"{key}: ENTRY PC differs from first instruction")
        if exit_state[16] != rows[-1][3]:
            raise ValueError(f"{key}: EXIT PC differs from final record")
    if first_stream < previous_exit[worker]:
        raise ValueError(f"{key}: Worker result overlaps its prior ACKed capture")
    if cycle == 1 and worker == 0:
        global_entry[0] = first_stream - 1
    if first_stream <= global_entry[0]:
        raise ValueError(f"{key}: ENTRY is not a newer live CPU boundary")
    global_entry[0] = first_stream
    previous_exit[worker] = end_stream
    cycle_counts[worker] += 1
    digest = hashlib.sha256(
        (meta_text + "\n" + entry_text + "\n" + exit_text + "\n").encode("ascii") + data
    ).hexdigest()
    return {
        "worker_id": worker, "cycle": cycle, "capture_id": capture,
        "generation": generation, "run_id": run, "epoch": epoch,
        "entry_stream_sequence": first_stream, "exit_stream_sequence": end_stream,
        "entry_instruction_sequence": first_instruction,
        "exit_instruction_sequence": end_instruction,
        "entry_control_flow_sequence": first_flow, "exit_control_flow_sequence": end_flow,
        "termination_reason": reason, "configured_depth": configured_depth,
        "consumed_depth": consumed_depth, "configured_memory_bytes": configured_memory,
        "consumed_memory_bytes": consumed_memory, "record_count": record_count,
        "valid": True, "copy_duration_ns": copy_ns,
        "host_export_ms": export_ms1 + export_ms2,
        "immutable_cpu_stream_delta": immutable_delta,
        "host_export_ms_pass1": export_ms1, "host_export_ms_pass2": export_ms2,
        "records_sha256": hashlib.sha256(data).hexdigest(), "segment_sha256": digest,
    }


def overlap_peak(segments: list[dict[str, object]]) -> int:
    edges: list[tuple[int, int]] = []
    for segment in segments:
        edges.append((int(segment["entry_stream_sequence"]), 1))
        edges.append((int(segment["exit_stream_sequence"]), -1))
    active = peak = 0
    for _, delta in sorted(edges, key=lambda item: (item[0], item[1])):
        active += delta
        peak = max(peak, active)
    return peak
