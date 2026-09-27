"""Progress state for the read-only LIVE checkpoint watcher."""

from __future__ import annotations

from rom_coverage_model import newly_classified, newly_covered


def _age(seconds: float) -> str:
    seconds = max(0, int(seconds))
    minutes, remainder = divmod(seconds, 60)
    return f"{minutes}:{remainder:02d}" if minutes else f"{remainder}s"


class LiveProgress:
    """Summarize checkpoint refreshes without implying capture activity."""

    def __init__(self) -> None:
        self.last_checkpoint_at: float | None = None
        self.last_progress_at: float | None = None
        self.covered_total = 0
        self.classified_total = 0
        self.latest_covered = 0
        self.latest_classified = 0
        self.error: str | None = None

    def record(self, previous: bytes | None, current: bytes, now: float) -> None:
        self.last_checkpoint_at = now
        self.latest_covered = 0
        self.latest_classified = 0
        self.error = None
        if previous is None:
            return
        self.latest_covered = newly_covered(current, previous)
        self.latest_classified = newly_classified(current, previous)
        self.covered_total += self.latest_covered
        self.classified_total += self.latest_classified
        if self.latest_covered or self.latest_classified:
            self.last_progress_at = now

    def fail(self, error: Exception) -> None:
        self.error = str(error)

    def label(self, mode: str, pending: bool, now: float) -> str:
        if mode != "LIVE":
            return "MONITOR OFF · select LIVE to watch checkpoint updates"
        if self.error:
            return f"LIVE · checkpoint refresh failed · {self.error}"
        if pending:
            return "LIVE · reading changed checkpoint in background…"
        if self.last_checkpoint_at is None:
            return "LIVE · waiting for a readable checkpoint"
        progress = (f"session progress +{self.covered_total:,} covered / "
                    f"+{self.classified_total:,} classified bytes")
        if self.last_progress_at is None:
            progress += " · no coverage increase seen yet"
        else:
            progress += f" · last increase {_age(now - self.last_progress_at)} ago"
        return f"LIVE · checkpoint checked {_age(now - self.last_checkpoint_at)} ago · {progress}"
