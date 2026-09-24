"""Content identities and stale-parent checks for canonical map proposals."""

from __future__ import annotations

from typing import Any

try:
    from .rom_knowledge_map import canonical, sha256_bytes, stable_id
except ImportError:
    from rom_knowledge_map import canonical, sha256_bytes, stable_id


def set_generation_identity(store: Any, generation_id: str,
                           parent_generation: str | None) -> None:
    if store._legacy or not generation_id:
        raise ValueError("STOP_KNOWLEDGE_GENERATION_IDENTITY_INVALID")
    values = {"generation_id": generation_id,
              "parent_generation_id": parent_generation or ""}
    for key, value in values.items():
        store.db.execute("INSERT OR REPLACE INTO map_meta VALUES (?,?)", (key, value))


def record_derivation(store: Any, rule_id: str, rule_version: str,
                      implementation_hash: str, parameters_hash: str,
                      inputs: list[dict[str, str]], output_type: str,
                      output_id: str, result: Any,
                      assumptions: list[Any] | None = None) -> str:
    if store._legacy:
        raise ValueError("STOP_KNOWLEDGE_V1_PARENT_REQUIRES_STAGED_MIGRATION")
    normalized = sorted(inputs, key=canonical)
    assumptions = assumptions or []
    payload = {"rule_id": rule_id, "rule_version": rule_version,
        "implementation_hash": implementation_hash, "parameters_hash": parameters_hash,
        "inputs": normalized, "output_type": output_type, "output_id": output_id,
        "result": result, "assumptions": assumptions}
    identifier = stable_id("derivation", payload)
    store.insert_rows("derivation", [{"derivation_id": identifier, "rule_id": rule_id,
        "rule_version": rule_version, "implementation_hash": implementation_hash,
        "parameters_hash": parameters_hash, "output_type": output_type,
        "output_id": output_id, "result_json": canonical(result),
        "assumptions_json": canonical(assumptions)}])
    store.insert_rows("derivation_input", [{"derivation_id": identifier, "ordinal": index,
        "subject_type": item["subject_type"], "subject_id": item["subject_id"],
        "role": item["role"]} for index, item in enumerate(normalized)])
    return identifier


def create_map_proposal(store: Any, proposal_id: str, base_generation: str,
                        base_map_hash: str, graph_hash: str,
                        validator_version: str,
                        operations: list[dict[str, Any]]) -> str:
    if store._legacy:
        raise ValueError("STOP_KNOWLEDGE_V1_PARENT_REQUIRES_STAGED_MIGRATION")
    if not base_generation or len(base_map_hash) != 64 or len(graph_hash) != 64 or \
            not validator_version or store.meta().get("generation_id") != base_generation or \
            store.hashes()["map_hash"] != base_map_hash:
        raise ValueError("STOP_MAP_PROPOSAL_STALE_PARENT")
    rows = [{"ordinal": index, "operation": value}
            for index, value in enumerate(operations)]
    proposal_hash = sha256_bytes(canonical(rows).encode("utf-8"))
    store.insert_rows("map_proposal", [{"proposal_id": proposal_id,
        "base_generation": base_generation, "base_map_hash": base_map_hash,
        "graph_hash": graph_hash, "validator_version": validator_version,
        "status": "PROPOSED", "proposal_set_hash": proposal_hash}])
    store.insert_rows("map_proposal_operation", [{"proposal_id": proposal_id,
        "ordinal": row["ordinal"], "operation_json": canonical(row["operation"])}
        for row in rows])
    return proposal_hash


def validate_map_proposal_parent(store: Any, proposal_id: str,
                                 active_generation: str) -> None:
    row = store.db.execute("SELECT base_generation,base_map_hash FROM map_proposal "
                           "WHERE proposal_id=?", (proposal_id,)).fetchone()
    if row is None or row["base_generation"] != active_generation or \
            store.meta().get("generation_id") != active_generation or \
            row["base_map_hash"] != store.hashes()["map_hash"]:
        raise ValueError("STOP_MAP_PROPOSAL_STALE_PARENT")
