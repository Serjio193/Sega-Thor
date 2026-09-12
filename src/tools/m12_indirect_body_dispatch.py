"""Close the two finite selector-masked indirect body dispatches."""
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
BODY_CONTRACTS = {
    "primary": {
        "address": 0x03AA12,
        "bytes": bytes.fromhex(
            "303900FFAFAE02400007D040D04041FA0E84207000004E90"
        ),
        "table_base": 0x03B8A6,
        "call": 0x03AA28,
    },
    "secondary": {
        "address": 0x03AA92,
        "bytes": bytes.fromhex(
            "303900FFAFAE02400007D040D04041FA0E20207000004E90"
        ),
        "table_base": 0x03B8C2,
        "call": 0x03AAA8,
    },
}

# Boundaries and call edges are pinned to bounded oasis_re_slice outputs.
# The final address is exclusive and its preceding instruction is RTS, except
# for the explicitly classified data alias.
ROUTINES = {
    0x03AAAE: (0x03AAEE, [(0x03AACA, 0x03C956, "4EB90003C956")]),
    0x03AAEE: (0x03AB98, [
        (0x03AB10, 0x02EE2, "4EB900002EE2"),
        (0x03AB3E, 0x03C956, "4EB90003C956"),
        (0x03AB8C, 0x03B832, "61000CA4"),
    ]),
    0x03AB98: (0x03ABDA, [(0x03ABBC, 0x03C956, "4EB90003C956")]),
    0x03ABDA: (0x03AC16, [
        (0x03ABEA, 0x03C956, "4EB90003C956"),
        (0x03AC0E, 0x03C956, "4EB90003C956"),
    ]),
    0x03AC16: (0x03AC68, [(0x03AC58, 0x03C956, "4EB90003C956")]),
    0x03AC68: (0x03AC6E, [(0x03AC68, 0x03B5E8, "6100097E")]),
    0x03AC6E: (0x03AC92, [(0x03AC82, 0x03C956, "4EB90003C956")]),
    0x03AC92: (0x03ACA8, []),
    0x03ACA8: (0x03ACE4, [
        (0x03ACB4, 0x03820, "4EB900003820"),
        (0x03ACCE, 0x03C956, "4EB90003C956"),
    ]),
    0x03ACE4: (0x03AD0C, [(0x03ACFC, 0x08E32, "4EB900008E32")]),
    0x03AD0C: (0x03AD66, [(0x03AD56, 0x03C956, "4EB90003C956")]),
    0x03AD66: (0x03ADB4, [
        (0x03AD9E, 0x03C956, "4EB90003C956"),
        (0x03ADA8, 0x03B5E8, "6100083E"),
        (0x03ADAC, 0x03B092, "4EB90003B092"),
    ]),
    0x03ADB4: (0x03AE74, [
        (0x03ADC0, 0x03820, "4EB900003820"),
        (0x03AE28, 0x03C956, "4EB90003C956"),
    ]),
    0x03AE74: (0x03B092, [
        (0x03AE74, 0x03B5E8, "61000772"),
        (0x03AE94, 0x02EE2, "4EB900002EE2"),
        (0x03AEB6, 0x08E32, "4EB900008E32"),
        (0x03AF08, 0x02FDA, "4EB900002FDA"),
        (0x03AF60, 0x02FDA, "4EB900002FDA"),
        (0x03AF96, 0x02FDA, "4EB900002FDA"),
        (0x03AFB0, 0x02FDA, "4EB900002FDA"),
    ]),
}

ROUTINE_IO = {
    0x03AAAE: ([], ["0x00FFAFB0", "0x00FF1344", "0x00FF1348", "0x00FF134A", "0x00FFAFB2", "0x00FFAFCA", "0x00FFAFE7", "0x00FFAFFF"]),
    0x03AAEE: (["0x00FFAFB0", "0x00FFAFB2"], ["0x00FF1344", "0x00FF1348", "0x00FF134A", "0x00FFAFA8", "0x00FFAFB0", "0x00FFAFCA", "0x00FFAFB1", "0x00FFAFCF", "0x00FFAFE7", "0x00FFAFFF"]),
    0x03AB98: ([], ["0x00FFAFB0", "0x00FF1344", "0x00FF1346", "0x00FF1348", "0x00FF134A", "0x00FFAFB2", "0x00FFAFCA", "0x00FFAFE7"]),
    0x03ABDA: (["0x00FF1348", "0x00FFAFB1", "0x00FF1344"], []),
    0x03AC16: (["0x00FFAFBA"], ["0x00FFAFC2", "0x00FFAFBE", "0x00FFAFB0", "0x00FFAFC0", "0x00FFAFC6", "0x00FFAFC8", "0x00FFAFCA"]),
    0x03AC68: ([], []),
    0x03AC6E: ([], ["0x00FFAFB0", "0x00FFAFCA", "0x00FF1344", "0x00FF1348"]),
    0x03AC92: ([], ["0x00FFAFDA", "0x00FFAFDE"]),
    0x03ACA8: ([], ["0x00FFAFB0", "0x00FFAFCA", "0x00FF1344", "0x00FF1348", "0x00FFAFCF"]),
    0x03ACE4: (["0x00FFAFB0"], ["0x00FFAFCA", "0x00FFAFCF"]),
    0x03AD0C: (["0x00FFAFBA"], ["0x00FFAFC2", "0x00FFAFBE", "0x00FFAFB0", "0x00FFAFC4", "0x00FFAFC0", "0x00FFAFC6", "0x00FFAFC8", "0x00FFAFCA"]),
    0x03AD66: (["0x00FFAFB0", "0x00FFAFD0", "0x00FFAFE4", "0x00FFAFB0"], ["0x00FFAFCA", "0x00FFAFD0"]),
    0x03ADB4: (["0x00FFAFBA"], ["0x00FFAFB0", "0x00FFAFB4", "0x00FFAFB6", "0x00FFAFB8", "0x00FFAFC2", "0x00FFAFBE", "0x00FFAFC4", "0x00FFAFC0", "0x00FFAFC6", "0x00FFAFC8", "0x00FFAFCA", "0x00FFAFE7", "0x00FFAFFF", "0x00FFB017", "0x00FFB02F", "0x00FFB047", "0x00FFB05F", "0x00FFB077", "0x00FFB08F"]),
    0x03AE74: (["0x00FFAFB0", "0x00FFAFB4", "0x00FFAFB6", "0x00FFAFB8", "0x00FFAFBA", "0x00FFAFDE", "0x00FFAFF6"], ["0x00FFAFA8", "0x00FFAFBA", "0x00FFAFBE", "0x00FFAFB0", "0x00FFAFCA", "0x00FFAFD0", "0x00FFAFE7", "0x00FFAFE8", "0x00FFAFFE", "0x00FFB000", "0x00FFB00E", "0x00FFB018", "0x00FFB026", "0x00FFB030", "0x00FFB03E", "0x00FFB048", "0x00FFB056", "0x00FFB060", "0x00FFB06E", "0x00FFB078", "0x00FFB086", "0x00FFB08F"]),
}


def _hex(value, width=6):
    return f"0x{value:0{width}X}"


def _check(rom, address, expected, label):
    if rom[address:address + len(expected)] != expected:
        raise ValueError(f"{label} changed at {_hex(address)}")


def _identity(rom):
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")


def parse_dispatch(rom):
    _identity(rom)
    catalog = DISPATCH.parse_catalog(rom)
    sources = []
    targets = {}
    for name, spec in BODY_CONTRACTS.items():
        _check(rom, spec["address"], spec["bytes"], f"{name} body")
        table = next(item for item in catalog["dispatch_tables"] if item["name"] == name)
        entries = []
        for entry in table["entries"]:
            target = int(entry["target_address"], 16)
            entries.append({"selector": entry["selector_value"], "target": _hex(target)})
            targets.setdefault(target, []).append({"family": name, "selector": entry["selector_value"]})
        sources.append({
            "family": name,
            "selector_ram": "0x00FFAFAE",
            "selector_read": _hex(spec["address"]),
            "selector_mask": "0x0007",
            "index_scale": 4,
            "table_base": _hex(spec["table_base"]),
            "index_register": "D0",
            "target_register": "A0",
            "indirect_call": _hex(spec["call"]),
            "entries": entries,
            "static_proof": (
                "MOVE.W selector,D0; ANDI.W #7,D0; ADD.W D0,D0 twice; "
                "LEA table(PC),A0; MOVEA.L (A0,D0.W),A0; JSR (A0)"
            ),
        })

    routines = []
    for start, (end, calls) in sorted(ROUTINES.items()):
        if rom[end - 2:end] != bytes.fromhex("4E75"):
            raise ValueError(f"RTS boundary changed at {_hex(end - 2)}")
        call_records = []
        for pc, target, expected_hex in calls:
            expected = bytes.fromhex(expected_hex)
            _check(rom, pc, expected, "target call")
            call_records.append({"pc": _hex(pc), "target": _hex(target), "bytes": expected_hex})
        routines.append({
            "start": _hex(start),
            "end_exclusive": _hex(end),
            "boundary": "RTS",
            "dispatch_selectors": targets[start],
            "direct_calls": call_records,
            "static_io_sites": {
                "ram_inputs": ROUTINE_IO[start][0],
                "ram_outputs": ROUTINE_IO[start][1],
            },
        })

    alias = 0x03BA46
    return {
        "schema": "oasis.m68k.m12-indirect-body-dispatch.v1",
        "status": "TWO_INDIRECT_BODY_CALLS_FINITE_TARGETS_PROVEN",
        "canonical_rom_sha256": ROM_SHA256,
        "sources": sources,
        "unique_target_count": len(targets),
        "routine_count": len(routines),
        "routines": routines,
        "non_routine_targets": [{
            "address": _hex(alias),
            "dispatch_selectors": targets[alias],
            "classification": "DATA_ALIAS_NOT_ROUTINE",
            "bytes": rom[alias:alias + 8].hex().upper(),
            "boundary_evidence": "secondary index 7 shares descriptor field +0 storage",
        }],
        "downstream_unresolved": [{
            "pc": "0x0003B0A8",
            "form": "JMP (A0)",
            "scope": "callee reached by 0x03ADAC, not either body dispatch call",
        }],
        "join": (
            "FF10AC -> dispatcher -> 0x03A748 -> selector -> descriptor +12 "
            "-> child relative pairs -> 0x03B448 -> 0x0000B730 -> SAT"
        ),
        "semantics": "NEUTRAL_STATIC_DISPATCH_TARGETS",
        "fail_closed": [
            "no object, animation, frame, or sprite labels assigned",
            "0x03BA46 is not decoded as code",
            "0x03B0A8 indirect tail remains outside this bounded closure",
            "no ROM payload or decoded graphics bytes emitted",
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom")
    parser.add_argument("output")
    args = parser.parse_args()
    result = parse_dispatch(Path(args.rom).read_bytes())
    Path(args.output).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
