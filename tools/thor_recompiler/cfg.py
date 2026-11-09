#!/usr/bin/env python3
"""
Control Flow Graph builder for Thor Recompiler Prototype.
Partitions decoded M68000 instructions into basic blocks and edges.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Set, Tuple
from tools.thor_recompiler.decoder import DecodedInstruction, decode_at

@dataclass
class BasicBlock:
    block_id: int
    start_pc: int
    end_pc: int
    instructions: List[DecodedInstruction] = field(default_factory=list)
    successors: List[int] = field(default_factory=list) # List of target PCs
    is_terminal: bool = False

@dataclass
class FunctionCFG:
    entry_pc: int
    blocks: Dict[int, BasicBlock] = field(default_factory=dict) # start_pc -> BasicBlock
    exit_pcs: List[int] = field(default_factory=list)

def build_cfg(rom: bytes, entry_pc: int, end_limit_pc: int) -> FunctionCFG:
    # 1. Decode all sequential instructions from entry_pc up to end_limit_pc
    insns_by_pc: Dict[int, DecodedInstruction] = {}
    pc = entry_pc
    while pc <= end_limit_pc:
        insn = decode_at(rom, pc)
        insns_by_pc[pc] = insn
        insn_len = len(insn.raw_words) * 2
        pc += insn_len

    # 2. Identify block leaders
    leaders: Set[int] = {entry_pc}
    for pc, insn in insns_by_pc.items():
        insn_len = len(insn.raw_words) * 2
        next_pc = pc + insn_len
        
        if insn.branch_target is not None:
            leaders.add(insn.branch_target)
            if next_pc <= end_limit_pc and insn.mnemonic != "BRA":
                leaders.add(next_pc)
        elif insn.mnemonic in ("BRA", "RTS"):
            if next_pc <= end_limit_pc:
                leaders.add(next_pc)

    sorted_leaders = sorted(list(leaders))
    cfg = FunctionCFG(entry_pc=entry_pc)

    # 3. Construct basic blocks
    for idx, leader in enumerate(sorted_leaders):
        next_leader = sorted_leaders[idx + 1] if idx + 1 < len(sorted_leaders) else end_limit_pc + 2
        block_insns: List[DecodedInstruction] = []
        
        curr_pc = leader
        while curr_pc < next_leader and curr_pc in insns_by_pc:
            insn = insns_by_pc[curr_pc]
            block_insns.append(insn)
            curr_pc += len(insn.raw_words) * 2
            if insn.branch_target is not None or insn.mnemonic in ("BRA", "RTS"):
                break

        if not block_insns:
            continue

        last_insn = block_insns[-1]
        last_len = len(last_insn.raw_words) * 2
        end_pc = last_insn.pc + last_len
        succs: List[int] = []
        is_term = False

        if last_insn.mnemonic == "RTS":
            is_term = True
            cfg.exit_pcs.append(last_insn.pc)
        elif last_insn.mnemonic == "BRA":
            if last_insn.branch_target is not None:
                succs.append(last_insn.branch_target)
        elif last_insn.mnemonic == "DBRA":
            # DBRA falls through on counter underflow, or loops to target
            if last_insn.branch_target is not None:
                succs.append(last_insn.branch_target)
            if end_pc <= end_limit_pc:
                succs.append(end_pc)
        elif last_insn.branch_target is not None: # Conditional branch
            succs.append(last_insn.branch_target)
            if end_pc <= end_limit_pc:
                succs.append(end_pc)
        else: # Normal fallthrough
            if end_pc <= end_limit_pc:
                succs.append(end_pc)

        block = BasicBlock(
            block_id=idx,
            start_pc=leader,
            end_pc=end_pc,
            instructions=block_insns,
            successors=succs,
            is_terminal=is_term
        )
        cfg.blocks[leader] = block

    return cfg
