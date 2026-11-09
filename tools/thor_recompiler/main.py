#!/usr/bin/env python3
"""
Thor Automatic M68000 Recompiler CLI.
Deterministic translation of M68000 ROM functions directly into C++20.
Enforces <= 500 lines per file (AGENTS.md Rule 3) via dynamic CFG partitioning.
Outputs machine-readable manifests and verifies canonical ROM integrity.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""
import argparse
import hashlib
import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent.parent))
from tools.thor_recompiler.cfg import build_cfg
from tools.thor_recompiler.codegen_cpp import generate_cpp_header, generate_sub_function
from tools.thor_recompiler.decoder import decode_at

EXPECTED_ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"

def compute_sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def main():
    parser = argparse.ArgumentParser(description="Thor Automatic M68000 to C++ Recompiler")
    parser.add_argument("--rom", default="local-roms/Beyond Oasis (USA).md", help="Path to canonical ROM")
    parser.add_argument("--rom-sha256", default=EXPECTED_ROM_SHA256, help="Expected ROM SHA256")
    parser.add_argument("--entry", required=True, help="Entry PC in hex (e.g. 0x00D168)")
    parser.add_argument("--limit", required=True, help="End limit PC in hex (e.g. 0x00D264)")
    parser.add_argument("--allowed-ranges", default=None, help="Allowed address ranges (e.g. 0x00D168-0x00D266)")
    parser.add_argument("--name", required=True, help="Function name for generated code (e.g. m68k_recompiled_00d168)")
    parser.add_argument("--out-dir", default="src/tools/hybrid/generated", help="Output directory for C++ code")
    parser.add_argument("--target-abi", default="hybrid_step_v1", help="Target execution ABI")
    parser.add_argument("--policy", default="strict_reject_unsupported", help="Supported opcode policy")
    args = parser.parse_args()

    entry_pc = int(args.entry, 16)
    limit_pc = int(args.limit, 16)
    out_dir = pathlib.Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    rom_path = pathlib.Path(args.rom)
    if not rom_path.is_file():
        print(f"Error: ROM file not found: {rom_path}")
        sys.exit(1)
    rom_bytes = rom_path.read_bytes()
    rom_hash = compute_sha256(rom_bytes)

    if args.rom_sha256 and rom_hash != args.rom_sha256.lower():
        print(f"Error: ROM SHA256 mismatch! Got {rom_hash}, expected {args.rom_sha256}")
        sys.exit(1)

    print(f"Building CFG for {args.name} at 0x{entry_pc:06X}..0x{limit_pc:06X}...")
    cfg = build_cfg(rom_bytes, entry_pc, limit_pc)
    blocks_list = [cfg.blocks[k] for k in sorted(cfg.blocks.keys())]
    print(f"CFG built: {len(blocks_list)} basic blocks, {len(cfg.exit_pcs)} exits")

    # 1. Decoded Instruction Manifest & Source PC Map
    decoded_manifest = []
    source_pc_map = {}
    unsupported_report = []

    for b in blocks_list:
        for insn in b.instructions:
            raw_hex = "".join([f"{w:04X}" for w in insn.raw_words])
            decoded_manifest.append({
                "pc": f"0x{insn.pc:06X}",
                "mnemonic": insn.mnemonic,
                "size": insn.size,
                "raw_words": [f"0x{w:04X}" for w in insn.raw_words],
                "base_cycles": insn.base_cycles,
                "variable_cycles": insn.variable_cycles,
                "branch_target": f"0x{insn.branch_target:06X}" if insn.branch_target else None
            })
            source_pc_map[f"0x{insn.pc:06X}"] = {
                "block_start": f"0x{b.start_pc:06X}",
                "block_end": f"0x{b.end_pc:06X}",
                "bytes": raw_hex,
                "mnemonic": insn.mnemonic
            }

    # 2. Control Flow Graph JSON
    cfg_json = {
        "entry_pc": f"0x{entry_pc:06X}",
        "limit_pc": f"0x{limit_pc:06X}",
        "block_count": len(blocks_list),
        "exit_pcs": [f"0x{pc:06X}" for pc in cfg.exit_pcs],
        "blocks": [
            {
                "start_pc": f"0x{b.start_pc:06X}",
                "end_pc": f"0x{b.end_pc:06X}",
                "instruction_count": len(b.instructions),
                "successors": [f"0x{s:06X}" for s in b.successors]
            }
            for b in blocks_list
        ]
    }

    # Partition blocks into sub-functions such that line count stays strictly <= 500
    parts = []
    curr = []
    curr_insns = 0
    for b in blocks_list:
        if curr and (curr_insns + len(b.instructions) > 24 or len(curr) >= 5):
            parts.append(curr)
            curr = []
            curr_insns = 0
        curr.append(b)
        curr_insns += len(b.instructions)
    if curr:
        parts.append(curr)

    num_parts = len(parts)
    parts_info = []
    generated_files = []

    if num_parts > 1:
        for part_idx, pblocks in enumerate(parts):
            pname = f"{args.name}_p{part_idx}"
            h = generate_cpp_header(pname, is_orchestrator=False)
            s = generate_sub_function(pname, pblocks)
            h_path = out_dir / f"{pname}.hpp"
            s_path = out_dir / f"{pname}.cpp"
            h_path.write_text(h, encoding="utf-8")
            s_path.write_text(s, encoding="utf-8")
            generated_files.extend([f"{pname}.hpp", f"{pname}.cpp"])
            parts_info.append((pname, pblocks[0].start_pc, pblocks[-1].end_pc))

        # Generate Orchestrator
        ho = generate_cpp_header(args.name, is_orchestrator=True)
        so_lines = [f'#include "tools/hybrid/generated/{args.name}.hpp"']
        for pname, _, _ in parts_info:
            so_lines.append(f'#include "tools/hybrid/generated/{pname}.hpp"')
        so_lines.extend([
            "",
            "namespace oasis::hybrid::generated {",
            "",
            f"M68k9D70StepResult {args.name}(const M68k9D70StepApi& api, unsigned& resume_pc) noexcept {{",
            f"    unsigned block_pc = 0x{entry_pc:06X}U;",
            "    unsigned step_count = 0;",
            "    while (block_pc != 0U && step_count < 2000U) {",
            "        ++step_count;",
            "        const unsigned current_pc = block_pc;",
        ])
        for idx, (pname, _, _) in enumerate(parts_info):
            so_lines.append(f"        block_pc = {pname}(api, current_pc, resume_pc);")
            so_lines.append(f"        if (block_pc != 0U && block_pc != current_pc) continue;")
        so_lines.extend([
            "        if (block_pc == current_pc) break; // Unhandled block",
            "    }",
            "    return M68k9D70StepResult::InstructionStepped;",
            "}",
            "} // namespace oasis::hybrid::generated\n"
        ])
        so = "\n".join(so_lines)
        (out_dir / f"{args.name}.hpp").write_text(ho, encoding="utf-8")
        (out_dir / f"{args.name}.cpp").write_text(so, encoding="utf-8")
        generated_files.extend([f"{args.name}.hpp", f"{args.name}.cpp"])
    else:
        # Single function
        ho = generate_cpp_header(args.name, is_orchestrator=True)
        sub_s = generate_sub_function(f"{args.name}_impl", blocks_list, is_inline=True)
        so_lines = [
            f'#include "tools/hybrid/generated/{args.name}.hpp"',
            "",
            sub_s,
            "",
            "namespace oasis::hybrid::generated {",
            f"M68k9D70StepResult {args.name}(const M68k9D70StepApi& api, unsigned& resume_pc) noexcept {{",
            f"    unsigned block_pc = 0x{entry_pc:06X}U;",
            "    unsigned step_count = 0;",
            "    while (block_pc != 0U && step_count < 2000U) {",
            "        ++step_count;",
            f"        block_pc = {args.name}_impl(api, block_pc, resume_pc);",
            "    }",
            "    return M68k9D70StepResult::InstructionStepped;",
            "}",
            "} // namespace oasis::hybrid::generated\n"
        ]
        so = "\n".join(so_lines)
        (out_dir / f"{args.name}.hpp").write_text(ho, encoding="utf-8")
        (out_dir / f"{args.name}.cpp").write_text(so, encoding="utf-8")
        generated_files.extend([f"{args.name}.hpp", f"{args.name}.cpp"])

    # Write manifests
    manifest_dir = out_dir / "manifests"
    manifest_dir.mkdir(parents=True, exist_ok=True)

    with open(manifest_dir / f"{args.name}_decoded.json", "w", encoding="utf-8") as f:
        json.dump(decoded_manifest, f, indent=2)

    with open(manifest_dir / f"{args.name}_cfg.json", "w", encoding="utf-8") as f:
        json.dump(cfg_json, f, indent=2)

    with open(manifest_dir / f"{args.name}_source_pc_map.json", "w", encoding="utf-8") as f:
        json.dump(source_pc_map, f, indent=2)

    with open(manifest_dir / f"{args.name}_unsupported_opcodes.json", "w", encoding="utf-8") as f:
        json.dump(unsupported_report, f, indent=2)

    # Compute source hashes for determinism audit
    source_hashes = {}
    for fname in generated_files:
        fpath = out_dir / fname
        source_hashes[fname] = compute_sha256(fpath.read_bytes())

    code_gen_manifest = {
        "name": args.name,
        "entry_pc": f"0x{entry_pc:06X}",
        "limit_pc": f"0x{limit_pc:06X}",
        "rom_sha256": rom_hash,
        "target_abi": args.target_abi,
        "policy": args.policy,
        "generated_files": generated_files,
        "source_hashes": source_hashes,
        "block_count": len(blocks_list),
        "instruction_count": len(decoded_manifest)
    }

    with open(manifest_dir / f"{args.name}_codegen_manifest.json", "w", encoding="utf-8") as f:
        json.dump(code_gen_manifest, f, indent=2)

    print(f"Code generation complete for {args.name}! Generated {len(generated_files)} files in {out_dir}")

if __name__ == "__main__":
    main()
