"""Deterministic config and bounded status decoding for the 2H window."""

from __future__ import annotations

import json
import os
from pathlib import Path
import re
import tempfile
from typing import Any


DEFAULT_WORKER_COUNT = 16
DEFAULT_CHAIN_DEPTH = 20
INT32_MAX = 2**31 - 1
STATUS_SCHEMA = "oasis.m12.live-worker-control.v1"
METRIC_NAMES = (
    "configured_workers", "workers_ever_used", "pending", "capturing",
    "complete", "occupied", "peak_capturing", "peak_complete",
    "peak_occupied", "captures_started", "captures_completed",
    "captures_validated", "captures_invalid", "captures_dropped",
    "stale_ack", "identity_collisions", "ring_retention_failures",
    "memory_limit_endings", "depth_limit_endings", "unsupported_endings",
    "records_produced", "instructions", "control_flow", "unique_entries",
    "duplicate_entries", "total_segment_bytes", "ring_wraps", "ring_capacity",
    "descriptor_bytes_each", "memory_bytes_each",
)
PLAN_NAMES = (
    "worker_count", "depth", "memory_bytes", "record_capacity",
    "identity_capacity", "descriptor_bytes_each", "descriptor_bytes_total",
    "result_bytes_each", "result_buffers_bytes_total", "pending_queue_bytes",
    "active_queue_bytes", "identity_table_bytes", "shared_ring_bytes",
    "instruction_stack_bytes", "dynamic_bytes", "total_native_bytes",
)
STATE_NAMES = {0: "FREE", 1: "PENDING", 2: "CAPTURING", 3: "COMPLETE",
               4: "ANALYZING", 5: "STARTING"}
_DECIMAL = re.compile(r"[0-9]+\Z", re.ASCII)


def positive_integer(value: object, label: str) -> int:
    text = str(value)
    if not _DECIMAL.fullmatch(text):
        raise ValueError(f"{label} must be a positive integer")
    number = int(text, 10)
    if number <= 0:
        raise ValueError(f"{label} must be a positive integer")
    return number


def validate_config(worker_count: object, chain_depth: object) -> dict[str, int]:
    return {"chain_depth": positive_integer(chain_depth, "chain_depth"),
            "worker_count": positive_integer(worker_count, "worker_count")}


def load_next_run_config(path: Path) -> tuple[dict[str, int], str]:
    if not path.exists():
        return validate_config(DEFAULT_WORKER_COUNT, DEFAULT_CHAIN_DEPTH), "ACCEPTED_DEFAULTS"
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
        if not isinstance(value, dict) or set(value) != {"worker_count", "chain_depth"}:
            raise ValueError("unexpected config fields")
        return validate_config(value["worker_count"], value["chain_depth"]), "SAVED"
    except (OSError, UnicodeError, json.JSONDecodeError, ValueError, TypeError) as error:
        raise ValueError(f"STOP_WORKER_CONTROL_CONFIG_MALFORMED: {error}") from error


def save_next_run_config(path: Path, worker_count: object,
                         chain_depth: object) -> dict[str, int]:
    config = validate_config(worker_count, chain_depth)
    path.parent.mkdir(parents=True, exist_ok=True)
    encoded = json.dumps(config, indent=2, sort_keys=True) + "\n"
    temporary: str | None = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", newline="\n",
                                         dir=path.parent, prefix=path.name + ".",
                                         suffix=".tmp", delete=False) as target:
            target.write(encoded)
            target.flush()
            os.fsync(target.fileno())
            temporary = target.name
        os.replace(temporary, path)
        temporary = None
    finally:
        if temporary:
            Path(temporary).unlink(missing_ok=True)
    return config


def parse_native_plan(value: str) -> dict[str, int] | None:
    if not value:
        return None
    fields = [int(part, 10) for part in value.split(",")]
    if len(fields) != len(PLAN_NAMES):
        raise ValueError("native memory plan field count mismatch")
    return dict(zip(PLAN_NAMES, fields, strict=True))


def calculate_resource_budget(available_bytes: int | None, *,
                              native_budget_cap: int, process_budget_cap: int,
                              core_reserve_cap: int, system_reserve: int,
                              memory_bytes_each: int) -> dict[str, int]:
    """Keep the preview and the accepted launcher's preflight arithmetic identical."""
    available = process_budget_cap if available_bytes is None else int(available_bytes)
    process_budget = min(process_budget_cap, max(0, available - system_reserve))
    transport_budget = memory_bytes_each + 1024 * 1024
    core_reserve = min(core_reserve_cap, process_budget)
    native_budget = min(native_budget_cap,
                        max(0, process_budget - core_reserve - transport_budget))
    return {"available_physical_bytes": available,
            "process_budget_bytes": process_budget,
            "host_transport_budget_bytes": transport_budget,
            "core_reserve_bytes": core_reserve,
            "native_budget_bytes": native_budget}


def allocation_preflight(config: dict[str, int], plan: dict[str, int] | None,
                         native_budget_bytes: int,
                         rejection_reason: str | None = None) -> dict[str, int | str | None]:
    requested = validate_config(config["worker_count"], config["chain_depth"])
    required = plan.get("total_native_bytes") if plan else None
    shortfall = max(0, required - native_budget_bytes) if required is not None else None
    status = "REJECTED" if rejection_reason or shortfall else (
        "PENDING" if required is None else "PASS")
    return {"status": status,
            "requested_worker_count": requested["worker_count"],
            "requested_chain_depth": requested["chain_depth"],
            "required_native_bytes": required,
            "available_native_budget_bytes": native_budget_bytes,
            "shortfall_bytes": shortfall,
            "rejection_reason": rejection_reason or
                ("NATIVE_BUDGET_SHORTFALL" if shortfall else None)}


def process_memory_total(processes: list[tuple[int | None, int | None]]) -> int | None:
    """Sum distinct process working sets; worker/native memory is a subset of EmuHawk."""
    seen: set[int] = set()
    values: list[int] = []
    for process_id, working_set in processes:
        if process_id is None or working_set is None or process_id in seen:
            continue
        seen.add(process_id)
        values.append(working_set)
    return sum(values) if values else None


def system_memory_values(total_bytes: int | None,
                         available_bytes: int | None) -> dict[str, int | None]:
    used = total_bytes - available_bytes if total_bytes is not None and \
        available_bytes is not None and 0 <= available_bytes <= total_bytes else None
    return {"total_bytes": total_bytes, "available_bytes": available_bytes,
            "used_bytes": used}


def parse_live_control(value: str) -> dict[str, Any]:
    parts = value.split("|", 3)
    if len(parts) != 4:
        raise ValueError("LIVE_CONTROL snapshot framing is malformed")
    header = [int(item, 10) for item in parts[0].split(",")]
    if len(header) != 4 + len(METRIC_NAMES):
        raise ValueError("LIVE_CONTROL metrics field count mismatch")
    next_fields = parts[1].split(",", 2)
    if len(next_fields) != 3:
        raise ValueError("LIVE_CONTROL next-run planner fields are malformed")
    if next_fields[2] == "UNREPRESENTABLE":
        next_config, next_plan, next_plan_status = None, None, "MANAGED_API_RANGE_REJECTED"
    else:
        next_config = validate_config(next_fields[0], next_fields[1])
        next_plan = None if next_fields[2] == "REJECTED" else parse_native_plan(next_fields[2])
        next_plan_status = "NATIVE_PLANNER_REJECTED" if next_plan is None else "READY"
    offset = int(parts[2], 10)
    rows: list[dict[str, int | str]] = []
    if parts[3]:
        for encoded_row in parts[3].split(";"):
            fields = [int(item, 10) for item in encoded_row.split(",")]
            if len(fields) != 8:
                raise ValueError("LIVE_CONTROL Worker row width mismatch")
            worker, state, progress, depth, started, completed, analyzing, released = fields
            if worker < offset or state not in STATE_NAMES or depth <= 0 or \
                    progress < 0 or progress > depth:
                raise ValueError("LIVE_CONTROL Worker state/progress is inconsistent")
            rows.append({"worker_id": worker, "state": STATE_NAMES[state],
                         "progress": progress, "depth": depth,
                         "capture_starts": started, "capture_completions": completed,
                         "analyses": analyzing, "releases": released})
    frame, worker_count, depth, pool_active = header[:4]
    return {"frame": frame, "worker_count": worker_count, "depth": depth,
            "pool_active": bool(pool_active),
            "metrics": dict(zip(METRIC_NAMES, header[4:], strict=True)),
            "next_run": next_config, "next_plan": next_plan,
            "next_plan_status": next_plan_status,
            "worker_offset": offset, "workers": rows}


def format_bytes(value: int | None) -> str:
    if value is None:
        return "Unavailable"
    number = float(value)
    for unit in ("B", "KiB", "MiB", "GiB", "TiB"):
        if number < 1024 or unit == "TiB":
            return f"{number:.1f} {unit}"
        number /= 1024
    return f"{number:.1f} TiB"


class EvidenceGrowthMeter:
    """Run-average rate from measured bytes; the first sample is never a rate."""

    def __init__(self) -> None:
        self._start_at: float | None = None
        self._start_bytes: int | None = None
        self._last_at: float | None = None
        self.samples = 0

    def sample(self, evidence_bytes: int, at: float) -> dict[str, float | int | str | None]:
        if evidence_bytes < 0:
            raise ValueError("evidence byte count cannot be negative")
        if self._start_at is None or (self._last_at is not None and at <= self._last_at):
            self._start_at, self._start_bytes = at, evidence_bytes
            self._last_at, self.samples = at, 1
            return {"state": "CALCULATING", "bytes": evidence_bytes,
                    "elapsed_seconds": 0.0, "rate_bytes_per_second": None,
                    "projected_gib_per_hour": None}
        self.samples += 1
        self._last_at = at
        elapsed = at - self._start_at
        delta = evidence_bytes - int(self._start_bytes or 0)
        if delta < 0:
            self._start_at, self._start_bytes = at, evidence_bytes
            elapsed, delta = 0.0, 0
        if elapsed < 5.0:
            return {"state": "CALCULATING", "bytes": evidence_bytes,
                    "elapsed_seconds": elapsed, "rate_bytes_per_second": None,
                    "projected_gib_per_hour": None}
        rate = delta / elapsed
        return {"state": "MEASURED_RATE", "bytes": evidence_bytes,
                "elapsed_seconds": elapsed, "rate_bytes_per_second": rate,
                "projected_gib_per_hour": rate * 3600 / (1024 ** 3)}
