#pragma once

#include "tools/thor_ir/entity_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/exp246_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/exp247_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/exp248_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/exp249_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/exp250_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_router.hpp"

#include <cstdint>

namespace oasis::recompiler::ir {

struct Exp256DispatchHybridMetrics {
    std::uint64_t m15256_entries{0};
    std::uint64_t m15256_guest_steps{0};
    std::uint64_t disjoint_new_native_steps{0};
    std::uint64_t original_cpu_fallbacks{0};
    std::uint64_t valid_uncompiled_exits{0};
    std::uint64_t cpu_exceptions{0};
    std::uint64_t contract_faults{0};
    std::uint64_t completed_returns{0};
};

class HybridM68kStepBus256 final {
public:
    explicit HybridM68kStepBus256(const oasis::hybrid::M68k9D70StepApi& api) noexcept
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
inline Exp256DispatchHybridMetrics& exp256_metrics_mut() noexcept {
    static Exp256DispatchHybridMetrics m{};
    return m;
}
inline bool& in_exp256_mut() noexcept {
    static bool active{false};
    return active;
}
} // namespace detail

inline const Exp256DispatchHybridMetrics& get_exp256_dispatch_hybrid_metrics() noexcept {
    return detail::exp256_metrics_mut();
}

inline void reset_exp256_dispatch_hybrid_metrics() noexcept {
    detail::exp256_metrics_mut() = {};
    detail::in_exp256_mut() = false;
}

inline oasis::hybrid::M68k9D70StepResult step_m68k_portable_m15256(
    unsigned pc_in, const oasis::hybrid::M68k9D70StepApi& api, bool enabled) noexcept {
    const std::uint32_t pc = static_cast<std::uint32_t>(pc_in) & kBusAddressMask;
    auto& m = detail::exp256_metrics_mut();
    bool& in_port = detail::in_exp256_mut();

    if (!enabled) return oasis::hybrid::M68k9D70StepResult::NotHandled;

    if (!is_compiled_exp256_dispatch_pc(pc)) {
        if (in_port) {
            in_port = false;
            ++m.valid_uncompiled_exits;
        }
        return oasis::hybrid::M68k9D70StepResult::NotHandled;
    }

    if (!in_port) {
        in_port = true;
        ++m.m15256_entries;
    }

    HybridM68kStepBus256 bus{api};
    std::uint32_t next_pc = pc;
    if (!step_one_exp256_dispatch_ir(bus, pc, next_pc)) {
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

    ++m.m15256_guest_steps;
    ++m.disjoint_new_native_steps;

    const unsigned op = static_cast<unsigned>(
        ((api.peek(api.context, pc) & 0xFF) << 8) | (api.peek(api.context, (pc + 1U) & kBusAddressMask) & 0xFF));
    if (op == 0x4E75U) ++m.completed_returns;
    if (!is_compiled_exp256_dispatch_pc(next_pc)) {
        in_port = false;
        ++m.valid_uncompiled_exits;
    }
    return oasis::hybrid::M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::recompiler::ir
