"""Validate the bounded second-level dispatch rooted at 0x03B092."""
import argparse
import hashlib
import json
from pathlib import Path


ROM_SIZE = 0x300000
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
ROOT = 0x03B092
TABLE_START = 0x03B0AA
TABLE_END = 0x03B0BA
TABLE_VALUES = (0x03B0BC, 0x03B0E8, 0x03B132, 0x03B0BA)
ROOT_CONTRACT = bytes.fromhex(
    "303900FFAFB002400003D040D04041FA0008207000004ED0"
)
TABLE_CONTRACT = bytes.fromhex(
    "0003B0BC0003B0E80003B1320003B0BA"
)

ROUTINES = {
    0x03B0BA: {
        "end": 0x03B0BC,
        "calls": [],
        "ram_inputs": [],
        "ram_outputs": [],
        "rom_accesses": [],
        "behavior": "immediate RTS; no state or data access",
    },
    0x03B0BC: {
        "end": 0x03B0E8,
        "calls": [],
        "ram_inputs": ["0x00FF316C"],
        "ram_outputs": [],
        "rom_accesses": [],
        "behavior": "bounded register/stack transform with DBcc loops",
    },
    0x03B0E8: {
        "end": 0x03B132,
        "calls": [],
        "ram_inputs": ["0x00FF164E", "0x00FF1892", "0x00FF316C"],
        "ram_outputs": ["0x00FF1892", "A0-indirect"],
        "rom_accesses": [],
        "behavior": "flag-gated RAM transform with bounded wait loop",
    },
    0x03B132: {
        "end": 0x03B188,
        "calls": [],
        "ram_inputs": ["0x00FF164E", "0x00FF1892", "0x00FF316C"],
        "ram_outputs": ["0x00FF1892", "A0-indirect"],
        "rom_accesses": [],
        "behavior": "flag-gated RAM transform with bounded wait loop",
    },
}


def _hex(value, width=6):
    return f"0x{value:0{width}X}"


def _check(rom, address, expected, label):
    if rom[address:address + len(expected)] != expected:
        raise ValueError(f"{label} changed at {_hex(address)}")


def _check_identity(rom):
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")


def parse_tail(rom):
    _check_identity(rom)
    _check(rom, ROOT, ROOT_CONTRACT, "tail dispatch")
    _check(rom, TABLE_START, TABLE_CONTRACT, "tail table")
    if tuple(int.from_bytes(rom[TABLE_START + index * 4:TABLE_START + index * 4 + 4], "big")
             for index in range(4)) != TABLE_VALUES:
        raise ValueError("tail table values changed")
    if rom[ROOT + len(ROOT_CONTRACT) - 2:ROOT + len(ROOT_CONTRACT)] != bytes.fromhex("4ED0"):
        raise ValueError("indirect jump contract changed")

    targets = []
    for index, target in enumerate(TABLE_VALUES):
        item = {
            "index": index,
            "source_value": "0x00FFAFB0 & 0x0003",
            "entry_address": _hex(TABLE_START + index * 4),
            "target": _hex(target),
            "classification": "CODE_ROUTINE" if target in ROUTINES else "UNRESOLVED",
        }
        if target in ROUTINES:
            routine = ROUTINES[target]
            if rom[routine["end"] - 2:routine["end"]] != bytes.fromhex("4E75"):
                raise ValueError(f"RTS boundary changed at {_hex(routine['end'] - 2)}")
            item["routine_boundary"] = [_hex(target), _hex(routine["end"])]
            item["terminal_behavior"] = "RTS"
        targets.append(item)

    routines = []
    for start, spec in sorted(ROUTINES.items()):
        routines.append({
            "start": _hex(start),
            "end_exclusive": _hex(spec["end"]),
            "boundary": "RTS",
            "dispatch_indices": [index for index, target in enumerate(TABLE_VALUES) if target == start],
            "direct_calls": spec["calls"],
            "static_io": {
                "ram_inputs": spec["ram_inputs"],
                "ram_outputs": spec["ram_outputs"],
                "rom_accesses": spec["rom_accesses"],
            },
            "behavior": spec["behavior"],
            "a6_selector_descriptor_effect": "NO_STATIC_REFERENCE",
        })

    return {
        "schema": "oasis.m68k.m12-b092-tail-dispatch.v1",
        "status": "SECOND_LEVEL_TAIL_FINITE_DOMAIN_PROVEN",
        "canonical_rom_sha256": ROM_SHA256,
        "root": {
            "routine": [_hex(ROOT), _hex(0x03B0AA)],
            "contract": ROOT_CONTRACT.hex().upper(),
            "index_source": "0x00FFAFB0",
            "index_width": "word",
            "index_signedness": "unsigned after ANDI.W",
            "mask": "0x0003",
            "scale_bytes": 4,
            "target_register": "A0",
            "table_base": _hex(TABLE_START),
            "table_consumer_pc": _hex(0x03B0A0),
            "jump_pc": _hex(0x03B0A8),
            "direct_caller": _hex(0x03ADAC),
        },
        "table": {
            "start": _hex(TABLE_START),
            "end_exclusive": _hex(TABLE_END),
            "entry_width_bytes": 4,
            "entry_count": 4,
            "domain": [0, 1, 2, 3],
            "encoding": "absolute big-endian longword PC",
            "values": [_hex(value) for value in TABLE_VALUES],
            "all_known_base_loaders": [_hex(0x03B0A0)],
        },
        "targets": targets,
        "routines": routines,
        "selector_source": {
            "classification": "RAM_STATE_WORD_NOT_FFAFAE",
            "value_is_inherited": True,
            "local_path_writes_before_jump": [],
            "note": "0x03AD66 reads FFAFB0 and calls 0x03B092 without assigning it locally",
        },
        "interpreter_test": {
            "command_fetch": False,
            "variable_record_advance": False,
            "3820_calls": [],
            "B730_calls": [],
            "conclusion": "NO_PROVEN_COMMAND_OR_FRAME_INTERPRETER",
        },
        "join": (
            "FF10AC -> 0x03A748 -> body selector -> 0x03AD66 -> 0x03B092 "
            "-> FFAFB0 mask/index -> 0x03B0AA absolute target table"
        ),
        "fail_closed": [
            "FFAFB0 upstream writers are not reclassified as semantic object/frame state",
            "0xFFFF0017 selector-7 child remains unrelated and unresolved",
            "no ROM payload or decoded graphics emitted",
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom")
    parser.add_argument("output")
    args = parser.parse_args()
    result = parse_tail(Path(args.rom).read_bytes())
    Path(args.output).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
