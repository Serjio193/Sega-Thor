"""Worker-segment frame identity and fail-closed transition partitioning."""

from __future__ import annotations

from collections import defaultdict
from typing import Any, Iterable, Mapping

from live_forward_scaling_audit import (
    EVENT_FRAME_BOUNDARY, EVENT_SUBTYPE_MASK, EVENT_SUBTYPE_SHIFT, FLAG_EVENT,
)


UNRESOLVED = "UNRESOLVED"


def _frame_marker(row: tuple[int, ...]) -> int:
    return int(row[3]) | (int(row[4]) << 32)


def _identity(run_id: int, epoch: int, frame: int | str) -> dict[str, Any]:
    return {"run_id": run_id, "epoch": epoch, "frame": frame}


def _is_frame_boundary(row: tuple[int, ...]) -> bool:
    flags = int(row[6])
    return bool(flags & FLAG_EVENT) and ((flags & EVENT_SUBTYPE_MASK) >>
        EVENT_SUBTYPE_SHIFT) == EVENT_FRAME_BOUNDARY


def _partition(segment: Mapping[str, Any], rows: list[tuple[int, ...]]) -> tuple[
        dict[int, dict[str, Any]], dict[str, Any]]:
    run_id = int(segment["run_id"])
    epoch = int(segment["epoch"])
    entry = segment.get("entry_frame")
    exit_frame = segment.get("exit_frame")
    unresolved = _identity(run_id, epoch, UNRESOLVED)
    result = {
        "worker_id": int(segment["worker_id"]), "cycle": int(segment["cycle"]),
        "run_id": run_id, "epoch": epoch, "entry_frame": entry,
        "exit_frame": exit_frame, "frame_status": "MULTI_FRAME_UNRESOLVED",
        "frame_transition_count": 0,
    }
    if entry is None or exit_frame is None:
        return {int(row[0]): unresolved for row in rows}, result
    entry, exit_frame = int(entry), int(exit_frame)
    result["entry_frame"], result["exit_frame"] = entry, exit_frame
    if entry == exit_frame:
        if any(_is_frame_boundary(row) for row in rows):
            return {int(row[0]): unresolved for row in rows}, result
        result["frame_status"] = "SINGLE_FRAME"
        identity = _identity(run_id, epoch, entry)
        return {int(row[0]): identity for row in rows}, result

    current = entry
    mapped: dict[int, dict[str, Any]] = {}
    transitions = 0
    for row in rows:
        if _is_frame_boundary(row):
            marker = _frame_marker(row)
            if marker <= current or marker > exit_frame:
                return {int(item[0]): unresolved for item in rows}, result
            current = marker
            transitions += 1
        else:
            mapped[int(row[0])] = _identity(run_id, epoch, current)
    if current != exit_frame or transitions == 0:
        return {int(row[0]): unresolved for row in rows}, result
    result["frame_status"] = "MULTI_FRAME_PARTITIONED"
    result["frame_transition_count"] = transitions
    for row in rows:
        if _is_frame_boundary(row):
            mapped[int(row[0])] = _identity(run_id, epoch, current)
    return mapped, result


def resolve_segment_frames(segments: Iterable[Mapping[str, Any]],
                           rows: Iterable[tuple[int, ...]]) -> dict[str, Any]:
    ordered = sorted(rows, key=lambda row: int(row[0]))
    assignments: dict[int, list[dict[str, Any]]] = defaultdict(list)
    segment_reports: list[dict[str, Any]] = []
    errors: list[str] = []
    for segment in segments:
        start, end = int(segment["entry_stream_sequence"]), int(segment["exit_stream_sequence"])
        contained = [row for row in ordered if start <= int(row[0]) < end]
        mapped, report = _partition(segment, contained)
        segment_reports.append(report)
        for stream, identity in mapped.items():
            if identity not in assignments[stream]:
                assignments[stream].append(identity)
    resolved: dict[int, dict[str, Any]] = {}
    unresolved = 0
    for stream, identities in assignments.items():
        if len(identities) == 1:
            resolved[stream] = identities[0]
        else:
            resolved[stream] = _identity(int(identities[0]["run_id"]),
                                         int(identities[0]["epoch"]), UNRESOLVED)
            errors.append(f"stream {stream} has conflicting frame identities")
        if resolved[stream]["frame"] == UNRESOLVED:
            unresolved += 1
    return {
        "by_stream": resolved, "segments": sorted(segment_reports,
            key=lambda item: (item["cycle"], item["worker_id"])),
        "errors": sorted(set(errors)), "unresolved_streams": unresolved,
    }


__all__ = ["UNRESOLVED", "resolve_segment_frames"]
