"""Catalog the bounded static dispatch consumers around the M12 sprite table."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location(
    "re_m12_table_03b8de_promote", ROOT / "re_m12_table_03b8de_promote.py"
)
TABLE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(TABLE)

ROM_SHA256 = TABLE.ROM_SHA256
DESCRIPTOR_START = TABLE.TABLE_START
DESCRIPTOR_END = TABLE.TABLE_END
ENTRY_COUNT = 8
ENTRY_WIDTH = 4
DISPATCHES = (
    {
        "name": "primary",
        "base": 0x03B8A6,
        "selector_contract": (0x03AA18, bytes.fromhex("02400007D040D04041FA0E84")),
        "indirect_call": (0x03AA28, bytes.fromhex("4E90")),
    },
    {
        "name": "secondary",
        "base": 0x03B8C2,
        "selector_contract": (0x03AA98, bytes.fromhex("02400007D040D04041FA0E20")),
        "indirect_call": (0x03AAA8, bytes.fromhex("4E90")),
    },
)
DESCRIPTOR_CONTRACTS = {
    0x03A9F2: bytes.fromhex("303900FFAFAE"),
    0x03AA0E: bytes.fromhex("610007C0"),
}
DECOMPRESSOR_CALLS = {
    0x03B236: bytes.fromhex("4EB900003820"),
    0x03B28A: bytes.fromhex("4EB900003820"),
    0x03B2FE: bytes.fromhex("4EB900003820"),
}


def read_longwords(data, base, count=ENTRY_COUNT):
    result = []
    for index in range(count):
        start = base + index * ENTRY_WIDTH
        end = start + ENTRY_WIDTH
        if end > len(data):
            raise ValueError(f"table exceeds input at 0x{start:06X}")
        result.append(int.from_bytes(data[start:end], "big"))
    return result


def classify_entry_address(address):
    if DESCRIPTOR_START <= address < DESCRIPTOR_END:
        return "DESCRIPTOR_TABLE_ADDRESS"
    return "STATIC_TARGET_ADDRESS"


def overlap_ranges(start_a, end_a, start_b, end_b):
    start = max(start_a, start_b)
    end = min(end_a, end_b)
    return None if start >= end else {"start": start, "end": end}


def _hex(value, width=6):
    return f"0x{value:0{width}X}"


def parse_catalog(rom):
    if len(rom) != 0x300000 or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    for address, expected in DESCRIPTOR_CONTRACTS.items():
        if rom[address:address + len(expected)] != expected:
            raise ValueError(f"descriptor consumer changed at 0x{address:06X}")
    for address, expected in DECOMPRESSOR_CALLS.items():
        if rom[address:address + len(expected)] != expected:
            raise ValueError(f"decompressor call changed at 0x{address:06X}")
    dispatches = []
    for spec in DISPATCHES:
        contract_address, contract_bytes = spec["selector_contract"]
        if rom[contract_address:contract_address + len(contract_bytes)] != contract_bytes:
            raise ValueError(f"selector contract changed at 0x{contract_address:06X}")
        call_address, call_bytes = spec["indirect_call"]
        if rom[call_address:call_address + len(call_bytes)] != call_bytes:
            raise ValueError(f"indirect call contract changed at 0x{call_address:06X}")
        end = spec["base"] + ENTRY_COUNT * ENTRY_WIDTH
        entries = []
        for index, target in enumerate(read_longwords(rom, spec["base"])):
            entry_address = spec["base"] + index * ENTRY_WIDTH
            item = {
                "selector_value": index,
                "entry_address": _hex(entry_address),
                "target_address": _hex(target),
                "target_classification": classify_entry_address(target),
            }
            if DESCRIPTOR_START <= entry_address < DESCRIPTOR_END:
                item["entry_overlap"] = "M12_DESCRIPTOR_TABLE"
            entries.append(item)
        dispatches.append({
            "name": spec["name"],
            "base": _hex(spec["base"]),
            "end": _hex(end),
            "record_count": ENTRY_COUNT,
            "entry_width": ENTRY_WIDTH,
            "selector_mask": "0x0007",
            "index_stride": ENTRY_WIDTH,
            "selector_contract": {
                "address": _hex(contract_address),
                "bytes": contract_bytes.hex().upper(),
            },
            "indirect_call": {
                "address": _hex(call_address),
                "bytes": call_bytes.hex().upper(),
            },
            "entries": entries,
        })
    descriptor = TABLE.parse_contract(rom)
    return {
        "schema": "oasis.m68k.m12-static-sprite-dispatch-catalog.v1",
        "status": "STATIC_BOUNDED",
        "canonical_rom_sha256": ROM_SHA256,
        "descriptor_table": {
            "base": _hex(DESCRIPTOR_START),
            "end": _hex(DESCRIPTOR_END),
            "record_count": descriptor["table"]["record_count"],
            "record_width": descriptor["table"]["record_width"],
            "records": [
                {
                    "index": record["index"],
                    "address": _hex(record["start"]),
                    "fields": [_hex(value, 8) for value in record["fields"]],
                }
                for record in descriptor["records"]
            ],
        },
        "descriptor_consumer": {
            "selector_ram": "0x00FFAFAE",
            "selector_read": {
                "pc": "0x0003A9F2",
                "bytes": "303900FFAFAE",
            },
            "index_stride": 16,
            "field_offsets": [0, 4, 8, 12],
            "selected_field_destinations": [
                "0x00FFAFA8", "A3", "A4", "A5"
            ],
            "consumer_pc": "0x0003AA0E",
            "consumer_target": "0x0003B1D0",
            "consumer_bytes": "610007C0",
        },
        "dispatch_tables": dispatches,
        "overlaps": [
            {
                "left": "primary_dispatch",
                "right": "secondary_dispatch",
                **overlap_ranges(0x03B8A6, 0x03B8C6, 0x03B8C2, 0x03B8E2),
            },
            {
                "left": "secondary_dispatch",
                "right": "descriptor_table",
                **overlap_ranges(0x03B8C2, 0x03B8E2, DESCRIPTOR_START, DESCRIPTOR_END),
            },
        ],
        "static_callers": [
            {
                "pc": "0x0003AA0E",
                "target": "0x0003B1D0",
                "contract": "direct BSR from descriptor setup",
            },
            {
                "pc": "0x0003B1D0",
                "target": "0x00003820",
                "contract": "bounded static slice observes decompressor calls in B1D0",
            },
        ],
        "decompressor_calls_in_0x03B1D0": [
            {
                "pc": _hex(address),
                "target": "0x00003820",
                "bytes": expected.hex().upper(),
            }
            for address, expected in sorted(DECOMPRESSOR_CALLS.items())
        ],
        "semantic_assignment": "UNRESOLVED",
        "fail_closed": [
            "dispatch targets are not object, animation, frame, or sprite names",
            "static selector domain does not prove live runtime coverage",
            "no ROM payload or decoded graphics bytes are emitted",
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom")
    parser.add_argument("output")
    args = parser.parse_args()
    catalog = parse_catalog(Path(args.rom).read_bytes())
    Path(args.output).write_text(json.dumps(catalog, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(catalog, indent=2))


if __name__ == "__main__":
    main()
