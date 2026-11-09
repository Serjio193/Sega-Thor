#include "tools/hybrid/generated/m68k_recompiled_0059a8.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {
namespace {

constexpr unsigned kStart = 0x0059A8U;
bool active = false;
bool interrupted = false;
unsigned resume_pc = 0U;
StepMetrics_0059a8 metrics{};

} // namespace

M68k9D70StepResult step_m68k_0059a8(unsigned pc_in, const M68k9D70StepApi& api, bool enabled) noexcept {
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
    case 0x0059A8U: {
        const std::uint16_t opcode = 0x3E39U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegD7, (api.read_register(api.context, kRegD7) & 0xFFFF0000U) | (static_cast<std::uint16_t>(api.read(api.context, 0xFF165EU, 2)) & 0xFFFFU));
        update_logic_ccr_w(api, static_cast<std::uint16_t>(api.read(api.context, 0xFF165EU, 2)));
        complete_step(api, 0x0059AEU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0059AEU: {
        const std::uint16_t opcode = 0x302EU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegD0, (api.read_register(api.context, kRegD0) & 0xFFFF0000U) | (static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x4U) & kMask24, 2)) & 0xFFFFU));
        update_logic_ccr_w(api, static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x4U) & kMask24, 2)));
        complete_step(api, 0x0059B2U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0059B2U: {
        const std::uint16_t opcode = 0x41FBU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const auto idx = static_cast<std::int16_t>(api.read_register(api.context, kRegD0) & 0xFFFFU);
        api.write_register(api.context, kRegA0, (0x0059BAU + static_cast<std::uint32_t>(static_cast<std::int32_t>(idx))) & kMask24);
        complete_step(api, 0x0059B6U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0059B6U: {
        const std::uint16_t opcode = 0xD0D0U;
        prepare_step(api, pc, opcode);
        const auto src = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(static_cast<std::uint16_t>(api.read(api.context, api.read_register(api.context, kRegA0) & kMask24, 2)))));
        const auto dst = api.read_register(api.context, kRegA0);
        const auto res = (dst + src) & kMask24;
        api.write_register(api.context, kRegA0, res);
        complete_step(api, 0x0059B8U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0059B8U: {
        const std::uint16_t opcode = 0x4ED0U;
        prepare_step(api, pc, opcode);
        const auto target = api.read_register(api.context, kRegA0) & kMask24;
        if ((target & 1U) != 0U || target < 0x000200U || target >= 0x300000U) {
            active = false; interrupted = false; return M68k9D70StepResult::ContractFault;
        }
        complete_step(api, target, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        metrics.completed_calls++;
        active = false;
        interrupted = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        active = false;
        interrupted = false;
        return M68k9D70StepResult::NotHandled;
    }
}

const StepMetrics_0059a8& get_metrics_0059a8() noexcept { return metrics; }
void reset_metrics_0059a8() noexcept { metrics = {}; active = false; interrupted = false; resume_pc = 0; }

} // namespace oasis::hybrid::generated
