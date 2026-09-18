#!/usr/bin/env python3
"""Independent occurrence and source-chain audit for CONTROL_PROVENANCE_V1."""

from __future__ import annotations

from typing import Any


def _signed(value: int, width: int) -> int:
    sign = 1 << (width * 8 - 1)
    return value - (1 << (width * 8)) if value & sign else value


def _register(opcode: int) -> tuple[str, str, int] | None:
    if (opcode & 0xF100) == 0x7000:
        return "D", f"D{(opcode >> 9) & 7}", _signed(opcode & 0xFF, 1)
    if opcode >> 12 in (1, 2, 3):
        width = {1: 1, 2: 4, 3: 2}[opcode >> 12]
        dest_mode, dest = (opcode >> 6) & 7, (opcode >> 9) & 7
        if dest_mode == 1 and width == 4 and ((opcode >> 3) & 7) == 7 and (opcode & 7) == 4:
            pc_ext = 0  # immediate starts after the opcode
            value = 0  # filled by caller when ROM bytes are available
            return "IMMEDIATE_A", f"A{dest}", pc_ext + value
        if dest_mode in (0, 1):
            return "MOVE", f"{'A' if dest_mode else 'D'}{dest}", width
    return None


def _record_occurrence(event: dict[str, Any], row: tuple[int, ...], index: int,
                       segment: dict[str, Any]) -> bool:
    occurrence = event
    return all(int(occurrence.get(name, -1)) == expected for name, expected in {
        "run_id": int(segment["run_id"]), "epoch": int(segment["epoch"]),
        "worker_id": int(segment["worker_id"]),
        "capture_id": int(segment["capture_id"]),
        "generation": int(segment["generation"]), "record_index": index,
        "stream_sequence": row[0], "instruction_sequence": row[1],
        "pc": row[2], "opcode": row[4], "next_pc": row[3]}.items())


def _instruction_end(rom: bytes, pc: int, opcode: int) -> int:
    if (opcode & 0xFFC0) in (0x4E80, 0x4EC0):
        mode, register = (opcode >> 3) & 7, opcode & 7
        extension = 2 if mode in (5, 6) or (mode == 7 and register in (2, 3)) else 0
        return pc + 2 + extension
    top = opcode >> 12
    if top in (1, 2, 3):
        mode, register = (opcode >> 3) & 7, opcode & 7
        width = {1: 1, 2: 4, 3: 2}[top]
        extension = (2 if mode in (5, 6) or (mode == 7 and register in (0, 2, 3))
                     else 4 if mode == 7 and register == 1
                     else (4 if width == 4 else 2)
                     if mode == 7 and register == 4 else 0)
        return pc + 2 + extension
    if (opcode & 0xF1C0) == 0x41C0:
        mode, register = (opcode >> 3) & 7, opcode & 7
        extension = (2 if mode in (5, 6) or (mode == 7 and register in (0, 2, 3))
                     else 4 if mode == 7 and register == 1 else 0)
        return pc + 2 + extension
    return pc + 2


def _audit_instruction_occurrence(rom: bytes, row: tuple[int, ...],
                                  occurrence: dict[str, Any]) -> None:
    pc, opcode = row[2], row[4]
    end = _instruction_end(rom, pc, opcode)
    expected_range = {"start": pc, "end": end}
    if int.from_bytes(rom[pc:pc + 2], "big") != opcode or \
            occurrence.get("instruction_range") != expected_range or \
            occurrence.get("instruction_bytes_hex") != rom[pc:end].hex().upper():
        raise ValueError("STOP_PRODUCER_OPCODE_BYTES_MISMATCH")


def _target_register(opcode: int) -> str | None:
    mode = (opcode >> 3) & 7
    return f"A{opcode & 7}" if mode in (2, 5, 6) else None


def _effective_read_address(rom: bytes, producer: tuple[int, ...],
                            before: dict[str, int]) -> int | None:
    _, _, pc, _, opcode, _, _ = producer
    mode, register = (opcode >> 3) & 7, opcode & 7
    cursor = pc + 2
    width = {1: 1, 2: 4, 3: 2}.get(opcode >> 12, 0)
    base = before.get(f"A{register}")
    if mode in (2, 3):
        return base
    if mode == 4:
        if base is None or not width:
            return None
        delta = 2 if width == 1 and register == 7 else width
        return (base - delta) & 0xFFFFFF
    if mode == 7 and register == 0:
        return _signed(int.from_bytes(rom[cursor:cursor + 2], "big"), 2) & 0xFFFFFF
    if mode == 7 and register == 1:
        return int.from_bytes(rom[cursor:cursor + 4], "big") & 0xFFFFFF
    if mode == 5:
        base = before.get(f"A{register}")
        if base is None: return None
        disp = _signed(int.from_bytes(rom[cursor:cursor + 2], "big"), 2)
        return (base + disp) & 0xFFFFFF
    if mode == 6:
        base = before.get(f"A{register}")
        if base is None: return None
        ext = int.from_bytes(rom[cursor:cursor + 2], "big")
        index = ("A" if ext & 0x8000 else "D") + str((ext >> 12) & 7)
        value = before.get(index)
        if value is None: return None
        if not ext & 0x0800: value = _signed(value & 0xFFFF, 2)
        return (base + value + _signed(ext & 0xFF, 1)) & 0xFFFFFF
    if mode == 7 and register == 3:
        ext = int.from_bytes(rom[cursor:cursor + 2], "big")
        index = ("A" if ext & 0x8000 else "D") + str((ext >> 12) & 7)
        value = before.get(index)
        if value is None:
            return None
        if not ext & 0x0800:
            value = _signed(value & 0xFFFF, 2)
        return (pc + 2 + value + _signed(ext & 0xFF, 1)) & 0xFFFFFF
    return None


def _replay_registers(rom: bytes, rows: list[tuple[int, ...]], before_index: int) -> dict[str, int]:
    values: dict[str, int] = {}
    for row in rows[:before_index]:
        _, _, pc, _, opcode, flags, _ = row
        if not flags & 1: continue
        decoded = _register(opcode)
        if decoded is None: continue
        kind, name, operand = decoded
        if kind == "IMMEDIATE_A":
            values[name] = int.from_bytes(rom[pc + 2:pc + 6], "big")
        elif (opcode & 0xF100) == 0x7000:
            values[name] = operand & 0xFFFFFFFF
        else:
            mode, reg = (opcode >> 3) & 7, opcode & 7
            if mode in (0, 1) and operand == 4:
                source_register = f"{'A' if mode == 1 else 'D'}{reg}"
                if source_register in values:
                    values[name] = values[source_register]
            elif operand in (1, 2, 4) and mode in (2, 3, 4, 5, 6, 7):
                address = _effective_read_address(rom, row, values)
                if address is not None and address + operand <= len(rom):
                    loaded = int.from_bytes(rom[address:address + operand], "big")
                    if name.startswith("A") and operand == 2:
                        loaded = _signed(loaded, 2)
                    values[name] = loaded & 0xFFFFFFFF
                    if mode in (3, 4):
                        source_register = f"A{reg}"
                        if source_register != name:
                            delta = 2 if operand == 1 and reg == 7 else operand
                            prior = values.get(source_register)
                            if prior is not None:
                                values[source_register] = (prior + delta if mode == 3
                                                           else prior - delta) & 0xFFFFFFFF
        if opcode & 0xFFF8 == 0x48C0:
            name = f"D{opcode & 7}"
            if name in values:
                values[name] = _signed(values[name] & 0xFFFF, 2) & 0xFFFFFFFF
    return values


def _audit_relation(rom: bytes, rows: list[tuple[int, ...]], segment: dict[str, Any],
                    event: dict[str, Any]) -> None:
    consumer = event.get("consumer_occurrence", {})
    ci = int(consumer.get("record_index", -1))
    if not 0 <= ci < len(rows) or not _record_occurrence(consumer, rows[ci], ci, segment):
        raise ValueError("STOP_CONSUMER_OCCURRENCE_IDENTITY_MISMATCH")
    row = rows[ci]
    opcode = row[4]
    if (opcode & 0xFFC0) not in (0x4E80, 0x4EC0) or row[3] != event.get("actual_next_pc"):
        raise ValueError("STOP_CONTROL_PROVENANCE_FLOW_MISMATCH")
    if int.from_bytes(rom[row[2]:row[2] + 2], "big") != opcode:
        raise ValueError("STOP_CONSUMER_OPCODE_BYTES_MISMATCH")
    target = int(event["target_rom_pc"])
    if target != row[3] or not 0 <= target + 1 < len(rom):
        raise ValueError("STOP_TARGET_ROM_MAPPING_UNRESOLVED")
    if int.from_bytes(rom[target:target + 2], "big") != event.get("target_opcode"):
        raise ValueError("STOP_TARGET_ROM_BYTES_MISMATCH")
    target_opcode = int(event["target_opcode"])
    target_length, target_type = ((2, "M68K_RTS") if target_opcode == 0x4E75 else
                                  (6, "M68K_JMP_ABSOLUTE_LONG")
                                  if target_opcode == 0x4EF9 else (0, ""))
    target_range = event.get("target_rom_instruction", {})
    if not target_length or target + target_length > len(rom) or \
            target_range != {"start": target, "end": target + target_length,
                             "type": target_type,
                             "bytes_hex": rom[target:target + target_length].hex().upper()}:
        raise ValueError("STOP_TARGET_ROM_MAPPING_UNRESOLVED")
    source = event.get("source", {})
    start, end, width = int(source["start"]), int(source["end"]), int(source["width"])
    if width != end - start or start < 0 or end > len(rom):
        raise ValueError("STOP_SOURCE_RANGE_INVALID")
    raw = rom[start:end]
    source_value = source.get("value", source.get("stored_value"))
    if raw.hex().upper() != source.get("bytes_hex") or \
            int.from_bytes(raw, "big") != source_value:
        raise ValueError("STOP_SOURCE_ROM_BYTES_OR_VALUE_MISMATCH")
    mode, reg = (opcode >> 3) & 7, opcode & 7
    if source.get("kind") == "ROM_INSTRUCTION_OFFSET":
        expected_range = (row[2] + 2, row[2] + 4) if mode == 7 and reg == 2 else \
                         (row[2] + 3, row[2] + 4) if mode == 7 and reg == 3 else None
        if expected_range != (start, end):
            raise ValueError("STOP_POINTER_OFFSET_CLASSIFICATION_AMBIGUOUS")
    if source.get("kind") == "ROM_INSTRUCTION_OFFSET":
        if mode == 7 and reg == 2:
            displacement = _signed(int.from_bytes(raw, "big"), 2)
        elif mode == 7 and reg == 3:
            ext = int.from_bytes(rom[row[2] + 2:row[2] + 4], "big")
            displacement = _signed(ext & 0xFF, 1)
        else:
            raise ValueError("STOP_POINTER_OFFSET_CLASSIFICATION_AMBIGUOUS")
        base = row[2] + 2
        index_value = 0
        if mode == 7 and reg == 3:
            index_name = ("A" if ext & 0x8000 else "D") + str((ext >> 12) & 7)
            index_value = _replay_registers(rom, rows, ci).get(index_name)
            if index_value is None:
                raise ValueError("STOP_SOURCE_EFFECTIVE_ADDRESS_UNRESOLVED")
            if not ext & 0x0800:
                index_value = _signed(index_value & 0xFFFF, 2)
        if (base + displacement + index_value) & 0xFFFFFF != target:
            raise ValueError("STOP_TRANSFORM_VALUE_MISMATCH")

    chain = event.get("register_chain", [])
    producers = event.get("producer_occurrences", [])
    prior_sequences = []
    by_sequence = {int(item[1]): (index, item) for index, item in enumerate(rows)
                   if item[5] & 1}
    for producer in producers:
        pi = int(producer.get("record_index", -1))
        if not 0 <= pi < len(rows) or not _record_occurrence(producer, rows[pi], pi, segment):
            raise ValueError("STOP_PRODUCER_OCCURRENCE_IDENTITY_MISMATCH")
        _audit_instruction_occurrence(rom, rows[pi], producer)
        if producer["instruction_sequence"] >= consumer["instruction_sequence"]:
            raise ValueError("STOP_PREDECESSOR_GAP")
        prior_sequences.append(int(producer["instruction_sequence"]))
    if prior_sequences != sorted(prior_sequences) or len(set(prior_sequences)) != len(prior_sequences):
        raise ValueError("STOP_PREDECESSOR_GAP")
    for step in chain:
        occurrence = step.get("instruction_occurrence", {})
        pi = int(occurrence.get("record_index", -1))
        if not 0 <= pi < len(rows) or not _record_occurrence(occurrence, rows[pi], pi, segment):
            raise ValueError("STOP_PRODUCER_OCCURRENCE_IDENTITY_MISMATCH")
        _audit_instruction_occurrence(rom, rows[pi], occurrence)
        op = rows[pi][4]
        if step.get("operation") == "MEMORY_READ":
            address = _effective_read_address(rom, rows[pi], _replay_registers(rom, rows, pi))
            if address != start or rows[pi][2] not in [item["pc"] for item in producers]:
                raise ValueError("STOP_SOURCE_EFFECTIVE_ADDRESS_UNRESOLVED")
            if step.get("source_width") != width or step.get("source_value") != int.from_bytes(raw, "big"):
                raise ValueError("STOP_SOURCE_ROM_BYTES_OR_VALUE_MISMATCH")
        elif step.get("operation") == "COPY":
            if op >> 12 != 2 or ((op >> 3) & 7) not in (0, 1) or \
                    ((op >> 6) & 7) not in (0, 1):
                raise ValueError("STOP_REGISTER_COPY_UNSUPPORTED")
            source_mode, source_index = (op >> 3) & 7, op & 7
            source_reg = f"{'A' if source_mode == 1 else 'D'}{source_index}"
            dest_mode, dest_index = (op >> 6) & 7, (op >> 9) & 7
            dest_reg = f"{'A' if dest_mode == 1 else 'D'}{dest_index}"
            before = _replay_registers(rom, rows, pi)
            if step.get("register") != dest_reg or step.get("source_register") != source_reg or \
                    step.get("source_register_value") != before.get(source_reg) or \
                    step.get("register_value") != before.get(source_reg):
                raise ValueError("STOP_REGISTER_VALUE_CHAIN_MISMATCH")
        elif step.get("operation") == "SIGN_EXTEND":
            if op & 0xFFF8 != 0x48C0:
                raise ValueError("STOP_TRANSFORM_UNSUPPORTED")
            if int(step["input_value"]) != int.from_bytes(raw, "big") or \
                    _signed(int(step["input_value"]), 2) & 0xFFFFFFFF != int(step["output_value"]):
                raise ValueError("STOP_TRANSFORM_VALUE_MISMATCH")
        elif step.get("operation") == "ADD_REGISTER_BASE":
            if op >> 12 != 13 or ((op >> 6) & 7) != 7:
                raise ValueError("STOP_TRANSFORM_UNSUPPORTED")
            if (int(step["base_value"]) + int(step["offset_value"])) & 0xFFFFFFFF != \
                    int(step["output_value"]):
                raise ValueError("STOP_TRANSFORM_VALUE_MISMATCH")
            base_register = str(step.get("base_register"))
            if _replay_registers(rom, rows, pi).get(base_register) != int(step["base_value"]):
                raise ValueError("STOP_REGISTER_VALUE_CHAIN_MISMATCH")
        else:
            raise ValueError("STOP_TRANSFORM_UNSUPPORTED")
    relation = event.get("relation")
    transforms = event.get("register_transforms", [])
    if relation == "OBSERVED_CODE_POINTER_TO":
        if width != 4 or int.from_bytes(raw, "big") != target or transforms or \
                any(step.get("operation") not in ("MEMORY_READ", "COPY") for step in chain):
            raise ValueError("STOP_POINTER_OFFSET_CLASSIFICATION_AMBIGUOUS")
    elif relation == "OBSERVED_CODE_OFFSET_TO":
        allowed_widths = (1, 2) if source.get("kind") == "ROM_INSTRUCTION_OFFSET" else (2,)
        if width not in allowed_widths or not transforms or \
                int(event["actual_next_pc"]) != target:
            raise ValueError("STOP_POINTER_OFFSET_CLASSIFICATION_AMBIGUOUS")
    else:
        raise ValueError("STOP_PROVENANCE_RELATION_UNSUPPORTED")
    entry = event.get("jump_table_entry")
    if entry:
        if source.get("kind") != "ROM_MEMORY_READ" or \
                int(entry.get("start", -1)) != start or int(entry.get("end", -1)) != end or \
                int(entry.get("entry_width", -1)) != width or \
                entry.get("bytes_hex") != raw.hex().upper() or \
                int(entry.get("value", -1)) != int.from_bytes(raw, "big"):
            raise ValueError("STOP_JUMP_TABLE_ENTRY_RANGE_UNPROVEN")
        read_step = next((step for step in chain if step.get("operation") == "MEMORY_READ"), None)
        if read_step is None:
            raise ValueError("STOP_JUMP_TABLE_ENTRY_RANGE_UNPROVEN")
        producer_index = int(read_step["instruction_occurrence"]["record_index"])
        producer_opcode = rows[producer_index][4]
        mode, register = (producer_opcode >> 3) & 7, producer_opcode & 7
        if mode != 6 and not (mode == 7 and register == 3):
            raise ValueError("STOP_JUMP_TABLE_ENTRY_RANGE_UNPROVEN")
        ext = int.from_bytes(rom[rows[producer_index][2] + 2:
                                 rows[producer_index][2] + 4], "big")
        index_name = ("A" if ext & 0x8000 else "D") + str((ext >> 12) & 7)
        index_value = _replay_registers(rom, rows, producer_index).get(index_name)
        if index_value is None:
            raise ValueError("STOP_JUMP_TABLE_ENTRY_RANGE_UNPROVEN")
        signed_index = index_value if ext & 0x0800 else _signed(index_value & 0xFFFF, 2)
        entry_base = (rows[producer_index][2] + 2 if mode == 7 else
                      _replay_registers(rom, rows, producer_index).get(f"A{register}"))
        entry_offset = signed_index + _signed(ext & 0xFF, 1)
        if index_value is None or entry.get("index_register") != index_name or \
                int(entry.get("index_value", -1)) != index_value or \
                int(entry.get("index", -1)) != entry_offset // width or \
                int(entry.get("entry_base", -1)) != entry_base or \
                int(entry.get("entry_offset", -1)) != entry_offset or \
                entry_base + entry_offset != start or entry_offset < 0 or \
                entry_offset % width:
            raise ValueError("STOP_JUMP_TABLE_ENTRY_RANGE_UNPROVEN")


def audit_segment(rom: bytes, segment: dict[str, Any], rows: list[tuple[int, ...]],
                  analysis: dict[str, Any], expected_rom_sha256: str | None = None
                  ) -> dict[str, int]:
    """Reconcile every actual consumer and every emitted fact independently."""
    if expected_rom_sha256 is not None:
        import hashlib
        if hashlib.sha256(rom).hexdigest() != expected_rom_sha256:
            raise ValueError("STOP_CONTROL_PROVENANCE_ROM_IDENTITY_MISMATCH")
    instructions = [(i, row) for i, row in enumerate(rows) if row[5] & 1]
    if any(right[1] != left[1] + 1 for (_, left), (_, right) in
           zip(instructions, instructions[1:])):
        raise ValueError("STOP_PREDECESSOR_GAP")
    expected = []
    for index, row in instructions:
        pc, opcode = row[2], row[4]
        if int.from_bytes(rom[pc:pc + 2], "big") != opcode:
            raise ValueError("STOP_CONSUMER_OPCODE_BYTES_MISMATCH")
        form = opcode & 0xFFC0
        mode, reg = (opcode >> 3) & 7, opcode & 7
        if form in (0x4E80, 0x4EC0) and (mode in (2, 5, 6) or
                (mode == 7 and reg in (2, 3))):
            expected.append((index, row))
    events = analysis.get("consumers", [])
    if len(events) != len(expected):
        raise ValueError("STOP_CONSUMER_OCCURRENCE_IDENTITY_MISMATCH")
    facts = pointer_count = offset_count = table_count = 0
    for event, (index, row) in zip(events, expected):
        if not _record_occurrence(event.get("consumer_occurrence", {}), row, index, segment):
            raise ValueError("STOP_CONSUMER_OCCURRENCE_IDENTITY_MISMATCH")
        consumer_occurrence = event.get("consumer_occurrence", {})
        consumer_end = _instruction_end(rom, row[2], row[4])
        if event.get("consumer_instruction_range") != {"start": row[2],
                "end": consumer_end} or event.get("consumer_bytes_hex") != \
                rom[row[2]:consumer_end].hex().upper():
            raise ValueError("STOP_CONSUMER_OPCODE_BYTES_MISMATCH")
        if consumer_occurrence.get("instruction_bytes_hex"):
            _audit_instruction_occurrence(rom, row, consumer_occurrence)
        if event.get("relation"):
            _audit_relation(rom, rows, segment, event)
            facts += 1
            pointer_count += event["relation"] == "OBSERVED_CODE_POINTER_TO"
            offset_count += event["relation"] == "OBSERVED_CODE_OFFSET_TO"
            table_count += event.get("jump_table_relation") == "OBSERVED_JUMP_TABLE_ENTRY_TO"
    return {"runtime_indirect_consumers": len(events), "audited_facts": facts,
            "pointer_relations": pointer_count, "offset_relations": offset_count,
            "jump_table_entries": table_count, "false_provenance_detections": 0,
            "flow_mismatches": 0, "identity_conflicts": 0}
