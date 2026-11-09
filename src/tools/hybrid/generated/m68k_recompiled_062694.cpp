#include "tools/hybrid/generated/m68k_recompiled_062694.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {
namespace {

constexpr unsigned kStart = 0x062694U;
bool active = false;
bool interrupted = false;
unsigned resume_pc = 0U;
StepMetrics_062694 metrics{};

} // namespace

M68k9D70StepResult step_m68k_062694(unsigned pc_in, const M68k9D70StepApi& api, bool enabled) noexcept {
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
    case 0x062694U: {
        const std::uint16_t opcode = 0x122EU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegD1, (api.read_register(api.context, kRegD1) & 0xFFFFFF00U) | (static_cast<std::uint8_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x17U) & kMask24, 1)) & 0xFFU));
        update_logic_ccr_b(api, static_cast<std::uint8_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x17U) & kMask24, 1)));
        complete_step(api, 0x062698U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x062698U: {
        const std::uint16_t opcode = 0x0201U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const auto val = ((api.read_register(api.context, kRegD1) & 0xFFU)) & (0x3U);
        api.write_register(api.context, kRegD1, (api.read_register(api.context, kRegD1) & 0xFFFFFF00U) | (val & 0xFFU));
        update_logic_ccr_b(api, val);
        complete_step(api, 0x06269CU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x06269CU: {
        const std::uint16_t opcode = 0x0C01U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const auto src = 0x3U;
        const auto dst = (api.read_register(api.context, kRegD1) & 0xFFU);
        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);
        update_cmp_ccr_b(api, dst, src, res);
        complete_step(api, 0x0626A0U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0626A0U: {
        const std::uint16_t opcode = 0x6700U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x04U) != 0U;
        const unsigned target = taken ? 0x0626A6U : 0x0626A4U;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0626A4U: {
        const auto res = step_rts(api, pc, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            metrics.completed_calls++;
            active = false;
            interrupted = false;
        }
        return res;
    }
    case 0x0626A6U: {
        const std::uint16_t opcode = 0x3200U;
        prepare_step(api, pc, opcode);
        api.write_register(api.context, kRegD1, (api.read_register(api.context, kRegD1) & 0xFFFF0000U) | ((api.read_register(api.context, kRegD0) & 0xFFFFU) & 0xFFFFU));
        update_logic_ccr_w(api, (api.read_register(api.context, kRegD0) & 0xFFFFU));
        complete_step(api, 0x0626A8U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0626A8U: {
        const std::uint16_t opcode = 0x0200U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const auto val = ((api.read_register(api.context, kRegD0) & 0xFFU)) & (0xFU);
        api.write_register(api.context, kRegD0, (api.read_register(api.context, kRegD0) & 0xFFFFFF00U) | (val & 0xFFU));
        update_logic_ccr_b(api, val);
        complete_step(api, 0x0626ACU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0626ACU: {
        const std::uint16_t opcode = 0x802DU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const auto val = ((api.read_register(api.context, kRegD0) & 0xFFU)) | (static_cast<std::uint8_t>(api.read(api.context, (api.read_register(api.context, kRegA5) + 0x4U) & kMask24, 1)));
        api.write_register(api.context, kRegD0, (api.read_register(api.context, kRegD0) & 0xFFFFFF00U) | (val & 0xFFU));
        update_logic_ccr_b(api, val);
        complete_step(api, 0x0626B0U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0626B0U: {
        const std::uint16_t opcode = 0x1680U;
        prepare_step(api, pc, opcode);
        api.write(api.context, api.read_register(api.context, kRegA3) & kMask24, 1, (api.read_register(api.context, kRegD0) & 0xFFU));
        update_logic_ccr_b(api, (api.read_register(api.context, kRegD0) & 0xFFU));
        complete_step(api, 0x0626B2U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0626B2U: {
        const std::uint16_t opcode = 0xE849U;
        prepare_step(api, pc, opcode);
        const auto old_d = (api.read_register(api.context, kRegD1) & 0xFFFFU);
        const auto shift = 4U;
        const auto val = static_cast<std::uint16_t>((old_d >> shift) & 0xFFFFU);
        api.write_register(api.context, kRegD1, (api.read_register(api.context, kRegD1) & 0xFFFF0000U) | (val & 0xFFFFU));
        update_shift_ccr_lsr_w(api, old_d, shift, val);
        complete_step(api, 0x0626B4U, opcode, 56, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0626B4U: {
        const std::uint16_t opcode = 0x0201U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const auto val = ((api.read_register(api.context, kRegD1) & 0xFFU)) & (0x3FU);
        api.write_register(api.context, kRegD1, (api.read_register(api.context, kRegD1) & 0xFFFFFF00U) | (val & 0xFFU));
        update_logic_ccr_b(api, val);
        complete_step(api, 0x0626B8U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0626B8U: {
        const std::uint16_t opcode = 0x1681U;
        prepare_step(api, pc, opcode);
        api.write(api.context, api.read_register(api.context, kRegA3) & kMask24, 1, (api.read_register(api.context, kRegD1) & 0xFFU));
        update_logic_ccr_b(api, (api.read_register(api.context, kRegD1) & 0xFFU));
        complete_step(api, 0x0626BAU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0626BAU: {
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

const StepMetrics_062694& get_metrics_062694() noexcept { return metrics; }
void reset_metrics_062694() noexcept { metrics = {}; active = false; interrupted = false; resume_pc = 0; }

} // namespace oasis::hybrid::generated
