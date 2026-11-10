#pragma once

#include "tools/thor_ir/exp250_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_mode_b.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_router.hpp"

#include <cstdint>

namespace oasis::recompiler::ir {

class HybridM68kFusedBus250 final {
public:
    explicit HybridM68kFusedBus250(const oasis::hybrid::M68k9D70StepApi& api) noexcept
        : api_(api) {}

    std::uint32_t d(int i) const noexcept { return api_.read_register(api_.context, i); }
    void set_d(int i, std::uint32_t v) noexcept { api_.write_register(api_.context, i, v); }
    std::uint32_t a(int i) const noexcept { return api_.read_register(api_.context, 8 + i); }
    void set_a(int i, std::uint32_t v) noexcept { api_.write_register(api_.context, 8 + i, v); }

    std::uint16_t ccr() const noexcept {
        return static_cast<std::uint16_t>(api_.read_register(api_.context, 17) & 0x1FU);
    }
    void set_ccr(std::uint16_t c) noexcept {
        const auto sr = api_.read_register(api_.context, 17);
        api_.write_register(api_.context, 17, (sr & ~0x1FU) | (c & 0x1FU));
    }

    bool check_align(std::uint32_t addr, IrOpSize sz) noexcept {
        if (sz != IrOpSize::Byte && (addr & 1U) != 0U) {
            address_fault_ = true;
            return false;
        }
        return true;
    }

    std::uint32_t read_m(std::uint32_t addr, IrOpSize sz) noexcept {
        const int w = (sz == IrOpSize::Byte) ? 1 : (sz == IrOpSize::Word ? 2 : 4);
        return api_.read(api_.context, addr & kBusAddressMask, w);
    }

    void write_m(std::uint32_t addr, IrOpSize sz, std::uint32_t val) noexcept {
        const int w = (sz == IrOpSize::Byte) ? 1 : (sz == IrOpSize::Word ? 2 : 4);
        api_.write(api_.context, addr & kBusAddressMask, w, val);
    }

    bool faulted() const noexcept { return div_zero_ || address_fault_; }
    void fault_addr() noexcept { address_fault_ = true; }
    void fault_div0() noexcept { div_zero_ = true; }
    bool slice_yield() const noexcept { return slice_yield_; }

    bool begin_insn(std::uint32_t pc, std::uint16_t op) noexcept {
        if (step_count_ > 0 && api_.read_scheduler_field) {
            const int cur_cyc = api_.read_scheduler_field(api_.context, 0);
            const int end_cyc = api_.read_scheduler_field(api_.context, 1);
            const int int_lvl = api_.read_scheduler_field(api_.context, 8);
            const int int_msk = api_.read_scheduler_field(api_.context, 9);
            if (int_lvl > int_msk || (end_cyc > 0 && cur_cyc >= end_cyc)) {
                slice_yield_ = true;
                return false;
            }
        }
        last_insn_pc_ = pc & kBusAddressMask;
        last_insn_op_ = op;
        ++step_count_;
        if (api_.observe_instruction) api_.observe_instruction(api_.context, pc & kBusAddressMask, op);
        if (api_.begin_instruction) api_.begin_instruction(api_.context, op);
        return true;
    }

    bool end_insn(std::uint32_t next_pc, std::uint16_t op, int m68k_cyc, int adj) noexcept {
        const auto np = next_pc & kBusAddressMask;
        const int b0 = api_.peek(api_.context, np);
        const int b1 = api_.peek(api_.context, (np + 1U) & kBusAddressMask);
        if (b0 < 0 || b1 < 0) return false;
        const auto next_word = static_cast<unsigned>(((b0 & 0xFF) << 8) | (b1 & 0xFF));
        api_.set_return_state(api_.context, np, (np + 2U) & kBusAddressMask, next_word);

        last_next_pc_ = np;
        last_op_ = op;
        accumulated_cycles_ += m68k_cyc;
        if (adj != 0) {
            const int delta = (adj < 0)
                ? (api_.byte_not_taken_adjustment ? api_.byte_not_taken_adjustment(api_.context) : -14)
                : (api_.word_not_taken_adjustment ? api_.word_not_taken_adjustment(api_.context) : 14);
            if (delta != 0 && api_.add_cycles) api_.add_cycles(api_.context, delta);
        }
        if (api_.finish_instruction) api_.finish_instruction(api_.context, op);
        return true;
    }

    bool end_insn_mc(std::uint32_t next_pc, std::uint16_t op, int m68k_cyc, int hybrid_delta_mc, bool skip_refresh) noexcept {
        const auto np = next_pc & kBusAddressMask;
        const int b0 = api_.peek(api_.context, np);
        const int b1 = api_.peek(api_.context, (np + 1U) & kBusAddressMask);
        if (b0 < 0 || b1 < 0) return false;
        const auto next_word = static_cast<unsigned>(((b0 & 0xFF) << 8) | (b1 & 0xFF));
        api_.set_return_state(api_.context, np, (np + 2U) & kBusAddressMask, next_word);

        last_next_pc_ = np;
        last_op_ = op;
        accumulated_cycles_ += m68k_cyc;
        if (hybrid_delta_mc != 0 && api_.add_cycles) api_.add_cycles(api_.context, hybrid_delta_mc);
        if (skip_refresh && api_.skip_bus_refresh) api_.skip_bus_refresh(api_.context);
        if (api_.finish_instruction) api_.finish_instruction(api_.context, op);
        return true;
    }

    std::uint32_t step_count() const noexcept { return step_count_; }
    int accumulated_cycles() const noexcept { return accumulated_cycles_; }
    std::uint16_t last_op() const noexcept { return last_op_; }
    std::uint32_t last_next_pc() const noexcept { return last_next_pc_; }

private:
    const oasis::hybrid::M68k9D70StepApi& api_;
    std::uint32_t step_count_{0};
    int accumulated_cycles_{0};
    std::uint32_t last_insn_pc_{0};
    std::uint16_t last_insn_op_{0};
    std::uint32_t last_next_pc_{0};
    std::uint16_t last_op_{0};
    bool div_zero_{false};
    bool address_fault_{false};
    bool slice_yield_{false};
};

inline oasis::hybrid::M68k9D70StepResult step_m68k_portable_m15250_fused(
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

    HybridM68kFusedBus250 bus{api};
    std::uint32_t next_pc = pc;
    std::uint32_t fused_blocks = 0;
    std::uint32_t fused_steps = 0;

    const auto prev_steps = bus.step_count();
    if (run_exp250_mode_b_all(bus, pc, next_pc)) {
        ++fused_blocks;
        fused_steps = bus.step_count() - prev_steps;
    } else if (bus.slice_yield()) {
        fused_steps = bus.step_count() - prev_steps;
        next_pc = bus.last_next_pc();
    } else if (step_one_exp250_dispatch_ir(bus, pc, next_pc)) {
        fused_steps = bus.step_count() - prev_steps;
    }

    if (fused_steps == 0) {
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

    m.m15250_guest_steps += fused_steps;
    m.disjoint_new_native_steps += fused_steps;
    m.fused_blocks += fused_blocks;

    if (bus.last_op() == 0x4E75U) ++m.completed_returns;
    if (!is_compiled_exp250_dispatch_pc(next_pc)) {
        in_port = false;
        ++m.valid_uncompiled_exits;
    }
    return oasis::hybrid::M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::recompiler::ir
