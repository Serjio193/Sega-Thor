#!/usr/bin/env python3
"""
M15.250 High-Displacement Connected Hotpath Callee IR + Auto-Split C++20 Generator.
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
from tools.generate_entity_dispatch_m15_249 import load_m15_248_pcs
from tools.thor_recompiler.cfg_reconstructor import format_insn, reconstruct_cfg
from tools.thor_recompiler.cpp_emitter_m15_250 import emit_m15_250_cpp_files
from tools.thor_recompiler.decoder_ext_m15_250 import decode_m15_250_at
from tools.thor_recompiler.insn_lowering_m15_250 import exact_m68k_cycles_250 as exact_m68k_cycles

ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"

M15_250_ENTRY_ROOTS = {
    0x0255D8: "hotpath_255d8_enemy_substates",
    0x00A4C8: "hotpath_0a4c8_camera_scroll_tracker",
    0x017908: "hotpath_17908_directional_router",
    0x00EA16: "hotpath_0ea16_enemy_substate_timer",
    0x02E8B4: "hotpath_2e8b4_water_shrine_stream",
    0x00A550: "hotpath_0a550_entity_priority_sort",
    0x008778: "hotpath_08778_combat_collision_scan",
    0x02EC18: "hotpath_2ec18_water_shrine_gate",
}

M15_250_WINDOWS = [
    (0x0255D8, 0x0256B8),
    (0x00A4C8, 0x00A51A),
    (0x017908, 0x017920),
    (0x00EA16, 0x00EA76),
    (0x02E8B4, 0x02E910),
    (0x00A550, 0x00A58A),
    (0x008778, 0x0087E2),
    (0x02EC18, 0x02EC5A),
]


def operand_to_ir_dict(op) -> dict:
    return {"raw_kind": op.kind, "reg_index": op.reg, "val": op.val, "size": op.size, "ext": op.ext}


def load_m15_249_pcs(root: Path) -> set[int]:
    p = root / "src" / "tools" / "thor_ir" / "m15_249_callee_ir.json"
    d = json.loads(p.read_text(encoding="utf-8"))
    return {int(x["pc"], 16) for b in d["blocks"] for x in b["instructions"]}


def emit_hybrid_adapter(path: Path) -> None:
    code = """#pragma once

#include "tools/thor_ir/entity_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/exp246_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/exp247_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/exp248_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/exp249_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_router.hpp"

#include <cstdint>

namespace oasis::recompiler::ir {

struct Exp250DispatchHybridMetrics {
    std::uint64_t m15250_entries{0};
    std::uint64_t m15250_guest_steps{0};
    std::uint64_t disjoint_new_native_steps{0};
    std::uint64_t original_cpu_fallbacks{0};
    std::uint64_t valid_uncompiled_exits{0};
    std::uint64_t cpu_exceptions{0};
    std::uint64_t contract_faults{0};
    std::uint64_t completed_returns{0};
};

class HybridM68kStepBus250 final {
public:
    explicit HybridM68kStepBus250(const oasis::hybrid::M68k9D70StepApi& api) noexcept
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
inline Exp250DispatchHybridMetrics& exp250_metrics_mut() noexcept {
    static Exp250DispatchHybridMetrics m{};
    return m;
}
inline bool& in_exp250_mut() noexcept {
    static bool active{false};
    return active;
}
} // namespace detail

inline const Exp250DispatchHybridMetrics& get_exp250_dispatch_hybrid_metrics() noexcept {
    return detail::exp250_metrics_mut();
}

inline void reset_exp250_dispatch_hybrid_metrics() noexcept {
    detail::exp250_metrics_mut() = {};
    detail::in_exp250_mut() = false;
}

inline oasis::hybrid::M68k9D70StepResult step_m68k_portable_m15250(
    unsigned pc_in, const oasis::hybrid::M68k9D70StepApi& api, bool enabled) noexcept {
    const std::uint32_t pc = static_cast<std::uint32_t>(pc_in) & kBusAddressMask;
    auto& m = detail::exp250_metrics_mut();
    bool& in_port = detail::in_exp250_mut();

    if (!enabled) return oasis::hybrid::M68k9D70StepResult::NotHandled;

    if (!is_compiled_exp250_dispatch_pc(pc)) {
        if (in_port) {
            in_port = false;
            ++m.valid_uncompiled_exits;
        }
        return oasis::hybrid::M68k9D70StepResult::NotHandled;
    }

    if (!in_port) {
        in_port = true;
        ++m.m15250_entries;
    }

    HybridM68kStepBus250 bus{api};
    std::uint32_t next_pc = pc;
    if (!step_one_exp250_dispatch_ir(bus, pc, next_pc)) {
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

    ++m.m15250_guest_steps;
    ++m.disjoint_new_native_steps;

    const unsigned op = static_cast<unsigned>(
        ((api.peek(api.context, pc) & 0xFF) << 8) | (api.peek(api.context, (pc + 1U) & kBusAddressMask) & 0xFF));
    if (op == 0x4E75U) ++m.completed_returns;
    if (!is_compiled_exp250_dispatch_pc(next_pc)) {
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
    m249_pcs = load_m15_249_pcs(root)
    stop_pcs = set(prod_owned.keys()) | m244_pcs | m245_pcs | m246_pcs | m247_pcs | m248_pcs | m249_pcs | {0x00A51A, 0x017920, 0x02E910, 0x00A58A, 0x0087E2, 0x02EC5A}

    cfg = reconstruct_cfg(
        rom_bytes,
        M15_250_ENTRY_ROOTS,
        jump_table_base=0x0059BA,
        jump_table_entries=59,
        compiled_targets=set(M15_250_ENTRY_ROOTS.keys()),
        compiled_windows=M15_250_WINDOWS,
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
        "schema": "oasis.m15_250.callee_ir.v1",
        "milestone": "M15.250",
        "name": "connected_hotpaths_subsystem_250",
        "rom_sha256": ROM_SHA256,
        "entry_roots": {f"0x{k:06X}": v for k, v in M15_250_ENTRY_ROOTS.items()},
        "compiled_windows": [[f"0x{s:06X}", f"0x{e:06X}"] for s, e in M15_250_WINDOWS],
        "total_basic_blocks": len(ir_blocks),
        "total_instructions": len(compiled_pcs),
        "blocks": ir_blocks,
        "cfg_edges": cfg["edges"],
    }

    ir_path = root / "src" / "tools" / "thor_ir" / "m15_250_callee_ir.json"
    ir_path.write_text(json.dumps(ir_doc, indent=2), encoding="utf-8")
    print(f"Emitted Thor IR: {ir_path} ({len(compiled_pcs)} insns, {len(ir_blocks)} blocks, {len(cfg['edges'])} edges)")

    gen_dir = root / "src" / "tools" / "thor_ir" / "generated_m15_250"
    source_map, parts = emit_m15_250_cpp_files(ir_doc, gen_dir)
    print(f"Emitted {len(parts)} C++20 headers under {gen_dir}")

    sm_path = root / "src" / "tools" / "thor_ir" / "m15_250_source_map.json"
    sm_path.write_text(json.dumps(source_map, indent=2), encoding="utf-8")
    print(f"Emitted source map: {sm_path} ({len(source_map)} mapped instructions)")

    adapter_path = root / "src" / "tools" / "thor_ir" / "exp250_dispatch_hybrid_adapter.hpp"
    emit_hybrid_adapter(adapter_path)
    print(f"Emitted hybrid adapter: {adapter_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
