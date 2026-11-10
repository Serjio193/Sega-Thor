#pragma once

#include "tools/thor_ir/entity_dispatch_hybrid_adapter.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_router.hpp"

#include <cstdint>

namespace oasis::recompiler::ir {

struct ExpDispatchHybridMetrics {
    std::uint64_t new_handler_entries{0};
    std::uint64_t new_handler_guest_steps{0};
    std::uint64_t disjoint_new_native_steps{0};
    std::uint64_t original_cpu_fallbacks{0};
    std::uint64_t valid_uncompiled_exits{0};
    std::uint64_t nonlocal_returns{0};
    std::uint64_t cpu_exceptions{0};
    std::uint64_t contract_faults{0};
    std::uint64_t completed_returns{0};
};

namespace detail {
inline ExpDispatchHybridMetrics& exp_metrics_mut() noexcept {
    static ExpDispatchHybridMetrics m{};
    return m;
}
inline bool& in_exp_mut() noexcept {
    static bool active{false};
    return active;
}
inline bool& armed_nonlocal_mut() noexcept {
    static bool armed{false};
    return armed;
}
} // namespace detail

inline const ExpDispatchHybridMetrics& get_exp_dispatch_hybrid_metrics() noexcept {
    return detail::exp_metrics_mut();
}

inline void reset_exp_dispatch_hybrid_metrics() noexcept {
    detail::exp_metrics_mut() = {};
    detail::in_exp_mut() = false;
    detail::armed_nonlocal_mut() = false;
}

inline oasis::hybrid::M68k9D70StepResult step_m68k_portable_m15245(
    unsigned pc_in, const oasis::hybrid::M68k9D70StepApi& api, bool enabled) noexcept {
    const std::uint32_t pc = static_cast<std::uint32_t>(pc_in) & kBusAddressMask;
    auto& m = detail::exp_metrics_mut();
    bool& in_port = detail::in_exp_mut();

    if (!enabled) return oasis::hybrid::M68k9D70StepResult::NotHandled;

    if (!is_compiled_exp_dispatch_pc(pc)) {
        if (in_port) {
            in_port = false;
            ++m.valid_uncompiled_exits;
        }
        return oasis::hybrid::M68k9D70StepResult::NotHandled;
    }

    if (!in_port) {
        in_port = true;
        ++m.new_handler_entries;
    }

    HybridM68kStepBus bus{api};
    std::uint32_t next_pc = pc;
    if (!step_one_exp_dispatch_ir(bus, pc, next_pc)) {
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

    ++m.new_handler_guest_steps;
    ++m.disjoint_new_native_steps;

    if (pc == 0x007E96U) detail::armed_nonlocal_mut() = true;
    const unsigned op = static_cast<unsigned>(
        ((api.peek(api.context, pc) & 0xFF) << 8) | (api.peek(api.context, (pc + 1U) & kBusAddressMask) & 0xFF));
    if (op == 0x4E75U) {
        ++m.completed_returns;
        if (detail::armed_nonlocal_mut()) {
            ++m.nonlocal_returns;
            detail::armed_nonlocal_mut() = false;
        }
    }
    if (!is_compiled_exp_dispatch_pc(next_pc)) {
        in_port = false;
        ++m.valid_uncompiled_exits;
    }
    return oasis::hybrid::M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::recompiler::ir
