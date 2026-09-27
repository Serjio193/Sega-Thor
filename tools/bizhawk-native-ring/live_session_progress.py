"""Atomic sidecar for bounded live MAP-1 session progress."""

from __future__ import annotations

import json
import math
import os
from pathlib import Path
import time
from typing import Any


SCHEMA = "oasis.m12.live-forward-session-progress.v1"
STATES = {"STARTING", "RUNNING", "FINALIZING", "CLOSED", "FAILED"}
MAX_SNAPSHOT_BYTES = 16 * 1024


class LiveSessionProgressPublisher:
    def __init__(self, path: Path, session_id: str, rom_sha256: str,
                 expected_segments: int):
        self.path = Path(path)
        self.session_id = session_id
        self.rom_sha256 = _sha256(rom_sha256)
        self.expected_segments = _nonnegative(expected_segments, "expected_segments")
        if not session_id or self.expected_segments == 0:
            raise ValueError("session identity and expected segment count are required")
        if self.path.exists():
            raise FileExistsError(f"refusing to replace existing session progress: {self.path}")

    def publish(self, state: str, segments_admitted: int, segments_rejected: int,
                nodes: int, edges: int, source_owned_bytes: int = 0,
                error: str | None = None) -> None:
        if state not in STATES:
            raise ValueError("invalid session progress state")
        admitted = _nonnegative(segments_admitted, "segments_admitted")
        if admitted > self.expected_segments:
            raise ValueError("admitted segments exceed the configured run")
        payload = {
            "schema": SCHEMA,
            "session_id": self.session_id,
            "rom_sha256": self.rom_sha256,
            "state": state,
            "segments_admitted": admitted,
            "segments_expected": self.expected_segments,
            "segments_rejected": _nonnegative(segments_rejected, "segments_rejected"),
            "nodes": _nonnegative(nodes, "nodes"),
            "edges": _nonnegative(edges, "edges"),
            "source_owned_bytes": _nonnegative(source_owned_bytes, "source_owned_bytes"),
            "updated_at": time.time(),
        }
        if error:
            payload["error"] = " ".join(str(error).split())[:240]
        encoded = json.dumps(payload, sort_keys=True, separators=(",", ":")) + "\n"
        if len(encoded.encode("utf-8")) > MAX_SNAPSHOT_BYTES:
            raise ValueError("session progress snapshot exceeds its size limit")
        self.path.parent.mkdir(parents=True, exist_ok=True)
        temporary = self.path.with_name(self.path.name + ".tmp")
        try:
            with temporary.open("w", encoding="utf-8", newline="\n") as stream:
                stream.write(encoded)
                stream.flush()
                os.fsync(stream.fileno())
            os.replace(temporary, self.path)
        except Exception:
            temporary.unlink(missing_ok=True)
            raise


def read_live_session_progress(path: Path, expected_rom_sha256: str) -> dict[str, Any]:
    path = Path(path)
    if path.stat().st_size > MAX_SNAPSHOT_BYTES:
        raise ValueError("session progress snapshot exceeds its size limit")
    payload = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(payload, dict) or payload.get("schema") != SCHEMA:
        raise ValueError("unsupported session progress schema")
    if _sha256(str(payload.get("rom_sha256", ""))) != _sha256(expected_rom_sha256):
        raise ValueError("session progress ROM identity does not match the map")
    if not isinstance(payload.get("session_id"), str) or not payload["session_id"]:
        raise ValueError("session progress identity is missing")
    if payload.get("state") not in STATES:
        raise ValueError("session progress state is invalid")
    admitted = _nonnegative(payload.get("segments_admitted"), "segments_admitted")
    expected = _nonnegative(payload.get("segments_expected"), "segments_expected")
    if expected == 0 or admitted > expected:
        raise ValueError("session progress segment bounds are invalid")
    for field in ("segments_rejected", "nodes", "edges", "source_owned_bytes"):
        _nonnegative(payload.get(field), field)
    if "error" in payload and not isinstance(payload["error"], str):
        raise ValueError("session progress error is invalid")
    updated_at = payload.get("updated_at")
    if isinstance(updated_at, bool) or not isinstance(updated_at, (int, float)) or \
            not math.isfinite(updated_at):
        raise ValueError("session progress timestamp is invalid")
    if updated_at > time.time() + 5:
        raise ValueError("session progress timestamp is in the future")
    return payload


class LiveSessionProgressView:
    def __init__(self, path: Path | None = None):
        self.path = Path(path) if path else None
        self.error: str | None = None

    def set_path(self, path: Path) -> None:
        self.path = Path(path)
        self.error = None

    def label(self, rom_sha256: str | None, now: float | None = None) -> str:
        if self.path is None:
            return "SESSION MAP · select a live-session progress file"
        if not rom_sha256:
            return "SESSION MAP · load the matching ROM map before showing session data"
        try:
            payload = read_live_session_progress(self.path, rom_sha256)
            self.error = None
        except FileNotFoundError:
            return "SESSION MAP · waiting for the run to publish its first snapshot"
        except (OSError, ValueError, json.JSONDecodeError) as error:
            self.error = str(error)
            return f"SESSION MAP · rejected · {self.error}"
        now = time.time() if now is None else now
        age = max(0.0, now - float(payload["updated_at"]))
        state = str(payload["state"])
        if state == "RUNNING" and age > 5:
            state = "STALE"
        admitted = int(payload["segments_admitted"])
        expected = int(payload["segments_expected"])
        percent = admitted * 100.0 / expected
        error = f" · {payload['error']}" if payload.get("error") else ""
        return (f"SESSION MAP · {state} · admitted segments {admitted:,}/{expected:,} "
                f"({percent:.1f}%) · nodes {payload['nodes']:,} · edges {payload['edges']:,} "
                f"· session SOURCE_OWNED {payload['source_owned_bytes']:,} "
                f"· {age:.1f}s ago{error}")


def _nonnegative(value: object, field: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise ValueError(f"{field} must be a nonnegative integer")
    return value


def _sha256(value: str) -> str:
    normalized = value.lower()
    if len(normalized) != 64 or any(char not in "0123456789abcdef" for char in normalized):
        raise ValueError("invalid session progress ROM SHA-256")
    return normalized
