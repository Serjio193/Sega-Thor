"""Separate, virtualized Tk window for the bounded 2H control snapshot."""

from __future__ import annotations

import argparse
import json
import math
import os
from pathlib import Path
from typing import Any

try:
    import tkinter as tk
    from tkinter import ttk
except ImportError:
    tk = None
    ttk = None

from live_forward_worker_control_model import (
    INT32_MAX, format_bytes, load_next_run_config,
    positive_integer, save_next_run_config,
)


POLL_MS = 250
PAGE_ROWS = 64
ROW_HEIGHT = 26
STATE_COLORS = {"FREE": "#78848b", "PENDING": "#9aabb4",
                "CAPTURING": "#3398db", "COMPLETE": "#42b883",
                "ANALYZING": "#e0aa3e", "STARTING": "#8b9ca5"}


def _write_preview(path: Path, worker_text: str, depth_text: str, offset: int) -> None:
    try:
        workers = positive_integer(worker_text, "Workers")
        depth = positive_integer(depth_text, "Depth")
    except ValueError:
        path.unlink(missing_ok=True)
        return
    representable = workers <= INT32_MAX and depth <= INT32_MAX
    contents = (f"status={'READY' if representable else 'UNREPRESENTABLE'}\n"
                f"worker_count={workers if representable else 0}\n"
                f"chain_depth={depth if representable else 0}\n"
                f"worker_offset={max(0, offset)}\nworker_limit={PAGE_ROWS}\n")
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(contents, encoding="ascii", newline="\n")
    os.replace(temporary, path)


def read_snapshot(path: Path) -> dict | None:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
        return value if value.get("schema") == "oasis.m12.live-worker-control.v1" else None
    except (OSError, json.JSONDecodeError, AttributeError):
        return None


def snapshot_for_display(snapshot: dict | None) -> dict:
    return snapshot if snapshot is not None else {}


def close_window(preview_path: Path, destroy: object) -> None:
    """Close only this UI process; it has no handle to the emulator runtime."""
    preview_path.unlink(missing_ok=True)
    destroy()


class WorkerControlWindow:
    def __init__(self, snapshot_path: Path, config_path: Path,
                 preview_path: Path) -> None:
        if tk is None or ttk is None:
            raise RuntimeError("THOR WORKER CONTROL requires a Tk-enabled Python runtime")
        self.snapshot_path = snapshot_path
        self.config_path = config_path
        self.preview_path = preview_path
        self.snapshot: dict = {}
        self.worker_offset = 0
        self.saved_config, _ = load_next_run_config(config_path)
        self.root = tk.Tk()
        self.root.title("THOR WORKER CONTROL")
        self.root.geometry("820x760")
        self.root.minsize(680, 560)
        self.root.configure(background="#12191e")
        self._build()
        self._set_inputs(self.saved_config)
        self._write_preview()
        self.root.protocol("WM_DELETE_WINDOW", self.close)
        self.root.after(POLL_MS, self.refresh)

    def _build(self) -> None:
        style = ttk.Style(self.root)
        style.theme_use("clam")
        style.configure("TFrame", background="#12191e")
        style.configure("TLabel", background="#12191e", foreground="#e5edf1",
                        font=("Segoe UI", 10))
        style.configure("Title.TLabel", font=("Segoe UI Semibold", 16),
                         foreground="#f2f6f8")
        style.configure("Section.TLabel", font=("Segoe UI Semibold", 10),
                         foreground="#83c7e8")
        style.configure("Status.TLabel", foreground="#a8bac4")
        style.configure("TButton", padding=(12, 5))

        root_frame = ttk.Frame(self.root, padding=(16, 12))
        root_frame.pack(fill="both", expand=True)
        ttk.Label(root_frame, text="THOR WORKER CONTROL", style="Title.TLabel").pack(anchor="w")
        self.connection = ttk.Label(root_frame, text="WAITING FOR LIVE SNAPSHOT",
                                    style="Status.TLabel")
        self.connection.pack(anchor="w", pady=(2, 10))
        self.current = ttk.Label(root_frame, text="CURRENT RUN   Workers —   Depth —")
        self.current.pack(anchor="w", pady=(0, 8))

        self.bars: dict[str, tuple[tk.Canvas, tk.Label]] = {}
        for key, label in (("system", "SYSTEM USED"), ("thor", "THOR USED"),
                           ("free", "FREE RAM"), ("next", "NEXT RUN EXPECTED")):
            line = ttk.Frame(root_frame)
            line.pack(fill="x", pady=2)
            ttk.Label(line, text=label, width=19, style="Section.TLabel").pack(side="left")
            bar = tk.Canvas(line, height=15, highlightthickness=0,
                            background="#27343b")
            bar.pack(side="left", fill="x", expand=True, padx=(5, 10))
            value = tk.Label(line, text="—", background="#12191e",
                             foreground="#e5edf1", width=20, anchor="e",
                             font=("Segoe UI", 9))
            value.pack(side="right")
            self.bars[key] = (bar, value)

        self.evidence = ttk.Label(root_frame, text="Evidence growth: CALCULATING…")
        self.evidence.pack(anchor="w", pady=(7, 10))

        worker_header = ttk.Frame(root_frame)
        worker_header.pack(fill="x")
        ttk.Label(worker_header, text="WORKERS", style="Section.TLabel").pack(side="left")
        self.worker_page = ttk.Label(worker_header, text="—", style="Status.TLabel")
        self.worker_page.pack(side="right")
        list_frame = ttk.Frame(root_frame)
        list_frame.pack(fill="both", expand=True, pady=(4, 10))
        self.worker_canvas = tk.Canvas(list_frame, background="#19242a",
                                       highlightthickness=0)
        self.worker_scroll = ttk.Scrollbar(list_frame, orient="vertical",
                                           command=self._scroll)
        self.worker_canvas.configure(yscrollcommand=self._scroll_changed)
        self.worker_canvas.pack(side="left", fill="both", expand=True)
        self.worker_scroll.pack(side="right", fill="y")
        self.worker_canvas.bind("<Configure>", lambda _event: self._draw_workers())
        self.worker_canvas.bind("<MouseWheel>", self._wheel)

        ttk.Label(root_frame, text="NEXT RUN ONLY", style="Section.TLabel").pack(anchor="w")
        config_line = ttk.Frame(root_frame)
        config_line.pack(fill="x", pady=(4, 2))
        ttk.Label(config_line, text="Workers").pack(side="left")
        self.worker_var = tk.StringVar()
        self.worker_entry = ttk.Entry(config_line, textvariable=self.worker_var, width=16)
        self.worker_entry.pack(side="left", padx=(6, 18))
        ttk.Label(config_line, text="Depth").pack(side="left")
        self.depth_var = tk.StringVar()
        self.depth_entry = ttk.Entry(config_line, textvariable=self.depth_var, width=16)
        self.depth_entry.pack(side="left", padx=6)
        self.save_button = ttk.Button(config_line, text="SAVE", command=self.save)
        self.save_button.pack(side="right")
        self.save_state = ttk.Label(root_frame, text="", style="Status.TLabel")
        self.save_state.pack(anchor="w")
        buttons = ttk.Frame(root_frame)
        buttons.pack(fill="x", pady=(5, 0))
        ttk.Button(buttons, text="DETAILS", command=self.show_details).pack(side="left")
        self.projection = ttk.Label(buttons, text="", style="Status.TLabel")
        self.projection.pack(side="right")
        self.worker_var.trace_add("write", lambda *_: self._inputs_changed())
        self.depth_var.trace_add("write", lambda *_: self._inputs_changed())

    def _set_inputs(self, config: dict[str, int]) -> None:
        self.worker_var.set(str(config["worker_count"]))
        self.depth_var.set(str(config["chain_depth"]))

    def _write_preview(self) -> None:
        _write_preview(self.preview_path, self.worker_var.get(),
                       self.depth_var.get(), self.worker_offset)

    def _inputs_changed(self) -> None:
        self._write_preview()
        try:
            candidate = {"worker_count": positive_integer(self.worker_var.get(), "Workers"),
                         "chain_depth": positive_integer(self.depth_var.get(), "Depth")}
        except ValueError:
            self.save_button.state(["disabled"])
            self.save_state.configure(text="Workers and Depth must be positive integers")
            self.projection.configure(text="Planner preview unavailable")
            return
        self.save_button.state(["!disabled"])
        if candidate == self.saved_config:
            self.save_state.configure(text="SAVED FOR NEXT RUN")
        else:
            self.save_state.configure(text="UNSAVED")
        if candidate["worker_count"] > INT32_MAX or candidate["chain_depth"] > INT32_MAX:
            self.projection.configure(text="PREFLIGHT REJECTS: managed native API range")
        else:
            self.projection.configure(text="Allocation preview updates from the live native planner")

    def save(self) -> None:
        try:
            config = save_next_run_config(self.config_path,
                self.worker_var.get(), self.depth_var.get())
        except ValueError as error:
            self.save_state.configure(text=str(error))
            return
        self.saved_config = config
        self.save_state.configure(text="SAVED FOR NEXT RUN")

    def _scroll(self, *args: str) -> None:
        self.worker_canvas.yview(*args)
        self._update_worker_offset()

    def _scroll_changed(self, first: str, last: str) -> None:
        self.worker_scroll.set(first, last)
        self._update_worker_offset()

    def _wheel(self, event: Any) -> str:
        self.worker_canvas.yview_scroll(int(-event.delta / 120), "units")
        self._update_worker_offset()
        return "break"

    def _update_worker_offset(self) -> None:
        total = int(self.snapshot.get("current_worker_count", 0))
        if not total:
            return
        top = float(self.worker_canvas.yview()[0])
        offset = min(max(0, int(top * total)), max(0, total - PAGE_ROWS))
        if offset != self.worker_offset:
            self.worker_offset = offset
            self._write_preview()
            self._draw_workers()

    def _draw_workers(self) -> None:
        canvas = self.worker_canvas
        canvas.delete("all")
        total = int(self.snapshot.get("current_worker_count", 0))
        rows = {int(item["worker_id"]): item
                for item in self.snapshot.get("workers", [])}
        canvas.configure(scrollregion=(0, 0, max(500, canvas.winfo_width()),
                                       max(1, total) * ROW_HEIGHT))
        top = max(0, int(canvas.canvasy(0) // ROW_HEIGHT))
        visible = min(PAGE_ROWS, max(1, math.ceil(canvas.winfo_height() / ROW_HEIGHT) + 2))
        for worker_id in range(top, min(total, top + visible)):
            y = worker_id * ROW_HEIGHT
            item = rows.get(worker_id)
            canvas.create_text(12, y + 13, text=f"W{worker_id:04d}", anchor="w",
                               fill="#d9e4e9", font=("Segoe UI", 9))
            state = item["state"] if item else "WAITING"
            color = STATE_COLORS.get(state, "#52636c")
            canvas.create_text(92, y + 13, text=state, anchor="w",
                               fill=color, font=("Segoe UI Semibold", 9))
            progress = int(item["progress"]) if item else 0
            depth = int(item["depth"]) if item else int(self.snapshot.get("current_depth", 0))
            left, right = 205, max(260, canvas.winfo_width() - 120)
            canvas.create_rectangle(left, y + 6, right, y + 20,
                                    outline="#41525b", fill="#24343c")
            filled = int((right - left) * min(1.0, progress / depth)) if depth else 0
            if filled:
                canvas.create_rectangle(left, y + 6, left + filled, y + 20,
                    outline="", fill=color)
            canvas.create_text(right + 8, y + 13, text=f"{progress}/{depth}" if depth else "—",
                               anchor="w", fill="#d9e4e9", font=("Segoe UI", 9))
        self.worker_page.configure(text=f"{total:,} total · showing {top:,}–"
            f"{min(total, top + visible):,}")

    def _draw_bar(self, key: str, used: int | None, total: int | None,
                  color: str, text: str) -> None:
        canvas, label = self.bars[key]
        canvas.delete("all")
        width, height = max(1, canvas.winfo_width()), 15
        if used is not None and total and total > 0:
            fill_width = int(width * min(1.0, max(0.0, used / total)))
            if fill_width:
                canvas.create_rectangle(0, 0, fill_width, height,
                                        outline="", fill=color)
        label.configure(text=text)

    def _render(self) -> None:
        if not self.snapshot:
            self.connection.configure(text="WAITING FOR LIVE SNAPSHOT — emulator/runtime is disconnected")
            self.current.configure(text="CURRENT RUN   Workers —   Depth —")
            self._draw_workers()
            return
        s = self.snapshot
        connected = s.get("runtime_state") == "RUNNING"
        self.connection.configure(text=("LIVE · bounded snapshot" if connected else
                                        s.get("runtime_state", "DISCONNECTED")))
        self.current.configure(text=f"CURRENT RUN   Workers {s.get('current_worker_count', '—')}"
            f"      Depth {s.get('current_depth', '—')}"
            f"      Captures {s.get('worker_captures_started', 0):,}"
            f"      Segments {s.get('segments', 0):,}")
        total = s.get("system_total_ram_bytes")
        available = s.get("system_available_ram_bytes")
        system_used = total - available if total is not None and available is not None else None
        thor_used = s.get("thor_process_memory_bytes")
        color = "#42b883"
        if s.get("projected_next_run_free_ram_bytes") is not None and \
                s["projected_next_run_free_ram_bytes"] < 0:
            color = "#d9534f"
        elif available is not None and total and available / total < 0.15:
            color = "#e0aa3e"
        self._draw_bar("system", system_used, total, "#54a5d4",
                       format_bytes(system_used))
        self._draw_bar("thor", thor_used, total, "#8774cf", format_bytes(thor_used))
        self._draw_bar("free", available, total, color, format_bytes(available))
        next_total = s.get("projected_next_run_total_thor_bytes")
        projected_free = s.get("projected_next_run_free_ram_bytes")
        self._draw_bar("next", next_total, total, color, format_bytes(next_total))
        self.evidence.configure(text=s.get("evidence_growth_display", "Evidence growth: CALCULATING…"))
        plan = s.get("next_native_plan")
        if plan:
            self.projection.configure(text=f"Projected free after launch: {format_bytes(projected_free)}")
        elif s.get("next_plan_status"):
            self.projection.configure(text=f"Planner: {s['next_plan_status']}")
        self._draw_workers()

    def refresh(self) -> None:
        latest = read_snapshot(self.snapshot_path)
        self.snapshot = snapshot_for_display(latest)
        self._render()
        self.root.after(POLL_MS, self.refresh)

    def show_details(self) -> None:
        window = tk.Toplevel(self.root)
        window.title("THOR WORKER CONTROL — DETAILS")
        window.geometry("620x420")
        body = tk.Text(window, wrap="word", background="#101418",
                       foreground="#e5edf1", padx=12, pady=10)
        body.pack(fill="both", expand=True)
        body.insert("1.0", json.dumps(self.snapshot, indent=2, sort_keys=True))
        body.configure(state="disabled")

    def close(self) -> None:
        close_window(self.preview_path, self.root.destroy)

    def run(self) -> None:
        self.root.mainloop()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot", type=Path, required=True)
    parser.add_argument("--config", type=Path, required=True)
    parser.add_argument("--preview", type=Path, required=True)
    args = parser.parse_args()
    WorkerControlWindow(args.snapshot, args.config, args.preview).run()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
