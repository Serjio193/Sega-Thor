"""Read-only Defraggler-style viewer for sealed ROM property checkpoints."""

from __future__ import annotations

import argparse
from concurrent.futures import Future, ThreadPoolExecutor
import hashlib
from pathlib import Path
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
import time

from rom_coverage_model import COLORS, Checkpoint, coverage
from rom_coverage_model import classification_coverage
from rom_coverage_model import load_checkpoint, load_overlay_checkpoint
from rom_coverage_model import newly_classified, newly_covered
from rom_coverage_model import newly_covered_cells
from rom_coverage_evidence import CanonicalEvidenceIndex, STATUS_NAMES
from rom_coverage_evidence import resolve_knowledge_pointer
from rom_coverage_gui_evidence import RangeDetailsMixin, add_evidence_legend
from rom_coverage_gui_evidence import choose_overlay
from rom_coverage_gui_live import LiveProgress
from rom_coverage_gui_session import LiveSessionViewMixin
from rom_coverage_gui_map import RomCoverageMapMixin


class RomCoverageApp(RangeDetailsMixin, LiveSessionViewMixin, RomCoverageMapMixin):
    def __init__(self, root: tk.Tk, checkpoint_path: Path | None,
                 canonical_path: Path | None, evidence_pointer: Path | None,
                 overlay_receipt_path: Path | None = None,
                 live_session_progress_path: Path | None = None):
        self.root = root
        self.root.title("ROM Coverage Map")
        self.root.geometry("1180x780")
        self.current: Checkpoint | None = None
        self.canonical: Checkpoint | None = None
        self.overlay: Checkpoint | None = None
        self.checkpoint_path = checkpoint_path
        self.canonical_path = canonical_path
        self.evidence_pointer = evidence_pointer
        self.overlay_receipt_path = overlay_receipt_path
        self.evidence: CanonicalEvidenceIndex | None = None
        self.rom_name = "ROM name unavailable (checkpoint stores hash only)"
        self.rom_name_hash: str | None = None
        self.mode = tk.StringVar(value="CHECKPOINT")
        self.source = tk.StringVar(value="Current run only")
        self.filter_name = tk.StringVar(value="All properties")
        self.level = 1
        self.region: tuple[int, int] | None = None
        self.cells: list[tuple[int, int, int, int]] = []
        self.rectangles: list[int] = []
        self.hovered: int | None = None
        self.tooltip: tk.Toplevel | None = None
        self.executor = ThreadPoolExecutor(max_workers=1, thread_name_prefix="rom-map-reader")
        self.pending: Future[Checkpoint] | None = None
        self.pending_mtime: int | None = None
        self.evidence_pending: Future[CanonicalEvidenceIndex] | None = None
        self.evidence_pending_mtime: int | None = None
        self.evidence_mtime: int | None = None
        self.last_poll = 0.0
        self.last_mtime: int | None = None
        self.live_progress = LiveProgress()
        self._init_live_session_view(live_session_progress_path)
        self._build_ui()
        if checkpoint_path or overlay_receipt_path:
            self._load_paths()
        self.root.after(250, self._tick)

    def _build_ui(self) -> None:
        self.root.configure(bg="#171b22")
        style = ttk.Style()
        style.theme_use("clam")
        style.configure("TFrame", background="#171b22")
        style.configure("TLabel", background="#171b22", foreground="#e5e9f0")
        style.configure("Title.TLabel", font=("Segoe UI", 16, "bold"))
        style.configure("Metric.TLabel", font=("Segoe UI", 11))
        style.configure("TButton", padding=5)

        header = ttk.Frame(self.root, padding=(14, 12, 14, 6))
        header.pack(fill="x")
        ttk.Label(header, text="ROM Coverage Map", style="Title.TLabel").pack(side="left")
        self.mode_label = ttk.Label(header, text="CHECKPOINT", foreground="#7dd3fc")
        self.mode_label.pack(side="right", padx=8)

        controls = ttk.Frame(self.root, padding=(14, 3, 14, 8))
        controls.pack(fill="x")
        ttk.Button(controls, text="Open run checkpoint…", command=self._choose_current).pack(side="left")
        ttk.Button(controls, text="Open canonical union…", command=self._choose_canonical).pack(side="left", padx=5)
        ttk.Button(controls, text="Open overlay receipt…", command=self._choose_overlay).pack(side="left", padx=5)
        ttk.Button(controls, text="Verify ROM name…", command=self._choose_rom).pack(side="left", padx=5)
        ttk.Button(controls, text="Evidence map…", command=self._choose_evidence).pack(side="left", padx=5)
        ttk.Button(controls, text="LIVE", command=lambda: self._set_mode("LIVE")).pack(side="left", padx=(18, 2))
        ttk.Button(controls, text="CHECKPOINT", command=lambda: self._set_mode("CHECKPOINT")).pack(side="left", padx=2)
        ttk.Button(controls, text="CANONICAL", command=lambda: self._set_mode("CANONICAL")).pack(side="left", padx=2)
        ttk.Label(controls, text="Show:").pack(side="left", padx=(18, 4))
        source_box = ttk.Combobox(controls, textvariable=self.source, state="readonly", width=18,
                                  values=("Current run only", "Canonical union", "Overlay receipt"))
        source_box.pack(side="left")
        source_box.bind("<<ComboboxSelected>>", lambda _event: self._redraw())
        ttk.Label(controls, text="Filter:").pack(side="left", padx=(12, 4))
        filters = ("All properties", "M68K", "Z80", "VDP", "Graphics", "Audio", "Unknown only",
                   "Observed only", "Classified only", "Mixed only")
        filter_box = ttk.Combobox(controls, textvariable=self.filter_name, state="readonly", width=17,
                                  values=filters)
        filter_box.pack(side="left")
        filter_box.bind("<<ComboboxSelected>>", lambda _event: self._redraw())
        ttk.Button(controls, text="Whole ROM", command=self._reset_zoom).pack(side="right")

        self.identity = ttk.Label(self.root, text="No checkpoint loaded", justify="left", padding=(16, 4))
        self.identity.pack(fill="x")
        summary = ttk.Frame(self.root, padding=(16, 3, 16, 8))
        summary.pack(fill="x")
        for column in range(3):
            summary.columnconfigure(column, weight=1)
        self.total_text = ttk.Label(summary, style="Metric.TLabel")
        self.total_text.grid(row=0, column=0, sticky="w", pady=2)
        self.unknown_text = ttk.Label(summary, style="Metric.TLabel")
        self.unknown_text.grid(row=0, column=1, sticky="w", pady=2)
        self.run_text = ttk.Label(summary, style="Metric.TLabel")
        self.run_text.grid(row=0, column=2, sticky="w", pady=2)
        self.canonical_text = ttk.Label(summary, style="Metric.TLabel")
        self.canonical_text.grid(row=1, column=0, sticky="w", pady=2)
        self.new_cells_text = ttk.Label(summary, style="Metric.TLabel")
        self.new_cells_text.grid(row=1, column=1, sticky="w", pady=2)
        self.classified_text = ttk.Label(summary, style="Metric.TLabel")
        self.classified_text.grid(row=1, column=2, sticky="w", pady=2)
        self.observed_text = ttk.Label(summary, style="Metric.TLabel")
        self.observed_text.grid(row=3, column=0, columnspan=2, sticky="w", pady=2)
        self.new_classified_text = ttk.Label(summary, style="Metric.TLabel")
        self.new_classified_text.grid(row=2, column=0, columnspan=2, sticky="w", pady=2)
        self.evidence_text = ttk.Label(summary, style="Metric.TLabel")
        self.evidence_text.grid(row=4, column=0, columnspan=3, sticky="w", pady=2)
        self.progress_text = ttk.Label(summary, style="Metric.TLabel")
        self.progress_text.grid(row=5, column=0, columnspan=3, sticky="w", pady=2)
        self._build_live_session_status(summary)

        self.canvas = tk.Canvas(self.root, bg="#101319", highlightthickness=1,
                                highlightbackground="#303844")
        self.canvas.pack(fill="both", expand=True, padx=14, pady=(2, 8))
        self.canvas.bind("<Configure>", lambda _event: self._schedule_redraw())
        self.canvas.bind("<Motion>", self._on_motion)
        self.canvas.bind("<Leave>", lambda _event: self._hide_tooltip())
        self.canvas.bind("<Button-1>", self._on_click)
        self._make_legend()
        self.footer = ttk.Label(self.root, text="Click a cell to zoom. Hover for exact byte counts.", padding=(14, 4))
        self.footer.pack(fill="x")

    def _make_legend(self) -> None:
        legend = ttk.Frame(self.root, padding=(14, 4, 14, 10))
        legend.pack(fill="x")
        for index, (name, color) in enumerate(COLORS.items()):
            item = ttk.Frame(legend)
            item.grid(row=index // 3, column=index % 3, sticky="w", padx=(0, 14), pady=1)
            swatch = tk.Canvas(item, width=13, height=13, bg=color, highlightthickness=1,
                               highlightbackground="#d0d5dc")
            swatch.pack(side="left", padx=(0, 4))
            label = {
                "M68K_DATA_READ": "M68K DATA READ (OBSERVED)",
                "Z80_DATA_READ": "Z80 DATA READ (OBSERVED)",
                "OBSERVED_UNCLASSIFIED": "OBSERVED, UNCLASSIFIED",
            }.get(name, name.replace("_", " "))
            ttk.Label(item, text=label, font=("Segoe UI", 8),
                      wraplength=330).pack(side="left")
        add_evidence_legend(legend, tk, ttk)

    def _choose_current(self) -> None:
        filename = filedialog.askopenfilename(title="Select ROM property checkpoint",
            filetypes=(("ROM property checkpoint", "*.rom-properties.v1 *.bin *.checkpoint"), ("All files", "*.*")))
        if filename:
            self.checkpoint_path = Path(filename)
            self._load_paths()

    def _choose_canonical(self) -> None:
        filename = filedialog.askopenfilename(title="Select canonical union checkpoint",
            filetypes=(("ROM property checkpoint", "*.rom-properties.v1 *.bin *.checkpoint"), ("All files", "*.*")))
        if filename:
            self.canonical_path = Path(filename)
            self._load_paths()

    def _choose_overlay(self) -> None:
        choose_overlay(self)

    def _choose_rom(self) -> None:
        filename = filedialog.askopenfilename(title="Select ROM to verify its displayed name",
            filetypes=(("ROM image", "*.md *.bin *.gen *.smd"), ("All files", "*.*")))
        if not filename or not self.current:
            return
        path = Path(filename)
        try:
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            if digest != self.current.identity["rom_sha256"]:
                raise ValueError("Selected ROM SHA-256 does not match the checkpoint")
            self.rom_name = path.name
            self.rom_name_hash = digest
            self._update_identity()
        except Exception as error:
            messagebox.showerror("ROM identity mismatch", str(error))

    def _choose_evidence(self) -> None:
        filename = filedialog.askopenfilename(title="Select published M14 or MASTER V2 current.json",
            filetypes=(("Canonical generation pointer", "current.json"), ("JSON", "*.json"),
                       ("All files", "*.*")))
        if filename:
            self.evidence_pointer = Path(filename)
            self.evidence_mtime = None
            self._schedule_evidence_load()

    def _load_paths(self) -> None:
        try:
            if self.overlay_receipt_path and self.checkpoint_path:
                self.overlay = load_overlay_checkpoint(self.overlay_receipt_path,
                                                       self.checkpoint_path)
                self.mode.set("CANONICAL")
                self.mode_label.configure(text="CANONICAL OVERLAY")
            if self.checkpoint_path:
                if not self.overlay_receipt_path:
                    self.current = load_checkpoint(self.checkpoint_path)
                    self.last_mtime = self.checkpoint_path.stat().st_mtime_ns
                    self.live_progress.record(None, self.current.masks, time.monotonic())
                    if self.rom_name_hash != self.current.identity["rom_sha256"]:
                        self.rom_name_hash = None
                        self.rom_name = "ROM name unavailable (checkpoint stores hash only)"
            if self.canonical_path:
                self.canonical = load_checkpoint(self.canonical_path)
            self._validate_pair()
            self._validate_evidence_identity()
            if self.evidence_pointer and not self.evidence:
                self._schedule_evidence_load()
            self._update_identity()
            self._update_summary()
            self._redraw()
        except Exception as error:  # display malformed/untrusted input as data error
            messagebox.showerror("Cannot load ROM map", str(error))

    def _validate_pair(self) -> None:
        if self.current and self.canonical:
            for key in ("rom_sha256", "rom_size", "schema_id", "contract_sha256",
                        "core_build_id", "capabilities", "validation_state"):
                if self.current.identity[key] != self.canonical.identity[key]:
                    raise ValueError(f"Current and canonical identity mismatch: {key}")

    def _validate_evidence_identity(self) -> None:
        active = self._current_map()
        if self.evidence and active and (
                self.evidence.rom_sha256 != active.identity["rom_sha256"] or
                self.evidence.rom_size != active.rom_size):
            self.evidence = None
            self.evidence_text.configure(text="CANONICAL EVIDENCE rejected: ROM identity changed")

    def _schedule_evidence_load(self) -> None:
        active = self._current_map()
        if not self.evidence_pointer or not active or self.evidence_pending:
            return
        stamp = self.evidence_pointer.stat().st_mtime_ns
        self.evidence_pending_mtime = stamp
        self.evidence_pending = self.executor.submit(CanonicalEvidenceIndex.load,
            self.evidence_pointer, str(active.identity["rom_sha256"]), active.rom_size)
        self.evidence_text.configure(text="CANONICAL EVIDENCE loading published generation…")

    def _set_mode(self, mode: str) -> None:
        self.mode.set(mode)
        self.mode_label.configure(text=mode)
        self.last_poll = 0
        self._redraw()

    def _current_map(self) -> Checkpoint | None:
        if self.source.get() == "Overlay receipt":
            return self.overlay
        if self.source.get() == "Canonical union" or self.mode.get() == "CANONICAL":
            return self.canonical or self.overlay
        return self.current

    def _update_identity(self) -> None:
        active = self._current_map()
        if not active:
            return
        self.identity.configure(text=(f"ROM: {self.rom_name}    SHA-256: {active.identity['rom_sha256']}\n"
            f"Size: {active.rom_size:,} bytes    Runtime build ID: {active.identity['core_build_id']}\n"
            f"Proof contract ID: {active.identity['contract_sha256']}    Run: {active.identity['run_id']}"))

    def _update_summary(self) -> None:
        active = self._current_map()
        if not active:
            return
        covered, total, pct = coverage(active.masks)
        unknown = total - covered
        classified, observed_only, _ = classification_coverage(active.masks)
        self.total_text.configure(text=f"TOTAL COVERAGE  {covered:,} / {total:,}  ({pct:.3f}%)")
        self.unknown_text.configure(text=f"UNKNOWN  {unknown:,}  ({100.0 - pct:.3f}%)")
        baseline = self.canonical
        if self.current and baseline:
            new_bytes = newly_covered(self.current.masks, baseline.masks)
            self.run_text.configure(
                text=f"THIS RUN  +{new_bytes:,} new bytes  ({new_bytes * 100.0 / total:.3f}%)")
            new_cells = newly_covered_cells(self.current.masks, baseline.masks)
            self.new_cells_text.configure(text=f"NEW CELLS  {new_cells}")
            new_classified = newly_classified(self.current.masks, baseline.masks)
            self.new_classified_text.configure(
                text=f"NEW CLASSIFIED  +{new_classified:,} bytes "
                     f"({new_classified * 100.0 / total:.3f}%)")
        else:
            self.run_text.configure(text="THIS RUN  delta unavailable (load canonical union)")
            self.new_cells_text.configure(text="NEW CELLS  baseline not loaded")
            self.new_classified_text.configure(text="NEW CLASSIFIED  baseline not loaded")
        if baseline:
            canon_covered = coverage(baseline.masks)[0]
            self.canonical_text.configure(text=f"CANONICAL TOTAL  {canon_covered:,} / {total:,}")
        else:
            self.canonical_text.configure(text="CANONICAL TOTAL  not loaded")
        self.classified_text.configure(
            text=f"CLASSIFIED  {classified:,} / {total:,} ({classified * 100.0 / total:.3f}%)")
        self.observed_text.configure(
            text=f"OBSERVED, UNCLASSIFIED  {observed_only:,} / {total:,} "
                 f"({observed_only * 100.0 / total:.3f}%)")
        if self.evidence:
            evidence = self.evidence.summarize(0, total, include_trace=False)
            parts = [f"{name} {evidence.counts[name]:,} B" for name in STATUS_NAMES
                     if evidence.counts[name]]
            self.evidence_text.configure(text=(
                f"CANONICAL EVIDENCE  {self.evidence.authority} · "
                f"{self.evidence.generation_id}  " +
                (" · ".join(parts) if parts else "no range-linked claims")))
        elif self.evidence_pointer:
            self.evidence_text.configure(text="CANONICAL EVIDENCE  loading or unavailable")
        else:
            self.evidence_text.configure(text="CANONICAL EVIDENCE  not loaded (use Evidence map…)")

    def _tick(self) -> None:
        now = time.monotonic()
        if self.mode.get() == "LIVE" and self.checkpoint_path and now - self.last_poll >= 0.5:
            self.last_poll = now
            try:
                stamp = self.checkpoint_path.stat().st_mtime_ns
                if stamp != self.last_mtime and self.pending is None:
                    self.pending = self.executor.submit(load_checkpoint, self.checkpoint_path)
                    self.pending_mtime = stamp
            except OSError as error:
                self.live_progress.fail(error)
        if self.mode.get() == "LIVE" and self.evidence_pointer and now - self.last_poll < 0.5:
            try:
                stamp = self.evidence_pointer.stat().st_mtime_ns
                if stamp != self.evidence_mtime and self.evidence_pending is None:
                    self._schedule_evidence_load()
            except OSError:
                pass
        if self.evidence_pending and self.evidence_pending.done():
            try:
                self.evidence = self.evidence_pending.result()
                self.evidence_mtime = self.evidence_pending_mtime
                self._validate_evidence_identity()
                self._redraw()
            except Exception as error:
                self.evidence_text.configure(text=f"CANONICAL EVIDENCE rejected: {error}")
            self.evidence_pending = None
        if self.pending and self.pending.done():
            try:
                previous = self.current
                self.current = self.pending.result()
                self.last_mtime = self.pending_mtime
                self._validate_pair()
                self._validate_evidence_identity()
                self.live_progress.record(previous.masks if previous else None,
                                          self.current.masks, time.monotonic())
                if self.evidence_pointer and not self.evidence:
                    self._schedule_evidence_load()
                self._redraw()
            except Exception as error:
                self.live_progress.fail(error)
                self.footer.configure(text=f"LIVE refresh rejected: {error}")
            self.pending = None
        self.progress_text.configure(text=self.live_progress.label(
            self.mode.get(), self.pending is not None, time.monotonic()))
        self._refresh_live_session_status()
        self.root.after(250, self._tick)

    def close(self) -> None:
        self.executor.shutdown(wait=False, cancel_futures=True)
        self.root.destroy()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("checkpoint", nargs="?", type=Path, help="Current run checkpoint")
    parser.add_argument("--canonical", type=Path, help="Accepted canonical union checkpoint")
    parser.add_argument("--knowledge-pointer", type=Path,
                        help="Override the default promoted MASTER V2 / M14 pointer")
    parser.add_argument("--overlay-receipt", type=Path,
                        help="Load checkpoint only when bound by this accepted receipt")
    parser.add_argument("--live-session-progress", type=Path,
                        help="Watch an exact-ROM live MAP-1 progress sidecar")
    args = parser.parse_args()
    project_root = Path(__file__).resolve().parents[2]
    knowledge_pointer = resolve_knowledge_pointer(project_root, args.knowledge_pointer)
    root = tk.Tk()
    app = RomCoverageApp(root, args.checkpoint, args.canonical, knowledge_pointer,
                         args.overlay_receipt, args.live_session_progress)
    root.protocol("WM_DELETE_WINDOW", app.close)
    root.mainloop()


if __name__ == "__main__":
    main()
