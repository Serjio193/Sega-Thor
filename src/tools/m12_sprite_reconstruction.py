"""Fail-closed Genesis hardware-sprite reconstruction helpers for M12.

This module is developer-only. It consumes emulator snapshots or synthetic
fixtures and never reads, writes, or embeds the canonical ROM by itself.
"""

from dataclasses import dataclass
import hashlib
from typing import Iterable, Mapping, Sequence


SCHEMA = "oasis.m68k.m12-sprite-reconstruction.v1"
MAX_SAT_ENTRIES = 80


class ReconstructionError(ValueError):
    """Input is incomplete, malformed, or fails a provenance contract."""


@dataclass(frozen=True)
class TileAttributes:
    tile_index: int
    palette: int
    priority: bool
    flip_h: bool
    flip_v: bool


@dataclass(frozen=True)
class SpritePixel:
    """One visible sprite pixel with its CRAM line and priority metadata."""

    palette_line: int
    pixel_index: int
    priority: bool

    def __post_init__(self) -> None:
        if not 0 <= self.palette_line <= 3:
            raise ReconstructionError("sprite palette line is outside 0..3")
        if not 1 <= self.pixel_index <= 15:
            raise ReconstructionError("visible sprite pixel is outside 1..15")

    @property
    def cram_index(self) -> int:
        return self.palette_line * 16 + self.pixel_index


@dataclass(frozen=True)
class SpritePiece:
    x: int
    y: int
    width_cells: int
    height_cells: int
    tile: TileAttributes
    link: int = 0


def _u16(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 2 > len(data):
        raise ReconstructionError("word read outside snapshot")
    return (data[offset] << 8) | data[offset + 1]


def decode_tile_attributes(value: int) -> TileAttributes:
    if not 0 <= value <= 0xFFFF:
        raise ReconstructionError("tile attribute is not a 16-bit word")
    return TileAttributes(value & 0x7FF, (value >> 13) & 3,
                          bool(value & 0x8000), bool(value & 0x0800),
                          bool(value & 0x1000))


def decode_sat(data: bytes, base: int, count: int = MAX_SAT_ENTRIES) -> list[SpritePiece]:
    """Decode raw 8-byte Genesis SAT entries without game semantics."""
    if count < 0 or count > MAX_SAT_ENTRIES:
        raise ReconstructionError("SAT count is outside the Genesis limit")
    result: list[SpritePiece] = []
    for index in range(count):
        offset = base + index * 8
        word0, word1 = _u16(data, offset), _u16(data, offset + 2)
        word2, word3 = _u16(data, offset + 4), _u16(data, offset + 6)
        width = ((word1 >> 10) & 3) + 1
        height = ((word1 >> 8) & 3) + 1
        result.append(SpritePiece(word3 & 0x1FF, word0 & 0x1FF, width,
                                  height, decode_tile_attributes(word2),
                                  word1 & 0x7F))
    return result


def active_sat_chain(entries: Sequence[SpritePiece], first: int = 0) -> list[SpritePiece]:
    """Follow hardware links and reject loops or out-of-range links."""
    if not entries or first < 0 or first >= len(entries):
        raise ReconstructionError("SAT chain start is outside decoded entries")
    result: list[SpritePiece] = []
    seen: set[int] = set()
    index = first
    while True:
        if index in seen or index >= len(entries):
            raise ReconstructionError("SAT chain is cyclic or out of range")
        seen.add(index)
        result.append(entries[index])
        index = entries[index].link
        if index == 0:
            break
    return result


def decode_genesis_tile(data: bytes, offset: int = 0) -> tuple[tuple[int, ...], ...]:
    """Decode one packed-nibble Genesis 4bpp tile."""
    if offset < 0 or offset + 32 > len(data):
        raise ReconstructionError("Genesis tile requires 32 bytes")
    rows: list[tuple[int, ...]] = []
    for row in range(8):
        packed = data[offset + row * 4:offset + row * 4 + 4]
        pixels: list[int] = []
        for value in packed:
            pixels.extend(((value >> 4) & 0x0F, value & 0x0F))
        rows.append(tuple(pixels))
    return tuple(rows)


def decode_genesis_palette(data: bytes, offset: int = 0) -> tuple[tuple[int, int, int], ...]:
    if offset < 0 or offset + 32 > len(data):
        raise ReconstructionError("Genesis palette requires 32 bytes")
    result = []
    for index in range(16):
        word = _u16(data, offset + index * 2)
        result.append(tuple((((word >> shift) & 7) * 255 + 3) // 7
                            for shift in (1, 5, 9)))
    return tuple(result)


def _validate_tile(tile: tuple[tuple[int, ...], ...]) -> None:
    if len(tile) != 8 or any(len(row) != 8 for row in tile):
        raise ReconstructionError("missing or malformed tile in piece")
    if any(value < 0 or value > 15 for row in tile for value in row):
        raise ReconstructionError("tile pixel is outside 4bpp range")


def compose_piece(
    piece: SpritePiece,
    tiles: Mapping[int, tuple[tuple[int, ...], ...]],
) -> list[list[SpritePixel | None]]:
    """Compose one SAT piece using Genesis column-major pattern order."""
    if not 1 <= piece.width_cells <= 4 or not 1 <= piece.height_cells <= 4:
        raise ReconstructionError("sprite dimensions must be 1..4 cells")
    pixels: list[list[SpritePixel | None]] = [
        [None] * (piece.width_cells * 8) for _ in range(piece.height_cells * 8)
    ]
    for cell_y in range(piece.height_cells):
        for cell_x in range(piece.width_cells):
            source_cell_x = piece.width_cells - 1 - cell_x if piece.tile.flip_h else cell_x
            source_cell_y = piece.height_cells - 1 - cell_y if piece.tile.flip_v else cell_y
            tile_index = (piece.tile.tile_index
                          + source_cell_x * piece.height_cells
                          + source_cell_y)
            tile = tiles.get(tile_index)
            if tile is None:
                raise ReconstructionError("missing or malformed tile in piece")
            _validate_tile(tile)
            for y in range(8):
                for x in range(8):
                    source_x = 7 - x if piece.tile.flip_h else x
                    source_y = 7 - y if piece.tile.flip_v else y
                    pixel_index = tile[source_y][source_x]
                    if pixel_index:
                        pixels[cell_y * 8 + y][cell_x * 8 + x] = SpritePixel(
                            piece.tile.palette, pixel_index, piece.tile.priority)
    return pixels


def compose_frame(
    pieces: Iterable[SpritePiece],
    tiles: Mapping[int, tuple[tuple[int, ...], ...]],
) -> list[list[SpritePixel | None]]:
    """Compose SAT-chain pieces; earlier sprites win visible overlaps.

    ``pieces`` must be in active SAT-chain order. Pixel zero is transparent;
    sprite-vs-background priority and scanline limits are intentionally not
    modeled here.
    """
    pieces = list(pieces)
    if not pieces:
        raise ReconstructionError("logical frame has no sprite pieces")
    left = min(piece.x for piece in pieces)
    top = min(piece.y for piece in pieces)
    right = max(piece.x + piece.width_cells * 8 for piece in pieces)
    bottom = max(piece.y + piece.height_cells * 8 for piece in pieces)
    if right <= left or bottom <= top or right - left > 512 or bottom - top > 512:
        raise ReconstructionError("logical frame bounds are unreasonable")
    frame: list[list[SpritePixel | None]] = [
        [None] * (right - left) for _ in range(bottom - top)
    ]
    for piece in pieces:
        pixels = compose_piece(piece, tiles)
        for y, row in enumerate(pixels):
            for x, value in enumerate(row):
                target_y = piece.y - top + y
                target_x = piece.x - left + x
                if value is not None and frame[target_y][target_x] is None:
                    frame[target_y][target_x] = value
    return frame


def reconstruction_metadata() -> dict[str, object]:
    """Return explicit limits of this deterministic reconstruction model."""
    return {
        "sprite_vs_sprite_order": "earlier_sat_chain_sprite_wins",
        "sprite_vs_background_priority_modelled": False,
        "scanline_limit_modelled": False,
    }


def validate_provenance(snapshot: Mapping[str, object], rom: bytes) -> dict[str, object]:
    expected = hashlib.sha256(rom).hexdigest()
    if snapshot.get("schema") != SCHEMA:
        raise ReconstructionError("unsupported reconstruction schema")
    if str(snapshot.get("canonical_rom_sha256", "")).lower() != expected:
        raise ReconstructionError("snapshot ROM identity does not match input ROM")
    capture = str(snapshot.get("capture_sha256", ""))
    if len(capture) != 64 or any(char not in "0123456789abcdefABCDEF" for char in capture):
        raise ReconstructionError("capture identity is required")
    if snapshot.get("writes_emitted") is not False:
        raise ReconstructionError("state-writing capture is not accepted")
    return {"schema": SCHEMA, "canonical_rom_sha256": expected,
            "capture_sha256": capture.lower(),
            "provenance_status": "PASS"}
