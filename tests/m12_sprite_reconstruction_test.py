"""Synthetic regression tests for M12 hardware-sprite reconstruction."""

from pathlib import Path
import hashlib
import importlib.util


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_sprite_reconstruction", ROOT / "src/tools/m12_sprite_reconstruction.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_tile_and_palette_decoding() -> None:
    row = bytes.fromhex("12 34 56 78")
    tile = row * 8
    pixels = MODULE.decode_genesis_tile(tile)
    assert pixels[0] == (1, 2, 3, 4, 5, 6, 7, 8)
    assert pixels == tuple((1, 2, 3, 4, 5, 6, 7, 8) for _ in range(8))

    # This is the same packed-nibble contract as the authoritative C++ decoder.
    assert pixels == tuple(tuple((value >> shift) & 0x0F
                                 for value in byte for shift in (4, 0))
                           for byte in (row,) * 8)
    palette = bytearray(32)
    palette[:2] = bytes((0x0E, 0xEE))
    assert MODULE.decode_genesis_palette(bytes(palette))[0] == (255, 255, 255)


def test_sat_decoding_and_piece_flips() -> None:
    sat = bytes((0x00, 0x10, 0x05, 0x01, 0xA8, 0x00, 0x00, 0x20))
    entry = MODULE.decode_sat(sat, 0, 1)[0]
    assert (entry.x, entry.y, entry.width_cells, entry.height_cells) == (0x20, 0x10, 2, 2)
    tile = tuple(tuple(range(1, 9)) for _ in range(8))
    flipped = MODULE.SpritePiece(0, 0, 1, 1,
                                 MODULE.TileAttributes(0, 0, False, True, True))
    result = MODULE.compose_piece(flipped, {0: tile})
    assert result[0][0].pixel_index == 8 and result[0][7].pixel_index == 1


def _constant_tiles(count: int) -> dict[int, tuple[tuple[int, ...], ...]]:
    return {index: ((index + 1,) * 8,) * 8 for index in range(count)}


def _tile_layout(width: int, height: int, flip_h: bool = False,
                 flip_v: bool = False) -> list[list[int]]:
    piece = MODULE.SpritePiece(
        0, 0, width, height,
        MODULE.TileAttributes(0, 0, False, flip_h, flip_v))
    image = MODULE.compose_piece(piece, _constant_tiles(width * height))
    return [[image[y * 8][x * 8].pixel_index - 1
             for x in range(width)] for y in range(height)]


def test_column_major_tile_addressing() -> None:
    assert _tile_layout(1, 4) == [[0], [1], [2], [3]]
    assert _tile_layout(4, 1) == [[0, 1, 2, 3]]
    assert _tile_layout(2, 3) == [[0, 3], [1, 4], [2, 5]]
    assert _tile_layout(3, 2) == [[0, 2, 4], [1, 3, 5]]


def test_non_square_flips_and_priority_metadata() -> None:
    assert _tile_layout(2, 3, flip_h=True) == [[3, 0], [4, 1], [5, 2]]
    assert _tile_layout(2, 3, flip_v=True) == [[2, 5], [1, 4], [0, 3]]
    assert _tile_layout(2, 3, flip_h=True, flip_v=True) == [[5, 2], [4, 1], [3, 0]]
    tile = MODULE.compose_piece(
        MODULE.SpritePiece(0, 0, 1, 1, MODULE.TileAttributes(0, 3, True, False, False)),
        {0: ((5,) * 8,) * 8})
    pixel = tile[0][0]
    assert pixel.palette_line == 3 and pixel.pixel_index == 5
    assert pixel.cram_index == 53 and pixel.priority


def test_palette_lines_and_overlap_order() -> None:
    tiles = {0: ((5,) * 8,) * 8, 1: ((9,) * 8,) * 8, 2: ((12,) * 8,) * 8}
    pixels = []
    for palette in range(4):
        piece = MODULE.compose_piece(
            MODULE.SpritePiece(0, 0, 1, 1,
                               MODULE.TileAttributes(0, palette, False, False, False)),
            tiles)
        pixels.append(piece[0][0])
    assert [pixel.cram_index for pixel in pixels] == [5, 21, 37, 53]
    first = MODULE.SpritePiece(0, 0, 1, 1, MODULE.TileAttributes(0, 0, False, False, False))
    second = MODULE.SpritePiece(0, 0, 1, 1, MODULE.TileAttributes(1, 0, False, False, False))
    third = MODULE.SpritePiece(0, 0, 1, 1, MODULE.TileAttributes(2, 0, False, False, False))
    frame = MODULE.compose_frame([first, second, third], tiles)
    assert frame[0][0].pixel_index == 5
    transparent = {0: ((0,) * 8,) * 8}
    exposed = MODULE.compose_frame([first, second], {**transparent, 1: tiles[1]})
    assert exposed[0][0].pixel_index == 9


def test_malformed_tiles_and_model_limits() -> None:
    try:
        MODULE.decode_genesis_tile(bytes(31))
    except MODULE.ReconstructionError:
        pass
    else:
        raise AssertionError("short tile was accepted")
    assert MODULE.reconstruction_metadata() == {
        "sprite_vs_sprite_order": "earlier_sat_chain_sprite_wins",
        "sprite_vs_background_priority_modelled": False,
        "scanline_limit_modelled": False,
    }


def test_chain_frame_and_provenance_fail_closed() -> None:
    first = MODULE.SpritePiece(0, 0, 1, 1, MODULE.TileAttributes(0, 0, False, False, False), 2)
    second = MODULE.SpritePiece(8, 0, 1, 1, MODULE.TileAttributes(1, 0, False, False, False), 0)
    entries = [MODULE.SpritePiece(0, 0, 1, 1, MODULE.TileAttributes(0, 0, False, False, False), 2), first, second]
    assert MODULE.active_sat_chain(entries) == [entries[0], second]
    assert MODULE.active_sat_chain(entries, 1) == [first, second]
    piece = MODULE.SpritePiece(-2, -1, 1, 1, MODULE.TileAttributes(0, 0, False, False, False))
    frame = MODULE.compose_frame([piece], {0: ((1,) * 8,) * 8})
    assert len(frame) == 8 and len(frame[0]) == 8
    try:
        MODULE.compose_piece(piece, {})
    except MODULE.ReconstructionError:
        pass
    else:
        raise AssertionError("missing tile was accepted")
    rom = b"synthetic-rom"
    snapshot = {"schema": MODULE.SCHEMA,
                "canonical_rom_sha256": hashlib.sha256(rom).hexdigest(),
                "capture_sha256": hashlib.sha256(b"capture").hexdigest(), "writes_emitted": False}
    assert MODULE.validate_provenance(snapshot, rom)["provenance_status"] == "PASS"
    for key, value in (("capture_sha256", ""), ("writes_emitted", True)):
        bad = dict(snapshot)
        bad[key] = value
        try:
            MODULE.validate_provenance(bad, rom)
        except MODULE.ReconstructionError:
            pass
        else:
            raise AssertionError("invalid provenance was accepted")


if __name__ == "__main__":
    test_tile_and_palette_decoding()
    test_sat_decoding_and_piece_flips()
    test_column_major_tile_addressing()
    test_non_square_flips_and_priority_metadata()
    test_palette_lines_and_overlap_order()
    test_malformed_tiles_and_model_limits()
    test_chain_frame_and_provenance_fail_closed()
    print("m12 sprite reconstruction tests passed")
