#include "tools/hybrid/generated/m68k_recompiled_062a78.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {
namespace {

constexpr unsigned kStart = 0x062A78U;
bool active = false;
bool interrupted = false;
unsigned resume_pc = 0U;
StepMetrics_062a78 metrics{};

} // namespace

M68k9D70StepResult step_m68k_062a78(unsigned pc_in, const M68k9D70StepApi& api, bool enabled) noexcept {
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
    case 0x062A78U: {
        const std::uint16_t opcode = 0x4AAEU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const auto val = static_cast<std::uint32_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x1EU) & kMask24, 4));
        update_logic_ccr_l(api, val);
        complete_step(api, 0x062A7CU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x062A7CU: {
        const std::uint16_t opcode = 0x6700U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x04U) != 0U;
        const unsigned target = taken ? 0x062A98U : 0x062A80U;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x062A80U: {
        const std::uint16_t opcode = 0x0807U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const auto val = api.read_register(api.context, kRegD7);
        const bool is_zero = (val & (1U << (31U & 31U))) == 0U;
        update_btst_ccr(api, is_zero);
        complete_step(api, 0x062A84U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x062A84U: {
        const std::uint16_t opcode = 0x6600U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x04U) == 0U;
        const unsigned target = taken ? 0x062A98U : 0x062A88U;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x062A88U: {
        const std::uint16_t opcode = 0x082DU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto val = static_cast<std::uint8_t>(api.read(api.context, (api.read_register(api.context, kRegA5) + 0x0U) & kMask24, 1));
        const bool is_zero = (val & (1U << (2U & 7U))) == 0U;
        update_btst_ccr(api, is_zero);
        complete_step(api, 0x062A8EU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x062A8EU: {
        const std::uint16_t opcode = 0x6600U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x04U) == 0U;
        const unsigned target = taken ? 0x062A98U : 0x062A92U;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x062A92U: {
        const std::uint16_t opcode = 0x206EU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA0, static_cast<std::uint32_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x1EU) & kMask24, 4)));
        complete_step(api, 0x062A96U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x062A96U: {
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

const StepMetrics_062a78& get_metrics_062a78() noexcept { return metrics; }
void reset_metrics_062a78() noexcept { metrics = {}; active = false; interrupted = false; resume_pc = 0; }

} // namespace oasis::hybrid::generated
