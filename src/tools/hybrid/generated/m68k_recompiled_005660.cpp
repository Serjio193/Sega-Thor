#include "tools/hybrid/generated/m68k_recompiled_005660.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {
namespace {

constexpr unsigned kStart = 0x005660U;
bool active = false;
bool interrupted = false;
unsigned resume_pc = 0U;
StepMetrics_005660 metrics{};

} // namespace

M68k9D70StepResult step_m68k_005660(unsigned pc_in, const M68k9D70StepApi& api, bool enabled) noexcept {
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
    case 0x005660U: {
        const std::uint16_t opcode = 0x41F9U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA0, 0x00FF1972U);
        complete_step(api, 0x005666U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x005666U: {
        const std::uint16_t opcode = 0x30EEU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write(api.context, api.read_register(api.context, kRegA0) & kMask24, 2, static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x8U) & kMask24, 2)));
        api.write_register(api.context, kRegA0, api.read_register(api.context, kRegA0) + 2U);
        update_logic_ccr_w(api, static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x8U) & kMask24, 2)));
        complete_step(api, 0x00566AU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00566AU: {
        const std::uint16_t opcode = 0x30AEU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write(api.context, api.read_register(api.context, kRegA0) & kMask24, 2, static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0xCU) & kMask24, 2)));
        update_logic_ccr_w(api, static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0xCU) & kMask24, 2)));
        complete_step(api, 0x00566EU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00566EU: {
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

const StepMetrics_005660& get_metrics_005660() noexcept { return metrics; }
void reset_metrics_005660() noexcept { metrics = {}; active = false; interrupted = false; resume_pc = 0; }

} // namespace oasis::hybrid::generated
