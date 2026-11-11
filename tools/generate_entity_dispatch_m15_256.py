#!/usr/bin/env python3
"""
M15.256 High-ROI Clean Leaf Routine IR + Auto-Split C++20 Generator.
Generates portable Thor IR, manifest, CFG source map, partitioned C++20 headers,
and hybrid adapter for 5 unowned, clean, high-ROI leaf subroutines.
Strictly <= 350 lines per AGENTS.md Rule 3.
"""

import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from tools.audit_m15_245_candidates import load_m15_244_pcs, load_production_pcs
from tools.audit_m15_246_candidates import load_m15_245_pcs
from tools.audit_m15_247_candidates import load_m15_246_pcs
from tools.generate_entity_dispatch_m15_248 import load_m15_247_pcs
from tools.generate_entity_dispatch_m15_249 import load_m15_248_pcs
from tools.generate_entity_dispatch_m15_250 import load_m15_249_pcs
from tools.thor_recompiler.cfg_reconstructor import format_insn, reconstruct_cfg
from tools.thor_recompiler.cpp_emitter_m15_256 import emit_m15_256_cpp_files
from tools.thor_recompiler.decoder_ext_m15_250 import decode_m15_250_at
from tools.thor_recompiler.insn_lowering_m15_256 import exact_m68k_cycles_256 as exact_m68k_cycles

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"

M15_256_ENTRY_ROOTS = {
    0x010F48: "hotpath_10f48_entity_bounds_calc",
    0x0112CC: "hotpath_112cc_entity_slot_lookup",
    0x0302F4: "hotpath_302f4_entity_flag_sync",
    0x006B12: "hotpath_06b12_velocity_damp_math",
    0x0141CA: "hotpath_141ca_enemy_coord_clamp",
}

M15_256_WINDOWS = [
    (0x010F48, 0x010F84),
    (0x0112CC, 0x0112EE),
    (0x0302F4, 0x030324),
    (0x006B12, 0x006B2A),
    (0x0141CA, 0x014200),
]


def operand_to_ir_dict(op) -> dict:
    return {"raw_kind": op.kind, "reg_index": op.reg, "val": op.val, "size": op.size, "ext": op.ext}


def load_m15_250_pcs(root: Path) -> set[int]:
    p = root / "src" / "tools" / "thor_ir" / "m15_250_callee_ir.json"
    d = json.loads(p.read_text(encoding="utf-8"))
    return {int(ins["pc"], 16) for b in d["blocks"] for ins in b["instructions"]}


def main() -> int:
    root = Path(__file__).resolve().parent.parent
    rom_bytes = (root / "local-roms/Beyond Oasis (USA).md").read_bytes()
    rom_sha = hashlib.sha256(rom_bytes).hexdigest()
    assert rom_sha == ROM_SHA256, f"ROM mismatch: {rom_sha}"

    prod_owned = load_production_pcs(root)
    m244_pcs = load_m15_244_pcs(root)
    m245_pcs = load_m15_245_pcs(root)
    m246_pcs = load_m15_246_pcs(root)
    m247_pcs = load_m15_247_pcs(root)
    m248_pcs = load_m15_248_pcs(root)
    m249_pcs = load_m15_249_pcs(root)
    m250_pcs = load_m15_250_pcs(root)
    stop_pcs = set(prod_owned.keys()) | m244_pcs | m245_pcs | m246_pcs | m247_pcs | m248_pcs | m249_pcs | m250_pcs

    cfg = reconstruct_cfg(
        rom_bytes,
        M15_256_ENTRY_ROOTS,
        jump_table_base=0x0059BA,
        jump_table_entries=59,
        compiled_targets=set(M15_256_ENTRY_ROOTS.keys()),
        compiled_windows=M15_256_WINDOWS,
        stop_pcs=stop_pcs,
        decode_fn=decode_m15_250_at,
    )
    compiled_pcs = set(cfg["decoded_map"].keys())
    assert not (compiled_pcs & set(prod_owned.keys())), "Production helper overlap!"
    assert not (compiled_pcs & m244_pcs), "M15.244 overlap!"
    assert not (compiled_pcs & m245_pcs), "M15.245 overlap!"
    assert not (compiled_pcs & m246_pcs), "M15.246 overlap!"
    assert not (compiled_pcs & m247_pcs), "M15.247 overlap!"
    assert not (compiled_pcs & m248_pcs), "M15.248 overlap!"
    assert not (compiled_pcs & m249_pcs), "M15.249 overlap!"
    assert not (compiled_pcs & m250_pcs), "M15.250 overlap!"

    ir_blocks = []
    op_counter = 0
    for blk in cfg["basic_blocks"]:
        ir_insns = []
        for insn in blk["insns"]:
            op_dict = {
                "ir_op_id": op_counter,
                "pc": f"0x{insn.pc:06X}",
                "mnemonic": insn.mnemonic,
                "size": insn.size,
                "raw_words": insn.raw_words,
                "operands": [operand_to_ir_dict(o) for o in insn.ops],
                "base_cycles": insn.base_cycles,
                "cycles": insn.base_cycles,
                "branch_target": f"0x{insn.branch_target:06X}" if insn.branch_target is not None else None,
                "disasm": format_insn(insn),
            }
            op_dict["exact_cycles"] = exact_m68k_cycles(op_dict)
            ir_insns.append(op_dict)
            op_counter += 1
        ir_blocks.append({
            "block_id": blk["block_id"],
            "start_pc": blk["start_pc"],
            "end_pc": blk["end_pc"],
            "instruction_count": len(ir_insns),
            "instructions": ir_insns,
        })

    ir_doc = {
        "schema": "oasis.m15_256.callee_ir.v1",
        "milestone": "M15.256",
        "name": "connected_hotpaths_subsystem_m15_256",
        "rom_sha256": ROM_SHA256,
        "entry_roots": {f"0x{k:06X}": v for k, v in M15_256_ENTRY_ROOTS.items()},
        "compiled_windows": [[f"0x{s:06X}", f"0x{e:06X}"] for s, e in M15_256_WINDOWS],
        "total_basic_blocks": len(ir_blocks),
        "total_instructions": op_counter,
        "blocks": ir_blocks,
        "cfg_edges": cfg["edges"],
    }

    out_ir = root / "src" / "tools" / "thor_ir" / "m15_256_callee_ir.json"
    out_ir.write_text(json.dumps(ir_doc, indent=2), encoding="utf-8")
    print(f"Emitted IR -> {out_ir} ({op_counter} insns, {len(ir_blocks)} blocks)")

    gen_dir = root / "src" / "tools" / "thor_ir" / "generated_m15_256"
    source_map, parts = emit_m15_256_cpp_files(ir_doc, gen_dir)
    smap_path = root / "src" / "tools" / "thor_ir" / "m15_256_source_map.json"
    smap_path.write_text(json.dumps(source_map, indent=2), encoding="utf-8")
    print(f"Emitted {len(parts)} C++ headers in {gen_dir} and source map {smap_path}")

    # Emit manifest for fixtures
    manifest_doc = {
        "schema": "oasis.m15_256.manifest.v1",
        "milestone": "M15.256",
        "name": "m15_256_hotpath_manifest",
        "rom_sha256": ROM_SHA256,
        "entry_roots": {f"0x{k:06X}": v for k, v in M15_256_ENTRY_ROOTS.items()},
        "compiled_windows": [[f"0x{s:06X}", f"0x{e:06X}"] for s, e in M15_256_WINDOWS],
        "total_basic_blocks": len(ir_blocks),
        "total_instructions": op_counter,
        "blocks": [
            {
                "block_id": b["block_id"],
                "start_pc": f"0x{b['start_pc']:06X}",
                "end_pc": f"0x{b['end_pc']:06X}",
                "instruction_count": b["instruction_count"],
            }
            for b in ir_blocks
        ],
    }
    man_path = root / "data" / "thor-fixtures" / "m15_256_ir_manifest.json"
    man_path.write_text(json.dumps(manifest_doc, indent=2), encoding="utf-8")
    print(f"Emitted Manifest -> {man_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
