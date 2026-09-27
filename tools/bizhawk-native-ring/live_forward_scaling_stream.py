"""Incremental reader for the append-only BizHawk/Lua status stream."""

from __future__ import annotations

from pathlib import Path
from typing import Iterable


def ordered_runtime_events(changed: dict[str, str]) -> Iterable[tuple[str, str]]:
    """Keep segment, lifecycle, and round-ACK events in append-stream order."""
    return ((key, value) for key, value in changed.items()
            if key.startswith(("SEG_", "LIFECYCLE_")) or
            (key.startswith("ROUND_") and key.endswith("_ACKED")))


class TailLines:
    """Read newly appended key/value rows without retaining per-cycle history."""

    def __init__(self, path: Path):
        self.path, self.offset, self.pending = path, 0, b""
        self.values: dict[str, str] = {}

    def poll(self, final: bool = False) -> dict[str, str]:
        if not self.path.exists():
            return {}
        with self.path.open("rb") as source:
            source.seek(self.offset)
            data = source.read()
            self.offset = source.tell()
        lines = (self.pending + data).split(b"\n")
        self.pending = lines.pop()
        if final and self.pending:
            lines.append(self.pending)
            self.pending = b""
        changed: dict[str, str] = {}
        for line in lines:
            key, separator, value = line.decode("ascii", errors="replace").partition("=")
            if separator:
                value = value.removesuffix("\r")
                changed[key] = value
                if key.startswith(("SEG_", "LIFECYCLE_")):
                    continue
                if key.startswith("ROUND_"):
                    field = key.split("_", 2)[2]
                    if field == "ACKED":
                        self.values["ROUND_ACKED"] = value
                    elif field == "AUDIT_STREAM_DELTA":
                        self.values["ROUND_AUDIT_STREAM_DELTA"] = value
                    elif field == "METRICS":
                        self.values["ROUND_METRICS"] = value
                    continue
                self.values[key] = value
        return changed
