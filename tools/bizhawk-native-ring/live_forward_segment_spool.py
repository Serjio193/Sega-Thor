"""Append host-audited FLOW_V1 segments to a bounded-memory disk spool."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any

from live_forward_scaling_audit import RECORD


class LiveForwardSegmentSpool:
    """Index each accepted capture without keeping run history in Python memory."""

    def __init__(self, directory: Path):
        self.directory = Path(directory).resolve()
        self.directory.mkdir(parents=True, exist_ok=True)
        self.raw_path = self.directory / "flow-v1-records.bin"
        self.index_path = self.directory / "flow-v1-segments.jsonl"
        if self.raw_path.exists() or self.index_path.exists():
            raise FileExistsError("refusing to overwrite continuous FLOW_V1 spool")
        self._raw = self.raw_path.open("xb")
        try:
            self._index = self.index_path.open("x", encoding="utf-8", newline="\n")
        except Exception:
            self._raw.close()
            self.raw_path.unlink(missing_ok=True)
            raise
        self._offset = 0
        self._segments = 0
        self._raw_hash = hashlib.sha256()
        self._index_hash = hashlib.sha256()
        self._closed = False

    def admit(self, segment: dict[str, object], rows: list[tuple[int, ...]],
              records_blob: bytes) -> None:
        if self._closed:
            raise RuntimeError("cannot append to closed FLOW_V1 spool")
        if segment.get("valid") is not True or segment.get("ready_for_cartographer") is not True:
            raise ValueError("spool accepts only host-audited FLOW_V1 segments")
        count = int(segment["record_count"])
        records_hash = hashlib.sha256(records_blob).hexdigest()
        if count != len(rows) or len(records_blob) != count * RECORD.size or \
                records_hash != segment.get("records_sha256"):
            raise ValueError("FLOW_V1 spool bytes, rows, or digest do not reconcile")
        item: dict[str, Any] = {"segment": dict(segment), "raw_offset": self._offset,
                                "raw_length": len(records_blob), "raw_sha256": records_hash}
        encoded = (json.dumps(item, sort_keys=True, separators=(",", ":")) + "\n").encode("utf-8")
        self._raw.write(records_blob)
        self._index.write(encoded.decode("utf-8"))
        self._raw.flush()
        self._index.flush()
        self._raw_hash.update(records_blob)
        self._index_hash.update(encoded)
        self._offset += len(records_blob)
        self._segments += 1

    def close(self) -> dict[str, object]:
        if not self._closed:
            self._raw.flush()
            self._index.flush()
            self._raw.close()
            self._index.close()
            self._closed = True
        return {"segments": self._segments, "raw_bytes": self._offset,
                "raw_sha256": self._raw_hash.hexdigest(),
                "index_sha256": self._index_hash.hexdigest(),
                "raw_path": str(self.raw_path), "index_path": str(self.index_path)}
