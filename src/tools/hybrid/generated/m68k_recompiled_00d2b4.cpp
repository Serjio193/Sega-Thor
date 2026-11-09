#include "tools/hybrid/generated/m68k_recompiled_00d2b4.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {
namespace {

constexpr unsigned kStart = 0x00D2B4U;
bool active = false;
bool interrupted = false;
unsigned resume_pc = 0U;
StepMetrics_00d2b4 metrics{};

} // namespace

M68k9D70StepResult step_m68k_00d2b4(unsigned pc_in, const M68k9D70StepApi& api, bool enabled) noexcept {
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
    case 0x00D2B4U: {
        const std::uint16_t opcode = 0x6100U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const unsigned sp = (api.read_register(api.context, kRegA7) - 4U) & kMask24;
        api.write_register(api.context, kRegA7, sp);
        api.write(api.context, sp, 4, 0x00D2B8U);
        unsigned dummy_resume = 0;
        complete_step(api, 0x00D344U, opcode, 0, dummy_resume);
        resume_pc = 0x00D2B8U;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2B8U: {
        const std::uint16_t opcode = 0x08A8U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto val = static_cast<std::uint8_t>(api.read(api.context, (api.read_register(api.context, kRegA0) + 0x26U) & kMask24, 1));
        const bool is_zero = (val & (1U << (2U & 7U))) == 0U;
        update_btst_ccr(api, is_zero);
        const auto new_val = static_cast<std::uint8_t>(val & ~(1U << (2U & 7U)));
        api.write(api.context, (api.read_register(api.context, kRegA0) + 0x26U) & kMask24, 1, new_val);
        complete_step(api, 0x00D2BEU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2BEU: {
        const std::uint16_t opcode = 0x6700U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x04U) != 0U;
        const unsigned target = taken ? 0x00D342U : 0x00D2C2U;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2C2U: {
        const std::uint16_t opcode = 0x08F9U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto val = static_cast<std::uint8_t>(api.read(api.context, 0xFF16F0U, 1));
        const bool is_zero = (val & (1U << (3U & 7U))) == 0U;
        update_btst_ccr(api, is_zero);
        const auto new_val = static_cast<std::uint8_t>(val | (1U << (3U & 7U)));
        api.write(api.context, 0xFF16F0U, 1, new_val);
        complete_step(api, 0x00D2CAU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2CAU: {
        const std::uint16_t opcode = 0x3028U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegD0, (api.read_register(api.context, kRegD0) & 0xFFFF0000U) | (static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA0) + 0xEU) & kMask24, 2)) & 0xFFFFU));
        update_logic_ccr_w(api, static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA0) + 0xEU) & kMask24, 2)));
        complete_step(api, 0x00D2CEU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2CEU: {
        const std::uint16_t opcode = 0x0839U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto val = static_cast<std::uint8_t>(api.read(api.context, 0xFF16F0U, 1));
        const bool is_zero = (val & (1U << (1U & 7U))) == 0U;
        update_btst_ccr(api, is_zero);
        complete_step(api, 0x00D2D6U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2D6U: {
        const std::uint16_t opcode = 0x6700U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x04U) != 0U;
        const unsigned target = taken ? 0x00D330U : 0x00D2DAU;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2DAU: {
        const std::uint16_t opcode = 0x0839U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto val = static_cast<std::uint8_t>(api.read(api.context, 0xFF16F0U, 1));
        const bool is_zero = (val & (1U << (0U & 7U))) == 0U;
        update_btst_ccr(api, is_zero);
        complete_step(api, 0x00D2E2U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2E2U: {
        const std::uint16_t opcode = 0x6700U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x04U) != 0U;
        const unsigned target = taken ? 0x00D310U : 0x00D2E6U;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2E6U: {
        const std::uint16_t opcode = 0x43F9U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA1, 0x00FFFB5CU);
        complete_step(api, 0x00D2ECU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2ECU: {
        const std::uint16_t opcode = 0xB1FCU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto src = 0xFF1716U;
        const auto dst = api.read_register(api.context, kRegA0);
        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);
        update_cmp_ccr_l(api, dst, src, res);
        complete_step(api, 0x00D2F2U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2F2U: {
        const std::uint16_t opcode = 0x6702U;
        prepare_step(api, pc, opcode);
        const bool taken = (get_ccr(api) & 0x04U) != 0U;
        const unsigned target = taken ? 0x00D2F6U : 0x00D2F4U;
        const int adj = taken ? 0 : (api.byte_not_taken_adjustment ? api.byte_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2F4U: {
        const std::uint16_t opcode = 0x5489U;
        prepare_step(api, pc, opcode);
        const auto src = 0x2U;
        const auto dst = api.read_register(api.context, kRegA1);
        const auto res = static_cast<std::uint32_t>(dst) + static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegA1, (res & 0xFFFFFFFFU));
        complete_step(api, 0x00D2F6U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2F6U: {
        const std::uint16_t opcode = 0x7239U;
        prepare_step(api, pc, opcode);
        api.write_register(api.context, kRegD1, 0x39U);
        update_logic_ccr_l(api, 0x39U);
        complete_step(api, 0x00D2F8U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2F8U: {
        const std::uint16_t opcode = 0x3280U;
        prepare_step(api, pc, opcode);
        api.write(api.context, api.read_register(api.context, kRegA1) & kMask24, 2, (api.read_register(api.context, kRegD0) & 0xFFFFU));
        update_logic_ccr_w(api, (api.read_register(api.context, kRegD0) & 0xFFFFU));
        complete_step(api, 0x00D2FAU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2FAU: {
        const std::uint16_t opcode = 0x5889U;
        prepare_step(api, pc, opcode);
        const auto src = 0x4U;
        const auto dst = api.read_register(api.context, kRegA1);
        const auto res = static_cast<std::uint32_t>(dst) + static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegA1, (res & 0xFFFFFFFFU));
        complete_step(api, 0x00D2FCU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2FCU: {
        const std::uint16_t opcode = 0x3280U;
        prepare_step(api, pc, opcode);
        api.write(api.context, api.read_register(api.context, kRegA1) & kMask24, 2, (api.read_register(api.context, kRegD0) & 0xFFFFU));
        update_logic_ccr_w(api, (api.read_register(api.context, kRegD0) & 0xFFFFU));
        complete_step(api, 0x00D2FEU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D2FEU: {
        const std::uint16_t opcode = 0x5889U;
        prepare_step(api, pc, opcode);
        const auto src = 0x4U;
        const auto dst = api.read_register(api.context, kRegA1);
        const auto res = static_cast<std::uint32_t>(dst) + static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegA1, (res & 0xFFFFFFFFU));
        complete_step(api, 0x00D300U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D300U: {
        const std::uint16_t opcode = 0x3280U;
        prepare_step(api, pc, opcode);
        api.write(api.context, api.read_register(api.context, kRegA1) & kMask24, 2, (api.read_register(api.context, kRegD0) & 0xFFFFU));
        update_logic_ccr_w(api, (api.read_register(api.context, kRegD0) & 0xFFFFU));
        complete_step(api, 0x00D302U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D302U: {
        const std::uint16_t opcode = 0x5889U;
        prepare_step(api, pc, opcode);
        const auto src = 0x4U;
        const auto dst = api.read_register(api.context, kRegA1);
        const auto res = static_cast<std::uint32_t>(dst) + static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegA1, (res & 0xFFFFFFFFU));
        complete_step(api, 0x00D304U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D304U: {
        const std::uint16_t opcode = 0x3280U;
        prepare_step(api, pc, opcode);
        api.write(api.context, api.read_register(api.context, kRegA1) & kMask24, 2, (api.read_register(api.context, kRegD0) & 0xFFFFU));
        update_logic_ccr_w(api, (api.read_register(api.context, kRegD0) & 0xFFFFU));
        complete_step(api, 0x00D306U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D306U: {
        const std::uint16_t opcode = 0x5889U;
        prepare_step(api, pc, opcode);
        const auto src = 0x4U;
        const auto dst = api.read_register(api.context, kRegA1);
        const auto res = static_cast<std::uint32_t>(dst) + static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegA1, (res & 0xFFFFFFFFU));
        complete_step(api, 0x00D308U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D308U: {
        const std::uint16_t opcode = 0x51C9U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const auto old_d = api.read_register(api.context, kRegD1);
        const auto count = static_cast<std::int16_t>(old_d & 0xFFFFU) - 1;
        api.write_register(api.context, kRegD1, (old_d & 0xFFFF0000U) | static_cast<std::uint16_t>(count));
        const bool taken = (count != -1);
        const unsigned target = taken ? 0x00D2F8U : 0x00D30CU;
        const int adj = taken ? 0 : 4;
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D30CU: {
        const std::uint16_t opcode = 0x6000U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        complete_step(api, 0x00D342U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D310U: {
        const std::uint16_t opcode = 0x43F9U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA1, 0x00FFFB5CU);
        complete_step(api, 0x00D316U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D316U: {
        const std::uint16_t opcode = 0xB1FCU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto src = 0xFF1716U;
        const auto dst = api.read_register(api.context, kRegA0);
        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);
        update_cmp_ccr_l(api, dst, src, res);
        complete_step(api, 0x00D31CU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D31CU: {
        const std::uint16_t opcode = 0x6702U;
        prepare_step(api, pc, opcode);
        const bool taken = (get_ccr(api) & 0x04U) != 0U;
        const unsigned target = taken ? 0x00D320U : 0x00D31EU;
        const int adj = taken ? 0 : (api.byte_not_taken_adjustment ? api.byte_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D31EU: {
        const std::uint16_t opcode = 0x5489U;
        prepare_step(api, pc, opcode);
        const auto src = 0x2U;
        const auto dst = api.read_register(api.context, kRegA1);
        const auto res = static_cast<std::uint32_t>(dst) + static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegA1, (res & 0xFFFFFFFFU));
        complete_step(api, 0x00D320U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D320U: {
        const std::uint16_t opcode = 0x721CU;
        prepare_step(api, pc, opcode);
        api.write_register(api.context, kRegD1, 0x1CU);
        update_logic_ccr_l(api, 0x1CU);
        complete_step(api, 0x00D322U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D322U: {
        const std::uint16_t opcode = 0x3280U;
        prepare_step(api, pc, opcode);
        api.write(api.context, api.read_register(api.context, kRegA1) & kMask24, 2, (api.read_register(api.context, kRegD0) & 0xFFFFU));
        update_logic_ccr_w(api, (api.read_register(api.context, kRegD0) & 0xFFFFU));
        complete_step(api, 0x00D324U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D324U: {
        const std::uint16_t opcode = 0x43E9U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA1, (api.read_register(api.context, kRegA1) + 0x20U) & kMask24);
        complete_step(api, 0x00D328U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D328U: {
        const std::uint16_t opcode = 0x51C9U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const auto old_d = api.read_register(api.context, kRegD1);
        const auto count = static_cast<std::int16_t>(old_d & 0xFFFFU) - 1;
        api.write_register(api.context, kRegD1, (old_d & 0xFFFF0000U) | static_cast<std::uint16_t>(count));
        const bool taken = (count != -1);
        const unsigned target = taken ? 0x00D322U : 0x00D32CU;
        const int adj = taken ? 0 : 4;
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D32CU: {
        const std::uint16_t opcode = 0x6000U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        complete_step(api, 0x00D342U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D330U: {
        const std::uint16_t opcode = 0x43F9U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA1, 0x00FF1344U);
        complete_step(api, 0x00D336U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D336U: {
        const std::uint16_t opcode = 0xB1FCU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto src = 0xFF1716U;
        const auto dst = api.read_register(api.context, kRegA0);
        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);
        update_cmp_ccr_l(api, dst, src, res);
        complete_step(api, 0x00D33CU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D33CU: {
        const std::uint16_t opcode = 0x6702U;
        prepare_step(api, pc, opcode);
        const bool taken = (get_ccr(api) & 0x04U) != 0U;
        const unsigned target = taken ? 0x00D340U : 0x00D33EU;
        const int adj = taken ? 0 : (api.byte_not_taken_adjustment ? api.byte_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D33EU: {
        const std::uint16_t opcode = 0x5489U;
        prepare_step(api, pc, opcode);
        const auto src = 0x2U;
        const auto dst = api.read_register(api.context, kRegA1);
        const auto res = static_cast<std::uint32_t>(dst) + static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegA1, (res & 0xFFFFFFFFU));
        complete_step(api, 0x00D340U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D340U: {
        const std::uint16_t opcode = 0x3280U;
        prepare_step(api, pc, opcode);
        api.write(api.context, api.read_register(api.context, kRegA1) & kMask24, 2, (api.read_register(api.context, kRegD0) & 0xFFFFU));
        update_logic_ccr_w(api, (api.read_register(api.context, kRegD0) & 0xFFFFU));
        complete_step(api, 0x00D342U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00D342U: {
        const auto res = step_rts(api, pc, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            metrics.completed_calls++;
            active = false;
            interrupted = false;
        }
        return res;
    }
    case 0x00D344U: {
        const std::uint16_t opcode = 0x08A8U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto val = static_cast<std::uint8_t>(api.read(api.context, (api.read_register(api.context, kRegA0) + 0x26U) & kMask24, 1));
        const bool is_zero = (val & (1U << (3U & 7U))) == 0U;
        update_btst_ccr(api, is_zero);
        const auto new_val = static_cast<std::uint8_t>(val & ~(1U << (3U & 7U)));
        api.write(api.context, (api.read_register(api.context, kRegA0) + 0x26U) & kMask24, 1, new_val);
        complete_step(api, 0x00D34AU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        active = false;
        interrupted = false;
        return M68k9D70StepResult::NotHandled;
    }
}

const StepMetrics_00d2b4& get_metrics_00d2b4() noexcept { return metrics; }
void reset_metrics_00d2b4() noexcept { metrics = {}; active = false; interrupted = false; resume_pc = 0; }

} // namespace oasis::hybrid::generated
