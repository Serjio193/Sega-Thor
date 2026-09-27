"""Read-only ROM property/evidence map drawing and cell interaction."""

from __future__ import annotations

import tkinter as tk

from rom_coverage_model import COLORS, matches_filter, summarize_cell
from rom_coverage_evidence import STATUS_COLORS


LEVEL_BYTES = {1: 1024, 2: 32, 3: 1}
CELL_GAP = 2


class RomCoverageMapMixin:
    def _schedule_redraw(self) -> None:
        if hasattr(self, "redraw_after"):
            self.root.after_cancel(self.redraw_after)
        self.redraw_after = self.root.after(90, self._redraw)

    def _redraw(self) -> None:
        self._update_identity()
        self._update_summary()
        active = self._current_map()
        self.canvas.delete("all")
        self.cells.clear()
        self.rectangles.clear()
        if not active:
            self.footer.configure(text="Load a canonical union checkpoint to view CANONICAL mode.")
            return
        rom_size = active.rom_size
        if self.level == 1:
            region_start, region_end, cell_bytes = 0, rom_size, LEVEL_BYTES[1]
        else:
            region_start, region_end = self.region or (0, rom_size)
            cell_bytes = LEVEL_BYTES[self.level]
        count = (region_end - region_start + cell_bytes - 1) // cell_bytes
        width = max(100, self.canvas.winfo_width())
        columns = max(1, width // 12)
        cell_w = max(2, min(12, (width - CELL_GAP) // columns - CELL_GAP))
        cell_h = cell_w
        for index in range(count):
            start = region_start + index * cell_bytes
            end = min(start + cell_bytes, region_end)
            summary = summarize_cell(active.masks, start, end)
            evidence_state = (self.evidence.state_for_range(start, end)
                              if self.evidence else "NONE")
            row, col = divmod(index, columns)
            x0 = CELL_GAP + col * (cell_w + CELL_GAP)
            y0 = CELL_GAP + row * (cell_h + CELL_GAP)
            x1, y1 = x0 + cell_w, y0 + cell_h
            color = COLORS[str(summary["visual_state"])]
            if not matches_filter(summary, self.filter_name.get()):
                color = "#303640"
            evidence_color = STATUS_COLORS[evidence_state] or "#171b22"
            rect = self.canvas.create_rectangle(x0, y0, x1, y1, fill=color,
                outline=evidence_color, width=2 if evidence_state != "NONE" else 1)
            self.cells.append((rect, start, end, index))
            self.rectangles.append(rect)
        rows = (count + columns - 1) // columns
        self.canvas.configure(scrollregion=(0, 0, width,
            max(0, rows * (cell_h + CELL_GAP) + CELL_GAP)))
        label = "whole ROM" if self.level == 1 else f"0x{region_start:06X}–0x{region_end - 1:06X}"
        action = "click byte for provenance trace" if self.level == 3 else "click to zoom"
        self.footer.configure(text=(f"Level {self.level} · {label} · {count:,} cells · "
                                    f"{action} · hover for counts"))

    def _cell_at(self, x: int, y: int) -> tuple[int, int, int, int] | None:
        items = self.canvas.find_overlapping(x, y, x, y)
        if not items:
            return None
        return next((cell for cell in self.cells if cell[0] in items), None)

    def _on_motion(self, event: tk.Event) -> None:
        cell = self._cell_at(event.x, event.y)
        if cell is None:
            self._hide_tooltip()
            return
        _, start, end, _ = cell
        summary = summarize_cell(self._current_map().masks, start, end)
        evidence = (self.evidence.summarize(start, end, include_trace=False)
                    if self.evidence else None)
        self._show_tooltip(event.x_root + 14, event.y_root + 14, summary, evidence)

    def _on_click(self, event: tk.Event) -> None:
        cell = self._cell_at(event.x, event.y)
        if not cell:
            return
        _, start, end, _ = cell
        if self.level < 3:
            self.region, self.level = (start, end), self.level + 1
        else:
            self._hide_tooltip()
            summary = summarize_cell(self._current_map().masks, start, end)
            evidence = self.evidence.summarize(start, end) if self.evidence else None
            self._show_trace_dialog(start, end, summary, evidence)
            return
        self._hide_tooltip()
        self._redraw()

    def _reset_zoom(self) -> None:
        self.level, self.region = 1, None
        self._redraw()
