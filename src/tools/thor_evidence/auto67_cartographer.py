"""AUTO67 local-chain adapter for the existing MAP-1 Cartographer."""

from __future__ import annotations

from typing import Any

try:
    from .cartographer import digest
except ImportError:
    from cartographer import digest


LOCAL_CHAIN_SCHEMA = "oasis.m12.auto67.local-chain.v1"
PROOF_CONTRACT = "REGISTER_REACHING_DEFINITION_V1"
_REGISTERS = {"A4", "A5"}


def local_chain(event: dict[str, Any], materialized: dict[str, Any] | None,
                investigation_id: str, lease_id: str) -> dict[str, Any]:
    """Build a bounded worker result; it contains no global classification."""
    data = materialized or {}
    occurrence = {name: event.get(name) for name in
                  ("epoch", "seq", "occurrence_id", "window_item_id", "frame")}
    return {
        "local_chain_schema": LOCAL_CHAIN_SCHEMA,
        "investigation_id": investigation_id,
        "lease_id": lease_id,
        "occurrence": occurrence,
        "chain_steps": list(data.get("chain_steps", [])),
        "runtime_observations": list(data.get("runtime_observations", [])),
        "observed_facts": list(data.get("observed_facts", [])),
        "causal_facts": list(data.get("causal_facts", [])),
        "register_provenance": dict(data.get("register_provenance", {})),
        "capture_diagnostics": dict(data.get("capture_diagnostics", {})),
    }


def _valid_pc(value: Any) -> bool:
    try:
        number = int(str(value), 0)
    except (TypeError, ValueError):
        return False
    return 0 <= number <= 0xFFFFFF


def _valid_occurrence(value: Any) -> bool:
    if not isinstance(value, dict):
        return False
    try:
        int(value["epoch"])
        sequence = value.get("seq") if "seq" in value else value.get("sequence")
        int(sequence)
    except (KeyError, TypeError, ValueError):
        return False
    return True


def _valid_step(step: Any) -> bool:
    if not isinstance(step, dict) or step.get("kind") != "REGISTER_REACHING_DEFINITION":
        return False
    if step.get("register") not in _REGISTERS:
        return False
    if not _valid_pc(step.get("producer_pc")) or not _valid_pc(step.get("consumer_pc")):
        return False
    producer, consumer = step.get("producer_occurrence"), step.get("consumer_occurrence")
    evidence = step.get("evidence")
    if not _valid_occurrence(producer) or not _valid_occurrence(consumer):
        return False
    if not isinstance(evidence, dict) or evidence.get("complete_interval") is not True:
        return False
    if evidence.get("intervening_register_write") is not False:
        return False
    return int(producer["epoch"]) == int(consumer["epoch"])


def _node(kind: str, key: str, lineage: list[dict[str, str]]) -> dict[str, Any]:
    return {"kind": kind, "key": key, "scope": "global", "status": "PROVEN",
            "attributes": {}, "lineage": lineage}


def _node_id(node: dict[str, Any]) -> str:
    return digest({"kind": node["kind"], "key": node["key"],
                   "scope": node.get("scope", "global")})


def _stable_bundle(step: dict[str, Any]) -> tuple[dict[str, Any], str]:
    producer, consumer, register = (str(step["producer_pc"]),
                                    str(step["consumer_pc"]), str(step["register"]))
    stable = {"producer_pc": producer, "consumer_pc": consumer, "register": register,
              "proof_contract": PROOF_CONTRACT}
    stable_hash = digest(stable)
    lineage = [{"source": "AUTO67_LOCAL_CHAIN",
                "proof_contract": PROOF_CONTRACT,
                "schema": LOCAL_CHAIN_SCHEMA,
                "stable_dependency_fingerprint": stable_hash}]
    source = _node("ROM_INSTRUCTION", producer, lineage)
    target = _node("ROM_INSTRUCTION", consumer, lineage)
    edge = {"source": _node_id(source), "target": _node_id(target),
            "relation": f"REGISTER_REACHING_DEFINITION:{register}",
            "scope": "global", "status": "PROVEN", "rule": PROOF_CONTRACT,
            "assumptions": [], "lineage": lineage}
    return {"nodes": [source, target], "edges": [edge],
            "frontiers": [], "resolves_frontiers": []}, stable_hash


def candidate_bundle(chain: dict[str, Any]) -> tuple[dict[str, Any], str] | None:
    """Return one stable MAP-1 bundle only when the local proof contract holds."""
    for step in chain.get("chain_steps", []):
        if _valid_step(step):
            return _stable_bundle(step)
    return None


def import_ref(stable_hash: str) -> str:
    return "auto67-live:" + stable_hash
