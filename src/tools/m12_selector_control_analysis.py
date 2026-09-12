"""Prove the static upstream source of the M12 selector handler."""
import hashlib
import json
from pathlib import Path


ROM_SIZE = 0x300000
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
DISPATCHER = (0x00042C, 0x00045E)
DISPATCH_TABLE = (0x00045E, 0x000472)
HANDLER = (0x03A748, 0x03AAAC)
TARGET = 0x03A748
DISPATCH_VALUES = (0x0000307A, 0x000032F8, 0x000089B2, 0x000089B2, TARGET)
DISPATCH_BYTES = bytes.fromhex(
    "303900FF10AC0C40FFFF670E33FCFFFF00FF10AC33C000FF10AE"
    "303900FF10AE02800000007C43FA000A227100004E9160CE"
)
SELECTOR_CONTRACTS = {
    0x03A8C4: bytes.fromhex("33FCFFFF00FFAFAE"),
    0x03A916: bytes.fromhex("527900FFAFAE"),
    0x03A91C: bytes.fromhex("0C79000700FFAFAE"),
}
STATE_WRITES = {
    0x000438: 0xFFFF,
    0x000624: 0x0004,
    0x003198: 0x0010,
    0x0031A2: 0x0004,
    0x003454: 0x0008,
    0x0034AE: 0x000C,
    0x03A956: 0x0004,
    0x03A97C: 0x0004,
}
STATE_WRITE_BYTES = "33FC{value}00FF10AC"


def _hex(value, width=6):
    return f"0x{value:0{width}X}"


def _incoming_direct(rom, target):
    hits = []
    for address in range(0, len(rom) - 5, 2):
        opcode = int.from_bytes(rom[address:address + 2], "big")
        if opcode in (0x4EB9, 0x4EF9):
            if int.from_bytes(rom[address + 2:address + 6], "big") == target:
                hits.append({"pc": _hex(address), "kind": "absolute"})
        elif opcode == 0x6100:
            displacement = int.from_bytes(rom[address + 2:address + 4], "big", signed=True)
            if address + 2 + displacement == target:
                hits.append({"pc": _hex(address), "kind": "bsr_word"})
        elif (opcode & 0xF000) == 0x6000:
            if opcode & 0xFF:
                displacement = opcode & 0xFF
                if displacement & 0x80:
                    displacement -= 0x100
                kind = "branch_short"
            else:
                displacement = int.from_bytes(rom[address + 2:address + 4], "big", signed=True)
                kind = "branch_word"
            if address + 2 + displacement == target:
                hits.append({"pc": _hex(address), "kind": kind})
    return hits


def parse_control(rom):
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    if rom[DISPATCHER[0]:DISPATCHER[1]] != DISPATCH_BYTES:
        raise ValueError("dispatcher latch contract changed")
    for address, expected in SELECTOR_CONTRACTS.items():
        if rom[address:address + len(expected)] != expected:
            raise ValueError(f"selector contract changed at {_hex(address)}")
    writes = []
    for address, value in STATE_WRITES.items():
        expected = bytes.fromhex(STATE_WRITE_BYTES.format(value=f"{value:04X}"))
        if rom[address:address + len(expected)] != expected:
            raise ValueError(f"FF10AC writer changed at {_hex(address)}")
        writes.append({"pc": _hex(address), "value": _hex(value, 4)})
    values = tuple(int.from_bytes(rom[address:address + 4], "big")
                   for address in range(*DISPATCH_TABLE, 4))
    if values != DISPATCH_VALUES:
        raise ValueError("dispatcher target prefix changed")
    direct_incoming = _incoming_direct(rom, TARGET)
    if direct_incoming:
        raise ValueError("unexpected direct incoming edge to selector handler")
    return {
        "schema": "oasis.m68k.m12-selector-control-analysis.v1",
        "status": "UPSTREAM_CONTROL_PROVEN_WITH_EXACT_INDIRECT_ENTRY",
        "canonical_rom_sha256": ROM_SHA256,
        "dispatcher": {
            "reset_vector": _hex(4),
            "reset_target": _hex(0x20E),
            "latch_range": [_hex(DISPATCHER[0]), _hex(DISPATCHER[1])],
            "latch_bytes": DISPATCH_BYTES.hex().upper(),
            "state_ram": _hex(0xFF10AC, 8),
            "dispatch_ram": _hex(0xFF10AE, 8),
            "dispatch_mask": "0x0000007C",
            "table_range": [_hex(DISPATCH_TABLE[0]), _hex(DISPATCH_TABLE[1])],
            "table_entry_width": 4,
            "table_values": [_hex(value, 8) for value in values],
            "target_entry": {"selector_value": "0x10", "target": _hex(TARGET)},
            "indirect_source": {"pc": _hex(0x45A), "bytes": "4E91", "operation": "JSR (A1)"},
            "direct_incoming_to_handler": direct_incoming,
        },
        "selector_handler": {
            "range": [_hex(HANDLER[0]), _hex(HANDLER[1])],
            "selector_ram": _hex(0xFFAFAE, 8),
            "initialization": {"pc": _hex(0x03A8C4), "value": "-1", "bytes": SELECTOR_CONTRACTS[0x03A8C4].hex().upper()},
            "increment": {"pc": _hex(0x03A916), "amount": 1, "bytes": SELECTOR_CONTRACTS[0x03A916].hex().upper()},
            "bound": {"pc": _hex(0x03A91C), "value": 7, "bytes": SELECTOR_CONTRACTS[0x03A91C].hex().upper()},
            "internal_entry_edges": [_hex(0x03AA8E), _hex(0x03AAAA)],
            "semantic_classification": "NEUTRAL_FINITE_SELECTOR_LOOP",
            "category": "NOT_PROVEN; do not label as object, animation, frame, or graphics bank",
            "control_ram": [
                _hex(value, 8) for value in (
                    0xFF0BFD, 0xFF164D, 0xFF164E, 0xFF1654, 0xFF165E,
                    0xFF165F, 0xFF1892, 0xFF316C, 0xFFAFA8, 0xFFAFAC,
                    0xFFAFB0, 0xFFAFCA, 0xFFAFCC, 0xFFAFAE,
                )
            ],
            "indirect_body_edges": [_hex(0x03AA28), _hex(0x03AAA8)],
        },
        "higher_level_state": {
            "ram": _hex(0xFF10AC, 8),
            "writes": writes,
            "handler_entry_condition": "(FF10AC & 0x007C) == 0x0010, latched to FF10AE before JSR (A1)",
            "handler_exit_writes": [{"pc": _hex(0x03A956), "value": "0x0004"},
                                    {"pc": _hex(0x03A97C), "value": "0x0004"}],
            "provenance": "reset vector -> 0x0000042C latch -> ROM table 0x0000045E + (FF10AE & 0x7C) -> 0x03A748",
        },
        "fail_closed": [
            "the indirect dispatch source is proven; no direct caller exists",
            "FF10AC controls entry into the handler, while the inner loop statically enumerates 0..7",
            "semantic category of selector values remains neutral",
            "indirect calls at 0x03AA28 and 0x03AAA8 remain outside static closure",
        ],
    }


def main():
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom")
    parser.add_argument("output")
    args = parser.parse_args()
    result = parse_control(Path(args.rom).read_bytes())
    Path(args.output).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
