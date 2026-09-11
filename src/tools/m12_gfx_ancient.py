"""Independent Ancient graphics decoder and candidate classifier for M12-GFX-1."""

from __future__ import annotations

import hashlib
from dataclasses import dataclass


class DecodeError(ValueError):
    """The input is not a safely terminated Ancient stream."""


@dataclass(frozen=True)
class DecodedStream:
    start: int
    end: int
    mode: str
    output: bytes
    block_declarations: tuple[int, ...]

    @property
    def consumed(self) -> int:
        return self.end - self.start


class _Reader:
    def __init__(self, data: bytes, start: int, limit: int):
        if start < 0 or start >= limit or limit > len(data):
            raise DecodeError("invalid source bounds")
        self.data = data
        self.start = start
        self.limit = limit
        self.pos = start

    def read(self) -> int:
        if self.pos >= self.limit:
            raise DecodeError("source read past ROM")
        value = self.data[self.pos]
        self.pos += 1
        return value

    def peek(self) -> int:
        if self.pos >= self.limit:
            raise DecodeError("source peek past ROM")
        return self.data[self.pos]

    def skip(self, count: int) -> None:
        if count < 0 or self.pos + count > self.limit:
            raise DecodeError("source header past ROM")
        self.pos += count

    def read_many(self, count: int) -> bytes:
        if count < 0 or self.pos + count > self.limit:
            raise DecodeError("source block exceeds ROM")
        result = self.data[self.pos:self.pos + count]
        self.pos += count
        return result


class _Output:
    def __init__(self, cap: int):
        self.data = bytearray()
        self.cap = cap

    def write(self, value: int) -> None:
        if len(self.data) >= self.cap:
            raise DecodeError("output exceeds sanity cap")
        self.data.append(value)

    def write_many(self, values: bytes) -> None:
        if len(self.data) + len(values) > self.cap:
            raise DecodeError("output exceeds sanity cap")
        self.data.extend(values)

    def copy(self, distance: int, count: int) -> None:
        if distance <= 0 or distance > len(self.data):
            raise DecodeError("history underflow")
        for _ in range(count):
            self.write(self.data[-distance])


class _Bits:
    def __init__(self, reader: _Reader):
        self.reader = reader
        self.bits = reader.read()
        self.remaining = 8

    def bit(self) -> int:
        self.remaining -= 1
        if self.remaining < 0:
            self.bits = self.reader.read() | (self.reader.read() << 8)
            self.remaining = 15
        value = self.bits & 1
        self.bits >>= 1
        return value

    def code(self, count: int) -> int:
        value = 0
        for _ in range(count):
            value = (value << 1) | self.bit()
        return value


def _command_stream(reader: _Reader, output: _Output) -> tuple[int, ...]:
    declarations: list[int] = []
    while True:
        block_start = reader.pos
        declared = reader.read() | (reader.read() << 8)
        block_end = block_start + declared
        if declared < 2 or block_end > reader.limit:
            raise DecodeError("invalid command block declaration")
        declarations.append(declared)
        while reader.pos < block_end:
            command = reader.read()
            if command & 0x80:
                command &= 0x7F
                count = ((command & 0x60) >> 5) + 4
                distance = ((command & 0x1F) << 8) | reader.read()
                output.copy(distance, count)
                while reader.pos < block_end and reader.peek() & 0xE0 == 0x60:
                    extension = reader.read() & 0x1F
                    output.copy(distance, extension or 256)
                continue
            repeat = bool(command & 0x40)
            command &= 0x3F
            if repeat:
                extended = bool(command & 0x10)
                counter = command & 0x0F
                if extended:
                    counter = (counter << 8) | reader.read()
                value = reader.read()
                output.write_many(bytes((value,)) * (counter + 4))
                continue
            extended = bool(command & 0x20)
            count = command & 0x1F
            if extended:
                count = (count << 8) | reader.read()
            output.write_many(reader.read_many(count or 65536))
        if reader.pos != block_end:
            raise DecodeError("command crossed declared block")
        if reader.read() == 0:
            return tuple(declarations)


def _match_length(bits: _Bits, reader: _Reader) -> int:
    if bits.bit():
        return 2
    if bits.bit():
        return 3
    if bits.bit():
        return 4
    if bits.bit():
        return 5
    if bits.bit():
        return bits.code(3) + 6
    return reader.read() + 14


def _bit_stream(reader: _Reader, output: _Output) -> tuple[int, ...]:
    declarations: list[int] = []
    while True:
        block_start = reader.pos
        reader.skip(3)
        bits = _Bits(reader)
        while True:
            if not bits.bit():
                output.write(reader.read())
                continue
            if not bits.bit():
                distance = reader.read()
            else:
                distance = (bits.code(5) << 8) | reader.read()
                if distance == 0:
                    break
                if distance == 1:
                    extended = bits.bit()
                    count = bits.code(4)
                    if extended:
                        count = (count << 8) | reader.read()
                    value = reader.read()
                    output.write_many(bytes((value,)) * (count + 14))
                    continue
            output.copy(distance, _match_length(bits, reader))
        declarations.append(reader.pos - block_start)
        if reader.read() == 0:
            return tuple(declarations)


def decode(data: bytes, start: int, *, limit: int | None = None,
           output_cap: int = 0x40000) -> DecodedStream:
    """Decode one self-terminating stream without trusting external boundaries."""
    if limit is None:
        limit = len(data)
    if start + 4 > limit:
        raise DecodeError("stream header is truncated")
    reader = _Reader(data, start, limit)
    output = _Output(output_cap)
    if data[start + 2]:
        mode = "command"
        declarations = _command_stream(reader, output)
    else:
        mode = "bit"
        declarations = _bit_stream(reader, output)
    return DecodedStream(start, reader.pos, mode, bytes(output.data), declarations)


def deterministic_decode(data: bytes, start: int, *, limit: int | None = None) -> DecodedStream:
    first = decode(data, start, limit=limit)
    second = decode(data, start, limit=limit)
    if (first.end, first.mode, first.output, first.block_declarations) != (
            second.end, second.mode, second.output, second.block_declarations):
        raise DecodeError("non-deterministic decode")
    return first


def _cram_word(word: int) -> bool:
    return (word & ~0x0EEE) == 0


def classify(output: bytes) -> dict[str, object]:
    size = len(output)
    tiles = size // 32
    tile_hashes = {output[i:i + 32] for i in range(0, tiles * 32, 32)}
    zero_density = output.count(0) / size if size else 1.0
    words = [int.from_bytes(output[i:i + 2], "big") for i in range(0, size - 1, 2)]
    palette_words = sum(_cram_word(word) for word in words)
    palette_ratio = palette_words / len(words) if words else 0.0
    tilemap_words = sum((word & 0x7FF) < 0x400 and (word >> 13) < 4 for word in words)
    tilemap_ratio = tilemap_words / len(words) if words else 0.0
    if size >= 32 and size % 32 == 0 and zero_density < 0.98:
        classification = "GFX_TILE_CANDIDATE"
    elif size >= 8 and palette_ratio >= 0.9:
        classification = "PALETTE_CANDIDATE"
    elif size >= 8 and tilemap_ratio >= 0.9:
        classification = "TILEMAP_CANDIDATE"
    else:
        classification = "COMPRESSED_GENERIC"
    return {
        "output_size_mod_32": size % 32,
        "tile_count": tiles,
        "unique_tile_count": len(tile_hashes),
        "tile_repetition_ratio": 1.0 - len(tile_hashes) / tiles if tiles else 0.0,
        "transparent_zero_density": zero_density,
        "palette_like_word_ratio": palette_ratio,
        "tilemap_like_word_ratio": tilemap_ratio,
        "classification": classification,
    }


def output_sha256(output: bytes) -> str:
    return hashlib.sha256(output).hexdigest()
