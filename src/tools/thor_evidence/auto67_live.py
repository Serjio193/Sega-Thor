"""AUTO67 live opportunistic runtime sampler and bounded worker dispatcher."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import threading
import time
from collections import deque
from pathlib import Path
from typing import Any
from auto67_status import StatusPublisher


ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
BASELINE = "019fed68d7e906daabe184f3b74017c853b78ce3"


def digest(value: Any) -> str:
    payload = json.dumps(value, sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(payload.encode("utf-8")).hexdigest()[:24]


class RollingWindow:
    """Bounded current context; it never becomes a pending-event backlog."""

    def __init__(self, capacity: int):
        if capacity < 2:
            raise ValueError("rolling window capacity must be at least two")
        self.capacity = capacity
        self.items: deque[dict[str, Any]] = deque(maxlen=capacity)
        self.overwrites = 0
        self.retained = 0

    def append(self, event: dict[str, Any]) -> None:
        if len(self.items) == self.capacity:
            self.overwrites += 1
        self.items.append(event)

    def current(self) -> list[dict[str, Any]]:
        return list(self.items)


class Dispatcher:
    """Single claim authority with one mailbox per worker and no raw-event FIFO."""

    def __init__(self, worker_count: int = 16, capacity: int = 256,
                 processing_delay: float = 0.003):
        if worker_count not in {1, 2, 4, 8, 16, 32, 64}:
            raise ValueError("worker count must be one of 1,2,4,8,16,32,64")
        self.window = RollingWindow(capacity)
        self.worker_count = worker_count
        self.processing_delay = processing_delay
        self.lock = threading.RLock()
        self.stop_event = threading.Event()
        self.wake = [threading.Event() for _ in range(worker_count)]
        self.mailboxes: list[dict[str, Any] | None] = [None] * worker_count
        self.worker_states = ["STARTING"] * worker_count
        self.threads: list[threading.Thread] = []
        self.known: set[str] = set()
        self.covered: set[str] = set()
        self.active: dict[str, int] = {}
        self.metrics: dict[str, Any] = {
            "events_observed": 0, "events_retained_as_proof": 0,
            "seeds_considered": 0, "seeds_dispatched": 0,
            "known_rejected_before_dispatch": 0,
            "known_found_during_work": 0, "active_collisions": 0,
            "investigation_merges": 0, "new_chains": 0, "new_branches": 0,
            "new_edges": 0, "investigations_created": 0,
            "proven": 0, "waiting_runtime": 0, "blocked": 0, "exhausted": 0,
            "worker_leases": 0, "worker_returns": 0, "peak_workers_busy": 0,
            "peak_workers_working": 0,
            "capture_poll_count": 0, "worker_cpu_seconds": 0.0,
            "seed_age_sum": 0.0, "max_seed_age": 0.0, "same_session_known_replay": False,
            "new_roots": 0, "new_consumers": 0, "new_writers": 0,
            "structures_enumerated": 0, "promotion_candidates": 0,
            "bytes_promoted": 0,
        }
        self.investigations: dict[str, dict[str, Any]] = {}
        self.recent_investigations: deque[dict[str, Any]] = deque(maxlen=16)
        self.event_times: deque[float] = deque(maxlen=512)
        self.novelty_times: deque[float] = deque(maxlen=256)
        self.worker_info = [{"worker_id": i, "state": "STARTING",
                             "investigation_id": None, "chain_fingerprint": None,
                             "stage": "KNOWN_CHECK", "seed_age": 0.0,
                             "task_runtime": 0.0, "last_result": None,
                             "transitions": []} for i in range(worker_count)]

    @staticmethod
    def _branch(event: dict[str, Any]) -> str:
        return digest({"kind": event.get("kind"), "address": event.get("address"),
                       "pc": event.get("pc")})

    @staticmethod
    def _seed(event: dict[str, Any]) -> str:
        return digest({"branch": Dispatcher._branch(event), "seq": event.get("seq")})

    def start(self) -> None:
        for worker_id in range(self.worker_count):
            thread = threading.Thread(target=self._worker, args=(worker_id,),
                                      name=f"auto67-worker-{worker_id}", daemon=True)
            self.threads.append(thread)
            thread.start()
        with self.lock:
            self.worker_states = ["IDLE"] * self.worker_count
            for item in self.worker_info:
                self._transition(item, "IDLE", "KNOWN_CHECK")

    def stop(self) -> None:
        self.stop_event.set()
        for item in self.wake:
            item.set()
        for thread in self.threads:
            thread.join(timeout=5)

    def _choose_current(self) -> tuple[int, dict[str, Any]] | None:
        free = [i for i, state in enumerate(self.worker_states) if state == "IDLE"]
        if not free:
            return None
        for event in reversed(self.window.items):
            if event.get("dispatch_state"):
                continue
            self.metrics["seeds_considered"] += 1
            branch = self._branch(event)
            event["branch_fingerprint"] = branch
            if branch in self.known or branch in self.covered:
                event["dispatch_state"] = "KNOWN"
                self.metrics["known_rejected_before_dispatch"] += 1
                self.metrics["same_session_known_replay"] = True
                continue
            if branch in self.active:
                event["dispatch_state"] = "MERGED"
                self.metrics["active_collisions"] += 1
                self.metrics["investigation_merges"] += 1
                continue
            worker_id = free.pop(0)
            age = max(0.0, time.monotonic() - float(event.get("captured_monotonic", time.monotonic())))
            event["dispatch_state"] = "LEASED"
            event["lease_worker"] = worker_id
            self.active[branch] = worker_id
            task = {"event": dict(event), "branch": branch,
                    "seed": self._seed(event), "assigned_ns": time.time_ns()}
            self.mailboxes[worker_id] = task
            self.worker_states[worker_id] = "LEASED"
            info = self.worker_info[worker_id]
            info.update({"investigation_id": "INV-AUTO67-" + task["seed"],
                         "chain_fingerprint": branch, "stage": "KNOWN_CHECK",
                         "seed_age": age, "task_runtime": 0.0,
                         "last_result": None})
            self._transition(info, "LEASED", "KNOWN_CHECK")
            self.metrics["seeds_dispatched"] += 1
            self.metrics["worker_leases"] += 1
            busy = self.worker_count - len(free)
            self.metrics["peak_workers_busy"] = max(self.metrics["peak_workers_busy"], busy)
            self.metrics["seed_age_sum"] += age
            self.metrics["max_seed_age"] = max(self.metrics["max_seed_age"], age)
            return worker_id, task
        return None

    def _dispatch_current(self) -> None:
        assigned: list[int] = []
        with self.lock:
            while True:
                chosen = self._choose_current()
                if chosen is None:
                    break
                assigned.append(chosen[0])
        for worker_id in assigned:
            self.wake[worker_id].set()

    def ingest(self, event: dict[str, Any]) -> None:
        event = dict(event)
        event.setdefault("captured_monotonic", time.monotonic())
        with self.lock:
            self.window.append(event)
            self.metrics["events_observed"] += 1
            self.event_times.append(time.monotonic())
            self.metrics["events_retained_as_proof"] = self.window.retained
        self._dispatch_current()

    def _worker(self, worker_id: int) -> None:
        while not self.stop_event.is_set():
            self.wake[worker_id].wait(0.2)
            self.wake[worker_id].clear()
            with self.lock:
                task = self.mailboxes[worker_id]
                self.mailboxes[worker_id] = None
                if task is None:
                    if self.stop_event.is_set():
                        break
                    continue
                self.worker_states[worker_id] = "WORKING"
                self.metrics["peak_workers_working"] = max(
                    self.metrics["peak_workers_working"], self.worker_states.count("WORKING"))
                info = self.worker_info[worker_id]
                self._transition(info, "WORKING", "CHAIN_BUILD")
            started = time.perf_counter()
            cpu_started = time.thread_time()
            event = task["event"]
            if self.processing_delay:
                time.sleep(self.processing_delay)
            status = str(event.get("resolution", "BOUNDED_UNRESOLVED"))
            branch = task["branch"]
            inv_id = "INV-AUTO67-" + task["seed"]
            with self.lock:
                investigation = {"id": inv_id, "branch": branch,
                                 "seed_sequence": event.get("seq"),
                                 "status": status, "evidence": [event]}
                if event.get("known_during_work"):
                    status = "KNOWN"
                    investigation["status"] = status
                    self.metrics["known_found_during_work"] += 1
                self.investigations[inv_id] = investigation
                self.recent_investigations.append(investigation)
                self.metrics["investigations_created"] += 1
                if status == "PROVEN":
                    self.metrics["proven"] += 1
                    self.known.add(branch)
                    self.covered.add(branch)
                    self.window.retained += 1
                elif status == "WAITING_RUNTIME":
                    self.metrics["waiting_runtime"] += 1
                    self.covered.add(branch)
                elif status == "BLOCKED":
                    self.metrics["blocked"] += 1
                    self.covered.add(branch)
                elif status == "EXHAUSTED":
                    self.metrics["exhausted"] += 1
                    self.covered.add(branch)
                elif status == "KNOWN":
                    self.known.add(branch)
                    self.covered.add(branch)
                else:
                    self.metrics["new_branches"] += 1
                    self.novelty_times.append(time.monotonic())
                    self.covered.add(branch)
                self.metrics["new_edges"] += 1
                self.active.pop(branch, None)
                self.worker_states[worker_id] = "RETURNING"
                info = self.worker_info[worker_id]
                info["task_runtime"] = time.perf_counter() - started
                info["last_result"] = status
                self._transition(info, "RETURNING", status)
                self.metrics["worker_returns"] += 1
                self.worker_states[worker_id] = "IDLE"
                self._transition(info, "IDLE", status)
                self.metrics["worker_cpu_seconds"] += time.thread_time() - cpu_started
            self._dispatch_current()

    @staticmethod
    def _transition(info: dict[str, Any], state: str, stage: str) -> None:
        info["state"] = state
        info["stage"] = stage
        info["transitions"].append({"state": state, "stage": stage,
                                     "chain": info["chain_fingerprint"],
                                     "investigation_id": info["investigation_id"],
                                     "at": time.time()})
        if len(info["transitions"]) > 24:
            del info["transitions"][:-24]

    def snapshot(self, lightweight: bool = False) -> dict[str, Any] | None:
        if not self.lock.acquire(blocking=not lightweight):
            return None
        try:
            metrics = dict(self.metrics)
            total_age = metrics.pop("seed_age_sum")
            metrics["average_seed_age"] = total_age / max(1, metrics["worker_leases"])
            now = time.monotonic()
            metrics["event_rate"] = sum(item >= now - 1.0 for item in self.event_times)
            metrics["recent_novelty_rate"] = sum(item >= now - 1.0 for item in self.novelty_times)
            metrics["workers_busy"] = sum(state in {"LEASED", "WORKING", "RETURNING"}
                                           for state in self.worker_states)
            metrics["workers_configured"] = self.worker_count
            return {"metrics": metrics, "worker_states": list(self.worker_states),
                    "workers": [dict(item, transitions=list(item["transitions"]))
                                for item in self.worker_info],
                    "rolling_window": {"capacity": self.window.capacity,
                                        "utilization": len(self.window.items),
                                        "max_utilization": self.window.capacity,
                                        "overwrites": self.window.overwrites,
                                        "retained": self.window.retained},
                    "investigations": list(self.recent_investigations) if lightweight
                    else list(self.investigations.values())}
        finally:
            self.lock.release()


def run_live(args: argparse.Namespace) -> dict[str, Any]:
    rom = Path(args.rom).resolve()
    emulator = Path(args.emulator).resolve()
    lua = Path(args.lua).resolve()
    output = Path(args.output).resolve()
    if hashlib.sha256(rom.read_bytes()).hexdigest() != ROM_SHA:
        raise SystemExit("canonical ROM identity mismatch")
    output.parent.mkdir(parents=True, exist_ok=True)
    status_path = output.with_suffix(".status.json")
    final_path = output.with_suffix(".lua.json")
    stop_path = output.with_suffix(".stop")
    for path in (status_path, final_path, stop_path):
        if path.exists():
            path.unlink()
    dispatcher = Dispatcher(args.workers, args.window, args.worker_delay)
    dispatcher.start()
    environment = os.environ.copy()
    environment.update({
        "OASIS_LIVE_STATUS": str(status_path),
        "OASIS_LIVE_FINAL": str(final_path),
        "OASIS_LIVE_STOP": str(stop_path),
        "OASIS_LIVE_MAX_FRAMES": str(args.max_frames),
        "OASIS_LIVE_DEMO_INPUTS": args.demo_inputs,
        "OASIS_LIVE_WINDOW": str(args.window),
        "OASIS_LIVE_CAPTURE_DISABLED": "1" if args.capture_disabled else "0",
        "OASIS_LIVE_CAPTURE_MODE": args.capture_mode,
    })
    command = [str(emulator), f"--lua={lua}", str(rom)]
    started = time.monotonic()
    seen_sequence = -1
    launcher_log = output.with_suffix(".launcher.log")
    view_path = output.with_suffix(".view.json")
    publisher = StatusPublisher(dispatcher, view_path, DASHBOARD_HTML,
                                None if args.no_view else args.view_port)
    view_url = publisher.url

    with launcher_log.open("w", encoding="utf-8") as log:
        process = subprocess.Popen(command, cwd=emulator.parent, env=environment,
                                    stdout=log, stderr=subprocess.STDOUT, text=True)
        while process.poll() is None:
            lua_status = None
            if status_path.exists():
                try:
                    lua_status = json.loads(status_path.read_text(encoding="utf-8"))
                except (json.JSONDecodeError, OSError):
                    lua_status = None
            if lua_status:
                dispatcher.metrics["capture_poll_count"] += 1
                for event in lua_status.get("events", []):
                    if int(event.get("seq", -1)) > seen_sequence:
                        seen_sequence = int(event["seq"])
                        dispatcher.ingest(event)
            if lua_status:
                publisher.publish(lua_status)
            time.sleep(args.poll_interval)
        process.wait(timeout=10)
    if final_path.exists():
        lua_final = json.loads(final_path.read_text(encoding="utf-8"))
    else:
        lua_final = dict(publisher.latest, capture_complete=False)
    publisher.publish(lua_final)
    dispatcher.stop()
    publisher.stop()
    elapsed = time.monotonic() - started
    result = {"schema": "oasis.m12.auto67.live-session.v1", "baseline": BASELINE,
              "rom_sha256": ROM_SHA, "command": command, "returncode": process.returncode,
              "session_seconds": elapsed, "workers_configured": args.workers,
              "dispatcher": dispatcher.snapshot(), "lua": lua_final,
              "raw_event_backlog": 0, "raw_event_backlog_structure": "NONEXISTENT",
              "launcher_log": str(launcher_log), "status_path": str(status_path),
              "capture_path": str(final_path), "capture_disabled": args.capture_disabled,
              "view_url": view_url, "view_snapshot": str(view_path),
              "visualization_proof": {"two_workers_working":
                                       dispatcher.metrics["peak_workers_working"] >= 2,
                                       "worker_returned_idle": dispatcher.metrics["worker_returns"] > 0,
                                       "known_or_merge_visible":
                                       dispatcher.metrics["known_rejected_before_dispatch"] > 0 or
                                       dispatcher.metrics["investigation_merges"] > 0,
                                       "rolling_window_active":
                                       dispatcher.metrics["events_observed"] > 0}}
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description="AUTO67 live opportunistic RE launcher")
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--lua", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--workers", type=int, choices=(1, 2, 4, 8, 16, 32, 64), default=16)
    parser.add_argument("--window", type=int, default=256)
    parser.add_argument("--worker-delay", type=float, default=0.003)
    parser.add_argument("--poll-interval", type=float, default=0.05)
    parser.add_argument("--max-frames", type=int, default=0,
                        help="bounded validation limit; zero means play until emulator closes")
    parser.add_argument("--demo-inputs", default="",
                        help="optional validation-only frame:buttons list, not a scenario")
    parser.add_argument("--capture-disabled", action="store_true")
    parser.add_argument("--capture-mode", choices=("continuous", "burst"), default="continuous",
                        help="burst is lossy discovery only; neither mode proves complete chains")
    parser.add_argument("--view-port", type=int, default=0)
    parser.add_argument("--no-view", action="store_true")
    args = parser.parse_args()
    result = run_live(args)
    print(json.dumps({"returncode": result["returncode"],
                      "metrics": result["dispatcher"]["metrics"],
                      "lua": result["lua"]}, sort_keys=True))
    return int(result["returncode"] or 0)


DASHBOARD_HTML = """<!doctype html>
<meta charset="utf-8"><title>AUTO67 Live RE</title>
<style>body{font:14px sans-serif;background:#101418;color:#e8eef2;margin:20px}h1{margin:0 0 8px}.grid{display:grid;grid-template-columns:repeat(4,1fr);gap:8px}.card{background:#1b232b;padding:10px;border-radius:6px}.workers{display:grid;grid-template-columns:repeat(4,1fr);gap:5px;margin-top:10px}.worker{border:1px solid #42525f;padding:6px;border-radius:4px}.WORKING{border-color:#54d68a}.LEASED{border-color:#f6c85f}.IDLE{border-color:#71808c}pre{white-space:pre-wrap}</style>
<h1>AUTO67 Live Opportunistic RE</h1><div id="summary">waiting for snapshot</div><div class="grid" id="cards"></div><h2>Workers</h2><div class="workers" id="workers"></div><h2>Observed history (last 24 transitions per worker)</h2><pre id="transitions"></pre><h2>Recent investigations (16)</h2><pre id="investigations"></pre>
<script>
async function refresh(){const s=await (await fetch('/api/status',{cache:'no-store'})).json();const d=s.dispatcher||{},m=d.metrics||{},l=s.lua||{};
document.getElementById('summary').textContent=`frame ${s.frame||0} epoch ${l.epoch||1} | capture events ${l.events_observed||0} | window ${d.rolling_window?.utilization||0}/${d.rolling_window?.capacity||0} | raw backlog ${s.raw_event_backlog_structure||'unknown'}`;
const fields=[['free workers',(m.workers_configured||0)-(m.workers_busy||0)],
['busy workers',m.workers_busy||0],['event rate/s',m.event_rate||0],
['novelty rate/s',m.recent_novelty_rate||0],['leases',m.worker_leases||0],
['returns',m.worker_returns||0],['known rejected',m.known_rejected_before_dispatch||0],
['known by worker',m.known_found_during_work||0],['collision total',m.active_collisions||0],
['merges before dispatch',m.investigation_merges||0],['new chains',m.new_chains||0],
['new branches',m.new_branches||0],['new edges',m.new_edges||0],
['window overwrites',l.events_overwritten||0],['avg seed age/s',m.average_seed_age||0],
['max seed age/s',m.max_seed_age||0],['capture policy',l.sampling_policy||'unknown']];
document.getElementById('cards').innerHTML=fields.map(x=>`<div class="card"><b>${x[0]}</b><br>${x[1]}</div>`).join('');
document.getElementById('workers').innerHTML=(d.workers||[]).map(w=>`<div class="worker ${w.state}"><b>W${w.worker_id}</b> ${w.state}<br>${w.investigation_id||'-'}<br>stage ${w.stage}<br>age ${(w.seed_age||0).toFixed(3)}s<br>result ${w.last_result||'-'}</div>`).join('');
document.getElementById('transitions').textContent=(d.workers||[]).flatMap(w=>(w.transitions||[]).map(t=>({w:w.worker_id,...t}))).sort((a,b)=>b.at-a.at).map(t=>`${new Date(t.at*1000).toISOString().slice(11,23)} W${t.w} ${t.state} / ${t.stage} ${(t.chain||'-').slice(0,12)}`).join('\\n');
document.getElementById('investigations').textContent=JSON.stringify((d.investigations||[]).slice(-16),null,2);}
async function poll(){try{await refresh();}catch(e){document.getElementById('summary').textContent='Snapshot unavailable / stale';}finally{setTimeout(poll,250);}}poll();
</script>"""


if __name__ == "__main__":
    raise SystemExit(main())
