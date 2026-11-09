#include "tools/hybrid/generated/m68k_recompiled_00d266.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {
namespace {

constexpr unsigned kStart = 0x00D266U;
bool active = false;
bool interrupted = false;
unsigned resume_pc = 0U;
StepMetrics_00d266 metrics{};

} // namespace

M68k9D70StepResult step_m68k_00d266(unsigned pc_in, const M68k9D70StepApi& api, bool enabled) noexcept {
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
    case 0x00D266U: {
        const std::uint16_t opcode = 0x3C28U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegD6, (api.read_register(api.context, kRegD6) & 0xFFFF0000U) | (static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA0) + 0x20U) & kMask24, 2)) & 0xFFFFU));
        update_logic_ccr_w(api, static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA0) + 0x20U) & kMask24, 2)));
        complete_step(api, 0x00D26AU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D26AU: {
        const std::uint16_t opcode = 0xE74EU;
        prepare_step(api, pc, opcode);
        const auto old_d = (api.read_register(api.context, kRegD6) & 0xFFFFU);
        const auto shift = 3U;
        const auto val = static_cast<std::uint16_t>((old_d << shift) & 0xFFFFU);
        api.write_register(api.context, kRegD6, (api.read_register(api.context, kRegD6) & 0xFFFF0000U) | (val & 0xFFFFU));
        update_shift_ccr_lsl_w(api, old_d, shift, val);
        complete_step(api, 0x00D26CU, opcode, 42, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D26CU: {
        const std::uint16_t opcode = 0x3A28U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegD5, (api.read_register(api.context, kRegD5) & 0xFFFF0000U) | (static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA0) + 0x0U) & kMask24, 2)) & 0xFFFFU));
        update_logic_ccr_w(api, static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA0) + 0x0U) & kMask24, 2)));
        complete_step(api, 0x00D270U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D270U: {
        const std::uint16_t opcode = 0x4A45U;
        prepare_step(api, pc, opcode);
        const auto val = (api.read_register(api.context, kRegD5) & 0xFFFFU);
        update_logic_ccr_w(api, val);
        complete_step(api, 0x00D272U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D272U: {
        const std::uint16_t opcode = 0x6A00U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x08U) == 0U;
        const unsigned target = taken ? 0x00D27CU : 0x00D276U;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D276U: {
        const std::uint16_t opcode = 0xDA46U;
        prepare_step(api, pc, opcode);
        const auto src = (api.read_register(api.context, kRegD6) & 0xFFFFU);
        const auto dst = (api.read_register(api.context, kRegD5) & 0xFFFFU);
        const auto res = static_cast<std::uint32_t>(dst) + static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegD5, (api.read_register(api.context, kRegD5) & 0xFFFF0000U) | ((res & 0xFFFFU) & 0xFFFFU));
        update_add_ccr_w(api, dst, src, res);
        complete_step(api, 0x00D278U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D278U: {
        const std::uint16_t opcode = 0x6B00U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x08U) != 0U;
        const unsigned target = taken ? 0x00D276U : 0x00D27CU;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D27CU: {
        const std::uint16_t opcode = 0xBA46U;
        prepare_step(api, pc, opcode);
        const auto src = (api.read_register(api.context, kRegD6) & 0xFFFFU);
        const auto dst = (api.read_register(api.context, kRegD5) & 0xFFFFU);
        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);
        update_cmp_ccr_w(api, dst, src, res);
        complete_step(api, 0x00D27EU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D27EU: {
        const std::uint16_t opcode = 0x6500U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x01U) != 0U;
        const unsigned target = taken ? 0x00D288U : 0x00D282U;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D282U: {
        const std::uint16_t opcode = 0x9A46U;
        prepare_step(api, pc, opcode);
        const auto src = (api.read_register(api.context, kRegD6) & 0xFFFFU);
        const auto dst = (api.read_register(api.context, kRegD5) & 0xFFFFU);
        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegD5, (api.read_register(api.context, kRegD5) & 0xFFFF0000U) | ((res & 0xFFFFU) & 0xFFFFU));
        update_sub_ccr_w(api, dst, src, res);
        complete_step(api, 0x00D284U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D284U: {
        const std::uint16_t opcode = 0x6000U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        complete_step(api, 0x00D27CU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D288U: {
        const std::uint16_t opcode = 0x3145U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write(api.context, (api.read_register(api.context, kRegA0) + 0x0U) & kMask24, 2, (api.read_register(api.context, kRegD5) & 0xFFFFU));
        update_logic_ccr_w(api, (api.read_register(api.context, kRegD5) & 0xFFFFU));
        complete_step(api, 0x00D28CU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D28CU: {
        const std::uint16_t opcode = 0x3C28U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegD6, (api.read_register(api.context, kRegD6) & 0xFFFF0000U) | (static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA0) + 0x22U) & kMask24, 2)) & 0xFFFFU));
        update_logic_ccr_w(api, static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA0) + 0x22U) & kMask24, 2)));
        complete_step(api, 0x00D290U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D290U: {
        const std::uint16_t opcode = 0xE74EU;
        prepare_step(api, pc, opcode);
        const auto old_d = (api.read_register(api.context, kRegD6) & 0xFFFFU);
        const auto shift = 3U;
        const auto val = static_cast<std::uint16_t>((old_d << shift) & 0xFFFFU);
        api.write_register(api.context, kRegD6, (api.read_register(api.context, kRegD6) & 0xFFFF0000U) | (val & 0xFFFFU));
        update_shift_ccr_lsl_w(api, old_d, shift, val);
        complete_step(api, 0x00D292U, opcode, 42, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D292U: {
        const std::uint16_t opcode = 0x3A28U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegD5, (api.read_register(api.context, kRegD5) & 0xFFFF0000U) | (static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA0) + 0x4U) & kMask24, 2)) & 0xFFFFU));
        update_logic_ccr_w(api, static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA0) + 0x4U) & kMask24, 2)));
        complete_step(api, 0x00D296U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D296U: {
        const std::uint16_t opcode = 0x4A45U;
        prepare_step(api, pc, opcode);
        const auto val = (api.read_register(api.context, kRegD5) & 0xFFFFU);
        update_logic_ccr_w(api, val);
        complete_step(api, 0x00D298U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D298U: {
        const std::uint16_t opcode = 0x6A00U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x08U) == 0U;
        const unsigned target = taken ? 0x00D2A2U : 0x00D29CU;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D29CU: {
        const std::uint16_t opcode = 0xDA46U;
        prepare_step(api, pc, opcode);
        const auto src = (api.read_register(api.context, kRegD6) & 0xFFFFU);
        const auto dst = (api.read_register(api.context, kRegD5) & 0xFFFFU);
        const auto res = static_cast<std::uint32_t>(dst) + static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegD5, (api.read_register(api.context, kRegD5) & 0xFFFF0000U) | ((res & 0xFFFFU) & 0xFFFFU));
        update_add_ccr_w(api, dst, src, res);
        complete_step(api, 0x00D29EU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D29EU: {
        const std::uint16_t opcode = 0x6B00U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x08U) != 0U;
        const unsigned target = taken ? 0x00D29CU : 0x00D2A2U;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2A2U: {
        const std::uint16_t opcode = 0xBA46U;
        prepare_step(api, pc, opcode);
        const auto src = (api.read_register(api.context, kRegD6) & 0xFFFFU);
        const auto dst = (api.read_register(api.context, kRegD5) & 0xFFFFU);
        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);
        update_cmp_ccr_w(api, dst, src, res);
        complete_step(api, 0x00D2A4U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2A4U: {
        const std::uint16_t opcode = 0x6500U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x01U) != 0U;
        const unsigned target = taken ? 0x00D2AEU : 0x00D2A8U;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2A8U: {
        const std::uint16_t opcode = 0x9A46U;
        prepare_step(api, pc, opcode);
        const auto src = (api.read_register(api.context, kRegD6) & 0xFFFFU);
        const auto dst = (api.read_register(api.context, kRegD5) & 0xFFFFU);
        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegD5, (api.read_register(api.context, kRegD5) & 0xFFFF0000U) | ((res & 0xFFFFU) & 0xFFFFU));
        update_sub_ccr_w(api, dst, src, res);
        complete_step(api, 0x00D2AAU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2AAU: {
        const std::uint16_t opcode = 0x6000U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        complete_step(api, 0x00D2A2U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2AEU: {
        const std::uint16_t opcode = 0x3145U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write(api.context, (api.read_register(api.context, kRegA0) + 0x4U) & kMask24, 2, (api.read_register(api.context, kRegD5) & 0xFFFFU));
        update_logic_ccr_w(api, (api.read_register(api.context, kRegD5) & 0xFFFFU));
        complete_step(api, 0x00D2B2U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2B2U: {
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

const StepMetrics_00d266& get_metrics_00d266() noexcept { return metrics; }
void reset_metrics_00d266() noexcept { metrics = {}; active = false; interrupted = false; resume_pc = 0; }

} // namespace oasis::hybrid::generated
