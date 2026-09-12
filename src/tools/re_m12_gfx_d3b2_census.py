"""Build an exact caller/root census for the indexed 0x00D3B2 graphics loader."""
import argparse
import hashlib
import json
import re
from pathlib import Path


SCHEMA = "oasis.m68k.m12-gfx-d3b2-census.v1"
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
TARGET = 0x00D3B2
CALL_BYTES = b"\x4E\xB9\x00\x00\xD3\xB2"
TABLE_START = 0x05CE96
TABLE_ENTRIES = 108
TABLE_END = TABLE_START + TABLE_ENTRIES * 4
EXPECTED_CALLS = {
    0x02CFAA: (3, 0x4000),
    0x02CFB8: (4, 0x5000),
    0x02D410: (0x57, 0x4000),
    0x032174: (0x23, 0x4000),
    0x032182: (0x24, 0x5000),
    0x032884: (0x23, 0x4000),
    0x032892: (0x24, 0x5000),
}
RESOURCE_REASON = re.compile(r"resource table index (\d+)")


def _hex(value: int) -> str:
    return f"0x{value:06X}"


def direct_call_sites(rom: bytes) -> list[int]:
    """Return every exact absolute-long JSR to the proven indexed loader."""
    return [offset for offset in range(len(rom) - len(CALL_BYTES) + 1)
            if rom[offset:offset + len(CALL_BYTES)] == CALL_BYTES]


def parse_setup(rom: bytes, call_site: int) -> dict:
    """Parse the exact immediate D0/D1 setup immediately before a caller."""
    setup_start = call_site - 8
    if setup_start < 0:
        raise ValueError(f"caller setup underflow at {_hex(call_site)}")
    d0 = rom[setup_start:setup_start + 4]
    d1 = rom[setup_start + 4:setup_start + 8]
    call = rom[call_site:call_site + len(CALL_BYTES)]
    if d0[:2] != b"\x30\x3C" or d1[:2] != b"\x32\x3C":
        raise ValueError(f"non-immediate D0/D1 setup at {_hex(call_site)}")
    if call != CALL_BYTES:
        raise ValueError(f"unexpected D3B2 encoding at {_hex(call_site)}")
    return {
        "call_site": _hex(call_site),
        "setup_start": _hex(setup_start),
        "selector_d0": int.from_bytes(d0[2:4], "big"),
        "vram_destination_d1": int.from_bytes(d1[2:4], "big"),
        "classification": "EXACT_INDEXED_RESOURCE_SETUP",
        "proof_limit": "linear caller setup; loader ABI and table root are independently proven",
    }


def table_records(rom: bytes) -> list[dict]:
    """Read the complete fixed-width root table without interpreting payload bytes."""
    if len(rom) < TABLE_END:
        raise ValueError("ROM is shorter than the D3B2 resource table")
    records = []
    for index in range(TABLE_ENTRIES):
        offset = TABLE_START + index * 4
        pointer = int.from_bytes(rom[offset:offset + 4], "big")
        records.append({
            "index": index,
            "table_offset": _hex(offset),
            "pointer": _hex(pointer),
        })
    return records


def owned_resource_spans(manifest: dict) -> dict[int, dict]:
    """Collect only exact resource-table ownership records from the manifest."""
    result = {}
    for entry in manifest.get("entries", []):
        if entry.get("kind") != "LOCAL_ROM_DERIVED_ASSET":
            continue
        match = RESOURCE_REASON.search(entry.get("ownership_reason", ""))
        if match is None:
            continue
        index = int(match.group(1))
        if index in result:
            raise ValueError(f"duplicate owned resource-table index {index}")
        result[index] = entry
    return result


def build_report(rom: bytes, manifest: dict) -> dict:
    actual_sha = hashlib.sha256(rom).hexdigest()
    if actual_sha != ROM_SHA256:
        raise ValueError(f"canonical ROM SHA-256 mismatch: {actual_sha}")
    calls = direct_call_sites(rom)
    expected_sites = sorted(EXPECTED_CALLS)
    if calls != expected_sites:
        raise ValueError(
            f"D3B2 direct-call set mismatch: expected {expected_sites}, got {calls}")

    setups = []
    for site in calls:
        setup = parse_setup(rom, site)
        selector, destination = EXPECTED_CALLS[site]
        if (setup["selector_d0"], setup["vram_destination_d1"]) != (selector, destination):
            raise ValueError(f"unexpected D3B2 setup at {_hex(site)}")
        setups.append(setup)

    records = table_records(rom)
    if records[0]["pointer"] != "0x000000":
        raise ValueError("D3B2 table entry 0 is not the proven null entry")
    children = {record["index"]: record for record in records[1:]}
    if any(record["pointer"] == "0x000000" for record in children.values()):
        raise ValueError("finite D3B2 child domain contains a null pointer")
    owned = owned_resource_spans(manifest)
    expected_children = set(range(1, TABLE_ENTRIES))
    if set(owned) != expected_children:
        missing = sorted(expected_children - set(owned))
        extra = sorted(set(owned) - expected_children)
        raise ValueError(f"D3B2 child ownership mismatch: missing={missing}, extra={extra}")

    child_records = []
    for index in sorted(children):
        pointer = int(children[index]["pointer"], 16)
        entry = owned[index]
        if entry["start"] != pointer:
            raise ValueError(f"resource {index} pointer does not match manifest start")
        child_records.append({
            "index": index,
            "pointer": _hex(pointer),
            "end": _hex(entry["end"]),
            "compressed_bytes": entry["end"] - entry["start"],
            "kind": entry["kind"],
            "source": entry["source"],
            "ownership": "SOURCE_OWNED_ALREADY",
        })
    return {
        "schema": SCHEMA,
        "rom_sha256": actual_sha,
        "rom_size": len(rom),
        "loader": _hex(TARGET),
        "loader_contract": {
            "table_start": _hex(TABLE_START),
            "table_end": _hex(TABLE_END),
            "entry_width": 4,
            "entry_count": TABLE_ENTRIES,
            "null_entry": 0,
            "finite_child_indices": "1..107",
            "ram_destination": "0x00FF2FA8",
            "decompressor": "0x003820",
            "destination_provenance": "D1 is caller-supplied Genesis VRAM word address",
        },
        "direct_call_count": len(calls),
        "direct_call_sites": [_hex(site) for site in calls],
        "caller_setups": setups,
        "selected_indices": sorted({setup["selector_d0"] for setup in setups}),
        "selected_child_records": [
            record for record in child_records
            if record["index"] in {setup["selector_d0"] for setup in setups}],
        "root_table_children": child_records,
        "root_table_child_count": len(child_records),
        "source_owned_child_bytes": sum(record["compressed_bytes"] for record in child_records),
        "promotion": {
            "performed": False,
            "new_source_owned_bytes": 0,
            "reason": "all 107 finite child streams are already SOURCE_OWNED in the manifest",
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("rom", type=Path)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    report = build_report(args.rom.read_bytes(), json.loads(args.manifest.read_text()))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({
        "direct_calls": report["direct_call_count"],
        "root_children": report["root_table_child_count"],
        "source_owned_child_bytes": report["source_owned_child_bytes"],
        "promoted_bytes": report["promotion"]["new_source_owned_bytes"],
    }, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
