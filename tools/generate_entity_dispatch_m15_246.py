#!/usr/bin/env python3
"""
M15.246 Connected High-Frequency Callee IR + Auto-Split C++20 Generator.
Generates portable Thor IR, manifest, CFG source map, and partitioned C++20 headers
for 7 selected unowned ROM entry roots with 0 production/M15.244/M15.245 overlap.
Strictly <= 200 lines per AGENTS.md Rule 3.
"""

import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from tools.audit_m15_245_candidates import load_m15_244_pcs, load_production_pcs
from tools.audit_m15_246_candidates import EXPLICIT_EXTRA_STOPS, M15_246_ENTRY_ROOTS, M15_246_WINDOWS, load_m15_245_pcs
from tools.thor_recompiler.cfg_reconstructor import format_insn, reconstruct_cfg
from tools.thor_recompiler.cpp_emitter_m15_246 import emit_m15_246_cpp_files
from tools.thor_recompiler.decoder_ext_m15_246 import decode_m15_246_at
from tools.thor_recompiler.insn_lowering_m15_245 import exact_m68k_cycles

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"


def operand_to_ir_dict(op) -> dict:
    return {"raw_kind": op.kind, "reg_index": op.reg, "val": op.val, "size": op.size, "ext": op.ext}


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    rom_bytes = (root / "local-roms" / "Beyond Oasis (USA).md").read_bytes()
    rom_sha = hashlib.sha256(rom_bytes).hexdigest()
    assert rom_sha == ROM_SHA256, f"ROM mismatch: {rom_sha}"

    prod_owned = load_production_pcs(root)
    m244_pcs = load_m15_244_pcs(root)
    m245_pcs = load_m15_245_pcs(root)
    stop_pcs = set(prod_owned.keys()) | m244_pcs | m245_pcs | EXPLICIT_EXTRA_STOPS

    cfg = reconstruct_cfg(
        rom_bytes,
        M15_246_ENTRY_ROOTS,
        jump_table_base=0x0059BA,
        jump_table_entries=59,
        compiled_targets=set(M15_246_ENTRY_ROOTS.keys()),
        compiled_windows=M15_246_WINDOWS,
        stop_pcs=stop_pcs,
        decode_fn=decode_m15_246_at,
    )
    compiled_pcs = set(cfg["decoded_map"].keys())
    assert not (compiled_pcs & set(prod_owned.keys())), "Production helper overlap!"
    assert not (compiled_pcs & m244_pcs), "M15.244 overlap!"
    assert not (compiled_pcs & m245_pcs), "M15.245 overlap!"

    ir_blocks = []
    op_counter = 0
    for blk in cfg["basic_blocks"]:
        ir_insns = []
        for insn in blk["insns"]:
            op_id = f"IR246_{op_counter:04d}_0x{insn.pc:06X}"
            op_counter += 1
            rec = {
                "ir_op_id": op_id,
                "pc": f"0x{insn.pc:06X}",
                "raw_words": list(insn.raw_words),
                "mnemonic": insn.mnemonic,
                "size": insn.size,
                "operands": [operand_to_ir_dict(o) for o in insn.ops],
                "branch_target": f"0x{insn.branch_target:06X}" if insn.branch_target is not None else None,
                "cycles": insn.base_cycles,
                "disasm": format_insn(insn),
            }
            rec["cycles"] = exact_m68k_cycles(rec)
            ir_insns.append(rec)
        ir_blocks.append({
            "block_id": blk["block_id"],
            "start_pc": f"0x{blk['start_pc']:06X}",
            "end_pc": f"0x{blk['end_pc']:06X}",
            "instruction_count": len(ir_insns),
            "instructions": ir_insns,
        })

    assert op_counter == len(compiled_pcs)
    ir_doc = {
        "schema": "thor.ir.m15_246.callees.v1",
        "milestone": "M15.246",
        "name": "m15_246_high_freq_callees",
        "rom_sha256": rom_sha,
        "entry_roots": cfg["entry_roots"],
        "instruction_count": op_counter,
        "block_count": len(ir_blocks),
        "edge_summary": cfg["edge_summary"],
        "blocks": ir_blocks,
        "edges": cfg["edges"],
    }

    ir_path = root / "src" / "tools" / "thor_ir" / "m15_246_callee_ir.json"
    ir_path.write_text(json.dumps(ir_doc, indent=2), encoding="utf-8")
    manifest_path = root / "data" / "thor-fixtures" / "m15_246_ir_manifest.json"
    manifest_path.write_text(json.dumps(ir_doc, indent=2), encoding="utf-8")

    gen_dir = root / "src" / "tools" / "thor_ir" / "generated_m15_246"
    source_map, parts = emit_m15_246_cpp_files(ir_doc, gen_dir)

    map_doc = {
        "schema": "oasis.m15_246.callee-cfg-source-map.v1",
        "milestone": "M15.246",
        "instruction_count": op_counter,
        "block_count": len(ir_blocks),
        "edge_summary": cfg["edge_summary"],
        "omitted_instruction_count": 0,
        "production_overlap_count": 0,
        "m15_244_overlap_count": 0,
        "m15_245_overlap_count": 0,
        "generated_files": parts,
        "source_map": source_map,
    }
    (root / "data" / "thor-evidence" / "callee_cfg_source_map_m15_246.json").write_text(
        json.dumps(map_doc, indent=2), encoding="utf-8"
    )
    print(f"Generated M15.246: {op_counter} insns, {len(ir_blocks)} blocks, {len(parts)} files, max_lines check next.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
