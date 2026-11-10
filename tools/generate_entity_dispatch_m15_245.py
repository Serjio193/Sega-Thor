#!/usr/bin/env python3
"""
M15.245 Expanded Entity Handler & Connected Helper IR + Auto-Split C++20 Generator.
Compiles 16 disjoint unowned ROM roots (508 M68000 instructions, 230 CFG blocks, 397 edges)
with zero overlap against the 196 production helpers and zero overlap against M15.244 (130 ops).
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from tools.audit_m15_245_candidates import load_m15_244_pcs, load_production_pcs
from tools.thor_recompiler.cfg_reconstructor import format_insn, operand_to_ir_dict, reconstruct_cfg
from tools.thor_recompiler.cpp_emitter_m15_245 import emit_m15_245_cpp_files
from tools.thor_recompiler.insn_lowering_m15_245 import exact_m68k_cycles

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"

ENTRY_ROOTS = {
    0x005E78: "tail_5e78_damage_stagger",
    0x0061F6: "state_0_walk_move",
    0x006338: "state_24_action_338",
    0x006464: "state_53_action_464",
    0x0066AA: "state_28_action_6aa",
    0x006738: "state_58_action_738",
    0x006E28: "shared_rts_6e28",
    0x006E4A: "tail_6e4a_air_recover",
    0x006E7A: "callee_6e7a_input_rts",
    0x006EDA: "state_19_action_eda",
    0x007112: "state_11_jump_air",
    0x0071DA: "state_7_action_1da",
    0x007656: "state_5_attack_scan",
    0x007852: "state_4_combat_scan",
    0x0078F8: "state_27_action_8f8",
    0x007DC8: "helper_7dc8_input_combat_gate",
}

COMPILED_WINDOWS = [
    (0x005E78, 0x005EE8),
    (0x0061F6, 0x0062CC),
    (0x006338, 0x00638C),
    (0x006464, 0x0064C4),
    (0x0066AA, 0x006738),
    (0x006738, 0x0067DE),
    (0x006E28, 0x006E2A),
    (0x006E4A, 0x006E86),
    (0x006EDA, 0x006F3C),
    (0x007112, 0x007198),
    (0x0071DA, 0x007262),
    (0x007656, 0x00780E),
    (0x007852, 0x0078F8),
    (0x0078F8, 0x00792A),
    (0x007DC8, 0x007EC4),
]


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    rom_bytes = (root / "local-roms" / "Beyond Oasis (USA).md").read_bytes()
    rom_sha = hashlib.sha256(rom_bytes).hexdigest()
    assert rom_sha == ROM_SHA256, f"ROM mismatch: {rom_sha}"

    prod_owned = load_production_pcs(root)
    m244_pcs = load_m15_244_pcs(root)
    stop_pcs = set(prod_owned.keys()) | m244_pcs | {0x005EE8, 0x006E86, 0x007F82, 0x007FB6, 0x00822C, 0x00BCBA}

    cfg = reconstruct_cfg(
        rom_bytes,
        ENTRY_ROOTS,
        jump_table_base=0x0059BA,
        jump_table_entries=59,
        compiled_targets=set(ENTRY_ROOTS.keys()),
        compiled_windows=COMPILED_WINDOWS,
        stop_pcs=stop_pcs,
    )

    compiled_pcs = set(cfg["decoded_map"].keys())
    assert not (compiled_pcs & set(prod_owned.keys())), "Production helper overlap!"
    assert not (compiled_pcs & m244_pcs), "M15.244 overlap!"

    ir_blocks = []
    op_counter = 0
    for blk in cfg["basic_blocks"]:
        ir_insns = []
        for insn in blk["insns"]:
            op_id = f"IR245_{op_counter:04d}_0x{insn.pc:06X}"
            op_counter += 1
            raw_b = "".join(f"{w:04X}" for w in insn.raw_words)
            entry = {
                "ir_op_id": op_id,
                "pc": f"0x{insn.pc:06X}",
                "raw_words": insn.raw_words,
                "rom_bytes": raw_b,
                "decoded_instruction": format_insn(insn),
                "mnemonic": insn.mnemonic,
                "size": insn.size,
                "branch_target": f"0x{insn.branch_target:06X}" if insn.branch_target is not None else None,
                "cycles": insn.base_cycles,
                "operands": [operand_to_ir_dict(o) for o in insn.ops],
            }
            entry["m68k_cycles"] = exact_m68k_cycles(entry)
            ir_insns.append(entry)
        ir_blocks.append({
            "block_id": blk["block_id"],
            "start_pc": f"0x{blk['start_pc']:06X}",
            "end_pc": f"0x{blk['end_pc']:06X}",
            "instruction_count": len(ir_insns),
            "instructions": ir_insns,
        })

    ir_doc = {
        "schema": "oasis.thor-ir.expanded-entity-handlers.v1",
        "milestone": "M15.245",
        "name": "expanded_entity_handlers_and_helper_7dc8",
        "rom_sha256": rom_sha,
        "entry_roots": cfg["entry_roots"],
        "instruction_count": cfg["reachable_instructions"],
        "block_count": len(ir_blocks),
        "edge_summary": cfg["edge_summary"],
        "blocks": ir_blocks,
        "edges": cfg["edges"],
    }
    ir_text = json.dumps(ir_doc, indent=2)
    ir_path = root / "src" / "tools" / "thor_ir" / "entity_dispatch_expanded_ir.json"
    ir_path.write_text(ir_text, encoding="utf-8")
    (root / "data" / "thor-fixtures" / "m15_245_ir_manifest.json").write_text(ir_text, encoding="utf-8")

    gen_dir = root / "src" / "tools" / "thor_ir" / "generated_m15_245"
    src_map, parts = emit_m15_245_cpp_files(ir_doc, gen_dir)

    ir_sha = hashlib.sha256(ir_path.read_bytes()).hexdigest()
    umb_sha = hashlib.sha256((gen_dir / "exp_dispatch_portable_routines.hpp").read_bytes()).hexdigest()
    file_hashes = {}
    print(f"Generated M15.245 IR ({op_counter} ops across {len(ir_blocks)} CFG blocks)")
    print(f"Edges summary: {cfg['edge_summary']}")
    for fname in parts:
        p = gen_dir / fname
        ln = len(p.read_text(encoding="utf-8").splitlines())
        h = hashlib.sha256(p.read_bytes()).hexdigest()
        file_hashes[fname] = {"lines": ln, "sha256": h}
        assert ln <= 350, f"Generated file {fname} exceeded 350 lines ({ln})"
        print(f"  {fname}: {ln} lines (sha256={h[:12]}...)")

    mappings = []
    for blk in ir_doc["blocks"]:
        for insn in blk["instructions"]:
            sm = src_map[insn["pc"]]
            mappings.append({
                "ROM_PC": insn["pc"],
                "ROM_BYTES": insn["rom_bytes"],
                "DECODED_INSTRUCTION": insn["decoded_instruction"],
                "IR_OPERATION_IDS": [sm["ir_op_id"]],
                "GENERATED_CPP_FILE": sm["cpp_file"],
                "GENERATED_CPP_LINES": sm["cpp_lines"],
            })

    cov_doc = {
        "schema": "oasis.cfg-source-coverage-audit.m15_245.v1",
        "milestone": "M15.245",
        "rom_sha256": rom_sha,
        "NEW_UNIQUE_ROM_INSTRUCTIONS": op_counter,
        "TOTAL_COMPILED_ROM_INSTRUCTIONS_M15_244_PLUS_245": 130 + op_counter,
        "NEW_CFG_BLOCKS": len(ir_blocks),
        "TOTAL_CFG_BLOCKS_M15_244_PLUS_245": 43 + len(ir_blocks),
        "NEW_CFG_EDGES": cfg["edge_summary"],
        "OVERLAP_WITH_PRODUCTION": 0,
        "OVERLAP_WITH_M15_244": 0,
        "generated_ir_sha256": ir_sha,
        "generated_cpp_umbrella_sha256": umb_sha,
        "generated_files": file_hashes,
        "mappings": mappings,
    }
    cov_path = root / "data" / "thor-evidence" / "entity_dispatch_cfg_source_map_m15_245.json"
    cov_path.write_text(json.dumps(cov_doc, indent=2), encoding="utf-8")
    print(f"Wrote CFG source map to {cov_path} ({len(mappings)} mappings, 0 omitted)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
