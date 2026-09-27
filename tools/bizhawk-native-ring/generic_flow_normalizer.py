"""Convert the fixed native FLOW V2 record ABI into generic evidence."""

from __future__ import annotations

from typing import Any

FLAG_INSTRUCTION = 1
FLAG_CONTROL_FLOW = 8
FLAG_BRANCH_TAKEN = 16
FLAG_BRANCH_NOT_TAKEN = 32
FLAG_EXCEPTION_EVENT = 256
FLAG_EVENT = 0x8000
EVENT_SHIFT, EVENT_MASK = 11, 0x3800
BUS_READ, BUS_WRITE, FRAME_BOUNDARY = 1, 2, 3
CPU_M68K, CPU_Z80 = 0, 1
DOMAIN_NAMES = {0: "ROM", 1: "M68K_RAM", 2: "Z80_WINDOW", 3: "VDP",
                4: "YM2612", 5: "PSG", 6: "OTHER", 7: "Z80_RAM",
                8: "BANKED_ROM"}


def _control_class(flags: int, opcode: int, cpu_id: int) -> str:
    if flags & FLAG_EXCEPTION_EVENT:
        return "EXCEPTION"
    if cpu_id == CPU_M68K:
        if opcode & 0xF000 == 0x6000 and opcode & 0xFF00 == 0x6100:
            return "BSR"
        if opcode & 0xF000 == 0x6000:
            return "BRANCH_TAKEN" if flags & FLAG_BRANCH_TAKEN else (
                "BRANCH_NOT_TAKEN" if flags & FLAG_BRANCH_NOT_TAKEN else "BRANCH")
        if opcode & 0xFFC0 == 0x4E80:
            return "JSR"
        if opcode & 0xFFC0 == 0x4EC0:
            return "JMP"
        if opcode & 0xFFF0 == 0x4E70:
            return "RETURN_OR_EXCEPTION"
        if opcode & 0xFFF8 == 0x4E50:
            return "LINK"
        if opcode == 0x4E75:
            return "RTS"
        if opcode == 0x4E73:
            return "RTE"
    return "CONTROL_FLOW" if flags & FLAG_CONTROL_FLOW else "FALLTHROUGH"


def _indirect_ea(opcode: int) -> bool:
    effective_address = opcode & 0x3F
    mode, register = (effective_address >> 3) & 7, effective_address & 7
    return mode in {2, 3, 4, 5, 6} or (mode == 7 and register >= 2)


def normalize(records: list[tuple[int, ...]], segment: dict[str, Any],
              rom: bytes) -> dict[str, Any]:
    """Emit V2 collections without inferring absent register/decode evidence."""
    run_id, epoch = segment.get("run_id"), segment.get("epoch")
    entry_frame, exit_frame = segment.get("entry_frame"), segment.get("exit_frame")
    frame = entry_frame if entry_frame or exit_frame else None
    instructions: list[dict[str, Any]] = []
    memory: list[dict[str, Any]] = []
    rom_reads: list[dict[str, Any]] = []
    control_flow: list[dict[str, Any]] = []
    calls: list[dict[str, Any]] = []
    returns: list[dict[str, Any]] = []
    indirect_targets: list[dict[str, Any]] = []
    verification_counts: dict[str, int] = {}
    snapshots: list[dict[str, Any]] = []
    normalized_records: list[dict[str, Any]] = []
    snapshot_refs: dict[int, list[str]] = {}
    instruction_stream = {int(row[1]): int(row[0]) for row in records
                          if row[6] & FLAG_INSTRUCTION and row[7] == CPU_M68K}
    for boundary, field, instruction_sequence in (
            ("ENTRY", "entry_registers", segment.get("entry_instruction_sequence")),
            ("EXIT", "exit_registers", segment.get("exit_instruction_sequence"))):
        registers = segment.get(field)
        if not isinstance(registers, dict) or instruction_sequence is None:
            continue
        sequence = int(instruction_sequence)
        if boundary == "EXIT":
            sequence -= 1
        snapshot_id = f"{run_id}:{epoch}:{sequence}:{boundary}"
        snapshot_frame = segment.get("entry_frame") if boundary == "ENTRY" else exit_frame
        if not entry_frame and not exit_frame:
            snapshot_frame = None
        snapshots.append({"snapshot_id": snapshot_id, "run_id": run_id,
                          "epoch": epoch,
                          "frame": snapshot_frame,
                          "stream_sequence": instruction_stream.get(sequence),
                          "instruction_sequence": sequence,
                          "boundary": boundary, "cpu_id": "M68K",
                          "registers": {name: int(value)
                                        for name, value in registers.items()}})
        snapshot_refs.setdefault(sequence, []).append(snapshot_id)
    for row in records:
        (stream_seq, instruction_seq, _master_time, pc, address, value,
         flags, cpu_id, length, domain, _reserved, auxiliary) = row
        subtype = (flags & EVENT_MASK) >> EVENT_SHIFT if flags & FLAG_EVENT else 0
        identity = {"run_id": run_id, "epoch": epoch, "frame": frame,
                    "stream_sequence": stream_seq,
                    "instruction_sequence": instruction_seq,
                    "cpu_id": "M68K" if cpu_id == CPU_M68K else (
                        "Z80" if cpu_id == CPU_Z80 else "NONE")}
        if flags & FLAG_EVENT and subtype == FRAME_BOUNDARY:
            frame = (address << 32) | pc
            normalized_records.append({**identity, "frame": frame, "kind": "frame_boundary"})
            continue
        domain_name = DOMAIN_NAMES.get(domain, "UNKNOWN")
        if flags & FLAG_INSTRUCTION:
            opcode = value & 0xFFFF if cpu_id == CPU_M68K else value
            exact = cpu_id == CPU_M68K and 0 <= pc <= len(rom) - 2 and \
                int.from_bytes(rom[pc:pc + 2], "big") == opcode
            classification = ("NON_ROM_DOMAIN" if cpu_id == CPU_Z80 else
                              "ROM_OPCODE_EXACT" if exact else
                              "MISMATCH" if cpu_id == CPU_M68K and 0 <= pc and
                              pc + 2 <= len(rom) else "UNRESOLVED")
            verification_counts[classification] = verification_counts.get(classification, 0) + 1
            instruction = {**identity, "pc": pc, "opcode": opcode,
                           "opcode_bytes": list((opcode >> (8 * i)) & 0xFF
                                                 for i in reversed(range(2)))
                           if cpu_id == CPU_M68K else
                           [(value >> (8 * i)) & 0xFF for i in range(min(length, 4))],
                           "instruction_width": length or None,
                           "domain": "UNSPECIFIED",
                           "domain_id": domain,
                           "decoded_mnemonic": None, "decoded_form": None,
                           "next_pc": address, "control_flow_class":
                           _control_class(flags, opcode, cpu_id),
                           "opcode_verification": classification,
                           "register_snapshot_id": next(iter(snapshot_refs.get(
                               instruction_seq, [])), None),
                           "register_snapshot_ids": snapshot_refs.get(instruction_seq, [])}
            instructions.append(instruction)
            normalized_records.append({**identity, "pc": pc, "address": address,
                                       "opcode": opcode, "kind": "instruction",
                                       "opcode_verification": classification})
            flow = {**identity, "pc": pc, "next_pc": address,
                    "class": instruction["control_flow_class"], "flags": flags}
            control_flow.append(flow)
            if flow["class"] in {"JSR", "BSR"}:
                calls.append({**flow, "target": address,
                              "target_evidence": "runtime_next_pc"})
            if flow["class"] in {"JSR", "JMP"} and _indirect_ea(opcode):
                indirect_targets.append({**flow, "target": address,
                                         "target_evidence": "runtime_next_pc"})
            if flow["class"] in {"RTS", "RTE", "RETURN_OR_EXCEPTION"}:
                returns.append({**flow, "return_pc": address})
            continue
        if flags & FLAG_EVENT and subtype in (BUS_READ, BUS_WRITE):
            operation = "read" if subtype == BUS_READ else "write"
            event = {**identity, "pc": pc, "address": address, "value": value,
                     "width": length, "domain": domain_name, "operation": operation,
                     "kind": "rom" if operation == "read" and
                     domain_name in {"ROM", "BANKED_ROM"} else operation,
                     "resolved_address": auxiliary or None}
            memory.append(event)
            if operation == "read" and domain_name in {"ROM", "BANKED_ROM"}:
                rom_reads.append({**event, "rom_address": auxiliary or address,
                                  "consumer_pc": pc})
            normalized_records.append({**identity, "pc": pc, "address": address,
                                       "opcode": value, "kind": "bus"})
    m68k_count = sum(row["cpu_id"] == "M68K" for row in instructions)
    z80_count = sum(row["cpu_id"] == "Z80" for row in instructions)
    total = len(instructions)
    frame_known = sum(row["frame"] is not None for row in instructions)
    frame_boundaries = sum(row.get("kind") == "frame_boundary" for row in normalized_records)
    coverage = {"TOTAL_INSTRUCTION_EVENTS": total,
                "M68K_INSTRUCTION_EVENTS": m68k_count,
                "Z80_INSTRUCTION_EVENTS": z80_count,
                "INSTRUCTIONS_WITH_CPU_ID": total,
                "INSTRUCTIONS_WITH_OPCODE": total,
                "INSTRUCTIONS_WITH_NEXT_PC": total,
                "INSTRUCTIONS_WITH_FRAME": frame_known,
                "INSTRUCTIONS_WITH_REGISTER_REF": sum(
                    bool(row["register_snapshot_ids"]) for row in instructions),
                "MEMORY_EVENTS": len(memory),
                "MEMORY_WITH_WIDTH": sum(row["width"] > 0 for row in memory),
                "MEMORY_WITH_DOMAIN": sum(row["domain"] != "UNKNOWN" for row in memory),
                "MEMORY_WITH_INSTRUCTION_LINK": sum(
                    row["instruction_sequence"] > 0 for row in memory),
                "ROM_READ_EVENTS": len(rom_reads),
                "FRAME_BOUNDARY_EVENTS": frame_boundaries,
                "OPCODE_VERIFICATION_CLASS_COUNTS": verification_counts}
    coverage["PERCENTAGES"] = {
        "cpu_id": 100.0 if total else 0.0,
        "opcode": 100.0 if total else 0.0,
        "next_pc": 100.0 if total else 0.0,
        "frame": 100.0 * frame_known / total if total else 0.0,
        "register_ref": 100.0 * coverage["INSTRUCTIONS_WITH_REGISTER_REF"] / total
        if total else 0.0,
        "memory_width": 100.0 * coverage["MEMORY_WITH_WIDTH"] / len(memory)
        if memory else 0.0,
        "memory_domain": 100.0 * coverage["MEMORY_WITH_DOMAIN"] / len(memory)
        if memory else 0.0,
        "memory_instruction_link": 100.0 * coverage["MEMORY_WITH_INSTRUCTION_LINK"] /
        len(memory) if memory else 0.0}
    missing_register_refs = sum(not row["register_snapshot_ids"] for row in instructions)
    return {"schema": "oasis.m13.normalized-generic-corpus.v2",
            "identity_fields": ["run_id", "epoch", "frame", "stream_sequence",
                                "instruction_sequence"],
            "records": normalized_records, "instructions": instructions,
            "memory": memory, "rom_reads": rom_reads,
            "register_snapshots": snapshots, "control_flow": control_flow,
            "calls": calls, "returns": returns, "indirect_targets": indirect_targets,
            "schema_coverage": coverage,
            "capture_gaps": ([{"missing_evidence_type": "frame_identity",
                               "affected_instruction_events": total - frame_known}
                              ] if frame_known != total else []) +
                             [{"missing_evidence_type": "per_instruction_register_snapshots",
                              "affected_instruction_events": missing_register_refs},
                             {"missing_evidence_type": "post_run_instruction_decode",
                              "affected_instruction_events": len(instructions)}]}
