"""Post-run control provenance over immutable, audited FLOW_V1 occurrences."""

from __future__ import annotations

import hashlib
from typing import Any

from auto67_predecessor import register_writes, validate_flow_predecessor
from auto67_control_provenance_decode import (
    RegisterValue, effective_address as _effective_address,
    indirect_consumer as _consumer, memory_source as _memory_source,
    read_ea as _read_ea, register_name as _name,
    exact_target_range as _target_range, signed8 as _s8, signed16 as _s16,
)

FLOW_INSTRUCTION = 1
CONTROL_PROVENANCE_SCHEMA = "oasis.m12.control-provenance.v1"
ROM_SHA = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"

def static_indirect_candidates(rom: bytes) -> list[dict[str, Any]]:
    """Return syntax-only candidates; this function never asserts executed code."""
    result = []
    for pc in range(0, len(rom) - 1, 2):
        opcode = int.from_bytes(rom[pc:pc + 2], "big")
        candidate = _consumer(rom, pc, opcode)
        if candidate and candidate.get("kind") != "INDIRECT":
            result.append({"status": "STATIC_CANDIDATE", "pc": pc,
                           "opcode": opcode, "kind": candidate["kind"],
                           "addressing_mode": candidate["ea"]["kind"]})
    return result


def _exact_move(rom: bytes, pc: int, opcode: int,
                regs: dict[str, RegisterValue], segment: dict[str, Any],
                row: tuple[int, ...], index: int) -> tuple[set[str], bool]:
    top = opcode >> 12
    if top not in (1, 2, 3): return set(), False
    width = {1: 1, 2: 4, 3: 2}[top]
    sm, sr = (opcode >> 3) & 7, opcode & 7
    dm, dr = (opcode >> 6) & 7, (opcode >> 9) & 7
    decoded = _read_ea(rom, pc, sm, sr, width)
    writes = set()
    if decoded is None: return {f"D{dr}", f"A{dr}"}, False
    ea, _ = decoded
    if sm in (3, 4): writes.add(f"A{sr}")
    if dm in (0, 1):
        dest = _name(dm, dr)
        writes.add(dest)
        if ea["kind"] == "REGISTER":
            source = regs.get(ea["register"], RegisterValue()).copy()
            if width != 4:
                source = RegisterValue(unsupported="PARTIAL_REGISTER_COPY")
            elif source.source:
                source.producers.append(_occurrence(segment, row, index))
        elif ea["kind"] == "IMMEDIATE":
            value = int(ea["value"])
            if dm == 1 and width == 2: value = _s16(value)
            source = RegisterValue(value=value & 0xFFFFFFFF,
                                   low_value=value & ((1 << (8 * width)) - 1),
                                   known_width=4 if dm == 1 or width == 4 else width)
        elif ea["kind"] in {"ABSOLUTE_MEMORY", "MEMORY_REGISTER",
                             "INDEXED_REGISTER", "PC_MEMORY", "PC_INDEXED"}:
            read_regs = regs
            if ea["kind"] == "MEMORY_REGISTER" and ea.get("mode") == 4:
                read_regs = {key: value.copy() for key, value in regs.items()}
                prior_address = read_regs.get(ea["register"], RegisterValue())
                if prior_address.value is not None:
                    prior_address.value = (prior_address.value - width) & 0xFFFFFFFF
                    prior_address.low_value = prior_address.value
            source = _memory_source(ea, width, read_regs, rom)
            if source.source:
                source.producers.append(_occurrence(segment, row, index))
            if dm == 1 and width == 2 and source.low_value is not None:
                source.value = _s16(source.low_value) & 0xFFFFFFFF
                source.known_width = 4
                source.transforms.append({"kind": "SIGN_EXTEND", "from_width": 2,
                                          "to_width": 4})
        else:
            source = RegisterValue(unsupported="SOURCE_KIND_UNSUPPORTED")
        if dm == 0 and width != 4:
            # MOVE.B/W preserves upper Dn bits; retain only the exact low part.
            source.value = None
        if source.source:
            source.register_chain.append({
                "operation": "COPY" if ea["kind"] == "REGISTER" else "MEMORY_READ",
                "register": dest,
                "instruction_occurrence": _occurrence(segment, row, index),
                "source_register": ea.get("register") if ea["kind"] == "REGISTER" else None,
                "source_register_value": source.value if ea["kind"] == "REGISTER" else None,
                "source_effective_address": source.source.get("cpu_address"),
                "source_width": source.source.get("width"),
                "source_value": source.source.get("value"),
                "register_value": source.value if source.value is not None else source.low_value})
        regs[dest] = source
        if sm in (3, 4):
            source_register = f"A{sr}"
            if source_register != dest:
                regs[source_register] = RegisterValue(unsupported="MOVE_SIDE_EFFECT")
        return writes, True
    if dm in (3, 4): writes.add(f"A{dr}")
    for name in writes:
        regs[name] = RegisterValue(unsupported="MOVE_SIDE_EFFECT")
    return writes, True


def _apply(rom: bytes, row: tuple[int, ...], regs: dict[str, RegisterValue],
           segment: dict[str, Any], index: int) -> None:
    _, _, pc, _, opcode, flags, _ = row
    if not flags & FLOW_INSTRUCTION: return
    writes, decoded = _exact_move(rom, pc, opcode, regs, segment, row, index)
    if decoded:
        destination = None
        top = opcode >> 12
        if top in (1, 2, 3) and ((opcode >> 6) & 7) in (0, 1):
            destination = _name((opcode >> 6) & 7, (opcode >> 9) & 7)
        for name in writes:
            if name != destination:
                regs[name] = RegisterValue(unsupported="MOVE_SIDE_EFFECT")
        return
    top = opcode >> 12
    if (opcode & 0xF100) == 0x7000:
        reg, value = (opcode >> 9) & 7, _s8(opcode & 0xFF)
        regs[f"D{reg}"] = RegisterValue(value=value & 0xFFFFFFFF,
                                        low_value=value & 0xFFFFFFFF, known_width=4,
                                        producers=[_occurrence(segment, row, index)])
        return
    if (opcode & 0xFFF8) == 0x48C0:
        name = f"D{opcode & 7}"
        prior = regs.get(name, RegisterValue())
        if prior.low_value is None:
            regs[name] = RegisterValue(unsupported="SOURCE_TRANSFORMED_UNSUPPORTED")
        else:
            value = _s16(prior.low_value & 0xFFFF) & 0xFFFFFFFF
            current = prior.copy()
            current.value = current.low_value = value
            current.known_width = 4
            current.transforms.append({"kind": "SIGN_EXTEND", "from_width": 2,
                                       "to_width": 4})
            current.producers.append(_occurrence(segment, row, index))
            current.register_chain.append({"operation": "SIGN_EXTEND",
                "register": name, "instruction_occurrence": _occurrence(segment, row, index),
                "input_value": prior.low_value & 0xFFFF, "output_value": value})
            regs[name] = current
        return
    if top == 5 and ((opcode >> 3) & 7) in (0, 1):
        mode, reg = (opcode >> 3) & 7, opcode & 7
        name = f"{'A' if mode == 1 else 'D'}{reg}"
        prior = regs.get(name, RegisterValue())
        size = (opcode >> 6) & 3
        quick = (opcode >> 9) & 7 or 8
        delta = -quick if opcode & 0x0100 else quick
        if prior.value is None or (mode == 0 and size != 2) or \
                (mode == 1 and size == 0):
            reason = "SOURCE_TRANSFORMED_UNSUPPORTED" if prior.source else \
                     "SOURCE_PREDECESSOR_UNRESOLVED"
            regs[name] = RegisterValue(unsupported=reason)
        else:
            current = prior.copy()
            current.value = (prior.value + delta) & 0xFFFFFFFF
            current.low_value = current.value
            current.known_width = 4
            current.transforms.append({"kind": "ADD_CONSTANT", "value": delta})
            current.producers.append(_occurrence(segment, row, index))
            current.register_chain.append({"operation": "ADD_CONSTANT",
                "register": name, "instruction_occurrence": _occurrence(segment, row, index),
                "input_value": prior.value, "constant": delta,
                "output_value": current.value})
            regs[name] = current
        return
    if top == 13 and ((opcode >> 6) & 7) in (3, 7):
        if ((opcode >> 6) & 7) != 7:
            dest = f"A{(opcode >> 9) & 7}"
            regs[dest] = RegisterValue(unsupported="SOURCE_TRANSFORMED_UNSUPPORTED")
            return
        src_mode, src_reg = (opcode >> 3) & 7, opcode & 7
        dest = f"A{(opcode >> 9) & 7}"
        source_ea = _read_ea(rom, pc, src_mode, src_reg, 4)
        source = (regs.get(source_ea[0]["register"], RegisterValue()).copy()
                  if source_ea and source_ea[0]["kind"] == "REGISTER" else None)
        base = regs.get(dest, RegisterValue())
        if source is None or source.value is None or base.value is None:
            has_source = bool((source and source.source) or base.source)
            reason = "SOURCE_TRANSFORMED_UNSUPPORTED" if has_source else \
                     "SOURCE_PREDECESSOR_UNRESOLVED"
            regs[dest] = RegisterValue(unsupported=reason)
        elif source.source and not base.source:
            result = source.copy()
            result.value = (base.value + source.value) & 0xFFFFFFFF
            result.transforms.append({"kind": "ADD_REGISTER_BASE", "base_register": dest,
                                      "base_value": base.value})
            result.producers.append(_occurrence(segment, row, index))
            result.register_chain.append({"operation": "ADD_REGISTER_BASE",
                "source_register": source_ea[0].get("register"), "base_register": dest,
                "instruction_occurrence": _occurrence(segment, row, index),
                "base_value": base.value, "offset_value": source.value,
                "output_value": result.value})
            regs[dest] = result
        elif base.source or source.source:
            regs[dest] = RegisterValue(unsupported="POINTER_OFFSET_CLASSIFICATION_AMBIGUOUS")
        else:
            regs[dest] = RegisterValue(value=(base.value + source.value) & 0xFFFFFFFF,
                                       low_value=(base.value + source.value) & 0xFFFFFFFF,
                                       known_width=4)
        return
    if (opcode & 0xF1C0) == 0x41C0:
        dest = f"A{(opcode >> 9) & 7}"
        mode, reg = (opcode >> 3) & 7, opcode & 7
        ea = _read_ea(rom, pc, mode, reg, 4)
        value = _effective_address(ea[0], regs) if ea else None
        inherited = RegisterValue()
        if ea and ea[0]["kind"] in {"MEMORY_REGISTER", "INDEXED_REGISTER"}:
            inherited = regs.get(ea[0]["register"], RegisterValue()).copy()
        if value is None:
            regs[dest] = RegisterValue(unsupported="SOURCE_EFFECTIVE_ADDRESS_UNRESOLVED")
        elif inherited.source:
            inherited.value = inherited.low_value = value
            inherited.known_width = 4
            inherited.transforms.append({"kind": "ADDRESS_CALCULATION"})
            inherited.producers.append(_occurrence(segment, row, index))
            inherited.register_chain.append({"operation": "ADDRESS_CALCULATION",
                "register": dest, "instruction_occurrence": _occurrence(segment, row, index),
                "output_value": value})
            regs[dest] = inherited
        else:
            regs[dest] = RegisterValue(value=value, low_value=value, known_width=4)
        return
    decoded = register_writes(rom, pc, opcode)
    if decoded.get("status") == "PROVEN":
        for item in decoded.get("writes", []):
            prior = regs.get(item["register"], RegisterValue())
            reason = "SOURCE_TRANSFORMED_UNSUPPORTED" if prior.source else \
                     "SOURCE_PREDECESSOR_UNRESOLVED"
            regs[item["register"]] = RegisterValue(
                unsupported=reason)
    else:
        # An unsupported executed instruction may write any queried register.
        for name, prior in tuple(regs.items()):
            reason = "SOURCE_TRANSFORMED_UNSUPPORTED" if prior.source else \
                     "SOURCE_PREDECESSOR_UNRESOLVED"
            regs[name] = RegisterValue(unsupported=reason)


def _consumer_target(rom: bytes, consumer: dict[str, Any],
                     regs: dict[str, RegisterValue]) -> tuple[int | None, RegisterValue, dict[str, Any]]:
    ea = consumer["ea"]
    kind = ea["kind"]
    if kind in {"MEMORY_REGISTER", "INDEXED_REGISTER"}:
        address = _effective_address(ea, regs)
        register = ea["register"]
        value = regs.get(register, RegisterValue()).copy()
        if address is not None and (ea.get("displacement", 0) or kind == "INDEXED_REGISTER"):
            if value.source:
                value.transforms.append({"kind": "ADDRESS_CALCULATION",
                    "displacement": int(ea.get("displacement", 0)),
                    "index_register": ea.get("index_register"),
                    "index_value": regs.get(str(ea.get("index_register")), RegisterValue()).value})
            value.value = address
        return address, value, {"effective_address": address, "addressing_mode": kind}
    if kind in {"PC_MEMORY", "PC_INDEXED"}:
        address = _effective_address(ea, regs)
        width = 2 if kind == "PC_MEMORY" else 1
        source_start = consumer["pc"] + (2 if kind == "PC_MEMORY" else 3)
        source_bytes = rom[source_start:source_start + width]
        source = {"kind": "ROM_INSTRUCTION_OFFSET", "start": source_start,
                  "end": source_start + width, "width": width,
                  "bytes_hex": source_bytes.hex().upper(),
                  "stored_value": int.from_bytes(source_bytes, "big"),
                  "signed": True, "base": ea["base"], "scale": 1}
        if kind == "PC_INDEXED":
            source["selected_index_register"] = ea.get("index_register")
            source["selected_index_value"] = regs.get(
                str(ea.get("index_register")), RegisterValue()).value
        value = RegisterValue(value=address, low_value=address, known_width=4,
                              source=source,
                              transforms=[{"kind": "ADDRESS_CALCULATION",
                                           "base": ea["base"],
                                           "index_register": ea.get("index_register"),
                                           "index_value": regs.get(str(ea.get("index_register")),
                                               RegisterValue()).value}])
        return address, value, {"effective_address": address, "addressing_mode": kind}
    return None, RegisterValue(unsupported="SOURCE_UNRESOLVED"), {
        "addressing_mode": kind}


def _occurrence(segment: dict[str, Any], row: tuple[int, ...], index: int) -> dict[str, Any]:
    return {"run_id": int(segment["run_id"]), "epoch": int(segment["epoch"]),
            "worker_id": int(segment["worker_id"]),
            "capture_id": int(segment["capture_id"]),
            "generation": int(segment["generation"]), "record_index": index,
            "stream_sequence": row[0], "instruction_sequence": row[1],
            "pc": row[2], "opcode": row[4], "next_pc": row[3]}


def _occurrence_bytes(rom: bytes, segment: dict[str, Any],
                     occurrence: dict[str, Any]) -> dict[str, Any]:
    index = int(occurrence["record_index"])
    pc, opcode = int(occurrence["pc"]), int(occurrence["opcode"])
    end = pc + 2
    top = opcode >> 12
    if top in (1, 2, 3):
        decoded = _read_ea(rom, pc, (opcode >> 3) & 7, opcode & 7,
                           {1: 1, 2: 4, 3: 2}[top])
        if decoded:
            end = decoded[1]
    elif (opcode & 0xFFC0) in (0x4E80, 0x4EC0):
        decoded = _read_ea(rom, pc, (opcode >> 3) & 7, opcode & 7, 4)
        if decoded:
            end = decoded[1]
    elif (opcode & 0xF1C0) == 0x41C0:
        decoded = _read_ea(rom, pc, (opcode >> 3) & 7, opcode & 7, 4)
        if decoded:
            end = decoded[1]
    return {**occurrence, "instruction_range": {"start": pc, "end": end},
            "instruction_bytes_hex": rom[pc:end].hex().upper()}


def analyze_flow_segment(segment: dict[str, Any], rows: list[tuple[int, ...]],
                         rom: bytes, rom_sha256: str = ROM_SHA) -> dict[str, Any]:
    """Analyze only actual indirect consumers inside one audited FLOW segment."""
    if rom_sha256 != hashlib.sha256(rom).hexdigest():
        raise ValueError("STOP_CONTROL_PROVENANCE_ROM_IDENTITY_MISMATCH")
    predecessor = validate_flow_predecessor(segment, rows)
    if predecessor["status"] != "PASS_BOUNDED_FLOW_PREDECESSOR_V1":
        return {"status": predecessor["status"], "consumers": [],
                "segment_identity": predecessor.get("segment_identity")}
    identity = predecessor["segment_identity"]
    instructions = predecessor["instructions"]
    regs: dict[str, RegisterValue] = {}
    consumers = []
    for index, row in instructions:
        stream, sequence, pc, next_pc, opcode, flags, _ = row
        if pc < 0 or pc + 2 > len(rom) or int.from_bytes(rom[pc:pc + 2], "big") != opcode:
            return {"status": "STOP_CONSUMER_OCCURRENCE_IDENTITY_MISMATCH",
                    "consumers": [], "segment_identity": identity}
        decoded = _consumer(rom, pc, opcode)
        if decoded:
            item = {"consumer_occurrence": _occurrence(segment, row, index),
                    "addressing_mode": decoded.get("ea", {}).get("kind"),
                    "required_registers": sorted({name for name in (
                        decoded.get("ea", {}).get("register"),
                        decoded.get("ea", {}).get("index_register")) if name}),
                    "consumer_bytes_hex": rom[pc:decoded["instruction_end"]].hex().upper()
                    if decoded.get("instruction_end") else None,
                    "consumer_instruction_range": {
                        "start": pc, "end": decoded.get("instruction_end")},
                    "pre_consumer_register_values": {
                        name: regs.get(name, RegisterValue()).value for name in sorted({item for item in (
                            decoded.get("ea", {}).get("register"),
                            decoded.get("ea", {}).get("index_register")) if item})},
                    "classification": "UNRESOLVED"}
            if decoded.get("status"):
                item["status"] = decoded["status"]
            else:
                target, target_value, effective = _consumer_target(rom, decoded, regs)
                item.update(effective)
                item["actual_next_pc"] = next_pc
                if target is None or target_value.value is None:
                    item["status"] = target_value.unsupported or "SOURCE_UNRESOLVED"
                elif target != next_pc:
                    return {"status": "STOP_CONTROL_PROVENANCE_FLOW_MISMATCH",
                            "consumers": consumers + [item],
                            "segment_identity": identity}
                elif not 0 <= next_pc < len(rom):
                    item["status"] = "TARGET_NOT_CANONICAL_ROM"
                elif not target_value.source:
                    item["status"] = target_value.unsupported or "SOURCE_NON_MEMORY"
                else:
                    source = target_value.source
                    direct = (source.get("kind") == "ROM_MEMORY_READ" and
                              int(source.get("width", 0)) == 4 and
                              not target_value.transforms and
                              int(source.get("value", -1)) == next_pc)
                    transformed = bool(target_value.transforms)
                    if direct:
                        item["classification"] = "UNMODIFIED_FROM_MEMORY_READ"
                        item["relation"] = "OBSERVED_CODE_POINTER_TO"
                    elif source.get("kind") == "ROM_INSTRUCTION_OFFSET" or (
                            int(source.get("width", 0)) == 2 and transformed):
                        item["classification"] = "TRANSFORMED_OFFSET"
                        item["relation"] = "OBSERVED_CODE_OFFSET_TO"
                    elif transformed:
                        item["status"] = "POINTER_OFFSET_CLASSIFICATION_AMBIGUOUS"
                    else:
                        item["status"] = "SOURCE_TRANSFORMED_UNSUPPORTED"
                    if item.get("relation"):
                        target_range = _target_range(rom, next_pc)
                        if target_range is None:
                            return {"status": "STOP_TARGET_ROM_MAPPING_UNRESOLVED",
                                    "consumers": consumers + [item],
                                    "segment_identity": identity}
                        producer_occurrences = [_occurrence_bytes(rom, segment, producer)
                            for producer in target_value.producers]
                        register_chain = []
                        for step in target_value.register_chain:
                            current_step = dict(step)
                            occurrence = current_step.get("instruction_occurrence")
                            if occurrence:
                                current_step["instruction_occurrence"] = _occurrence_bytes(
                                    rom, segment, occurrence)
                            register_chain.append(current_step)
                        item.update({"consumer_occurrence": _occurrence_bytes(
                                rom, segment, item["consumer_occurrence"]),
                            "status": "OBSERVED_RUNTIME", "source": source,
                            "register_transforms": target_value.transforms,
                            "register_chain": register_chain,
                            "target_rom_pc": next_pc,
                            "target_opcode": int.from_bytes(rom[next_pc:next_pc + 2], "big"),
                            "target_rom_instruction": target_range,
                            "producer_occurrences": producer_occurrences,
                            "identity": {"rom_sha256": rom_sha256, **dict(zip(
                                ("run_id", "epoch", "worker_id", "capture_id", "generation"),
                                identity))}})
                        if source.get("table_entry"):
                            item["jump_table_entry"] = {
                                "start": source["start"], "end": source["end"],
                                "index_register": source.get("selected_index_register"),
                                "index_value": source.get("selected_index_value"),
                            "index": source.get("selected_index"),
                                "entry_base": source.get("table_base"),
                                "entry_offset": source.get("selected_entry_offset"),
                                "entry_width": source.get("entry_width"),
                                "bytes_hex": source["bytes_hex"],
                                "value": source["value"]}
                            item["jump_table_relation"] = "OBSERVED_JUMP_TABLE_ENTRY_TO"
            consumers.append(item)
        _apply(rom, row, regs, segment, index)
    return {"status": "PASS_CONTROL_PROVENANCE_V1", "consumers": consumers,
            "segment_identity": identity}


def merge_relation_evidence(analyses: list[dict[str, Any]]) -> dict[str, Any]:
    """Deduplicate canonical relation identity while preserving each witness."""
    relations: dict[tuple, dict[str, Any]] = {}
    for analysis in analyses:
        for event in analysis.get("consumers", []):
            relation = event.get("relation")
            source, target = event.get("source"), event.get("target_rom_pc")
            if not relation or not source or target is None: continue
            rom_sha256 = event.get("identity", {}).get("rom_sha256")
            key = (rom_sha256, source.get("start"), source.get("end"), relation, target)
            current = relations.setdefault(key, {"source": source, "target_rom_pc": target,
                "rom_sha256": rom_sha256,
                "target_rom_instruction": event.get("target_rom_instruction"),
                "relation": relation, "status": "OBSERVED_RUNTIME", "evidence": []})
            witness = {"consumer_occurrence": event["consumer_occurrence"],
                       "producer_occurrences": event.get("producer_occurrences", []),
                       "register_transforms": event.get("register_transforms", []),
                       "actual_next_pc": event.get("actual_next_pc")}
            if witness not in current["evidence"]: current["evidence"].append(witness)
    return {"relations": [relations[key] for key in sorted(relations)],
            "unique_relation_count": len(relations),
            "evidence_occurrence_count": sum(len(item["evidence"])
                                              for item in relations.values())}
