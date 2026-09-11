"""Adapters for existing deterministic M12 evidence producer reports."""

from copy import deepcopy


def _parse_int(value):
    return int(value, 0) if isinstance(value, str) else int(value)


def _bounds(item):
    if "range" in item:
        value = item["range"]
        if isinstance(value, str) and ".." in value:
            left, right = value.split("..", 1)
            return _parse_int(left), _parse_int(right)
        if isinstance(value, (list, tuple)) and len(value) == 2:
            return _parse_int(value[0]), _parse_int(value[1])
    if "start" in item and "end" in item:
        return _parse_int(item["start"]), _parse_int(item["end"])
    if "address" in item:
        start = _parse_int(item["address"])
        return start, start + 1
    return None


def _items(payload):
    for key in ("evidence_records", "records", "ranges", "streams", "routines",
                "evidence", "execution_facts", "range_execution_coverage",
                "promoted_ranges"):
        value = payload.get(key)
        if isinstance(value, list):
            return value
    addresses = payload.get("executed_addresses", [])
    return [{"address": address} for address in addresses] if isinstance(addresses, list) else []


def _adapt(payload, source, adapter):
    records = []
    producer = payload.get("source") or payload.get("producer") or source
    for item in _items(payload):
        if not isinstance(item, dict) or _bounds(item) is None:
            continue
        start, end = _bounds(item)
        record = deepcopy(item)
        record.update({"start": start, "end": end,
                       "type": item.get("type") or item.get("evidence_type") or adapter,
                       "producer": item.get("producer") or producer,
                       "adapter": adapter, "source_ref": source})
        if adapter == "runtime_pc_read":
            record["runtime"] = True
        if "confidence" not in record:
            record["confidence"] = "OBSERVED" if adapter == "runtime_pc_read" else "EVIDENCE_ONLY"
        records.append(record)
    return {"records": records, "edges": payload.get("edges", []),
            "nodes": payload.get("nodes", []), "adapter": adapter}


def adapt_exact_code_census(payload, source):
    return _adapt(payload, source, "exact_68000_code_census")


def adapt_z80_upload_proof(payload, source):
    return _adapt(payload, source, "z80_upload_proof")


def adapt_graphics_resource_scan(payload, source):
    return _adapt(payload, source, "graphics_decoder_resource_scan")


def adapt_screen_descriptor(payload, source):
    return _adapt(payload, source, "screen_descriptor")


def adapt_pointer_resource_table(payload, source):
    return _adapt(payload, source, "pointer_resource_table")


def adapt_structured_record_promoter(payload, source):
    return _adapt(payload, source, "structured_record_promoter")


def adapt_runtime_pc_read(payload, source):
    return _adapt(payload, source, "runtime_pc_read")


def adapt_padding_alignment(payload, source):
    return _adapt(payload, source, "padding_alignment_promoter")


ADAPTERS = {
    "exact_68000_code_census": adapt_exact_code_census,
    "z80_upload_proof": adapt_z80_upload_proof,
    "graphics_decoder_resource_scan": adapt_graphics_resource_scan,
    "screen_descriptor": adapt_screen_descriptor,
    "pointer_resource_table": adapt_pointer_resource_table,
    "structured_record_promoter": adapt_structured_record_promoter,
    "runtime_pc_read": adapt_runtime_pc_read,
    "padding_alignment_promoter": adapt_padding_alignment,
}


def infer_adapter(payload):
    explicit = payload.get("adapter")
    if explicit:
        if explicit not in ADAPTERS:
            raise ValueError(f"unknown Carver adapter: {explicit}")
        return explicit
    schema = str(payload.get("schema", "")).lower()
    if "gpgx" in schema or "runtime" in schema:
        return "runtime_pc_read"
    if "z80" in schema:
        return "z80_upload_proof"
    if "screen" in schema:
        return "screen_descriptor"
    if "graphics" in schema or "resource" in schema:
        return "graphics_decoder_resource_scan"
    if "padding" in schema or "alignment" in schema:
        return "padding_alignment_promoter"
    if "table" in schema or "pointer" in schema:
        return "pointer_resource_table"
    if "record" in schema:
        return "structured_record_promoter"
    return "exact_68000_code_census"
