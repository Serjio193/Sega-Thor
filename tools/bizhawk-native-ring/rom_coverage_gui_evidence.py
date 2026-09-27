"""Small presentation helpers for the canonical-evidence overlay."""

from __future__ import annotations

import tkinter as tk
from tkinter import filedialog, messagebox
from pathlib import Path

from rom_coverage_model import PROPERTIES
from rom_coverage_evidence import STATUS_COLORS
from rom_coverage_model import load_overlay_checkpoint


def choose_overlay(app) -> None:
    receipt = filedialog.askopenfilename(title="Select canonical overlay receipt",
        filetypes=(("Overlay receipt", "*.json"), ("All files", "*.*")))
    if not receipt:
        return
    checkpoint = filedialog.askopenfilename(
        title="Select the exact checkpoint named by the receipt",
        filetypes=(("ROM property checkpoint", "*.rom-properties.v1 *.bin *.checkpoint"),
                   ("All files", "*.*")))
    if not checkpoint:
        return
    try:
        app.overlay = load_overlay_checkpoint(receipt, checkpoint)
        app.overlay_receipt_path = Path(receipt)
        app.mode.set("CANONICAL")
        app.mode_label.configure(text="CANONICAL OVERLAY")
        app._validate_pair()
        app._update_identity()
        if app.evidence_pointer and not app.evidence:
            app._schedule_evidence_load()
        app._redraw()
    except Exception as error:
        messagebox.showerror("Cannot load overlay", str(error))


def add_evidence_legend(legend, tk, ttk) -> None:
    names = ("STATIC_VERIFIED", "DERIVED_EXACT", "OBSERVED_RUNTIME",
             "HYPOTHESIS", "CONFLICT", "OTHER_CLAIM")
    first_row = 4
    for index, name in enumerate(names):
        item = ttk.Frame(legend)
        item.grid(row=first_row + index // 3, column=index % 3,
                  sticky="w", padx=(0, 14), pady=1)
        tk.Canvas(item, width=13, height=13, bg="#101319", highlightthickness=2,
                  highlightbackground=STATUS_COLORS[name]).pack(side="left", padx=(0, 4))
        label = "UNMAPPED MASTER CLAIM" if name == "OTHER_CLAIM" else name.replace("_", " ")
        ttk.Label(item, text=f"EVIDENCE {label}",
                  font=("Segoe UI", 8)).pack(side="left")


def append_evidence_tooltip(body: str, evidence, generation_id: str,
                            authority: str) -> str:
    body += f"\n\nCANONICAL EVIDENCE ({authority} · {generation_id})\n"
    body += f"evidence state: {evidence.state}\n"
    body += "\n".join(f"{name}: {count} bytes" for name, count in evidence.counts.items())
    body += "\nObjects / claims:\n" + ("\n".join(evidence.facts) or "none in this range")
    if evidence.trace:
        body += "\n\nPROVENANCE TRACE (canonical links only):\n"
        body += "\n".join(evidence.trace)
    return body


def _range_detail_body(summary: dict[str, object], evidence,
                       generation_id: str, authority: str) -> str:
    body = (f"ROM range: 0x{summary['start']:06X}–0x{summary['end'] - 1:06X}\n"
            f"cell state: {summary['state']}\n"
            f"covered bytes: {summary['covered']} / {summary['total']}\n")
    if evidence:
        return append_evidence_tooltip(body, evidence, generation_id, authority)
    return body + "\nNo canonical evidence generation is loaded."


class RangeDetailsMixin:
    """Render one selected cell's byte properties and linked evidence."""

    def _show_tooltip(self, x: int, y: int, summary: dict[str, object], evidence=None) -> None:
        self._hide_tooltip()
        tip = tk.Toplevel(self.root)
        self.tooltip = tip
        tip.wm_overrideredirect(True)
        tip.geometry(f"+{x}+{y}")
        body = (f"ROM range: 0x{summary['start']:06X}–0x{summary['end'] - 1:06X}\n"
                f"cell state: {summary['state']}\n"
                f"covered bytes: {summary['covered']} / {summary['total']}\n"
                f"coverage: {summary['percent']:.2f}%\n")
        body += (f"classified: {summary['classified']}\n"
                 f"observed, unclassified: {summary['observed_unclassified']}\n")
        for _, name in PROPERTIES:
            body += f"{name}: {summary['counts'][name]}\n"
        body += f"UNKNOWN: {summary['unknown']}"
        if evidence:
            body = append_evidence_tooltip(body, evidence, self.evidence.generation_id,
                                           self.evidence.authority)
        tk.Label(tip, text=body, justify="left", bg="#202630", fg="#f2f4f8",
                 font=("Consolas", 9), padx=9, pady=7).pack()

    def _show_trace_dialog(self, start: int, end: int,
                           summary: dict[str, object], evidence=None) -> None:
        dialog = tk.Toplevel(self.root)
        dialog.title(f"ROM evidence trace · 0x{start:06X}–0x{end - 1:06X}")
        dialog.geometry("900x620")
        text = tk.Text(dialog, wrap="none", bg="#171b22", fg="#e5e9f0",
                       insertbackground="#e5e9f0", font=("Consolas", 10))
        vertical = tk.Scrollbar(dialog, orient="vertical", command=text.yview)
        horizontal = tk.Scrollbar(dialog, orient="horizontal", command=text.xview)
        text.configure(yscrollcommand=vertical.set, xscrollcommand=horizontal.set)
        text.grid(row=0, column=0, sticky="nsew")
        vertical.grid(row=0, column=1, sticky="ns")
        horizontal.grid(row=1, column=0, sticky="ew")
        dialog.rowconfigure(0, weight=1)
        dialog.columnconfigure(0, weight=1)
        generation_id = self.evidence.generation_id if self.evidence else "unavailable"
        authority = self.evidence.authority if self.evidence else "unavailable"
        text.insert("1.0", _range_detail_body(summary, evidence,
                                               generation_id, authority))
        text.configure(state="disabled")

    def _hide_tooltip(self) -> None:
        if self.tooltip:
            self.tooltip.destroy()
            self.tooltip = None
