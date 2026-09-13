"""Validation of the engine-derived V1/V2 FF13CC certificate boundary."""
from .identity import digest

TARGET = 0xFF13CC


def validate_ram_certificate(result):
    """Validate V2 output identities and the explicit V1/V2 bridge."""
    sealed = dict(result)
    certificate_hash = sealed.pop("certificate_sha256", None)
    if not isinstance(certificate_hash, str) or certificate_hash != digest(sealed):
        return False
    ram = result.get("ram_engine", {})
    trace = ram.get("trace")
    if not isinstance(trace, str) or trace != result.get("raw_sha256"):
        return False
    versions, operations = {}, {}
    for epoch_text, payload in ram.get("epochs", {}).items():
        try:
            epoch = int(epoch_text)
        except (TypeError, ValueError):
            return False
        for operation in payload.get("operations", []):
            operation_payload = {key: operation.get(key) for key in (
                "trace", "epoch", "temporal_seq", "execution_instance", "pc", "rule_id",
                "width", "effective_address", "value", "raw_witnesses", "decoded_instruction")}
            if operation.get("epoch") != epoch or operation.get("trace") != trace or \
                    digest({"kind": "ram-write-operation-v2", "value": operation_payload}) != operation.get("id") or \
                    operation.get("id") in operations:
                return False
            operations[operation["id"]] = operation
        for version in payload.get("versions", []):
            keys = ["trace", "epoch", "address", "version", "value", "origin"]
            if version.get("operation_id") is not None:
                keys += ["operation_id", "temporal_seq"]
            version_payload = {key: version.get(key) for key in keys}
            if version.get("epoch") != epoch or version.get("trace") != trace or \
                    digest({"kind": "ram-byte-version-v2", "value": version_payload}) != version.get("id") or \
                    version.get("id") in versions:
                return False
            versions[version["id"]] = version
    target = result.get("target", {})
    ids = target.get("ram_version_ids", [])
    operation_id = target.get("ram_operation_id")
    operation = operations.get(operation_id)
    if len(ids) != 4 or target.get("version_id") != ids[0] or operation is None or \
            operation.get("rule_id") != "MOVE_LONG_D2_TO_RAM" or operation.get("width") != 4 or \
            operation.get("effective_address") != TARGET:
        return False
    if not _legacy_bridge_valid(result, target, operation_id, operation, ids):
        return False
    coverage = [item for payload in ram.get("epochs", {}).values()
                for item in payload.get("coverage", [])]
    if not coverage:
        return False
    for item in coverage:
        if item.get("trace") != trace or item.get("completeness") != "PROVEN" or \
                item.get("start_seq", -1) > operation.get("temporal_seq", -1) or \
                item.get("end_seq", -1) < operation.get("temporal_seq", -1) or \
                not set(range(TARGET, TARGET + 4)).issubset(set(item.get("addresses", []))) or \
                not isinstance(item.get("basis_hash"), str) or len(item["basis_hash"]) != 64:
            return False
    value_hex = target.get("value_hex")
    if not isinstance(value_hex, str) or len(value_hex) != 8 or operation.get("value") != int(value_hex, 16):
        return False
    previous = operation.get("previous_versions", [])
    if operation.get("resulting_versions") != ids or len(previous) != 4:
        return False
    for offset, version_id in enumerate(ids):
        version = versions.get(version_id)
        prior = versions.get(previous[offset])
        if version is None or prior is None or version.get("status") != "OBSERVED" or \
                version.get("operation_id") != operation_id or version.get("origin") != "WRITE_OPERATION" or \
                version.get("address") != TARGET + offset or \
                version.get("value") != ((operation["value"] >> (8 * (3 - offset))) & 0xFF) or \
                prior.get("address") != TARGET + offset or prior.get("epoch") != version.get("epoch") or \
                prior.get("temporal_seq", -1) >= version.get("temporal_seq", -1):
            return False
    dependencies = result.get("v2_dependencies", [])
    if len(dependencies) != 4 or {item.get("target") for item in dependencies} != set(ids) or \
            any(item.get("source") != operation_id or item.get("role") != "RAM_BYTE_OUTPUT" or
                item.get("status") != "PROVEN" for item in dependencies):
        return False
    checks = result.get("checks", {})
    return not checks.get("pc2_inference") and not checks.get("address_only_edge")


def _legacy_bridge_valid(result, target, operation_id, operation, ids):
    legacy_id = target.get("legacy_version_id")
    legacy_versions = {item.get("id"): item for item in result.get("versions", [])}
    legacy = legacy_versions.get(legacy_id)
    legacy_edges = [item for item in result.get("dependencies", [])
                    if item.get("target") == legacy_id]
    if legacy is None or legacy.get("epoch") != operation.get("epoch") or \
            legacy.get("location", {}).get("space") != "RAM" or \
            legacy.get("location", {}).get("key") != TARGET or \
            legacy.get("value_hex") != target.get("value_hex") or \
            len([item for item in legacy_edges
                 if item.get("rule_id") == "MOVE_LONG_D2_TO_RAM" and
                 item.get("role") == "VALUE" and item.get("status") == "PROVEN"]) != 1:
        return False
    bridge = result.get("causal_bridge", {})
    bridge_payload = {key: bridge.get(key) for key in
                      ("source", "operation", "targets", "role", "rule_id", "status", "witness_event_id")}
    return bridge.get("id") == digest({"kind": "canary-causal-bridge", "value": bridge_payload}) and \
        bridge_payload == {"source": legacy_id, "operation": operation_id, "targets": ids,
                           "role": "RAM_BYTE_OUTPUT", "rule_id": "V1_V2_TARGET_BINDING",
                           "status": "PROVEN", "witness_event_id": operation.get("temporal_seq")}
