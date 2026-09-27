"""Responsive visual monitor for the post-run analysis backend."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import time
import tkinter as tk
from tkinter import ttk

from live_forward_progress import STAGES, heartbeat_state, overall_from_stages, stage_percent


COLORS = {"PASS": "#42b883", "NO_DELTA": "#42b883", "SKIPPED_NOT_APPLICABLE": "#89959b",
          "UNRESOLVED": "#e6b450",
          "ACTIVE": "#54a5d4", "PENDING": "#89959b", "WARNING": "#e6b450",
          "STOP": "#e06c75", "ERROR": "#e06c75", "BLOCKED": "#89959b"}
SYMBOLS = {"PASS": "✓", "NO_DELTA": "✓", "SKIPPED_NOT_APPLICABLE": "·", "ACTIVE": "▶",
           "UNRESOLVED": "!",
           "PENDING": "○", "WARNING": "⚠", "STOP": "✗", "ERROR": "✗", "BLOCKED": "○"}


def _read(path: Path) -> tuple[dict, bool]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
        return (value, isinstance(value, dict))
    except (OSError, json.JSONDecodeError, UnicodeError):
        return {}, False


def _alive(pid: object) -> bool:
    if not isinstance(pid, int) or pid <= 0:
        return False
    try:
        os.kill(pid, 0)
    except (OSError, ValueError):
        return False
    return True


class PostRunWindow:
    def __init__(self, status_path: Path, report_path: Path) -> None:
        self.status_path, self.report_path = status_path, report_path
        self.root = tk.Tk()
        self.root.title("THOR POST-RUN ANALYSIS")
        self.root.geometry("760x690")
        self.root.minsize(650, 560)
        self.root.configure(background="#12191e")
        self._indeterminate = False
        self._build()
        self.root.lift()
        self.root.attributes("-topmost", True)
        self.root.after_idle(self.root.attributes, "-topmost", False)
        self.root.after(250, self.refresh)

    def _build(self) -> None:
        background = "#12191e"
        panel = "#18232a"
        trough = "#26363f"
        foreground = "#e5edf1"
        accent = "#54a5d4"
        style = ttk.Style(self.root)
        style.theme_use("clam")
        style.configure("TFrame", background=background)
        style.configure("TLabel", background=background, foreground=foreground, font=("Segoe UI", 10))
        style.configure("TLabelframe", background=panel, foreground=foreground,
                        bordercolor="#30434d", relief="solid")
        style.configure("TLabelframe.Label", background=panel, foreground=foreground,
                        font=("Segoe UI Semibold", 9))
        style.configure("TButton", background=panel, foreground=foreground,
                        bordercolor="#49616d", focuscolor=panel)
        style.map("TButton", background=[("active", "#263b47"), ("pressed", "#304b5a")],
                  foreground=[("disabled", "#71818a")])
        style.configure("Dark.Horizontal.TProgressbar", troughcolor=trough,
                        background=accent, bordercolor=trough, lightcolor=accent,
                        darkcolor=accent, thickness=14)
        style.configure("Title.TLabel", background=background, foreground="#f2f6f8",
                        font=("Segoe UI Semibold", 17))
        style.configure("State.TLabel", background=background, foreground=accent,
                        font=("Segoe UI Semibold", 13))
        style.configure("Stage.TLabel", background=panel, foreground="#9aaab2",
                        font=("Consolas", 10))
        root = ttk.Frame(self.root, padding=18)
        root.pack(fill="both", expand=True)
        ttk.Label(root, text="THOR POST-RUN ANALYSIS", style="Title.TLabel").pack(anchor="w")
        self.summary = ttk.Label(root, text="Waiting for sealed runtime evidence…", style="State.TLabel")
        self.summary.pack(anchor="w", pady=(5, 2))
        self.counter = ttk.Label(root, text=f"Stage — / {len(STAGES)}")
        self.counter.pack(anchor="w")
        current = ttk.LabelFrame(root, text=" CURRENT STAGE ", padding=12)
        current.pack(fill="x", pady=(12, 10))
        self.current_name = ttk.Label(current, text="Waiting…", style="State.TLabel")
        self.current_name.pack(anchor="w")
        self.progress = ttk.Progressbar(current, style="Dark.Horizontal.TProgressbar",
                                        orient="horizontal", mode="determinate", maximum=100)
        self.progress.pack(fill="x", pady=(8, 6))
        self.progress_text = ttk.Label(current, text="Processed —")
        self.progress_text.pack(anchor="w")
        self.heartbeat = ttk.Label(current, text="Heartbeat —")
        self.heartbeat.pack(anchor="w")
        self.stage_elapsed = ttk.Label(current, text="Elapsed —")
        self.stage_elapsed.pack(anchor="w")
        stages = ttk.LabelFrame(root, text=" STAGES ", padding=10)
        stages.pack(fill="x")
        self.stage_labels = {}
        for stage in STAGES:
            label = ttk.Label(stages, text=f"○ {stage}", style="Stage.TLabel")
            label.pack(anchor="w", pady=1)
            self.stage_labels[stage] = label
        buttons = ttk.Frame(root)
        buttons.pack(fill="x", pady=(10, 0))
        ttk.Button(buttons, text="DETAILS", command=self.show_details).pack(side="right")
        self.result = tk.Text(root, height=7, wrap="word", background="#101418",
                              foreground="#d9e4e9", relief="flat", borderwidth=0)
        self.result.pack(fill="both", expand=True, pady=(10, 0))
        self.result.configure(state="disabled")

    @staticmethod
    def _fmt(value: object) -> str:
        return f"{int(value):,}" if isinstance(value, int) else "—"

    @staticmethod
    def _elapsed(value: object) -> str:
        seconds = max(0, int(float(value or 0)))
        return f"Elapsed {seconds // 60:02d}:{seconds % 60:02d}"

    def _set_bar(self, row: dict) -> int | None:
        percent = stage_percent(row)
        if percent is None:
            self.progress.configure(mode="indeterminate")
            if not self._indeterminate:
                self.progress.start(80)
                self._indeterminate = True
        else:
            if self._indeterminate:
                self.progress.stop()
                self._indeterminate = False
            self.progress.configure(mode="determinate", value=percent)
        return percent

    def _heartbeat(self, snapshot: dict) -> tuple[str, str]:
        state = heartbeat_state(snapshot)
        if state == "LIVE":
            return "● LIVE", "#42b883"
        if state == "BUSY / WAITING":
            return "● BUSY / WAITING", "#e6b450"
        return "● NO UPDATE", "#e06c75"

    def refresh(self) -> None:
        snapshot, valid = _read(self.status_path)
        report, report_valid = _read(self.report_path)
        if not self.status_path.exists():
            self.summary.configure(text="ANALYSIS RUNNING…", foreground="#54a5d4")
            self.current_name.configure(text="Waiting for backend heartbeat…", foreground="#54a5d4")
            self.root.after(250, self.refresh)
            return
        if not valid:
            self.summary.configure(text="STATUS SNAPSHOT INVALID", foreground="#e06c75")
            self.root.after(250, self.refresh)
            return
        rows = snapshot.get("stages") if isinstance(snapshot.get("stages"), dict) else {}
        pipeline = snapshot.get("pipeline_state")
        overall = snapshot.get("overall_state") or overall_from_stages(rows, pipeline)
        if pipeline is None:
            pipeline = {"ANALYSIS COMPLETE ✓": "COMPLETE",
                        "PARTIAL ANALYSIS COMPLETE ⚠": "PARTIAL_COMPLETE",
                        "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE": "COMPLETE_WITH_UNRESOLVED",
                        "ANALYSIS STOPPED ✗": "STOPPED",
                        "ANALYSIS FAILED ✗": "FAILED"}.get(overall, "RUNNING")
        terminal = pipeline in {"COMPLETE", "COMPLETE_WITH_UNRESOLVED", "PARTIAL_COMPLETE"}
        stage = (None if terminal else snapshot.get("stage"))
        if stage not in STAGES:
            stage = None
        row = rows.get(stage, {}) if stage else {}
        if pipeline == "COMPLETE":
            heartbeat, color = "Backend complete ✓", "#42b883"
        elif pipeline in {"PARTIAL_COMPLETE", "COMPLETE_WITH_UNRESOLVED"}:
            heartbeat, color = "Backend complete", "#42b883"
        elif pipeline == "STOPPED":
            heartbeat, color = "Backend stopped safely", "#e06c75"
        elif pipeline == "FAILED":
            heartbeat, color = "Backend stopped", "#e06c75"
        else:
            heartbeat, color = self._heartbeat(snapshot)
            stale = heartbeat == "● NO UPDATE" and overall == "ANALYSIS RUNNING…"
            if stale and not _alive(snapshot.get("backend_pid")):
                overall = "ANALYSIS FAILED ✗"
                heartbeat = "Backend stopped"
        self.summary.configure(text=overall, foreground="#e06c75" if "FAILED" in overall else color)
        self.counter.configure(text=f"Stage {snapshot.get('stage_index', '—')} / {snapshot.get('stage_count', len(STAGES))}")
        current_text = stage or "No active stage"
        if stage and row.get("state") in {"STOP", "ERROR", "UNRESOLVED"} and row.get("detail"):
            current_text = f"{stage} — {row['state']}\n{row['detail']}"
        self.current_name.configure(text=current_text, foreground=color)
        percent = self._set_bar(row)
        processed = self._fmt(row.get("processed_units"))
        total = self._fmt(row.get("total_units")) if isinstance(row.get("total_units"), int) else "unknown"
        unit = row.get("unit_name") or "units"
        percent_text = f"{percent}%" if percent is not None else "INDETERMINATE"
        self.progress_text.configure(text=f"{percent_text}   Processed {processed} / {total} {unit}")
        progress_stamp = snapshot.get("last_progress_change_time", time.monotonic())
        progress_age = max(0, int(time.monotonic() - float(progress_stamp)))
        self.heartbeat.configure(text=f"Heartbeat {heartbeat}   Progress age: {progress_age} s",
                                  foreground=color)
        self.stage_elapsed.configure(text=self._elapsed(snapshot.get("elapsed_seconds")))
        for name, label in self.stage_labels.items():
            state = rows.get(name, {}).get("state", "PENDING")
            detail = rows.get(name, {}).get("detail", "")
            suffix = f" — {detail}" if state in {"STOP", "ERROR", "UNRESOLVED"} and detail else ""
            label.configure(text=f"{SYMBOLS.get(state, '?')} {name}  [{state}]{suffix}",
                            foreground=COLORS.get(state, "#9aaab2"))
        if report_valid and overall in {"ANALYSIS COMPLETE ✓", "ANALYSIS COMPLETE WITH UNRESOLVED EVIDENCE", "PARTIAL ANALYSIS COMPLETE ⚠",
                                        "ANALYSIS STOPPED ✗"}:
            self._show_result(report, overall)
        self.root.after(250, self.refresh)

    def _show_result(self, report: dict, overall: str) -> None:
        lines = [overall, ""]
        for key, label in (("new_instructions", "New instructions"), ("new_edges", "New relations"),
                           ("terminal_facts", "Terminal facts"), ("raw_records_read", "Raw records")):
            if key in report:
                lines.append(f"{label:<18} {self._fmt(report[key])}")
        audio = report.get("stage_results", {}).get("AUDIO ANALYSIS")
        if audio:
            lines.append(f"Audio candidates    {self._fmt(audio.get('candidate_ranges_count'))}")
            lines.append(f"Strict audio chains {self._fmt(audio.get('strict_causal_chains'))}")
        vdp = report.get("stage_results", {}).get("VDP / DMA ANALYSIS")
        if vdp:
            lines.append(f"VDP commands        {self._fmt(vdp.get('complete_vdp_commands'))}")
            lines.append(f"DMA events          {self._fmt(vdp.get('dma_events'))}")
        lines.append(f"SOURCE_OWNED delta  {report.get('source_owned_delta', '—')}")
        if report.get("canonical_refresh"):
            lines.append(f"Map refresh         {report['canonical_refresh']}")
        cleanup = report.get("stage_results", {}).get("CLEANUP") or report.get("cleanup")
        if cleanup:
            lines.append(f"Cleanup             {cleanup.get('status', '—')}")
            if cleanup.get("absorbed"):
                lines.append(f"ABSORBED RUN        {cleanup.get('run_id', 'yes')}")
                lines.append(f"RAW DELETED         {self._fmt(cleanup.get('raw_bytes_deleted'))} bytes")
                lines.append(f"Deletion mode       {cleanup.get('deletion_mode', '—')}")
        if report.get("stop"):
            lines.append(f"STOP                 {report['stop']}")
        self.result.configure(state="normal")
        self.result.delete("1.0", "end")
        self.result.insert("1.0", "\n".join(lines))
        self.result.configure(state="disabled")

    def show_details(self) -> None:
        status, _ = _read(self.status_path)
        report, _ = _read(self.report_path)
        dialog = tk.Toplevel(self.root)
        dialog.title("THOR POST-RUN DETAILS")
        dialog.geometry("760x600")
        text = tk.Text(dialog, wrap="none", background="#101418", foreground="#d9e4e9")
        text.pack(fill="both", expand=True)
        text.insert("1.0", json.dumps({"status": status, "report": report}, indent=2, sort_keys=True))
        text.configure(state="disabled")

    def run(self) -> None:
        self.root.mainloop()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--status", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    PostRunWindow(args.status, args.report).run()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
