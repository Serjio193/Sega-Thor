#!/usr/bin/env python3
"""
M15.249 High-Displacement Connected Hotpath Callee IR + Auto-Split C++20 Generator.
Generates portable Thor IR, manifest, CFG source map, partitioned C++20 headers,
and hybrid adapter for unowned high-ROI hotpath subroutines.
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
from tools.thor_recompiler.cfg_reconstructor import format_insn, reconstruct_cfg
from tools.thor_recompiler.cpp_emitter_m15_249 import emit_m15_249_cpp_files
from tools.thor_recompiler.decoder_ext_m15_249 import decode_m15_249_at
from tools.thor_recompiler.insn_lowering_m15_249 import exact_m68k_cycles_249 as exact_m68k_cycles

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"

M15_249_ENTRY_ROOTS = {
    0x0254EA: "hotpath_254ea_enemy_controller",
    0x00EA04: "hotpath_ea04_enemy_substate_check",
    0x016D5C: "hotpath_16d5c_enemy_cooldown_init",
    0x0170E0: "hotpath_170e0_enemy_target_lock",
    0x0171C4: "hotpath_171c4_enemy_target_track",
    0x00E0B8: "hotpath_e0b8_dummy_rts",
    0x00F96A: "hotpath_f96a_movement_state_logic",
}

M15_249_WINDOWS = [
    (0x0254EA, 0x0255D8),
    (0x00EA04, 0x00EA16),
    (0x016D5C, 0x016D76),
    (0x0170E0, 0x01710A),
    (0x0171C4, 0x0171F8),
    (0x00E0B8, 0x00E0BA),
    (0x00F96A, 0x00F9C6),
]


def operand_to_ir_dict(op) -> dict:
    return {"raw_kind": op.kind, "reg_index": op.reg, "val": op.val, "size": op.size, "ext": op.ext}


def load_m15_248_pcs(root: Path) -> set[int]:
    p = root / "src" / "tools" / "thor_ir" / "m15_248_callee_ir.json"
    d = json.loads(p.read_text(encoding="utf-8"))
    return {int(x["pc"], 16) for b in d["blocks"] for x in b["instructions"]}


def emit_hybrid_adapter(path: Path) -> None:
    code = """#pragma once

#include "tools/thor_ir/entity_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/exp246_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/exp247_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/exp248_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/generated_m15_249/exp249_dispatch_router.hpp"

#include <cstdint>

namespace oasis::recompiler::ir {

struct Exp249DispatchHybridMetrics {
    std::uint64_t m15249_entries{0};
    std::uint64_t m15249_guest_steps{0};
    std::uint64_t disjoint_new_native_steps{0};
    std::uint64_t original_cpu_fallbacks{0};
    std::uint64_t valid_uncompiled_exits{0};
    std::uint64_t cpu_exceptions{0};
    std::uint64_t contract_faults{0};
    std::uint64_t completed_returns{0};
};

class HybridM68kStepBus249 final {
public:
    explicit HybridM68kStepBus249(const oasis::hybrid::M68k9D70StepApi& api) noexcept
        : api_(api), base_(api) {}

    std::uint32_t d(int i) const noexcept { return base_.d(i); }
    void set_d(int i, std::uint32_t v) noexcept { base_.set_d(i, v); }
    std::uint32_t a(int i) const noexcept { return base_.a(i); }
    void set_a(int i, std::uint32_t v) noexcept { base_.set_a(i, v); }
    std::uint16_t ccr() const noexcept { return base_.ccr(); }
    void set_ccr(std::uint16_t c) noexcept { base_.set_ccr(c); }
    bool check_align(std::uint32_t addr, IrOpSize sz) noexcept { return base_.check_align(addr, sz); }
    std::uint32_t read_m(std::uint32_t addr, IrOpSize sz) noexcept { return base_.read_m(addr, sz); }
    void write_m(std::uint32_t addr, IrOpSize sz, std::uint32_t val) noexcept { base_.write_m(addr, sz, val); }
    bool faulted() const noexcept { return div_zero_ || base_.faulted(); }
    void fault_addr() noexcept { base_.fault_addr(); }
    void fault_div0() noexcept { div_zero_ = true; }
    bool begin_insn(std::uint32_t pc, std::uint16_t op) noexcept { return base_.begin_insn(pc, op); }
    bool end_insn(std::uint32_t next_pc, std::uint16_t op, int m68k_cyc, int adj) noexcept {
        return base_.end_insn(next_pc, op, m68k_cyc, adj);
    }
    bool end_insn_mc(std::uint32_t next_pc, std::uint16_t op, int, int hybrid_delta_mc, bool skip_refresh) noexcept {
        const auto np = next_pc & kBusAddressMask;
        const int b0 = api_.peek(api_.context, np);
        const int b1 = api_.peek(api_.context, (np + 1U) & kBusAddressMask);
        if (b0 < 0 || b1 < 0) return false;
        const auto next_word = static_cast<unsigned>(((b0 & 0xFF) << 8) | (b1 & 0xFF));
        api_.set_return_state(api_.context, np, (np + 2U) & kBusAddressMask, next_word);
        if (hybrid_delta_mc != 0 && api_.add_cycles) api_.add_cycles(api_.context, hybrid_delta_mc);
        if (skip_refresh && api_.skip_bus_refresh) api_.skip_bus_refresh(api_.context);
        api_.finish_instruction(api_.context, op);
        return true;
    }

private:
    const oasis::hybrid::M68k9D70StepApi& api_;
    HybridM68kStepBus base_;
    bool div_zero_{false};
};

namespace detail {
inline Exp249DispatchHybridMetrics& exp249_metrics_mut() noexcept {
    static Exp249DispatchHybridMetrics m{};
    return m;
}
inline bool& in_exp249_mut() noexcept {
    static bool active{false};
    return active;
}
} // namespace detail

inline const Exp249DispatchHybridMetrics& get_exp249_dispatch_hybrid_metrics() noexcept {
    return detail::exp249_metrics_mut();
}

inline void reset_exp249_dispatch_hybrid_metrics() noexcept {
    detail::exp249_metrics_mut() = {};
    detail::in_exp249_mut() = false;
}

inline oasis::hybrid::M68k9D70StepResult step_m68k_portable_m15249(
    unsigned pc_in, const oasis::hybrid::M68k9D70StepApi& api, bool enabled) noexcept {
    const std::uint32_t pc = static_cast<std::uint32_t>(pc_in) & kBusAddressMask;
    auto& m = detail::exp249_metrics_mut();
    bool& in_port = detail::in_exp249_mut();

    if (!enabled) return oasis::hybrid::M68k9D70StepResult::NotHandled;

    if (!is_compiled_exp249_dispatch_pc(pc)) {
        if (in_port) {
            in_port = false;
            ++m.valid_uncompiled_exits;
        }
        return oasis::hybrid::M68k9D70StepResult::NotHandled;
    }

    if (!in_port) {
        in_port = true;
        ++m.m15249_entries;
    }

    HybridM68kStepBus249 bus{api};
    std::uint32_t next_pc = pc;
    if (!step_one_exp249_dispatch_ir(bus, pc, next_pc)) {
        if (bus.faulted()) {
            ++m.cpu_exceptions;
            ++m.original_cpu_fallbacks;
            in_port = false;
            return oasis::hybrid::M68k9D70StepResult::NotHandled;
        }
        ++m.contract_faults;
        in_port = false;
        return oasis::hybrid::M68k9D70StepResult::ContractFault;
    }

    ++m.m15249_guest_steps;
    ++m.disjoint_new_native_steps;

    const unsigned op = static_cast<unsigned>(
        ((api.peek(api.context, pc) & 0xFF) << 8) | (api.peek(api.context, (pc + 1U) & kBusAddressMask) & 0xFF));
    if (op == 0x4E75U) ++m.completed_returns;
    if (!is_compiled_exp249_dispatch_pc(next_pc)) {
        in_port = false;
        ++m.valid_uncompiled_exits;
    }
    return oasis::hybrid::M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::recompiler::ir
"""
    path.write_text(code, encoding="utf-8")


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
    stop_pcs = set(prod_owned.keys()) | m244_pcs | m245_pcs | m246_pcs | m247_pcs | m248_pcs

    cfg = reconstruct_cfg(
        rom_bytes,
        M15_249_ENTRY_ROOTS,
        jump_table_base=0x0059BA,
        jump_table_entries=59,
        compiled_targets=set(M15_249_ENTRY_ROOTS.keys()),
        compiled_windows=M15_249_WINDOWS,
        stop_pcs=stop_pcs,
        decode_fn=decode_m15_249_at,
    )
    compiled_pcs = set(cfg["decoded_map"].keys())
    assert not (compiled_pcs & set(prod_owned.keys())), "Production helper overlap!"
    assert not (compiled_pcs & m244_pcs), "M15.244 overlap!"
    assert not (compiled_pcs & m245_pcs), "M15.245 overlap!"
    assert not (compiled_pcs & m246_pcs), "M15.246 overlap!"
    assert not (compiled_pcs & m247_pcs), "M15.247 overlap!"
    assert not (compiled_pcs & m248_pcs), "M15.248 overlap!"

    ir_blocks = []
    op_counter = 0
    for blk in cfg["basic_blocks"]:
        ir_insns = []
        for insn in blk["insns"]:
            op_id = f"IR249_{op_counter:04d}_0x{insn.pc:06X}"
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
        "schema": "thor.ir.m15_249.callees.v1",
        "milestone": "M15.249",
        "name": "m15_249_hotpath_callees",
        "rom_sha256": rom_sha,
        "entry_roots": {f"0x{k:06X}": v for k, v in sorted(M15_249_ENTRY_ROOTS.items())},
        "compiled_windows": [[f"0x{s:06X}", f"0x{e:06X}"] for s, e in M15_249_WINDOWS],
        "total_basic_blocks": len(ir_blocks),
        "total_instructions": len(compiled_pcs),
        "blocks": ir_blocks,
        "cfg_edges": cfg["edges"],
    }

    ir_out = root / "src" / "tools" / "thor_ir" / "m15_249_callee_ir.json"
    ir_out.write_text(json.dumps(ir_doc, indent=2) + "\n", encoding="utf-8")
    print(f"WROTE: {ir_out} ({len(compiled_pcs)} insns, {len(ir_blocks)} blocks)")

    gen_dir = root / "src" / "tools" / "thor_ir" / "generated_m15_249"
    smap, parts = emit_m15_249_cpp_files(ir_doc, gen_dir)
    print(f"EMITTED {len(parts)} C++ headers in {gen_dir}")

    smap_out = root / "src" / "tools" / "thor_ir" / "m15_249_source_map.json"
    smap_out.write_text(json.dumps(smap, indent=2) + "\n", encoding="utf-8")

    adapter_path = root / "src" / "tools" / "thor_ir" / "exp249_dispatch_hybrid_adapter.hpp"
    emit_hybrid_adapter(adapter_path)
    print(f"WROTE: {adapter_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
