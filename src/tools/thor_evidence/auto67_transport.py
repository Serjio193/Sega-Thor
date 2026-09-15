"""Bounded AUTO67 status-to-dispatch transport cursor."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any, Iterable


@dataclass(frozen=True)
class TransportStats:
    accepted: int = 0
    duplicate: int = 0
    stale: int = 0
    invalid: int = 0
    epoch_changes: int = 0


class PreDispatchTransport:
    """Consume replaceable Lua snapshots without creating an event backlog.

    The transport keeps only the latest identity cursor.  Events are returned
    directly to the caller for immediate Dispatcher ingestion; no raw event is
    retained here after the call returns.
    """

    def __init__(self) -> None:
        self._last_identity: tuple[int, int, str] | None = None
        self._stats = TransportStats()

    @staticmethod
    def _identity(event: dict[str, Any], snapshot: dict[str, Any]) -> tuple[int, int, str] | None:
        try:
            epoch = int(event.get("epoch", snapshot.get("epoch", 1)))
            sequence = int(event["seq"])
        except (KeyError, TypeError, ValueError):
            return None
        if epoch < 0 or sequence < 0:
            return None
        expected = f"epoch={epoch}:seq={sequence}"
        explicit = event.get("occurrence_id")
        if explicit is not None and str(explicit) != expected:
            return None
        occurrence = str(explicit or expected)
        if not occurrence:
            return None
        return epoch, sequence, occurrence

    @staticmethod
    def _events(snapshot: dict[str, Any]) -> Iterable[Any]:
        events = snapshot.get("events")
        if (not isinstance(events, list) or
                (not events and isinstance(snapshot.get("discovery"), list))):
            events = snapshot.get("discovery", ())
        return events if isinstance(events, list) else ()

    def consume(self, snapshot: dict[str, Any]) -> list[dict[str, Any]]:
        """Return unseen events from one replaceable status snapshot."""
        accepted: list[dict[str, Any]] = []
        for candidate in self._events(snapshot):
            if not isinstance(candidate, dict):
                self._stats = self._stats.__class__(
                    self._stats.accepted, self._stats.duplicate,
                    self._stats.stale, self._stats.invalid + 1,
                    self._stats.epoch_changes)
                continue
            identity = self._identity(candidate, snapshot)
            if identity is None:
                self._stats = self._stats.__class__(
                    self._stats.accepted, self._stats.duplicate,
                    self._stats.stale, self._stats.invalid + 1,
                    self._stats.epoch_changes)
                continue
            if self._last_identity is not None:
                previous_epoch, previous_seq, _ = self._last_identity
                if identity == self._last_identity:
                    self._stats = self._stats.__class__(
                        self._stats.accepted, self._stats.duplicate + 1,
                        self._stats.stale, self._stats.invalid,
                        self._stats.epoch_changes)
                    continue
                if identity[0] < previous_epoch or (identity[0] == previous_epoch and
                                                    identity[1] <= previous_seq):
                    self._stats = self._stats.__class__(
                        self._stats.accepted, self._stats.duplicate,
                        self._stats.stale + 1, self._stats.invalid,
                        self._stats.epoch_changes)
                    continue
                changes = self._stats.epoch_changes + int(identity[0] != previous_epoch)
            else:
                changes = self._stats.epoch_changes
            item = dict(candidate)
            item.setdefault("epoch", identity[0])
            item.setdefault("occurrence_id", identity[2])
            accepted.append(item)
            self._last_identity = identity
            self._stats = self._stats.__class__(
                self._stats.accepted + 1, self._stats.duplicate,
                self._stats.stale, self._stats.invalid, changes)
        return accepted

    def snapshot(self) -> dict[str, Any]:
        """Expose counters and one cursor; never expose retained raw events."""
        return {"last_identity": self._last_identity,
                "accepted": self._stats.accepted,
                "duplicate": self._stats.duplicate,
                "stale": self._stats.stale,
                "invalid": self._stats.invalid,
                "epoch_changes": self._stats.epoch_changes}
