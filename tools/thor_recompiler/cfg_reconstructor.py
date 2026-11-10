#!/usr/bin/env python3
"""
M68000 Static CFG & Call-Graph Reconstructor for Thor Recompiler (M15.244).
Takes ROM bytes and compiled entry points, traverses reachable instructions without skipping
any reachable path, splits into true CFG basic blocks, and classifies all edges.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import collections
from dataclasses import dataclass, field
import struct
from typing import Dict, List, Set, Tuple

from tools.thor_recompiler.decoder import DecodedInstruction, Operand, decode_at


CONDITIONAL_BRANCHES = {
    "BNE", "BEQ", "BCC", "BCS", "BPL", "BMI",
    "BGT", "BLE", "BLT", "BGE", "BHI", "BLS", "BVC", "BVS",
}


def format_operand(op: Operand) -> str:
    if op.kind == "reg_d":
        return f"D{op.reg}"
    if op.kind == "reg_a":
        return f"A{op.reg}"
    if op.kind == "imm":
        return f"#0x{op.val & 0xFFFFFFFF:X}"
    if op.kind == "disp_a":
        return f"{op.val}(A{op.reg})"
    if op.kind == "disp_ax":
        idx = f"{'A' if (op.ext & 0x8000) else 'D'}{(op.ext >> 12) & 7}.{'l' if (op.ext & 0x0800) else 'w'}"
        return f"{op.val}(A{op.reg},{idx})"
    if op.kind == "indir_a":
        return f"(A{op.reg})"
    if op.kind == "postinc_a":
        return f"(A{op.reg})+"
    if op.kind == "predec_a":
        return f"-(A{op.reg})"
    if op.kind == "abs_l":
        return f"0x{op.val:06X}.L"
    if op.kind == "branch_disp":
        return f"0x{op.val:06X}"
    if op.kind == "pc_index":
        return f"0x{op.val:06X}(PC,D{op.reg}.{op.size})"
    return f"{op.kind}({op.reg},0x{op.val:X})"


def format_insn(insn: DecodedInstruction) -> str:
    sz = f".{insn.size}" if insn.size else ""
    ops = ", ".join(format_operand(o) for o in insn.ops)
    return f"{insn.mnemonic}{sz}" + (f" {ops}" if ops else "")


def operand_to_ir_dict(op: Operand) -> dict:
    kind_map = {
        "reg_d": "Register", "reg_a": "Register",
        "imm": "Immediate", "disp_a": "MemoryDisplacement",
        "indir_a": "MemoryIndirect", "abs_l": "AbsoluteAddress",
        "branch_disp": "BranchTarget", "pc_index": "PcIndexed",
        "postinc_a": "PostIncrement", "predec_a": "PreDecrement",
    }
    rk = "Data" if op.kind == "reg_d" else ("Address" if op.kind in ("reg_a", "disp_a", "indir_a", "postinc_a", "predec_a") else "None")
    d = {
        "kind": kind_map.get(op.kind, op.kind),
        "raw_kind": op.kind,
        "reg_kind": rk,
        "reg_index": op.reg,
        "val": op.val,
        "size": op.size,
    }
    ext = getattr(op, "ext", 0)
    if ext:
        d["ext"] = ext
    return d


def reconstruct_cfg(
    rom: bytes,
    entry_roots: Dict[int, str],
    jump_table_base: int,
    jump_table_entries: int,
    compiled_targets: Set[int],
    compiled_windows: List[Tuple[int, int]] | None = None,
    stop_pcs: Set[int] | None = None,
    decode_fn=None,
) -> dict:
    """Reconstruct complete reachable CFG, split into true basic blocks, and emit 1:1 IR."""
    if decode_fn is None:
        decode_fn = decode_at
    if compiled_windows is None:
        compiled_windows = [(0x0059A8, 0x0059BA), (0x005C1A, 0x005DE4), (0x00780E, 0x007852)]
    stop_set = set(stop_pcs or ())

    def in_windows(addr: int | None) -> bool:
        if addr is None or addr in stop_set:
            return False
        return any(s <= addr < e for s, e in compiled_windows)
    # Allowed contiguous code windows so external BRA targets outside these windows are classified UncompiledExit
    decoded_map: Dict[int, DecodedInstruction] = {}
    reachable_pcs: Set[int] = set()
    leaders: Set[int] = set(entry_roots.keys())
    raw_edges: List[Tuple[int, int, str]] = []  # (from_pc, to_pc, kind)

    queue = collections.deque(sorted(entry_roots.keys()))
    while queue:
        pc = queue.popleft()
        if pc in decoded_map or pc in stop_set:
            continue
        insn = decode_fn(rom, pc)
        decoded_map[pc] = insn
        reachable_pcs.add(pc)
        next_pc = pc + len(insn.raw_words) * 2
        m = insn.mnemonic

        if m == "RTS" or m == "RTE":
            raw_edges.append((pc, 0, "RETURN"))
        elif m == "JMP":
            if insn.branch_target is not None:
                tgt = insn.branch_target
                if in_windows(tgt):
                    raw_edges.append((pc, tgt, "UNCONDITIONAL"))
                    leaders.add(tgt)
                    if tgt not in decoded_map:
                        queue.append(tgt)
                else:
                    raw_edges.append((pc, tgt, "UNCOMPILED_EXIT"))
            else:
                # Dynamic jump table at 0x0059B8
                for i in range(jump_table_entries):
                    t_addr = jump_table_base + i * 2
                    off = struct.unpack_from(">h", rom, t_addr)[0]
                    tgt = (t_addr + off) & 0xFFFFFF
                    if tgt in compiled_targets:
                        raw_edges.append((pc, tgt, "DYNAMIC_COMPILED"))
                        leaders.add(tgt)
                        if tgt not in decoded_map:
                            queue.append(tgt)
                    else:
                        raw_edges.append((pc, tgt, "DYNAMIC_UNCOMPILED_EXIT"))
        elif m == "BRA":
            tgt = insn.branch_target
            if in_windows(tgt):
                raw_edges.append((pc, tgt, "UNCONDITIONAL"))
                leaders.add(tgt)
                if tgt not in decoded_map:
                    queue.append(tgt)
            else:
                raw_edges.append((pc, tgt, "UNCOMPILED_EXIT"))
        elif m in CONDITIONAL_BRANCHES:
            tgt = insn.branch_target
            if in_windows(tgt):
                raw_edges.append((pc, tgt, "CONDITIONAL_TAKEN"))
                leaders.add(tgt)
                if tgt not in decoded_map:
                    queue.append(tgt)
            else:
                raw_edges.append((pc, tgt, "UNCOMPILED_EXIT"))
            raw_edges.append((pc, next_pc, "CONDITIONAL_FALLTHROUGH"))
            leaders.add(next_pc)
            if next_pc not in decoded_map:
                queue.append(next_pc)
        elif m == "DBRA":
            tgt = insn.branch_target
            if in_windows(tgt):
                raw_edges.append((pc, tgt, "CONDITIONAL_TAKEN"))
                leaders.add(tgt)
                if tgt not in decoded_map:
                    queue.append(tgt)
            else:
                raw_edges.append((pc, tgt, "UNCOMPILED_EXIT"))
            raw_edges.append((pc, next_pc, "CONDITIONAL_FALLTHROUGH"))
            leaders.add(next_pc)
            if next_pc not in decoded_map:
                queue.append(next_pc)
        elif m in ("BSR", "JSR"):
            tgt = insn.branch_target
            if tgt is not None and (tgt in entry_roots or in_windows(tgt)):
                raw_edges.append((pc, tgt, "CALL_COMPILED"))
                leaders.add(tgt)
                if tgt not in decoded_map:
                    queue.append(tgt)
            else:
                raw_edges.append((pc, tgt, "CALL_UNCOMPILED"))
            # Fallthrough return site
            leaders.add(next_pc)
            raw_edges.append((pc, next_pc, "CALL_RETURN_CONTINUATION"))
            if next_pc not in decoded_map:
                queue.append(next_pc)
        else:
            if next_pc not in decoded_map:
                queue.append(next_pc)
            # Only add unconditional edge if next_pc is already a leader
            if next_pc in leaders:
                raw_edges.append((pc, next_pc, "UNCONDITIONAL"))

    # Add fallthrough edges between adjacent blocks where tail instruction fell through into a leader
    sorted_pcs = sorted(decoded_map.keys())
    pc_to_block: Dict[int, int] = {}
    blocks = []
    block_idx = 0

    cur_block_start = None
    cur_insns: List[DecodedInstruction] = []

    def flush_block():
        nonlocal block_idx, cur_block_start, cur_insns
        if cur_insns:
            end_pc = cur_insns[-1].pc + len(cur_insns[-1].raw_words) * 2
            bid = f"BB_{block_idx:02d}_0x{cur_block_start:06X}"
            blocks.append({
                "block_id": bid,
                "start_pc": cur_block_start,
                "end_pc": end_pc,
                "insns": list(cur_insns),
            })
            for insn in cur_insns:
                pc_to_block[insn.pc] = block_idx
            block_idx += 1
            cur_insns = []
            cur_block_start = None

    for pc in sorted_pcs:
        insn = decoded_map[pc]
        if pc in leaders or cur_block_start is None or (cur_insns and cur_insns[-1].pc + len(cur_insns[-1].raw_words) * 2 != pc):
            flush_block()
            cur_block_start = pc
        cur_insns.append(insn)
        m = insn.mnemonic
        if m in ("RTS", "RTE", "JMP", "BRA", "BSR", "JSR", "DBRA") or m in CONDITIONAL_BRANCHES:
            flush_block()
    flush_block()

    # Connect fallthrough block edges cleanly
    edge_records = []
    seen_edges = set()
    for from_pc, to_pc, ekind in raw_edges:
        key = (from_pc, to_pc, ekind)
        if key not in seen_edges:
            seen_edges.add(key)
            to_s = f"0x{to_pc:06X}" if to_pc is not None else "INDIRECT"
            edge_records.append({"from_pc": f"0x{from_pc:06X}", "to_pc": to_s, "kind": ekind})

    for b in blocks:
        last = b["insns"][-1]
        m = last.mnemonic
        if m not in ("RTS", "RTE", "JMP", "BRA", "BSR", "JSR", "DBRA") and m not in CONDITIONAL_BRANCHES:
            nxt = last.pc + len(last.raw_words) * 2
            if nxt in decoded_map:
                key = (last.pc, nxt, "UNCONDITIONAL")
                if key not in seen_edges:
                    seen_edges.add(key)
                    edge_records.append({"from_pc": f"0x{last.pc:06X}", "to_pc": f"0x{nxt:06X}", "kind": "UNCONDITIONAL"})

    # Count distinct edge categories
    edge_counts = collections.Counter(e["kind"] for e in edge_records)

    # Check contiguously scanned gaps in any compiled function interval for unreachable code
    return {
        "entry_roots": {f"0x{k:06X}": v for k, v in sorted(entry_roots.items())},
        "reachable_instructions": len(sorted_pcs),
        "basic_blocks": blocks,
        "decoded_map": decoded_map,
        "edges": edge_records,
        "edge_summary": {
            "total_edges": len(edge_records),
            "conditional_edges": edge_counts["CONDITIONAL_TAKEN"] + edge_counts["CONDITIONAL_FALLTHROUGH"],
            "unconditional_edges": edge_counts["UNCONDITIONAL"],
            "dynamic_edges": edge_counts["DYNAMIC_COMPILED"] + edge_counts["DYNAMIC_UNCOMPILED_EXIT"],
            "dynamic_compiled_edges": edge_counts["DYNAMIC_COMPILED"],
            "dynamic_uncompiled_edges": edge_counts["DYNAMIC_UNCOMPILED_EXIT"],
            "call_edges": edge_counts["CALL_COMPILED"] + edge_counts["CALL_UNCOMPILED"],
            "return_edges": edge_counts["RETURN"],
            "uncompiled_exit_edges": edge_counts["UNCOMPILED_EXIT"] + edge_counts["DYNAMIC_UNCOMPILED_EXIT"] + edge_counts["CALL_UNCOMPILED"],
        },
    }
