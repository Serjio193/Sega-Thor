"""Controls and identity-bound status line for live session maps."""

from __future__ import annotations

from pathlib import Path
from tkinter import filedialog, ttk

from live_session_progress import LiveSessionProgressView


class LiveSessionViewMixin:
    def _init_live_session_view(self, path: Path | None) -> None:
        self.live_session_view = LiveSessionProgressView(path)

    def _build_live_session_status(self, parent) -> None:
        self.live_session_text = ttk.Label(
            parent, style="Metric.TLabel", text=self.live_session_view.label(None))
        self.live_session_text.grid(row=6, column=0, columnspan=2, sticky="w", pady=2)
        ttk.Button(parent, text="Session progress…",
                   command=self._choose_live_session_progress).grid(
                       row=6, column=2, sticky="e", pady=2)

    def _choose_live_session_progress(self) -> None:
        filename = filedialog.askopenfilename(
            title="Select live session progress sidecar",
            filetypes=(("Live session progress", "live-session-progress.json"),
                       ("JSON", "*.json"), ("All files", "*.*")))
        if filename:
            self.live_session_view.set_path(Path(filename))

    def _refresh_live_session_status(self) -> None:
        identity = next((mapping.identity for mapping in
                         (self.current, self.canonical, self.overlay) if mapping), None)
        sha256 = str(identity.get("rom_sha256")) if identity else None
        self.live_session_text.configure(text=self.live_session_view.label(sha256))
