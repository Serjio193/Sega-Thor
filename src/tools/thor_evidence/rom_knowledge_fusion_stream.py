"""Bounded-memory reader for selected top-level arrays in large JSON artifacts."""

from __future__ import annotations

import json
import mmap
from pathlib import Path
import re
from typing import Any, Iterator


class _Stream:
    def __init__(self, path: Path, chunk: int = 1 << 20):
        self.file = path.open("r", encoding="utf-8")
        self.chunk = chunk
        self.buffer = ""
        self.pos = 0
        self.eof = False
        self.decoder = json.JSONDecoder()

    def fill(self) -> None:
        self.buffer = self.buffer[self.pos:]
        self.pos = 0
        data = self.file.read(self.chunk)
        self.buffer += data
        self.eof = not data

    def spaces(self) -> None:
        while True:
            while self.pos < len(self.buffer) and self.buffer[self.pos].isspace():
                self.pos += 1
            if self.pos < len(self.buffer) or self.eof:
                return
            self.fill()

    def char(self) -> str:
        self.spaces()
        if self.pos >= len(self.buffer):
            return ""
        result = self.buffer[self.pos]
        self.pos += 1
        return result

    def value(self) -> Any:
        while True:
            self.spaces()
            try:
                value, end = self.decoder.raw_decode(self.buffer, self.pos)
                self.pos = end
                return value
            except json.JSONDecodeError:
                if self.eof:
                    raise
                self.fill()

    def skip(self) -> None:
        self.spaces()
        if self.pos >= len(self.buffer):
            raise ValueError("STOP_FUSION_JSON_TRUNCATED")
        if self.buffer[self.pos] not in "[{":
            self.value()
            return
        depth, quoted, escaped = 0, False, False
        while True:
            if self.pos >= len(self.buffer):
                if self.eof:
                    raise ValueError("STOP_FUSION_JSON_TRUNCATED")
                self.fill()
                continue
            char = self.buffer[self.pos]
            self.pos += 1
            if quoted:
                if escaped:
                    escaped = False
                elif char == "\\":
                    escaped = True
                elif char == '"':
                    quoted = False
            elif char == '"':
                quoted = True
            elif char in "[{":
                depth += 1
            elif char in "]}":
                depth -= 1
                if depth == 0:
                    return

    def arrays(self, names: set[str]) -> Iterator[tuple[str, int, Any]]:
        if self.char() != "{":
            raise ValueError("STOP_FUSION_JSON_OBJECT_EXPECTED")
        while True:
            token = self.char()
            if token == "}":
                return
            if token == ",":
                token = self.char()
            if token != '"':
                raise ValueError("STOP_FUSION_JSON_KEY_EXPECTED")
            self.pos -= 1
            key = self.value()
            if self.char() != ":":
                raise ValueError("STOP_FUSION_JSON_COLON_EXPECTED")
            if key not in names:
                self.skip()
                continue
            if self.char() != "[":
                raise ValueError("STOP_FUSION_JSON_ARRAY_EXPECTED:" + str(key))
            index = 0
            while True:
                token = self.char()
                if token == "]":
                    break
                if token == ",":
                    token = self.char()
                self.pos -= 1
                yield str(key), index, self.value()
                index += 1


def iter_json_arrays(path: Path, names: set[str]) -> Iterator[tuple[str, int, Any]]:
    stream = _Stream(path)
    try:
        yield from stream.arrays(names)
    finally:
        stream.file.close()


def iter_target_instructions(path: Path, pcs: set[int], per_pc_limit: int = 1) -> Iterator[tuple[int, dict[str, Any]]]:
    """Select a deterministic bounded slice without decoding unrelated FLOW records."""
    if not pcs or per_pc_limit <= 0:
        return
    with path.open("rb") as stream:
        with mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
            start_match = re.search(rb'"instructions"\s*:\s*\[', data)
            if start_match is None:
                raise ValueError("STOP_FUSION_INSTRUCTIONS_ARRAY_MISSING")
            end_match = re.compile(rb'\]\s*,\s*"memory"\s*:').search(data, start_match.end())
            if end_match is None:
                raise ValueError("STOP_FUSION_INSTRUCTIONS_ARRAY_END_MISSING")
            numbers = b"|".join(str(value).encode("ascii") for value in sorted(pcs))
            pattern = re.compile(rb'"pc"\s*:\s*(?:' + numbers + rb')(?:\s*[,}])')
            counts = {pc: 0 for pc in pcs}
            positions: list[tuple[int, dict[str, Any]]] = []
            for match in pattern.finditer(data, start_match.end(), end_match.start()):
                event_start = data.rfind(b"{", start_match.end(), match.start())
                event_end = data.find(b"}", match.end())
                if event_start < 0 or event_end < 0:
                    raise ValueError("STOP_FUSION_INSTRUCTION_EVENT_MALFORMED")
                event = json.loads(data[event_start:event_end + 1])
                pc = int(event.get("pc", -1))
                if pc not in counts or counts[pc] >= per_pc_limit:
                    continue
                positions.append((match.start(), event))
                counts[pc] += 1
                if all(count >= per_pc_limit for count in counts.values()):
                    break
            yield from positions
