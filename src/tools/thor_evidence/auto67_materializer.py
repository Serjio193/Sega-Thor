"""Bounded worker-side materialization of decoded AUTO67 observations."""

from __future__ import annotations

from typing import Any

from auto67_capsule_codec import DecodedCapsule


def _hex(value: int) -> str:
    return f"0x{value:06X}"


def _seed_fields(seed: dict[str, Any]) -> dict[str, Any]:
    return {name: seed[name] for name in ("kind", "pc", "address")
            if seed.get(name) is not None}


def materialize(seed: dict[str, Any], capsule: DecodedCapsule) -> dict[str, Any]:
    """Return evidence, not a semantic graph or an inferred causal closure."""
    observations = []
    causal_facts = []
    seen_facts: set[tuple[str, str]] = set()
    seed_pair = (seed.get("pc"), seed.get("address"))
    for record in capsule.observations:
        observation = {"kind": record.kind or "LEGACY_UNKNOWN",
                       "pc": _hex(record.pc), "address": _hex(record.address)}
        observations.append(observation)
        if record.kind != "BUS_WRITE_PC":
            continue
        pair = (observation["pc"], observation["address"])
        if pair == seed_pair or pair in seen_facts:
            continue
        seen_facts.add(pair)
        causal_facts.append({
            "kind": "BUS_WRITE_PC",
            "writer_pc": observation["pc"],
            "address": observation["address"],
            "evidence": "RUNTIME_CAPSULE",
        })
    return {
        "seed": _seed_fields(seed),
        "runtime_observations": observations,
        "causal_facts": causal_facts,
        "unresolved_frontier": {
            "status": "UNKNOWN",
            "missing": ["STATIC_DECODE", "REGISTER_PROVENANCE"],
            "evidence": "RUNTIME_CAPSULE",
        },
        "capsule_format_version": capsule.format_version,
        "capsule_record_count": capsule.event_count,
    }
