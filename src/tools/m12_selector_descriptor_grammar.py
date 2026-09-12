"""Emit the bounded, payload-free M12 selector/descriptor grammar."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location(
    "m12_static_sprite_dispatch_catalog",
    ROOT / "m12_static_sprite_dispatch_catalog.py",
)
DISPATCH = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(DISPATCH)

ROM_SHA256 = DISPATCH.ROM_SHA256
ROM_SIZE = 0x300000
SELECTOR_RAM = 0x00FFAFAE
DESCRIPTOR_START = 0x03B8DE
DESCRIPTOR_END = 0x03B95E
DECOMPRESSOR = 0x00003820

SELECTOR_XREFS = (
    (0x03A8C4, "writer_initializer", "33FCFFFF00FFAFAE"),
    (0x03A916, "writer_increment", "527900FFAFAE"),
    (0x03A91C, "read_bound_compare", "0C79000700FFAFAE"),
    (0x03A9AC, "read_test", "4A7900FFAFAE"),
    (0x03A9F2, "descriptor_index_read", "303900FFAFAE"),
    (0x03AA12, "primary_dispatch_index_read", "303900FFAFAE"),
    (0x03AA54, "primary_tail_index_test", "4A7900FFAFAE"),
    (0x03AA92, "secondary_dispatch_index_read", "303900FFAFAE"),
    (0x03B3B6, "shared_table_index_read", "303900FFAFAE"),
    (0x03B41A, "b730_table_index_read", "303900FFAFAE"),
)

KEY_CONTRACTS = {
    0x03A9EE: "41FA0EEE303900FFAFAEE940",
    0x03AA0E: "610007C0",
    0x03B236: "4EB900003820",
    0x03B28A: "4EB900003820",
    0x03B2FE: "4EB900003820",
    0x03B426: "2050",
    0x03B448: "4EB90000B730",
}

CHILD_TABLES = {
    0: {
        "start": 0x03B95C,
        "end": 0x03B982,
        "records": [
            [0x0001, 0x00A0, 0x0088, 0x8000],
            [0x0000, 0x00A0, 0x0078, 0x6000],
            [0x0002, 0x0088, 0x0058, 0xE000],
        ],
    },
    1: {"start": 0x03B998, "end": 0x03B9A6, "records": []},
    2: {
        "start": 0x03B982,
        "end": 0x03B998,
        "records": [[0x0000, 0x0080, 0x0088, 0x0000]],
    },
    3: {
        "start": 0x03B9A6,
        "end": 0x03B9BC,
        "records": [[0x0000, 0x0008, 0x00E0, 0x0000]],
    },
    4: {
        "start": 0x03B9BC,
        "end": 0x03B9D2,
        "records": [[0x0000, 0x0078, 0x0018, 0x0000]],
    },
    5: {
        "start": 0x03B9D2,
        "end": 0x03B9E8,
        "records": [[0x0000, 0x00A0, 0x0078, 0x0000]],
    },
    6: {
        "start": 0x03B9E8,
        "end": 0x03BA46,
        "records": [
            [0x0003, 0x0044, 0x0030, 0x0000],
            [0x0002, 0x00F8, 0x0030, 0x0000],
            [0x0003, 0x0044, 0x0030, 0x4000],
            [0x0002, 0x00F8, 0x0030, 0x2000],
            [0x0003, 0x0044, 0x0030, 0x4000],
            [0x0002, 0x00F8, 0x0030, 0x2000],
            [0x0003, 0x0044, 0x0030, 0x4000],
            [0x0002, 0x00F8, 0x0030, 0x2000],
            [0x0004, 0xFFD4, 0x0068, 0x0000],
            [0x0005, 0x00F0, 0x0068, 0x0000],
        ],
    },
}

SHARED_TABLE_WORDS = [
    0x0024, 0x0032, 0x0078, 0x00CA, 0x00D8, 0x00DA,
    0x00E8, 0x00F2, 0x014C, 0x015E, 0x0168, 0x0146,
    0x0178, 0x0196, 0x01B4, 0x01B6, 0x01B8, 0x01BA,
]

SHARED_TABLE_POINTERS = {
    0: 0x03BDA6,
    1: 0x03BDAE,
    2: 0x03BDAC,
    3: 0x03BDAE,
    4: 0x03BDB0,
    5: 0x03BDB2,
    6: 0x03BDBE,
}

DISPATCH_TARGETS = {
    "primary": {
        0x03AAAE: "state constants; call 0x03C956",
        0x03AB98: "state constants; call 0x03C956",
        0x03AC16: "state constants from FF AFBA; call 0x03C956",
        0x03AC6E: "state constants; call 0x03C956",
        0x03ACA8: "ROM source 0x17A750 -> 0x3820; call 0x03C956",
        0x03AD0C: "state constants from FF AFBA; call 0x03C956",
        0x03ADB4: "ROM source 0x17E3BA -> 0x3820; call 0x03C956",
        0x03AAEE: "timed state path; call 0x03C956 and BSR 0x03B832",
    },
    "secondary": {
        0x03AAEE: "timed state path; call 0x03C956 and BSR 0x03B832",
        0x03ABDA: "FF1348 update; conditional call 0x03C956",
        0x03AC68: "BSR 0x03B5E8",
        0x03AC92: "VDP register writes through FF AFDA/AFDE",
        0x03ACE4: "conditional JSR 0x008E32",
        0x03AD66: "FF1348 update; calls 0x03C956, 0x03B5E8, 0x03B092",
        0x03AE74: "state/font-like RAM path; calls 0x002EE2, 0x008E32, 0x002FDA",
        0x03BA46: "table-boundary alias; full target semantics unresolved",
    },
}


def _hex(value, width=6):
    return f"0x{value:0{width}X}"


def _read_word(rom, address):
    return int.from_bytes(rom[address:address + 2], "big")


def _read_long(rom, address):
    return int.from_bytes(rom[address:address + 4], "big")


def _check_bytes(rom, contracts):
    for address, expected_hex in contracts.items():
        expected = bytes.fromhex(expected_hex)
        if rom[address:address + len(expected)] != expected:
            raise ValueError(f"ROM contract changed at {_hex(address)}")


def _descriptor_records(rom):
    records = []
    for index in range(8):
        start = DESCRIPTOR_START + index * 0x10
        fields = [_read_long(rom, start + offset) for offset in (0, 4, 8, 12)]
        records.append({
            "selector": index,
            "address": _hex(start),
            "fields": [_hex(value, 8) for value in fields],
            "field_offsets": [0, 4, 8, 12],
            "child_table": (
                _hex(fields[3]) if index in CHILD_TABLES else None
            ),
            "child_status": (
                "FINITE_SENTINEL_TABLE" if index in CHILD_TABLES
                else "NON_ROM_LONGWORD_ADDRESS_UNRESOLVED"
            ),
        })
    return records


def _child_table(rom, selector, spec):
    start = spec["start"]
    end = spec["end"]
    header = [_read_long(rom, start + offset) for offset in (0, 4, 8)]
    actual_records = []
    cursor = start + 0x0C
    while cursor + 2 <= end and _read_word(rom, cursor) != 0xFFFF:
        actual_records.append([_read_word(rom, cursor + offset) for offset in (0, 2, 4, 6)])
        cursor += 8
    if cursor + 2 != end or _read_word(rom, cursor) != 0xFFFF:
        raise ValueError(f"child {selector} sentinel boundary is not closed")
    if actual_records != spec["records"]:
        raise ValueError(f"child {selector} record grammar changed")
    return {
        "selector": selector,
        "start": _hex(start),
        "end": _hex(end),
        "header": [_hex(value, 8) for value in header],
        "record_width": 8,
        "records": [
            {"offset": _hex(0x0C + index * 8, 4),
             "words": [_hex(value, 4) for value in record]}
            for index, record in enumerate(actual_records)
        ],
        "sentinel": {"offset": _hex(cursor - start, 4), "word": "0xFFFF"},
        "finite_extent_status": "CLOSED_BY_HIGH_BIT_SENTINEL",
        "header_pointer_roles": [
            "child_plus_0_used_by_B730_table_path",
            "child_plus_4_used_as_decompressor_source",
            "child_plus_8_used_as_shared_relative_word_table",
        ],
    }


def _field_use_graph():
    return [
        {
            "field_offset": 0,
            "path": "descriptor +0 -> RAM 0xFFAFA8 -> 0x03B7C0 path",
            "evidence": ["0x03A9FA", "0x03A9A2", "0x03AA4A"],
            "classification": "LONGWORD_USED_BY_B7C0_PATH",
        },
        {
            "field_offset": 4,
            "path": "descriptor +4 -> A3 -> 0x03B28A -> 0x00003820",
            "evidence": ["0x03AA02", "0x03B280", "0x03B28A"],
            "classification": "ROM_SOURCE_POINTER_TO_DECOMPRESSOR",
        },
        {
            "field_offset": 8,
            "path": "descriptor +8 -> A4 -> 0x03B2FE -> 0x00003820",
            "evidence": ["0x03AA06", "0x03B2F4", "0x03B2FE"],
            "classification": "ROM_SOURCE_POINTER_TO_DECOMPRESSOR",
        },
        {
            "field_offset": 12,
            "path": "descriptor +12 -> A5 -> child records and child +4 decoder",
            "evidence": ["0x03AA0A", "0x03B1F6", "0x03B236"],
            "classification": "CHILD_STATE_RECORD_TABLE",
        },
        {
            "field_offset": 12,
            "path": "descriptor +12 -> child +8 -> relative word -> state fields",
            "evidence": ["0x03B3BE", "0x03B3D2", "0x03B422"],
            "classification": "SHARED_RELATIVE_WORD_TABLE",
        },
        {
            "field_offset": 12,
            "path": "descriptor +12 -> child +0 -> relative table -> 0x0000B730",
            "evidence": ["0x03B426", "0x03B436", "0x03B448"],
            "classification": "B730_INPUT_TABLE",
        },
    ]


def _overlap_proof():
    return [
        {
            "left": "primary_dispatch",
            "right": "secondary_dispatch",
            "range": [_hex(0x03B8C2), _hex(0x03B8C6)],
            "resolution": "physical_alias; keep logical contracts separate",
        },
        {
            "left": "secondary_dispatch",
            "right": "descriptor_table",
            "range": [_hex(0x03B8DE), _hex(0x03B8E2)],
            "resolution": "secondary index 7 aliases descriptor record 0 field +0",
        },
        {
            "left": "child_6",
            "right": "secondary_dispatch_index_7_target",
            "range": [_hex(0x03BA46), _hex(0x03BA46)],
            "resolution": "child sentinel end equals target entry address",
        },
    ]


def parse_grammar(rom):
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    _check_bytes(rom, {address: expected for address, _, expected in SELECTOR_XREFS})
    _check_bytes(rom, KEY_CONTRACTS)
    base = DISPATCH.parse_catalog(rom)
    children = [_child_table(rom, selector, CHILD_TABLES[selector])
                for selector in range(7)]
    shared_base = 0x03BDA6
    shared_actual = [_read_word(rom, shared_base + index * 2)
                     for index in range(len(SHARED_TABLE_WORDS))]
    if shared_actual != SHARED_TABLE_WORDS:
        raise ValueError("shared relative table candidate changed")
    xrefs = [
        {"pc": _hex(address), "role": role, "bytes": expected,
         "width": 2, "direct_absolute": True}
        for address, role, expected in SELECTOR_XREFS
    ]
    target_contracts = []
    for family, targets in DISPATCH_TARGETS.items():
        target_contracts.append({
            "family": family,
            "targets": [
                {"target": _hex(address), "static_observation": observation}
                for address, observation in targets.items()
            ],
        })
    result = {
        "schema": "oasis.m68k.m12-selector-descriptor-grammar.v1",
        "status": "STATIC_FINITE_GRAPH_WITH_UNRESOLVED_SEMANTICS",
        "canonical_rom": {"size": ROM_SIZE, "sha256": ROM_SHA256},
        "selector": {
            "ram_address": _hex(SELECTOR_RAM, 8),
            "domain": {"min": 0, "max": 7, "mask": "0x0007"},
            "direct_absolute_xrefs": xrefs,
            "writers": [
                {"pc": _hex(address), "role": role, "bytes": expected}
                for address, role, expected in SELECTOR_XREFS
                if role.startswith("writer_")
            ],
            "writer_incoming_edge": "UNKNOWN_INDIRECT_OR_ENCLOSING",
            "generation_contract": (
                "initializer -1, increment after timer path, compare against 7; "
                "higher-level object/state source is not proven"
            ),
        },
        "descriptor_table": {
            "start": _hex(DESCRIPTOR_START),
            "end": _hex(DESCRIPTOR_END),
            "record_count": 8,
            "record_width": 0x10,
            "selector_stride": 0x10,
            "records": _descriptor_records(rom),
        },
        "descriptor_consumer": {
            "setup_pc": _hex(0x03A9EE),
            "selector_read_pc": _hex(0x03A9F2),
            "field_offsets": [0, 4, 8, 12],
            "consumer_pc": _hex(0x03AA0E),
            "consumer_target": _hex(0x03B1D0),
            "decompressor": _hex(DECOMPRESSOR),
            "decompressor_calls": [_hex(address) for address in sorted(DISPATCH.DECOMPRESSOR_CALLS)],
        },
        "field_use_graph": _field_use_graph(),
        "child_tables": children,
        "selector_7_child": {
            "value": "0xFFFF0017",
            "classification": "NON_ROM_LONGWORD_ADDRESS_UNRESOLVED",
            "reason": "descriptor +12 is loaded into A5 but does not identify a ROM child table",
        },
        "shared_relative_table_candidate": {
            "base": _hex(shared_base),
            "pointer_targets": {str(key): _hex(value) for key, value in SHARED_TABLE_POINTERS.items()},
            "word_count_observed": len(SHARED_TABLE_WORDS),
            "words": [_hex(value, 4) for value in shared_actual],
            "extent_status": "PARTIAL_CANDIDATE_NO_COMPLETE_CONSUMER_BOUNDARY",
        },
        "dispatch_target_contracts": target_contracts,
        "overlap_proof": _overlap_proof(),
        "runtime_boundary": {
            "observed_selector_values": [0, 2],
            "unobserved_selector_values": [1, 3, 4, 5, 6, 7],
            "replay_expansion": "CLOSED_NO_FIFTH_EQUIVALENT_CAMPAIGN",
        },
        "controlled_state_forcing": "NOT_NEEDED_FOR_THIS_STRUCTURAL_CATALOG",
        "source_owned": {
            "before_bytes": 1475368,
            "after_bytes": 1475368,
            "delta_bytes": 0,
            "promotion": "NONE",
        },
        "fail_closed": [
            "dispatch targets retain neutral static labels",
            "object/animation/frame/sprite semantics are not assigned",
            "shared relative table total extent is unresolved",
            "selector writer caller is not proven",
            "no ROM payload or decoded graphics bytes are emitted",
        ],
        "base_dispatch_catalog": base,
    }
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom")
    parser.add_argument("output")
    args = parser.parse_args()
    result = parse_grammar(Path(args.rom).read_bytes())
    Path(args.output).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
