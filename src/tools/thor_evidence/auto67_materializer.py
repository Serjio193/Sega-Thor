"""Bounded separation of runtime observations and proven static dependencies."""

from __future__ import annotations

from typing import Any

from auto67_capsule_codec import DecodedCapsule
from auto67_predecessor import PredecessorCapture, resolve as resolve_predecessor


MATERIALIZED_SCHEMA_VERSION = 2
MOVE_SIZES = {1: ("B", 1), 2: ("L", 4), 3: ("W", 2)}


def _hex(value: int) -> str:
    return f"0x{value:06X}"


def _seed_fields(seed: dict[str, Any]) -> dict[str, Any]:
    return {name: seed[name] for name in ("kind", "pc", "address")
            if seed.get(name) is not None}


def _signed16(value: int) -> int:
    return value - 0x10000 if value & 0x8000 else value


def _operand(rom: bytes, cursor: int, mode: int, register: int,
             width: int) -> tuple[dict[str, Any], int] | None:
    if mode == 0:
        return {"kind": "DATA_REGISTER", "register": f"D{register}",
                "text": f"D{register}"}, cursor
    if mode == 1:
        return {"kind": "ADDRESS_REGISTER", "register": f"A{register}",
                "text": f"A{register}"}, cursor
    if mode in {2, 3, 4}:
        suffix = {2: "", 3: "+", 4: "-"}[mode]
        return {"kind": "MEMORY_REGISTER", "register": f"A{register}",
                "text": f"(A{register}){suffix}"}, cursor
    if mode == 5:
        if cursor + 2 > len(rom):
            return None
        displacement = _signed16(int.from_bytes(rom[cursor:cursor + 2], "big"))
        return {"kind": "MEMORY_REGISTER", "register": f"A{register}",
                "displacement": displacement,
                "text": f"{displacement}(A{register})"}, cursor + 2
    if mode == 6:
        if cursor + 2 > len(rom):
            return None
        extension = int.from_bytes(rom[cursor:cursor + 2], "big")
        index_kind = "A" if extension & 0x8000 else "D"
        index = (extension >> 12) & 7
        displacement = extension & 0xFF
        if displacement & 0x80:
            displacement -= 0x100
        return {"kind": "MEMORY_REGISTER_INDEXED", "register": f"A{register}",
                "index_register": f"{index_kind}{index}",
                "displacement": displacement,
                "text": f"d8(A{register},{index_kind}{index})"}, cursor + 2
    if mode != 7:
        return None
    if register == 0:
        if cursor + 2 > len(rom):
            return None
        address = _signed16(int.from_bytes(rom[cursor:cursor + 2], "big")) & 0xFFFFFF
        return {"kind": "ABSOLUTE_MEMORY", "address": address,
                "text": f"${address:06X}.W"}, cursor + 2
    if register == 1:
        if cursor + 4 > len(rom):
            return None
        address = int.from_bytes(rom[cursor:cursor + 4], "big") & 0xFFFFFF
        return {"kind": "ABSOLUTE_MEMORY", "address": address,
                "text": f"${address:06X}.L"}, cursor + 4
    if register == 2:
        if cursor + 2 > len(rom):
            return None
        displacement = _signed16(int.from_bytes(rom[cursor:cursor + 2], "big"))
        return {"kind": "PC_MEMORY", "register": "PC",
                "displacement": displacement, "text": f"{displacement}(PC)"}, cursor + 2
    if register == 3:
        if cursor + 2 > len(rom):
            return None
        extension = int.from_bytes(rom[cursor:cursor + 2], "big")
        index_kind = "A" if extension & 0x8000 else "D"
        index = (extension >> 12) & 7
        return {"kind": "PC_MEMORY_INDEXED", "register": "PC",
                "index_register": f"{index_kind}{index}",
                "displacement": extension & 0xFF,
                "text": f"d8(PC,{index_kind}{index})"}, cursor + 2
    if register == 4:
        immediate_bytes = 4 if width == 4 else 2
        if cursor + immediate_bytes > len(rom):
            return None
        value = int.from_bytes(rom[cursor:cursor + immediate_bytes], "big")
        return {"kind": "IMMEDIATE", "value": value,
                "text": f"#${value:X}"}, cursor + immediate_bytes
    return None


def _decode_move_write(rom: bytes, pc: int,
                       observed_address: int | None) -> dict[str, Any]:
    if pc < 0 or pc + 2 > len(rom):
        return {"status": "UNKNOWN", "missing": ["ROM_BYTES"],
                "reason": "instruction PC is outside the ROM"}
    opcode = int.from_bytes(rom[pc:pc + 2], "big")
    size = MOVE_SIZES.get(opcode >> 12)
    if size is None:
        return {"status": "UNKNOWN", "missing": ["STATIC_DECODE"],
                "reason": f"unsupported opcode 0x{opcode:04X}", "opcode": opcode}
    mnemonic, width = size
    source_mode, source_reg = (opcode >> 3) & 7, opcode & 7
    destination_mode, destination_reg = (opcode >> 6) & 7, (opcode >> 9) & 7
    source = _operand(rom, pc + 2, source_mode, source_reg, width)
    if source is None:
        return {"status": "UNKNOWN", "missing": ["STATIC_DECODE"],
                "reason": "source operand is truncated", "opcode": opcode}
    destination = _operand(rom, source[1], destination_mode, destination_reg, width)
    if destination is None:
        return {"status": "UNKNOWN", "missing": ["STATIC_DECODE"],
                "reason": "destination operand is truncated", "opcode": opcode}
    destination_data = destination[0]
    memory_destination = destination_data["kind"] in {
        "MEMORY_REGISTER", "MEMORY_REGISTER_INDEXED", "ABSOLUTE_MEMORY",
        "PC_MEMORY", "PC_MEMORY_INDEXED"}
    if not memory_destination:
        return {"status": "UNKNOWN", "missing": ["MEMORY_WRITE_SEMANTICS"],
                "reason": "decoded instruction does not write memory", "opcode": opcode}
    if observed_address is not None and \
            destination_data.get("kind") == "ABSOLUTE_MEMORY" and \
            destination_data.get("address") != observed_address:
        return {"status": "UNKNOWN", "missing": ["OBSERVED_ADDRESS_MATCH"],
                "reason": "absolute destination differs from observed address", "opcode": opcode}
    return {"status": "PROVEN", "opcode": opcode, "mnemonic": f"MOVE.{mnemonic}",
            "source": source[0], "destination": destination_data, "width_bytes": width}


def _derive(seed: dict[str, Any], rom: bytes | None) -> tuple[list[dict[str, Any]], dict[str, Any]]:
    if seed.get("kind") != "BUS_WRITE_PC":
        return [], {"status": "UNKNOWN", "missing": ["BUS_WRITE_PC_SEED"],
                    "evidence": "AUTO67.5_STATIC_DECODER"}
    if rom is None:
        return [], {"status": "UNKNOWN", "missing": ["ROM_BYTES"],
                    "evidence": "AUTO67.5_STATIC_DECODER"}
    try:
        pc, address = int(str(seed["pc"]), 16), int(str(seed["address"]), 16)
    except (KeyError, TypeError, ValueError):
        return [], {"status": "UNKNOWN", "missing": ["VALID_SEED_FIELDS"],
                    "evidence": "AUTO67.5_STATIC_DECODER"}
    decoded = _decode_move_write(rom, pc, address)
    if decoded.get("status") != "PROVEN":
        return [], {"status": "UNKNOWN", "missing": decoded.get("missing", ["STATIC_DECODE"]),
                    "reason": decoded.get("reason"), "opcode": decoded.get("opcode"),
                    "evidence": "AUTO67.5_STATIC_DECODER"}
    facts = []
    common = {"instruction_pc": _hex(pc), "opcode": f"0x{decoded['opcode']:04X}",
              "destination": decoded["destination"]["text"],
              "observed_address": _hex(address),
              "derivation_rule": "M68K_MOVE_MEMORY_WRITE_STATIC_SEMANTICS",
              "evidence": "STATIC_ROM_DECODER"}
    source = decoded["source"]
    if source["kind"] in {"DATA_REGISTER", "ADDRESS_REGISTER"}:
        facts.append({"kind": "INSTRUCTION_SOURCE_REGISTER", "source_register": source["register"],
                      **common})
    elif source["kind"] in {"ABSOLUTE_MEMORY", "PC_MEMORY", "PC_MEMORY_INDEXED",
                             "MEMORY_REGISTER", "MEMORY_REGISTER_INDEXED"}:
        memory_fact = {"kind": "INSTRUCTION_SOURCE_MEMORY",
                       "source_operand": source["text"], **common}
        if source["kind"] in {"MEMORY_REGISTER", "MEMORY_REGISTER_INDEXED"}:
            memory_fact["source_register"] = source["register"]
        facts.append(memory_fact)
    elif source["kind"] == "IMMEDIATE":
        facts.append({"kind": "INSTRUCTION_SOURCE_IMMEDIATE", "source_operand": source["text"],
                      **common})
    if "register" in decoded["destination"]:
        facts.append({"kind": "ADDRESS_DEPENDENCY", "address_register": decoded["destination"]["register"],
                      **common})
    next_missing = ["REGISTER_PROVENANCE"] if any(
        fact["kind"] in {"INSTRUCTION_SOURCE_REGISTER", "ADDRESS_DEPENDENCY"} for fact in facts) else []
    frontier = {"status": "UNKNOWN", "missing": next_missing,
                "next": "resolve source/address register provenance" if next_missing else
                "resolve RAM version producer", "evidence": "AUTO67.5_STATIC_DECODER"}
    return facts, frontier


def required_registers(seed: dict[str, Any], rom: bytes | None) -> list[str]:
    facts, _ = _derive(seed, rom)
    names = []
    for fact in facts:
        name = fact.get("source_register") or fact.get("address_register")
        if name and name not in names:
            names.append(name)
    return names


def register_provenance_targets(rom: bytes) -> dict[int, int]:
    """Return statically supported memory-write PCs requiring A4/A5 capture."""
    targets: dict[int, int] = {}
    for pc in range(0, len(rom) - 1, 2):
        decoded = _decode_move_write(rom, pc, None)
        if decoded.get("status") != "PROVEN":
            continue
        names: list[str] = []
        source = decoded["source"]
        if source.get("kind") in {"DATA_REGISTER", "ADDRESS_REGISTER"}:
            names.append(source["register"])
        elif source.get("kind") in {"MEMORY_REGISTER", "MEMORY_REGISTER_INDEXED"}:
            names.append(source["register"])
        destination = decoded["destination"]
        if "register" in destination:
            names.append(destination["register"])
        mask = 0
        if "A4" in names:
            mask |= 1
        if "A5" in names:
            mask |= 2
        if mask:
            targets[pc] = mask
    return targets


def materialize(seed: dict[str, Any], capsule: DecodedCapsule,
                rom: bytes | None = None,
                predecessor: PredecessorCapture | None = None) -> dict[str, Any]:
    """Materialize bounded evidence without temporal adjacency as causality."""
    observations = []
    observed_facts = []
    seen_observations: set[tuple[str, str, str]] = set()
    for record in capsule.observations:
        kind = record.kind or "LEGACY_UNKNOWN"
        observation = {"kind": kind, "pc": _hex(record.pc), "address": _hex(record.address)}
        observations.append(observation)
        key = (kind, observation["pc"], observation["address"])
        if record.kind == "BUS_WRITE_PC" and key not in seen_observations:
            observed_facts.append({"kind": kind, "pc": observation["pc"],
                                   "address": observation["address"],
                                   "evidence": "RUNTIME_CAPSULE"})
            seen_observations.add(key)
    causal_facts, frontier = _derive(seed, rom)
    requested = required_registers(seed, rom)
    if requested:
        if predecessor is None:
            provenance = {"status": "UNRESOLVED", "requested": requested,
                          "reason": "PREDECESSOR_CAPTURE_MISSING"}
            chain_steps = []
        else:
            resolved = resolve_predecessor(predecessor, rom or b"", requested)
            chain_steps = resolved["steps"]
            provenance = {"status": "PROVEN" if not resolved["unresolved"] else "UNRESOLVED",
                          "requested": requested, "resolved": [item["register"] for item in chain_steps],
                          "unresolved": resolved["unresolved"], "reason": resolved["reason"],
                          "capture": {"path": predecessor.path, "epoch": predecessor.epoch,
                                      "target_pc": _hex(predecessor.target_pc),
                                      "first_sequence": predecessor.first_sequence,
                                      "last_sequence": predecessor.last_sequence,
                                      "record_count": len(predecessor.records),
                                      "complete": predecessor.complete,
                                      "truncated": predecessor.truncated,
                                      "gap": predecessor.gap,
                                      "format_version": predecessor.format_version,
                                      "ring_capacity": predecessor.ring_capacity,
                                      "ring_wrapped": predecessor.ring_wrapped,
                                      "overwrites": predecessor.overwrites,
                                      "consumer_pc": _hex(predecessor.consumer_pc)
                                      if predecessor.consumer_pc is not None else None,
                                      "join_status": predecessor.join_status,
                                      "consumer_sequence": predecessor.consumer_sequence,
                                      "consumer_frame": predecessor.consumer_frame}}
        if chain_steps:
            frontier = {"status": "PROVEN" if not provenance["unresolved"] else "UNKNOWN",
                        "missing": provenance["unresolved"],
                        "next": "resolve remaining register provenance" if provenance["unresolved"] else
                        "resolve RAM version producer", "evidence": "AUTO67.6R_PREDECESSOR"}
        else:
            frontier["missing"] = ["REGISTER_PROVENANCE"]
            frontier["next"] = "capture complete predecessor interval for A4/A5"
    else:
        provenance = {"status": "NOT_REQUIRED", "requested": [], "resolved": [],
                      "unresolved": [], "reason": None}
        chain_steps = []
    return {
        "materialized_schema_version": MATERIALIZED_SCHEMA_VERSION,
        "seed": _seed_fields(seed), "runtime_observations": observations,
        "observed_facts": observed_facts, "causal_facts": causal_facts,
        "chain_steps": chain_steps, "register_provenance": provenance,
        "unresolved_frontier": frontier,
        "capsule_format_version": capsule.format_version,
        "capsule_record_count": capsule.event_count,
    }
