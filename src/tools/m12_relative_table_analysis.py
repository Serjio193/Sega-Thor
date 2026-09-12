"""Prove the bounded consumer grammar rooted at ROM address 0x03BDA6."""
import hashlib
import json
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("m12_selector_descriptor_grammar", ROOT / "m12_selector_descriptor_grammar.py")
GRAMMAR = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GRAMMAR)

ROM_SIZE = 0x300000
ROM_SHA256 = GRAMMAR.ROM_SHA256
BASE = 0x03BDA6
POINTER_END = 0x03BDCA
CODE_BOUNDARY = 0x03BF86
WORDS = tuple(GRAMMAR.SHARED_TABLE_WORDS)
POINTER_BASES = dict(GRAMMAR.SHARED_TABLE_POINTERS)
CONSUMER_CONTRACTS = {
    0x03B3C2: bytes.fromhex("20680008"),
    0x03B3CE: bytes.fromhex("D040"),
    0x03B3D2: bytes.fromhex("D0D0"),
    0x03B3D4: bytes.fromhex("302E000A"),
    0x03B3D8: bytes.fromhex("3D7000000008"),
    0x03B3DE: bytes.fromhex("3D7000020006"),
    0x03B3F6: bytes.fromhex("082E00070006"),
    0x03B426: bytes.fromhex("2050"),
    0x03B42E: bytes.fromhex("D0C0"),
    0x03B436: bytes.fromhex("D0D0"),
    0x03B448: bytes.fromhex("4EB90000B730"),
}


def _hex(value, width=6):
    return f"0x{value:0{width}X}"


def _stream(rom, start):
    pairs = []
    address = start
    while address + 4 <= CODE_BOUNDARY:
        first = int.from_bytes(rom[address:address + 2], "big")
        second = int.from_bytes(rom[address + 2:address + 4], "big")
        pairs.append({"address": _hex(address), "first": _hex(first, 4), "second": _hex(second, 4)})
        if second & 0x8000:
            return {"status": "CLOSED_BY_BIT15_MARKER", "end": address + 4, "pairs": pairs}
        address += 4
    return {"status": "UNRESOLVED_BEFORE_CODE_BOUNDARY", "end": None, "pairs": pairs}


def parse_relative_table(rom):
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    GRAMMAR.parse_grammar(rom)
    for address, expected in CONSUMER_CONTRACTS.items():
        if rom[address:address + len(expected)] != expected:
            raise ValueError(f"relative consumer contract changed at {_hex(address)}")
    actual = tuple(int.from_bytes(rom[BASE + 2 * index:BASE + 2 * index + 2], "big")
                   for index in range(len(WORDS)))
    if actual != WORDS:
        raise ValueError("relative pointer words changed")
    entries = []
    for selector, pointer_base in POINTER_BASES.items():
        indices = sorted({record[0] for record in GRAMMAR.CHILD_TABLES[selector]["records"]})
        for index in indices:
            entry_address = pointer_base + index * 2
            offset = int.from_bytes(rom[entry_address:entry_address + 2], "big")
            entries.append({
                "selector": selector,
                "index_source": "child record word +0 -> A6+2",
                "index": index,
                "entry_address": _hex(entry_address),
                "offset_word": _hex(offset, 4),
                "target": _hex(BASE + offset),
                "width_bytes": 2,
            })
    streams = []
    for entry in entries:
        stream = _stream(rom, BASE + int(entry["offset_word"], 16))
        streams.append({**entry, **stream, "pair_count": len(stream["pairs"])})
    unresolved = [item for item in streams if item["status"] != "CLOSED_BY_BIT15_MARKER"]
    closed = [item for item in streams if item["status"] == "CLOSED_BY_BIT15_MARKER"]
    resolved_end = max((item["end"] for item in closed), default=None)
    return {
        "schema": "oasis.m68k.m12-relative-table-analysis.v1",
        "status": "GRAMMAR_PROVEN_EXTENT_BLOCKED_BY_ONE_UNTERMINATED_STREAM" if unresolved else "GRAMMAR_AND_EXTENT_PROVEN",
        "canonical_rom_sha256": ROM_SHA256,
        "root": {
            "base": _hex(BASE),
            "pointer_storage_extent": [_hex(BASE), _hex(POINTER_END)],
            "entry_count": len(WORDS),
            "entry_width_bytes": 2,
            "words": [_hex(value, 4) for value in actual],
            "pointer_bases": {str(key): _hex(value) for key, value in POINTER_BASES.items()},
        },
        "consumer_grammar": {
            "consumer_pcs": [_hex(address) for address in CONSUMER_CONTRACTS],
            "index_source": "child record word +0 copied to A6+2; MOVE.W; doubled by ADD.W D0,D0",
            "index_width_bytes": 2,
            "index_interpretation": "unsigned 16-bit selector index after MOVE.W; closed observed domain is child-record values",
            "base": "child +8 pointer, then base + 2*index",
            "relative_entry": "ADDA.W (A0),A0; signed 16-bit word displacement from selected pointer-table entry",
            "element_width_bytes": 4,
            "element_fields": ["word +0 -> A6+8", "word +2 -> A6+6"],
            "cursor": "A6+10 starts at zero and advances by 4 bytes",
            "termination": "A6+6 low 15 bits count down; BTST.B #7 at A6+6 observes word bit15; marker resets cursor, clear marker advances",
        },
        "consumed_pointer_entries": entries,
        "streams": streams,
        "extent": {
            "resolved_prefix_extent": [_hex(BASE + 0x24), _hex(resolved_end)] if resolved_end else None,
            "independent_code_boundary": _hex(CODE_BOUNDARY),
            "full_logical_extent": None if unresolved else [_hex(BASE), _hex(resolved_end)],
            "status": "BLOCKED_BY_SELECTOR_0_INDEX_1_STREAM" if unresolved else "PROVEN",
            "blocker": "stream target 0x03BDD8 has no word-bit15 terminator before 0x03BF86" if unresolved else None,
        },
        "join": "higher-level FF10AC state -> selector 0..7 -> descriptor +12 -> child +8 -> relative pair -> A6+8 -> child +0 offset table -> 0x03B448 -> 0x0000B730 -> SAT",
        "fail_closed": [
            "no semantic object/animation/frame label is assigned",
            "no full extent is claimed while 0x03BDD8 lacks a consumer terminator before the code boundary",
            "no ROM payload or decoded asset is emitted",
        ],
    }


def main():
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom")
    parser.add_argument("output")
    args = parser.parse_args()
    result = parse_relative_table(Path(args.rom).read_bytes())
    Path(args.output).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
