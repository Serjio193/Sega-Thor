"""Validate the bounded static producer grammar around ROM PC 0xA372."""
import argparse
import hashlib
import json
from pathlib import Path


ROM_SIZE = 0x300000
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
PRODUCER = (0xA342, 0xA438)
ROOTS = {
    "default": (0xA438, 0xA480),
    "alternate": (0xA480, 0xA4C8),
}
RECORD_WIDTH = 8
RECORD_COUNT = 9
CODE_AFTER_ROOTS = 0xA4C8

CONTRACTS = {
    0xA342: bytes.fromhex("4BF900FF13CC"),
    0xA348: bytes.fromhex("DAF900FF188C"),
    0xA34E: bytes.fromhex("3A3900FF188A"),
    0xA354: bytes.fromhex("41FA00E2"),
    0xA358: bytes.fromhex("4A3900FF1858"),
    0xA35E: bytes.fromhex("6704"),
    0xA360: bytes.fromhex("41FA011E"),
    0xA364: bytes.fromhex("363900FF185A"),
    0xA36A: bytes.fromhex("7005"),
    0xA36C: bytes.fromhex("2418"),
    0xA36E: bytes.fromhex("5205"),
    0xA370: bytes.fromhex("1405"),
    0xA372: bytes.fromhex("2AC2"),
    0xA374: bytes.fromhex("3AD8"),
    0xA376: bytes.fromhex("3418"),
    0xA378: bytes.fromhex("D443"),
    0xA37A: bytes.fromhex("3AC2"),
    0xA37C: bytes.fromhex("51C8FFEE"),
    0xA380: bytes.fromhex("4A3900FF184F"),
    0xA386: bytes.fromhex("67000022"),
    0xA38A: bytes.fromhex("363C0158"),
    0xA38E: bytes.fromhex("967900FF185A"),
    0xA394: bytes.fromhex("7002"),
    0xA396: bytes.fromhex("2418"),
    0xA3A6: bytes.fromhex("51C8FFEE"),
    0xA3AA: bytes.fromhex("4A7900FF1856"),
    0xA3B0: bytes.fromhex("67000072"),
    0xA3CE: bytes.fromhex("363900FF185A"),
    0xA3E2: bytes.fromhex("B03900FF1855"),
    0xA424: bytes.fromhex("9BFC00FF13CC"),
    0xA42A: bytes.fromhex("33CD00FF188C"),
    0xA430: bytes.fromhex("33C500FF188A"),
    0xA436: bytes.fromhex("4E75"),
    0xA4C8: bytes.fromhex("0679003000FF188C"),
    0xA4D0: bytes.fromhex("5C7900FF188A"),
    0xA4D6: bytes.fromhex("4A3900FF184F"),
    0xA4E0: bytes.fromhex("0679001800FF188C"),
    0xA4E8: bytes.fromhex("567900FF188A"),
    0xA4EE: bytes.fromhex("4DFAFF48"),
    0xA4F2: bytes.fromhex("4A3900FF1858"),
    0xA4F8: bytes.fromhex("6704"),
    0xA4FA: bytes.fromhex("4DFAFF84"),
    0xA4FE: bytes.fromhex("23D600FF13CC"),
}


def hx(value, width=6):
    return f"0x{value:0{width}X}"


def check_contracts(rom):
    if len(rom) != ROM_SIZE or hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    for address, expected in CONTRACTS.items():
        if rom[address:address + len(expected)] != expected:
            raise ValueError(f"producer contract changed at {hx(address)}")


def direct_ff1858_xrefs(rom):
    needle = bytes.fromhex("00FF1858")
    refs = []
    for operand in range(len(rom) - len(needle) + 1):
        if rom[operand:operand + len(needle)] != needle or operand < 2:
            continue
        pc = operand - 2
        opcode = int.from_bytes(rom[pc:operand], "big")
        kind = {
            0x4A39: "TST.B reader",
            0x50F9: "ST.B writer(value=0xFF)",
            0x51F9: "SF.B writer(value=0x00)",
        }.get(opcode, "direct absolute access")
        refs.append({"pc": hx(pc), "opcode": hx(opcode, 4), "kind": kind})
    return refs


def root_geometry():
    result = {}
    for name, (start, end) in ROOTS.items():
        if end - start != RECORD_COUNT * RECORD_WIDTH:
            raise ValueError(f"{name} root geometry is not 9x8")
        result[name] = {
            "range": [hx(start), hx(end)],
            "record_count": RECORD_COUNT,
            "record_width_bytes": RECORD_WIDTH,
            "field_layout": [
                {"offset": 0, "width_bytes": 4, "use": "source longword; D2 upper 24 bits"},
                {"offset": 4, "width_bytes": 2, "use": "copied to destination"},
                {"offset": 6, "width_bytes": 2, "use": "copied after D3 word adjustment"},
            ],
            "termination": "consumer loop count, not an in-record sentinel",
        }
    return result


def parse_producer(rom):
    check_contracts(rom)
    if ROOTS["default"][1] != ROOTS["alternate"][0]:
        raise ValueError("root ranges are not contiguous")
    if ROOTS["alternate"][1] != CODE_AFTER_ROOTS:
        raise ValueError("alternate root does not end at the proven code boundary")
    roots = root_geometry()
    xrefs = direct_ff1858_xrefs(rom)
    return {
        "schema": "oasis.m68k.m12-a372-shadow-sat-producer.v1",
        "status": "STATIC_PRODUCER_GRAMMAR_PROVEN_SEMANTICS_NEUTRAL",
        "canonical_rom_sha256": ROM_SHA256,
        "routine": {
            "range": [hx(PRODUCER[0]), hx(PRODUCER[1])],
            "entry": hx(PRODUCER[0]),
            "end_exclusive": hx(PRODUCER[1]),
            "incoming_callers": [hx(0xA19C), hx(0xA6A0)],
            "incoming_direct_branches": [],
            "caller_context": {
                "A19C": "A196 sets FF1651 before BSR; A1A0 tests FF1996 after return",
                "A6A0": "direct BSR at the entry of the separately analyzed state routine",
            },
            "return": hx(0xA436),
            "loop_back_edges": [
                {"pc": hx(0xA37C), "target": hx(0xA36C), "iterations": 6},
                {"pc": hx(0xA3A6), "target": hx(0xA396), "iterations": 3},
            ],
            "sibling_root_setup": {
                "routine_range": [hx(0xA4C8), hx(0xA69C)],
                "caller": hx(0x8B1E),
                "selected_root_copy_pc": hx(0xA4FE),
                "note": "separate routine; no direct call edge into A342",
            },
        },
        "root_alias_correction": {
            "requested_labels": [hx(0xA43A), hx(0xA482)],
            "exact_pc_relative_targets": [hx(0xA438), hx(0xA480)],
            "reason": "68000 PC-relative LEA uses the displacement-word PC; requested labels are root+2 aliases",
        },
        "roots": roots,
        "record_selection": {
            "default": {"condition": "TST.B (0x00FF1858).L == 0", "root": hx(0xA438)},
            "alternate": {"condition": "TST.B (0x00FF1858).L != 0", "root": hx(0xA480)},
            "index": "A0 postincremented by 8 bytes per consumed record",
            "first_loop": {"setup_pc": hx(0xA36A), "dbf_value": 5, "records": 6},
            "second_loop": {
                "condition": "TST.B (0x00FF184F).L != 0",
                "setup_pc": hx(0xA394), "dbf_value": 2, "records": 3,
            },
        },
        "d2_provenance": {
            "source_pc": hx(0xA36C),
            "source": "longword (A0)+",
            "transform_pc": hx(0xA370),
            "formula": "D2 = (record.long_at_+0 & 0xFFFFFF00) | ((D5.B + 1) & 0xFF)",
            "counter_source": hx(0xFF188A, 8),
            "counter_update": "ADDQ.B #1,D5.B per record",
            "observed_value_explanation": {
                "runtime_value": hx(0x00880901, 8),
                "record_source": hx(0xA438),
                "record_index": 0,
                "source_upper_24": hx(0x008809, 6),
                "counter_low_byte": hx(1, 2),
                "qualification": "existing bounded runtime observation; no new replay",
            },
        },
        "a5_provenance": {
            "setup": "A5 = 0x00FF13CC + sign_extended_word((0x00FF188C).L)",
            "setup_pcs": [hx(0xA342), hx(0xA348)],
            "record_step_bytes": 8,
            "first_loop_destination_extent": "[A5, A5+0x30)",
            "second_loop_destination_extent": "[A5+0x30, A5+0x48) when FF184F != 0",
            "first_store": hx(0xA372),
            "stores_per_record": ["longword +0", "word +4", "word +6 after D3 adjustment"],
            "state_writeback": {
                "offset_pc": hx(0xA42A), "offset_ram": hx(0xFF188C, 8),
                "counter_pc": hx(0xA430), "counter_ram": hx(0xFF188A, 8),
            },
            "optional_tail_control": hx(0xFF1856, 8),
        },
        "controls_read_by_family": [
            hx(value, 8) for value in (
                0xFF188C, 0xFF188A, 0xFF1858, 0xFF185A, 0xFF184F,
                0xFF1856, 0xFF1854, 0xFF1855, 0xFF1892, 0xFF1996,
            )
        ],
        "ff1858": {
            "producer_family_readers": [hx(0xA358), hx(0xA4F2)],
            "direct_absolute_xrefs": xrefs,
            "local_role": "boolean root selector; zero selects A438, nonzero selects A480",
            "domain_proven_locally": ["0x00", "nonzero"],
            "mask_or_range": "none at the two producer-family readers",
            "external_writers": "direct ST/SF xrefs establish 0xFF/0x00 writes; their higher-level callers are outside this bounded family",
        },
        "join": {
            "producer": "A19C/A6A0 -> A342..A436 -> selected 9x8 record root -> A372",
            "shadow_sat": "A5 destination with zero FF188C offset -> 0x00FF13CC",
            "dma": "0x000027EC reads 0x00FF13CC and writes SAT VRAM 0x0000D000",
            "runtime_boundary": "the existing frame-2121 observation proved the shadow write; it did not prove same-frame VRAM publication",
        },
        "semantic_result": {
            "frame_or_piece_grammar": "NOT_PROVEN",
            "classification": "NEUTRAL_FIXED_RECORD_PRODUCER",
            "reason": "finite records and explicit cursor are proven, but no animation/object/frame label or selector edge is present in this routine",
        },
        "fail_closed": [
            "no semantic object, animation, frame, or sprite label is assigned",
            "A4C8 sibling control is recorded but not merged into the A342 function boundary",
            "no ROM payload or decoded graphics bytes are emitted",
            "no SOURCE_OWNED promotion is performed",
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom")
    parser.add_argument("output")
    args = parser.parse_args()
    result = parse_producer(Path(args.rom).read_bytes())
    Path(args.output).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
