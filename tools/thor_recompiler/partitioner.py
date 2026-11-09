#!/usr/bin/env python3
"""
Partitioned Code Generator for Thor Recompiler.
Splits M68000 routines into discrete partitioned block files (<=350 lines each).
Generates manifests with PC maps, hashes, and CFG structures per AGENTS.md Rule 3.
"""
import hashlib
import json
import pathlib
from typing import Dict, List, Optional
from tools.thor_recompiler.decoder import DecodedInstruction
from tools.thor_recompiler.codegen_step import emit_instruction_case

def generate_part_hpp(name: str, part_idx: int) -> str:
    suffix = name.split('_')[-1]
    guard = f"OASIS_HYBRID_GENERATED_M68K_RECOMPILED_{suffix.upper()}_PART{part_idx}_HPP_"
    return f"""#ifndef {guard}
#define {guard}

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include "tools/hybrid/generated/{name}.hpp"

namespace oasis::hybrid::generated {{

M68k9D70StepResult step_m68k_{suffix}_part{part_idx}(
    unsigned pc, const M68k9D70StepApi& api, unsigned& resume_pc,
    StepMetrics_{suffix}& metrics, bool& active, bool& interrupted) noexcept;

}} // namespace oasis::hybrid::generated

#endif // {guard}
"""

def generate_part_cpp(name: str, part_idx: int, insns: List[DecodedInstruction], rom: bytes) -> str:
    suffix = name.split('_')[-1]
    lines = [
        f'#include "tools/hybrid/generated/{name}_part{part_idx}.hpp"',
        '#include "tools/hybrid/generated/m68k_step_helpers.hpp"',
        "",
        "namespace oasis::hybrid::generated {",
        "",
        f"M68k9D70StepResult step_m68k_{suffix}_part{part_idx}(",
        f"    unsigned pc, const M68k9D70StepApi& api, unsigned& resume_pc,",
        f"    StepMetrics_{suffix}& metrics, bool& active, bool& interrupted) noexcept {{",
        "    switch (pc) {",
    ]
    for insn in insns:
        case_lines = emit_instruction_case(insn, rom)
        # Adapt RTS / exit terminal in case
        lines.extend(case_lines)
    lines.extend([
        "    default:",
        "        return M68k9D70StepResult::NotHandled;",
        "    }",
        "}",
        "",
        "} // namespace oasis::hybrid::generated",
        ""
    ])
    return "\n".join(lines)

def generate_coordinator_hpp(name: str) -> str:
    suffix = name.split('_')[-1]
    guard = f"OASIS_HYBRID_GENERATED_M68K_RECOMPILED_{suffix.upper()}_HPP_"
    return f"""#ifndef {guard}
#define {guard}

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include <cstdint>

namespace oasis::hybrid::generated {{

struct StepMetrics_{suffix} {{
    unsigned target_entries{{}};
    unsigned native_calls{{}};
    unsigned completed_calls{{}};
    unsigned stepped_instructions{{}};
    unsigned active_at_entry{{}};
}};

M68k9D70StepResult step_m68k_{suffix}(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
const StepMetrics_{suffix}& get_metrics_{suffix}() noexcept;
void reset_metrics_{suffix}() noexcept;

}} // namespace oasis::hybrid::generated

#endif // {guard}
"""

def generate_coordinator_cpp(name: str, entry_pc: int, part_count: int, exit_pcs: List[int]) -> str:
    suffix = name.split('_')[-1]
    lines = [
        f'#include "tools/hybrid/generated/{name}.hpp"',
        '#include "tools/hybrid/generated/m68k_step_helpers.hpp"',
    ]
    for i in range(part_count):
        lines.append(f'#include "tools/hybrid/generated/{name}_part{i}.hpp"')
    lines.extend([
        "",
        "namespace oasis::hybrid::generated {",
        "namespace {",
        "",
        f"constexpr unsigned kStart = 0x{entry_pc:06X}U;",
        "bool active = false;",
        "bool interrupted = false;",
        "unsigned resume_pc = 0U;",
        f"StepMetrics_{suffix} metrics{{}};",
        "",
        "} // namespace",
        "",
        f"M68k9D70StepResult step_m68k_{suffix}(unsigned pc_in, const M68k9D70StepApi& api, bool enabled) noexcept {{",
        "    const unsigned pc = pc_in & kMask24;",
        "    if (pc == kStart) {",
        "        metrics.target_entries++;",
        "        if (!enabled) return M68k9D70StepResult::NotHandled;",
        "        if (active) {",
        "            metrics.active_at_entry++;",
        "            active = false;",
        "            interrupted = false;",
        "            return M68k9D70StepResult::ContractFault;",
        "        }",
        "        active = true;",
        "        interrupted = false;",
        "        metrics.native_calls++;",
        "    }",
        "    if (!active) return M68k9D70StepResult::NotHandled;",
        "    if (active && pc != resume_pc && pc != kStart) {",
        "        interrupted = true;",
        "        return M68k9D70StepResult::NotHandled;",
        "    }",
        "    if (interrupted && pc == resume_pc) {",
        "        interrupted = false;",
        "    }",
        "",
        "    M68k9D70StepResult res = M68k9D70StepResult::NotHandled;",
    ])
    for i in range(part_count):
        lines.extend([
            f"    res = step_m68k_{suffix}_part{i}(pc, api, resume_pc, metrics, active, interrupted);",
            "    if (res != M68k9D70StepResult::NotHandled) goto step_done;",
        ])
    lines.extend([
        "step_done:",
        "    if (res == M68k9D70StepResult::InstructionStepped) {",
    ])
    if exit_pcs:
        checks = " || ".join(f"pc == 0x{ep:06X}U" for ep in exit_pcs)
        lines.extend([
            f"        if ({checks}) {{",
            "            active = false;",
            "            interrupted = false;",
            "        }",
        ])
    lines.extend([
        "        return res;",
        "    }",
        "    active = false;",
        "    interrupted = false;",
        "    return res;",
        "}",
        "",
        f"const StepMetrics_{suffix}& get_metrics_{suffix}() noexcept {{ return metrics; }}",
        f"void reset_metrics_{suffix}() noexcept {{ metrics = {{}}; active = false; interrupted = false; resume_pc = 0; }}",
        "",
        "} // namespace oasis::hybrid::generated",
        ""
    ])
    return "\n".join(lines)

def partition_and_generate(name: str, entry_pc: int, insns: List[DecodedInstruction],
                           rom: bytes, out_dir: pathlib.Path, max_insns_per_part: int = 8) -> Dict:
    out_dir.mkdir(parents=True, exist_ok=True)
    manifest_dir = out_dir / "manifests"
    manifest_dir.mkdir(parents=True, exist_ok=True)

    parts: List[List[DecodedInstruction]] = []
    for i in range(0, len(insns), max_insns_per_part):
        parts.append(insns[i:i + max_insns_per_part])

    generated_files: Dict[str, str] = {}
    pc_map: Dict[str, str] = {}
    exit_pcs: List[int] = []

    for insn in insns:
        if insn.mnemonic in ("RTS", "RTE") or (insn.mnemonic == "JMP" and insn.ops and insn.ops[0].kind == "indir_a"):
            exit_pcs.append(insn.pc)

    for idx, part in enumerate(parts):
        hpp_name = f"{name}_part{idx}.hpp"
        cpp_name = f"{name}_part{idx}.cpp"
        hpp_content = generate_part_hpp(name, idx)
        cpp_content = generate_part_cpp(name, idx, part, rom)

        (out_dir / hpp_name).write_text(hpp_content, encoding="utf-8")
        (out_dir / cpp_name).write_text(cpp_content, encoding="utf-8")
        generated_files[hpp_name] = hashlib.sha256(hpp_content.encode()).hexdigest()
        generated_files[cpp_name] = hashlib.sha256(cpp_content.encode()).hexdigest()

        for insn in part:
            pc_map[f"0x{insn.pc:06X}"] = cpp_name

    coord_hpp_name = f"{name}.hpp"
    coord_cpp_name = f"{name}.cpp"
    coord_hpp = generate_coordinator_hpp(name)
    coord_cpp = generate_coordinator_cpp(name, entry_pc, len(parts), exit_pcs)

    (out_dir / coord_hpp_name).write_text(coord_hpp, encoding="utf-8")
    (out_dir / coord_cpp_name).write_text(coord_cpp, encoding="utf-8")
    generated_files[coord_hpp_name] = hashlib.sha256(coord_hpp.encode()).hexdigest()
    generated_files[coord_cpp_name] = hashlib.sha256(coord_cpp.encode()).hexdigest()

    manifest = {
        "routine_name": name,
        "entry_pc": f"0x{entry_pc:06X}",
        "instruction_count": len(insns),
        "partition_count": len(parts),
        "exit_pcs": [f"0x{ep:06X}" for ep in exit_pcs],
        "pc_to_source_map": pc_map,
        "source_hashes": generated_files,
        "cfg_manifest": {
            "entry": f"0x{entry_pc:06X}",
            "blocks": len(parts),
            "instructions": [
                {
                    "pc": f"0x{insn.pc:06X}",
                    "mnemonic": insn.mnemonic,
                    "size": insn.size,
                    "target": f"0x{insn.branch_target:06X}" if insn.branch_target else None
                } for insn in insns
            ]
        },
        "dependency_manifest": {
            "headers": ["m68k_step_helpers.hpp"],
            "parts": [f"{name}_part{i}.hpp" for i in range(len(parts))]
        },
        "unsupported_opcodes": []
    }

    manifest_file = manifest_dir / f"{name}_manifest.json"
    manifest_file.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    return manifest
