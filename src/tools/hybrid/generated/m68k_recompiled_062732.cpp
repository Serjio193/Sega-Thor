#include "tools/hybrid/generated/m68k_recompiled_062732.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {
namespace {

constexpr unsigned kStart = 0x062732U;
bool active = false;
bool interrupted = false;
unsigned resume_pc = 0U;
StepMetrics_062732 metrics{};

} // namespace

M68k9D70StepResult step_m68k_062732(unsigned pc_in, const M68k9D70StepApi& api, bool enabled) noexcept {
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
    case 0x062732U: {
        const std::uint16_t opcode = 0x701FU;
        prepare_step(api, pc, opcode);
        api.write_register(api.context, kRegD0, 0x1FU);
        update_logic_ccr_l(api, 0x1FU);
        complete_step(api, 0x062734U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x062734U: {
        const std::uint16_t opcode = 0x8007U;
        prepare_step(api, pc, opcode);
        const auto val = ((api.read_register(api.context, kRegD0) & 0xFFU)) | ((api.read_register(api.context, kRegD7) & 0xFFU));
        api.write_register(api.context, kRegD0, (api.read_register(api.context, kRegD0) & 0xFFFFFF00U) | (val & 0xFFU));
        update_logic_ccr_b(api, val);
        complete_step(api, 0x062736U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x062736U: {
        const std::uint16_t opcode = 0x1680U;
        prepare_step(api, pc, opcode);
        api.write(api.context, api.read_register(api.context, kRegA3) & kMask24, 1, (api.read_register(api.context, kRegD0) & 0xFFU));
        update_logic_ccr_b(api, (api.read_register(api.context, kRegD0) & 0xFFU));
        complete_step(api, 0x062738U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x062738U: {
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

const StepMetrics_062732& get_metrics_062732() noexcept { return metrics; }
void reset_metrics_062732() noexcept { metrics = {}; active = false; interrupted = false; resume_pc = 0; }

} // namespace oasis::hybrid::generated
