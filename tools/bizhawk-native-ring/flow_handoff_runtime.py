"""Runtime-owned R7 FLOW producer/consumer boundary."""
from __future__ import annotations

from pathlib import Path
import threading
from typing import Any

from flow_stream import BoundedFlowHandoff, FlowChunk
from live_forward_control_provenance_stage import ControlProvenanceStream
from stage5_stream import Stage5StreamConsumer


class FlowHandoffRuntime:
    """Fan one validated FLOW stream into Stage 5 and Stage 6 concurrently."""

    def __init__(self, rom: Path, decoder: Path, identity: str, max_chunks: int = 32):
        self._handoff = BoundedFlowHandoff(max_chunks)
        self._rom = Path(rom)
        self._decoder = Path(decoder)
        self._identity = identity
        self._thread = threading.Thread(target=self._consume, name="flow-r7-consumer",
                                         daemon=False)
        self.stage5_memory = None
        self._stage5_builder: Stage5StreamConsumer | None = None
        self.stage6_result: dict[str, Any] | None = None
        self.error: BaseException | None = None

    def start(self) -> None:
        self._thread.start()

    def submit(self, segment: dict[str, Any], rows: list[tuple[int, ...]], blob: bytes) -> None:
        self._handoff.submit(segment, rows, blob)

    def close(self) -> None:
        self._handoff.close()
        self._thread.join()
        if self.error is not None:
            raise RuntimeError("R7 FLOW handoff consumer failed") from self.error
        if self._stage5_builder is None:
            raise RuntimeError("R7 FLOW handoff consumer produced no Stage 5 state")
        self._stage5_builder.raw_sha256 = self.stats()["raw_sha256"]
        self.stage5_memory = self._stage5_builder.finish()

    def fail(self, error: BaseException) -> None:
        self._handoff.fail(error)
        self._thread.join()

    def stats(self) -> dict[str, Any]:
        value = self._handoff.stats()
        return {"mode": value.mode, "segments": value.segments, "records": value.records,
                "chunks_transferred": value.segments, "logical_bytes": value.raw_bytes,
                "raw_bytes": value.raw_bytes, "raw_sha256": value.raw_sha256,
                "logical_sha256": value.logical_sha256, "disk_reads": value.disk_reads,
                "disk_writes": value.disk_writes, "flow_disk_read_bytes": 0,
                "flow_disk_write_bytes": 0, "buffer_peak_chunks": value.buffer_peak_chunks,
                "buffer_peak_bytes": value.buffer_peak_bytes,
                "fallback": value.fallback, "persistent_temp_files": 0,
                "legacy_flow_path": "NOT_USED", "sqlite_writes_bytes": 0}

    def _consume(self) -> None:
        stage5 = Stage5StreamConsumer(self._rom, self._decoder, self._identity)
        self._stage5_builder = stage5
        stage6 = ControlProvenanceStream(self._rom.read_bytes())
        try:
            for chunk in self._handoff.consume():
                stage5.accept(chunk)
                stage6.accept(chunk.segment, chunk.rows)
            stats = self.stats()
            identity = {"run_id": stage5.run_id, "raw_path": None,
                        "raw_sha256": stats["raw_sha256"], "index_path": None,
                        "index_sha256": stats["logical_sha256"], "segments": stats["segments"]}
            self.stage6_result = stage6.finish(None, identity, stats["segments"])
        except BaseException as error:
            self.error = error
            self._handoff.fail(error)


__all__ = ["FlowHandoffRuntime"]
