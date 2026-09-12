"""Build a bounded, payload-free catalog from the M12 live context capture."""
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
TABLE_START = TABLE.TABLE_START
TABLE_END = TABLE.TABLE_END
REQUIRED_SEQUENCE = (
    "0x0003B41A", "0x0003B422", "0x0003B426", "0x0003B448",
    "0x0000B730", "0x0000B752", "0x0000B764", "0x0000B76E",
    "0x0000B77A",
)
SOURCE_START = "0x0000B730"
CALLER = "0x0003B448"


def hex_value(value):
    if not isinstance(value, str) or not value.startswith("0x"):
        raise ValueError(f"expected hexadecimal value, got {value!r}")
    return int(value, 16)


def _execution_frames(context):
    frames = {}
    for item in context.get("executions", []):
        frame = int(item["frame"])
        frames.setdefault(frame, []).append(item)
    return frames


def summarize_context(context):
    if context.get("schema") != "oasis.m68k.m12-sprite-context.v1":
        raise ValueError("unsupported sprite context schema")
    if context.get("canonical_rom_sha256") != ROM_SHA256:
        raise ValueError("sprite context canonical ROM mismatch")
    if context.get("state_writes_emitted") is not False:
        raise ValueError("sprite context must be read-only")

    complete = []
    for frame, events in _execution_frames(context).items():
        positions = {pc: [] for pc in REQUIRED_SEQUENCE}
        for position, event in enumerate(events):
            if event["pc"] in positions:
                positions[event["pc"]].append(position)
        if not all(positions.values()):
            continue
        first = [positions[pc][0] for pc in REQUIRED_SEQUENCE]
        if first != sorted(first):
            raise ValueError(f"live PC sequence is not ordered in frame {frame}")
        complete.append(frame)

    starts = {}
    for event in context.get("executions", []):
        if event["pc"] != CALLER:
            continue
        address = event["registers"]["a"][0]
        starts[address] = starts.get(address, 0) + 1
    if not starts:
        raise ValueError("no bounded ROM-backed source starts were observed")
    if not complete:
        raise ValueError("no ordered bounded selector-to-producer edge was observed")
    return {
        "complete_sequence_frames": complete,
        "complete_sequence_frame_count": len(complete),
        "observed_source_starts": [
            {"address": address, "caller_pc": CALLER, "count": starts[address]}
            for address in sorted(starts, key=hex_value)
        ],
        "source_producer_pc": SOURCE_START,
        "sat_source_ram": "0x00FF13CC",
        "store_pcs": [
            "0x0000B752", "0x0000B764", "0x0000B76E", "0x0000B77A"
        ],
    }


def summarize_targeted_reads(context, targeted):
    if targeted.get("schema") != "oasis.m68k.m12-targeted-rom-reads.v1":
        raise ValueError("unsupported targeted-read schema")
    if targeted.get("canonical_rom_sha256") != ROM_SHA256:
        raise ValueError("targeted-read canonical ROM mismatch")
    if targeted.get("state_writes_emitted") is not False:
        raise ValueError("targeted-read capture must be read-only")
    events = targeted.get("read_events", [])

    def matching(address, value=None, pcs=()):
        return [
            event for event in events
            if event.get("address") == address
            and (value is None or event.get("value") == value)
            and (not pcs or event.get("pc") in pcs)
        ]

    table_field = matching("0x0003B8EA", "0x0003B95C", {"0x0003B426"})
    pointer = matching("0x0003B95C", "0x00171832", {"0x0003B428"})
    if not table_field or not pointer:
        raise ValueError("exact table-field to pointer read edge is missing")
    source_starts = [item["address"] for item in summarize_context(context)["observed_source_starts"]]
    source_reads = {}
    for address in source_starts:
        hits = matching(address, pcs={"0x0000B73E"})
        if not hits:
            raise ValueError(f"source start read is missing for {address}")
        source_reads[address] = len(hits)
    source_reads_from_probe = {}
    for event in events:
        if event.get("pc") != "0x0000B73E":
            continue
        address = event.get("address")
        source_reads_from_probe[address] = source_reads_from_probe.get(address, 0) + 1
    table_fields = []
    for index in range(TABLE.RECORD_COUNT):
        address = TABLE_START + index * TABLE.RECORD_WIDTH + 12
        address_text = f"0x{address:08X}"
        hits = [event for event in events if event.get("address") == address_text]
        if not hits:
            continue
        values = sorted({event.get("value") for event in hits})
        table_fields.append({
            "record_index": index,
            "address": address_text,
            "values": values,
            "events": len(hits),
            "callback_pcs": sorted({event.get("pc") for event in hits}),
        })
    selector_events = [
        event for event in events
        if event.get("address") == "0x00FFAFAE"
        and event.get("pc") in {"0x0003B3BC", "0x0003B420"}
    ]
    selector_values = {}
    for event in selector_events:
        value = hex_value(event["value"])
        selector_values[value] = selector_values.get(value, 0) + 1
    selector_to_fields = []
    for selector, count in sorted(selector_values.items()):
        address = TABLE_START + selector * TABLE.RECORD_WIDTH + 12
        address_text = f"0x{address:08X}"
        matches = [field for field in table_fields if field["address"] == address_text]
        if not matches:
            raise ValueError(f"selector {selector} has no matching table field read")
        selector_to_fields.append({
            "selector_value": selector,
            "record_index": selector,
            "table_field_address": address_text,
            "events": count,
        })
    if not selector_to_fields:
        raise ValueError("no exact selector reads were observed")
    record2_secondary = []
    if 2 in selector_values:
        record2_secondary = matching(
            "0x001742DC", "0x0000001C", {"0x0003B432", "0x0003B438"}
        )
        if not record2_secondary:
            raise ValueError("selector-2 secondary word read is missing")
        if not any(
            event.get("address") == "0x001742F8"
            and event.get("pc") == "0x0000B73E"
            for event in events
        ):
            raise ValueError("selector-2 observed source-start read is missing")
        record2_continuation = matching(
            "0x001742E2", "0x00000076", {"0x0003B432", "0x0003B438"}
        )
        if not record2_continuation:
            raise ValueError("selector-2 continuation word read is missing")
        if not any(
            event.get("address") == "0x00174358"
            and event.get("pc") == "0x0000B73E"
            for event in events
        ):
            raise ValueError("selector-2 second source-start read is missing")
    else:
        record2_continuation = []
    pointer_reads = []
    for target in sorted({value for field in table_fields for value in field["values"]}, key=hex_value):
        hits = [event for event in events if event.get("address") == target]
        if hits:
            pointer_reads.append({
                "address": target,
                "values": sorted({event.get("value") for event in hits}),
                "events": len(hits),
                "callback_pcs": sorted({event.get("pc") for event in hits}),
            })
    return {
        "selector_reads": {
            "address": "0x00FFAFAE",
            "values": [
                {"value": selector, "events": count}
                for selector, count in sorted(selector_values.items())
            ],
            "callback_pcs": sorted({event["pc"] for event in selector_events}),
        },
        "selector_to_table_fields": selector_to_fields,
        "selector2_secondary_word": {
            "address": "0x001742DC",
            "value": "0x0000001C",
            "callback_pcs": sorted({event["pc"] for event in record2_secondary}),
            "events": len(record2_secondary),
            "following_observed_source_start": "0x001742F8",
        } if record2_secondary else None,
        "selector2_continuation_word": {
            "address": "0x001742E2",
            "value": "0x00000076",
            "callback_pcs": sorted({event["pc"] for event in record2_continuation}),
            "events": len(record2_continuation),
            "following_observed_source_start": "0x00174358",
        } if record2_continuation else None,
        "observed_table_fields": table_fields,
        "observed_pointer_reads": pointer_reads,
        "table_field_read": {
            "address": "0x0003B8EA", "value": "0x0003B95C",
            "callback_pc": "0x0003B426", "events": len(table_field)
        },
        "pointer_read": {
            "address": "0x0003B95C", "value": "0x00171832",
            "callback_pc": "0x0003B428", "events": len(pointer)
        },
        "source_start_reads": [
            {"address": address, "callback_pc": "0x0000B73E", "events": count}
            for address, count in sorted(source_reads_from_probe.items(), key=lambda item: hex_value(item[0]))
        ],
        "context_source_start_reads": [
            {"address": address, "callback_pc": "0x0000B73E", "events": source_reads[address]}
            for address in sorted(source_reads, key=hex_value)
        ],
        "source_start_read_count": len(source_reads_from_probe),
        "total_read_events": len(events),
        "capture_frames": targeted.get("frames_executed"),
    }


def build_catalog(rom_path, context_path, targeted_path=None):
    rom_path = Path(rom_path).resolve()
    context = json.loads(Path(context_path).read_text(encoding="utf-8"))
    rom = rom_path.read_bytes()
    if hashlib.sha256(rom).hexdigest() != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    contract = TABLE.parse_contract(rom)
    live = summarize_context(context)
    records = []
    for record in contract["records"]:
        address = record["start"]
        records.append({
            "index": record["index"],
            "address": f"0x{address:06X}",
            "end": f"0x{address + TABLE.RECORD_WIDTH:06X}",
            "width": TABLE.RECORD_WIDTH,
            "classification": "STRUCTURED_DATA_CONFIRMED",
            "semantic_name": "UNRESOLVED",
        })
    catalog = {
        "schema": "oasis.m68k.m12-sprite-context-catalog.v1",
        "status": "OBSERVED_RUNTIME_BOUNDED",
        "canonical_rom_sha256": ROM_SHA256,
        "table": {
            "base": f"0x{TABLE_START:06X}",
            "end": f"0x{TABLE_END:06X}",
            "record_width": TABLE.RECORD_WIDTH,
            "record_count": len(records),
            "selector_ram": "0x00FFAFAE",
            "records": records,
        },
        "live_provenance": live,
        "provenance_scope": (
            "The context capture proves an ordered bounded execution edge "
            "and four ROM-backed source starts. When an exact-address read "
            "capture is supplied, the catalog additionally reports its "
            "bounded source-start census. Neither capture assigns object, "
            "animation, frame, or semantic record names."
        ),
        "fail_closed": [
            "complete object/animation enumeration is unresolved",
            "source starts are not promoted to extracted assets",
            "no ROM payload or graphics bytes are emitted",
        ],
    }
    if targeted_path:
        targeted = json.loads(Path(targeted_path).read_text(encoding="utf-8"))
        catalog["targeted_rom_read_provenance"] = summarize_targeted_reads(context, targeted)
    return catalog


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom")
    parser.add_argument("context")
    parser.add_argument("output")
    parser.add_argument("--targeted-reads")
    args = parser.parse_args()
    catalog = build_catalog(args.rom, args.context, args.targeted_reads)
    Path(args.output).write_text(json.dumps(catalog, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(catalog, indent=2))


if __name__ == "__main__":
    main()
