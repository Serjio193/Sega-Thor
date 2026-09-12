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
    tile = bytearray(32)
    tile[0] = 0x80
    tile[1] = 0x80
    tile[2] = 0x80
    tile[3] = 0x80
    pixels = MODULE.decode_genesis_tile(bytes(tile))
    assert pixels[0] == (15, 0, 0, 0, 0, 0, 0, 0)
    palette = bytearray(32)
    palette[:2] = bytes((0x0E, 0xEE))
    assert MODULE.decode_genesis_palette(bytes(palette))[0] == (255, 255, 255)


def test_sat_decoding_and_piece_flips() -> None:
    sat = bytes((0x00, 0x10, 0x05, 0x01, 0xA8, 0x00, 0x00, 0x20))
    entry = MODULE.decode_sat(sat, 0, 1)[0]
    assert (entry.x, entry.y, entry.width_cells, entry.height_cells) == (0x20, 0x10, 2, 2)
    tile = tuple(tuple(range(8)) for _ in range(8))
    flipped = MODULE.SpritePiece(0, 0, 1, 1,
                                 MODULE.TileAttributes(0, 0, False, True, True))
    result = MODULE.compose_piece(flipped, {0: tile})
    assert result[0][0] == 7 and result[0][7] == 0


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
    test_chain_frame_and_provenance_fail_closed()
    print("m12 sprite reconstruction tests passed")
