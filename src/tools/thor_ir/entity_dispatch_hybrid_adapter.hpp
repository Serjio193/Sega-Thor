#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include "tools/thor_ir/generated/entity_dispatch_router.hpp"

#include <cstdint>

namespace oasis::recompiler::ir {

struct PortableDispatchHybridMetrics {
    std::uint64_t portable_runtime_entries{0};
    std::uint64_t portable_guest_instructions{0};
    std::uint64_t disjoint_new_native_steps{0};
    std::uint64_t fallback_transfers{0};
    std::uint64_t uncompiled_continuations{0};
    std::uint64_t original_cpu_exceptions{0};
    std::uint64_t contract_faults{0};
    std::uint64_t completed_returns{0};
};

class HybridM68kStepBus final {
public:
    explicit HybridM68kStepBus(const oasis::hybrid::M68k9D70StepApi& api) noexcept
        : api_(api) {}

    std::uint32_t d(int i) const noexcept { return api_.read_register(api_.context, static_cast<unsigned>(i)); }
    void set_d(int i, std::uint32_t v) noexcept { api_.write_register(api_.context, static_cast<unsigned>(i), v); }
    std::uint32_t a(int i) const noexcept { return api_.read_register(api_.context, static_cast<unsigned>(8 + i)); }
    void set_a(int i, std::uint32_t v) noexcept {
        api_.write_register(api_.context, static_cast<unsigned>(8 + i), (i == 7) ? (v & kBusAddressMask) : v);
    }
    std::uint16_t ccr() const noexcept { return static_cast<std::uint16_t>(api_.read_register(api_.context, 17U) & 0x1FU); }
    void set_ccr(std::uint16_t c) noexcept {
        const auto sr = api_.read_register(api_.context, 17U);
        api_.write_register(api_.context, 17U, (sr & 0xFFE0U) | (c & 0x1FU));
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

    bool faulted() const noexcept { return address_fault_; }
    void fault_addr() noexcept { address_fault_ = true; }

    bool begin_insn(std::uint32_t pc, std::uint16_t op) noexcept {
        if (started_) return true;
        started_ = true;
        api_.observe_instruction(api_.context, pc & kBusAddressMask, op);
        api_.begin_instruction(api_.context, op);
        return true;
    }

    bool end_insn(std::uint32_t next_pc, std::uint16_t op, int, int branch_adj_sign) noexcept {
        const auto np = next_pc & kBusAddressMask;
        const int b0 = api_.peek(api_.context, np);
        const int b1 = api_.peek(api_.context, (np + 1U) & kBusAddressMask);
        if (b0 < 0 || b1 < 0) return false;
        const auto next_word = static_cast<unsigned>(((b0 & 0xFF) << 8) | (b1 & 0xFF));
        api_.set_return_state(api_.context, np, (np + 2U) & kBusAddressMask, next_word);
        if (branch_adj_sign != 0) {
            const int delta = (branch_adj_sign < 0)
                ? (api_.byte_not_taken_adjustment ? api_.byte_not_taken_adjustment(api_.context) : -14)
                : (api_.word_not_taken_adjustment ? api_.word_not_taken_adjustment(api_.context) : 14);
            if (delta != 0 && api_.add_cycles) api_.add_cycles(api_.context, delta);
        }
        api_.finish_instruction(api_.context, op);
        return true;
    }

private:
    const oasis::hybrid::M68k9D70StepApi& api_;
    bool started_{false};
    bool address_fault_{false};
};

namespace detail {
inline PortableDispatchHybridMetrics& metrics_mut() noexcept {
    static PortableDispatchHybridMetrics m{};
    return m;
}
inline bool& in_portable_mut() noexcept {
    static bool active{false};
    return active;
}
} // namespace detail

inline const PortableDispatchHybridMetrics& get_portable_dispatch_hybrid_metrics() noexcept {
    return detail::metrics_mut();
}

inline void reset_portable_dispatch_hybrid_metrics() noexcept {
    detail::metrics_mut() = {};
    detail::in_portable_mut() = false;
}

inline oasis::hybrid::M68k9D70StepResult step_m68k_portable_0059a8(
    unsigned pc_in, const oasis::hybrid::M68k9D70StepApi& api, bool enabled) noexcept {
    const std::uint32_t pc = static_cast<std::uint32_t>(pc_in) & kBusAddressMask;
    auto& m = detail::metrics_mut();
    bool& in_port = detail::in_portable_mut();

    if (!enabled) return oasis::hybrid::M68k9D70StepResult::NotHandled;

    if (!is_compiled_entity_dispatch_pc(pc)) {
        if (in_port) {
            in_port = false;
            ++m.uncompiled_continuations;
        }
        return oasis::hybrid::M68k9D70StepResult::NotHandled;
    }

    if (!in_port) {
        in_port = true;
        ++m.portable_runtime_entries;
    }

    HybridM68kStepBus bus{api};
    std::uint32_t next_pc = pc;
    if (!step_one_entity_dispatch_ir(bus, pc, next_pc)) {
        if (bus.faulted()) {
            ++m.original_cpu_exceptions;
            ++m.fallback_transfers;
            in_port = false;
            return oasis::hybrid::M68k9D70StepResult::NotHandled; // Yield pre-mutation so original CPU handles hardware Vector 3
        }
        ++m.contract_faults;
        in_port = false;
        return oasis::hybrid::M68k9D70StepResult::ContractFault;
    }

    ++m.portable_guest_instructions;
    ++m.disjoint_new_native_steps;

    const unsigned op = static_cast<unsigned>(
        ((api.peek(api.context, pc) & 0xFF) << 8) | (api.peek(api.context, (pc + 1U) & kBusAddressMask) & 0xFF));
    if (op == 0x4E75U) ++m.completed_returns;
    if (!is_compiled_entity_dispatch_pc(next_pc)) {
        in_port = false;
        ++m.uncompiled_continuations;
    }
    return oasis::hybrid::M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::recompiler::ir
