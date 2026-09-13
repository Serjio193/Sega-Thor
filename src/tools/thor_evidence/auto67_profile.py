"""Bounded dispatch-latency measurements for the AUTO67 realtime path."""

from __future__ import annotations

from collections import deque


TIMESTAMP_KEYS = tuple(f"t{i}" for i in range(9))
STAGES = {
    "T0_T1_knowledge_lookup_us": ("t0", "t1"),
    "T1_T2_claim_acquire_us": ("t1", "t2"),
    "T2_T3_capsule_allocate_reset_us": ("t2", "t3"),
    "T3_T4_pre_context_copy_us": ("t3", "t4"),
    "T4_T5_filter_submit_us": ("t4", "t5"),
    "T5_T6_worker_message_submit_us": ("t5", "t6"),
    "T6_T7_claim_lock_release_us": ("t6", "t7"),
    "T7_T8_worker_start_us": ("t7", "t8"),
    "T0_T8_dispatch_to_worker_start_us": ("t0", "t8"),
}


def _stats(values: deque[int]) -> dict[str, int]:
    ordered = sorted(values)
    if not ordered:
        return {name: 0 for name in ("min", "p50", "p95", "p99", "max")}

    def pick(fraction: float) -> int:
        return ordered[min(len(ordered) - 1, int((len(ordered) - 1) * fraction))]

    return {"min": ordered[0], "p50": pick(.50), "p95": pick(.95),
            "p99": pick(.99), "max": ordered[-1]}


class DispatchProfiler:
    def __init__(self, capacity: int = 512) -> None:
        self.samples: deque[dict] = deque(maxlen=capacity)
        self.stage_values = {name: deque(maxlen=capacity) for name in STAGES}
        self.claim_lock_values: deque[int] = deque(maxlen=capacity)

    def record_claim_lock(self, duration_ns: int) -> None:
        self.claim_lock_values.append(max(0, duration_ns // 1000))

    def record(self, trace: dict) -> None:
        timestamps = trace.get("timestamps_ns", {})
        if any(key not in timestamps for key in TIMESTAMP_KEYS):
            return
        durations = {}
        for name, (start, end) in STAGES.items():
            value = max(0, (timestamps[end] - timestamps[start]) // 1000)
            self.stage_values[name].append(value)
            durations[name] = value
        self.samples.append({"lease_id": trace.get("lease_id"),
                             "investigation_id": trace.get("investigation_id"),
                             "frame": trace.get("frame"), "durations_us": durations})

    def snapshot(self) -> dict:
        return {"sample_count": len(self.samples),
                "t5_definition": "compact_filter_descriptor_submitted;_lua_install_profiled_separately",
                "stages_us": {name: _stats(values)
                               for name, values in self.stage_values.items()},
                "claim_lock_us": _stats(self.claim_lock_values),
                "recent": list(self.samples)[-64:]}
