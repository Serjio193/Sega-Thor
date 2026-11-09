#include "tools/hybrid/generated/m68k_recompiled_006e7a.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {
namespace {

constexpr unsigned kStart = 0x006E7AU;
bool active = false;
bool interrupted = false;
unsigned resume_pc = 0U;
StepMetrics_006e7a metrics{};

} // namespace

M68k9D70StepResult step_m68k_006e7a(unsigned pc_in, const M68k9D70StepApi& api, bool enabled) noexcept {
    const unsigned pc = pc_in & kMask24;
    if (pc == kStart) {
        metrics.target_entries++;
        if (!enabled) return M68k9D70StepResult::NotHandled;
        if (active) {
            metrics.active_at_entry++;
            active = false;
            interrupted = false;
            return M68k9D70StepResult::ContractFault;
        }
        active = true;
        interrupted = false;
        metrics.native_calls++;
    }
    if (!active) return M68k9D70StepResult::NotHandled;
    if (active && pc != resume_pc && pc != kStart) {
        interrupted = true;
        return M68k9D70StepResult::NotHandled;
    }
    if (interrupted && pc == resume_pc) {
        interrupted = false;
    }
    switch (pc) {
    case 0x006E7AU: {
        const std::uint16_t opcode = 0x0839U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto val = static_cast<std::uint8_t>(api.read(api.context, 0xFF165FU, 1));
        const bool is_zero = (val & (1U << (4U & 7U))) == 0U;
        update_btst_ccr(api, is_zero);
        complete_step(api, 0x006E82U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x006E82U: {
        const std::uint16_t opcode = 0x6602U;
        prepare_step(api, pc, opcode);
        const bool taken = (get_ccr(api) & 0x04U) == 0U;
        const unsigned target = taken ? 0x006E86U : 0x006E84U;
        const int adj = taken ? 0 : (api.byte_not_taken_adjustment ? api.byte_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x006E84U: {
        const auto res = step_rts(api, pc, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            metrics.completed_calls++;
            active = false;
            interrupted = false;
        }
        return res;
    }
    default:
        active = false;
        interrupted = false;
        return M68k9D70StepResult::NotHandled;
    }
}

const StepMetrics_006e7a& get_metrics_006e7a() noexcept { return metrics; }
void reset_metrics_006e7a() noexcept { metrics = {}; active = false; interrupted = false; resume_pc = 0; }

} // namespace oasis::hybrid::generated
