#include "tools/hybrid/m68k_02575c_instruction_step.hpp"
#include "tools/hybrid/m68k_02575c_step_ops.hpp"

#include <array>
#include <fstream>

namespace oasis::hybrid {
namespace {

using step_623ac::read_be;
using step_623ac::in_work_ram;
using step_623ac::prepare_step;
using step_623ac::complete_step;
using step_623ac::branch_to;
using step_623ac::ccr;
using step_623ac::set_ccr;
using namespace step_02575c;

bool active = false;
bool child_call_active = false;
unsigned saved_child_sp = 0U;
unsigned resume_pc = 0U;
M68k02575CStepMetrics metrics{};

constexpr std::array<std::uint16_t, 6> kRomWords = {{
    0x302EU, 0x0004U, 0x41FBU, 0x0006U, 0xD0D0U, 0x4ED0U
}};

bool verify_rom(const M68k9D70StepApi& api) noexcept {
    for (std::size_t i = 0; i < kRomWords.size(); ++i) {
        unsigned word = 0U;
        if (!read_be(api, kStartPc + static_cast<unsigned>(i) * 2U, 2, word) || word != kRomWords[i]) return false;
    }
    return true;
}

M68k9D70StepResult step_state0(const M68k9D70StepApi& api, unsigned pc, unsigned a6) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x025896U: { // subq.w #1, 2(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned old_val = 0U;
        if (!read_be(api, a6 + 2U, 2, old_val)) return M68k9D70StepResult::ContractFault;
        const auto new_val = (old_val - 1U) & 0xFFFFU;
        api.write(api.context, a6 + 2U, 2, new_val);
        const bool v = (old_val == 0x8000U);
        const bool c = (old_val == 0U);
        const bool z = (new_val == 0U);
        const bool n = ((new_val & 0x8000U) != 0U);
        const std::uint16_t old_ccr = ccr(api);
        set_ccr(api, (old_ccr & 0xFFE0U) | (c ? 0x10U : 0U) | (n ? 0x08U : 0U) | (z ? 0x04U : 0U) | (v ? 0x02U : 0U) | (c ? 0x01U : 0U));
        if (!complete_step(api, 0x02589AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state0_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02589AU: { // ble.w $0255D8
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto c = ccr(api);
        const bool z = (c & 0x04U) != 0U;
        const bool n = (c & 0x08U) != 0U;
        const bool v = (c & 0x02U) != 0U;
        if (z || (n != v)) {
            if (!complete_step(api, kCommonJoinPc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            metrics.stepped_instructions++; metrics.state0_steps++; metrics.state0_loopback_exits++;
            active = false;
            return M68k9D70StepResult::InstructionStepped;
        }
        if (!complete_step(api, kState0ExitRts, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
        api.add_cycles(api.context, adj);
        metrics.stepped_instructions++; metrics.state0_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02589EU: { // rts
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        unsigned ret_pc = 0U;
        if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 15U, (sp + 4U) & kMask24);
        if (!branch_to(api, ret_pc & kMask24, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++; metrics.state0_steps++; metrics.state0_rts_exits++; metrics.completed_calls++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    default: return M68k9D70StepResult::ContractFault;
    }
}

M68k9D70StepResult step_target12(const M68k9D70StepApi& api, unsigned pc, unsigned a6) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x025880U: { // btst.b #1, 55(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 55U, 1, val)) return M68k9D70StepResult::ContractFault;
        const bool bit = (val & 0x02U) != 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (!bit ? 0x04U : 0U));
        if (!complete_step(api, 0x025886U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.target12_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025886U: { // bne.w $02588C
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) == 0U) {
            if (!complete_step(api, 0x02588CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, kTarget12ExitRts, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.target12_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02588AU: { // rts
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        unsigned ret_pc = 0U;
        if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 15U, (sp + 4U) & kMask24);
        if (!branch_to(api, ret_pc & kMask24, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++; metrics.target12_steps++; metrics.target12_rts_exits++; metrics.completed_calls++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x02588CU: { // andi.b #$F3, 56(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 56U, 1, val)) return M68k9D70StepResult::ContractFault;
        const auto res = static_cast<std::uint8_t>(val & 0xF3U);
        api.write(api.context, a6 + 56U, 1, res);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (res == 0U) new_ccr |= 0x04U;
        if ((res & 0x80U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x025892U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.target12_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025892U: { // bra.w $0255D8
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if (!complete_step(api, kCommonJoinPc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.target12_steps++; metrics.target12_loopback_exits++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    default: return M68k9D70StepResult::ContractFault;
    }
}

M68k9D70StepResult step_state7(const M68k9D70StepApi& api, unsigned pc, unsigned a6) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x025B22U: { // btst.b #1, 55(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned byte_val = 0U;
        if (!read_be(api, a6 + 55U, 1, byte_val)) return M68k9D70StepResult::ContractFault;
        const bool bit_zero = (byte_val & 0x02U) == 0U;
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & ~0x04U) | (bit_zero ? 0x04U : 0U));
        if (!complete_step(api, 0x025B28U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state7_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025B28U: { // beq.w $025B30
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) != 0U) {
            if (!complete_step(api, kState7ExitRts, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            if (!complete_step(api, 0x025B2CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
            const int adj = api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14;
            api.add_cycles(api.context, adj);
        }
        metrics.stepped_instructions++; metrics.state7_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025B2CU: { // clr.w 0(a6)
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        api.write(api.context, a6, 2, 0U);
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & 0x10U) | 0x04U);
        if (!complete_step(api, kState7ExitRts, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.state7_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025B30U: { // rts
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        unsigned ret_pc = 0U;
        if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, 15U, (sp + 4U) & kMask24);
        if (!branch_to(api, ret_pc & kMask24, resume_pc)) return M68k9D70StepResult::ContractFault;
        api.finish_instruction(api.context, opcode);
        metrics.stepped_instructions++; metrics.state7_steps++; metrics.state7_rts_exits++; metrics.completed_calls++;
        active = false;
        return M68k9D70StepResult::InstructionStepped;
    }
    default: return M68k9D70StepResult::ContractFault;
    }
}

} // namespace

M68k9D70StepResult step_m68k_02575c(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    pc &= kMask24;

    if (child_call_active) {
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        if (pc == resume_pc && sp == saved_child_sp) {
            child_call_active = false;
            metrics.rom_child_return_completions++;
            metrics.native_continuation_entries++;
            if (resume_pc == 0x025822U) metrics.target10_continuation_entries++;
        } else {
            return M68k9D70StepResult::NotHandled;
        }
    }

    if (!active) {
        if (pc != kStartPc) return M68k9D70StepResult::NotHandled;
        metrics.target_entries++;
        if (!enabled) { metrics.rom_calls++; return M68k9D70StepResult::NotHandled; }

        const auto a6 = api.read_register(api.context, 14U) & kMask24;
        if (!in_work_ram(a6, 0x100U)) { metrics.rom_fallback_calls++; return M68k9D70StepResult::NotHandled; }

        unsigned state_word = 0U;
        if (!read_be(api, a6 + 4U, 2, state_word)) { metrics.contract_faults++; return M68k9D70StepResult::ContractFault; }

        // Test States 0..4, 6..7, 10..12 (State 5 blocked by Z80 sound arbitration / cycle drift at frame 785)
        if (state_word != 0x0000U && state_word != 0x0002U && state_word != 0x0004U &&
            state_word != 0x0006U && state_word != 0x0008U &&
            state_word != 0x000CU && state_word != 0x000EU &&
            state_word != 0x0014U && state_word != 0x0016U && state_word != 0x0018U) {
            metrics.unsupported_state_entries++;
            return M68k9D70StepResult::NotHandled;
        }

        if (!verify_rom(api)) { metrics.contract_faults++; return M68k9D70StepResult::ContractFault; }

        active = true;
        resume_pc = kStartPc;
        metrics.native_calls++;
    }

    if (active && pc != resume_pc) return M68k9D70StepResult::NotHandled;

    std::uint16_t opcode = 0U;
    const auto a6 = api.read_register(api.context, 14U) & kMask24;

    switch (pc) {
    case 0x02575CU: { // move.w 4(a6), d0
        if (!prepare_step(api, pc, opcode)) { metrics.contract_faults++; return M68k9D70StepResult::ContractFault; }
        (void)api.fetch16(api.context);
        unsigned state_val = 0U;
        if (!read_be(api, a6 + 4U, 2, state_val)) { metrics.contract_faults++; return M68k9D70StepResult::ContractFault; }
        const auto old_d0 = api.read_register(api.context, 0U);
        api.write_register(api.context, 0U, (old_d0 & 0xFFFF0000U) | (state_val & 0xFFFFU));
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & 0x10U) | ((state_val == 0U) ? 0x04U : 0U));
        if (!complete_step(api, 0x025760U, opcode, 0, resume_pc)) { metrics.contract_faults++; return M68k9D70StepResult::ContractFault; }
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025760U: { // lea 6(pc, d0.w), a0
        if (!prepare_step(api, pc, opcode)) { metrics.contract_faults++; return M68k9D70StepResult::ContractFault; }
        (void)api.fetch16(api.context);
        const auto d0 = static_cast<std::int16_t>(api.read_register(api.context, 0U) & 0xFFFFU);
        const unsigned target_a0 = (0x025768U + static_cast<unsigned>(d0)) & kMask24;
        api.write_register(api.context, 8U, target_a0);
        if (!complete_step(api, 0x025764U, opcode, 0, resume_pc)) { metrics.contract_faults++; return M68k9D70StepResult::ContractFault; }
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025764U: { // adda.w (a0), a0
        if (!prepare_step(api, pc, opcode)) { metrics.contract_faults++; return M68k9D70StepResult::ContractFault; }
        const auto a0 = api.read_register(api.context, 8U) & kMask24;
        unsigned disp_w = 0U;
        if (!read_be(api, a0, 2, disp_w)) { metrics.contract_faults++; return M68k9D70StepResult::ContractFault; }
        const auto disp_s16 = static_cast<std::int16_t>(disp_w);
        const unsigned target_pc = (a0 + static_cast<unsigned>(disp_s16)) & kMask24;
        api.write_register(api.context, 8U, target_pc);
        if (!complete_step(api, 0x025766U, opcode, 0, resume_pc)) { metrics.contract_faults++; return M68k9D70StepResult::ContractFault; }
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x025766U: { // jmp (a0)
        if (!prepare_step(api, pc, opcode)) { metrics.contract_faults++; return M68k9D70StepResult::ContractFault; }
        const auto target_pc = api.read_register(api.context, 8U) & kMask24;
        if (!complete_step(api, target_pc, opcode, 0, resume_pc)) { metrics.contract_faults++; return M68k9D70StepResult::ContractFault; }
        metrics.stepped_instructions++;
        if (target_pc == kState0Entry) metrics.state0_calls++;
        else if (target_pc == kState1Entry) metrics.state1_calls++;
        else if (target_pc == kState2Entry) metrics.state2_calls++;
        else if (target_pc == kState3Entry) metrics.state3_calls++;
        else if (target_pc == kState4Entry) metrics.state4_calls++;
        else if (target_pc == kState5Entry) metrics.state5_calls++;
        else if (target_pc == kState6Entry) metrics.state6_calls++;
        else if (target_pc == kState7Entry) metrics.state7_calls++;
        else if (target_pc == kTarget10Entry) metrics.target10_calls++;
        else if (target_pc == kTarget11Entry) metrics.target11_calls++;
        else if (target_pc == kTarget12Entry) metrics.target12_calls++;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        break;
    }

    M68k9D70StepResult res = M68k9D70StepResult::NotHandled;
    if (pc >= kTarget10Entry && pc < kTarget10End) {
        res = step_target10(api, pc, resume_pc, child_call_active, saved_child_sp, metrics, active);
    } else if (pc >= kTarget11Entry && pc < kTarget11End) {
        res = step_target11(api, pc, resume_pc, metrics, active);
    } else if (pc >= kTarget12Entry && pc < kTarget12End) {
        res = step_target12(api, pc, a6);
    } else if (pc >= kState0Entry && pc < kState0End) {
        res = step_state0(api, pc, a6);
    } else if (pc >= kState1Entry && pc < kState1End) {
        res = step_state1(api, pc, resume_pc, child_call_active, saved_child_sp, metrics, active);
    } else if (pc >= kState2Entry && pc < kState4End) {
        res = step_state2_3_4(api, pc, resume_pc, child_call_active, saved_child_sp, metrics, active);
    } else if (pc >= kState5Entry && pc < kState5End) {
        res = step_state5(api, pc, resume_pc, child_call_active, saved_child_sp, metrics, active);
    } else if (pc >= kState6Entry && pc < kState6End) {
        res = step_state6(api, pc, resume_pc, child_call_active, saved_child_sp, metrics, active);
    } else if (pc >= kState7Entry && pc < kState7End) {
        res = step_state7(api, pc, a6);
    }

    if (res == M68k9D70StepResult::ContractFault) {
        metrics.contract_faults++;
        active = false;
    }
    return res;
}

const M68k02575CStepMetrics& get_02575c_step_metrics() noexcept { return metrics; }
void reset_02575c_step_metrics() noexcept {
    metrics = {};
    active = false;
    child_call_active = false;
    saved_child_sp = 0U;
    resume_pc = 0U;
}

void write_02575c_step_metrics(const std::filesystem::path& path) {
    if (path.empty()) return;
    std::ofstream out(path, std::ios::trunc);
    if (!out) return;
    out << "{\n"
        << "  \"target_entries\": " << metrics.target_entries << ",\n"
        << "  \"rom_calls\": " << metrics.rom_calls << ",\n"
        << "  \"native_calls\": " << metrics.native_calls << ",\n"
        << "  \"unsupported_state_entries\": " << metrics.unsupported_state_entries << ",\n"
        << "  \"rom_fallback_calls\": " << metrics.rom_fallback_calls << ",\n"
        << "  \"completed_calls\": " << metrics.completed_calls << ",\n"
        << "  \"stepped_instructions\": " << metrics.stepped_instructions << ",\n"
        << "  \"state0_calls\": " << metrics.state0_calls << ",\n"
        << "  \"state0_steps\": " << metrics.state0_steps << ",\n"
        << "  \"state0_rts_exits\": " << metrics.state0_rts_exits << ",\n"
        << "  \"state0_loopback_exits\": " << metrics.state0_loopback_exits << ",\n"
        << "  \"state1_calls\": " << metrics.state1_calls << ",\n"
        << "  \"state1_steps\": " << metrics.state1_steps << ",\n"
        << "  \"state1_rts_exits\": " << metrics.state1_rts_exits << ",\n"
        << "  \"state1_loopback_exits\": " << metrics.state1_loopback_exits << ",\n"
        << "  \"state1_child_calls\": " << metrics.state1_child_calls << ",\n"
        << "  \"state2_calls\": " << metrics.state2_calls << ",\n"
        << "  \"state2_steps\": " << metrics.state2_steps << ",\n"
        << "  \"state2_rts_exits\": " << metrics.state2_rts_exits << ",\n"
        << "  \"state2_loopback_exits\": " << metrics.state2_loopback_exits << ",\n"
        << "  \"state3_calls\": " << metrics.state3_calls << ",\n"
        << "  \"state3_steps\": " << metrics.state3_steps << ",\n"
        << "  \"state3_rts_exits\": " << metrics.state3_rts_exits << ",\n"
        << "  \"state3_loopback_exits\": " << metrics.state3_loopback_exits << ",\n"
        << "  \"state3_child_calls\": " << metrics.state3_child_calls << ",\n"
        << "  \"state3_transfer_exits\": " << metrics.state3_transfer_exits << ",\n"
        << "  \"state4_calls\": " << metrics.state4_calls << ",\n"
        << "  \"state4_steps\": " << metrics.state4_steps << ",\n"
        << "  \"state4_rts_exits\": " << metrics.state4_rts_exits << ",\n"
        << "  \"state4_child_calls\": " << metrics.state4_child_calls << ",\n"
        << "  \"state5_calls\": " << metrics.state5_calls << ",\n"
        << "  \"state5_steps\": " << metrics.state5_steps << ",\n"
        << "  \"state5_rts_exits\": " << metrics.state5_rts_exits << ",\n"
        << "  \"state5_loopback_exits\": " << metrics.state5_loopback_exits << ",\n"
        << "  \"state5_child_calls\": " << metrics.state5_child_calls << ",\n"
        << "  \"state6_calls\": " << metrics.state6_calls << ",\n"
        << "  \"state6_steps\": " << metrics.state6_steps << ",\n"
        << "  \"state6_rts_exits\": " << metrics.state6_rts_exits << ",\n"
        << "  \"state6_child_calls\": " << metrics.state6_child_calls << ",\n"
        << "  \"state7_calls\": " << metrics.state7_calls << ",\n"
        << "  \"state7_steps\": " << metrics.state7_steps << ",\n"
        << "  \"state7_rts_exits\": " << metrics.state7_rts_exits << ",\n"
        << "  \"target10_calls\": " << metrics.target10_calls << ",\n"
        << "  \"target10_steps\": " << metrics.target10_steps << ",\n"
        << "  \"target10_rts_exits\": " << metrics.target10_rts_exits << ",\n"
        << "  \"target10_child_calls\": " << metrics.target10_child_calls << ",\n"
        << "  \"target10_continuation_entries\": " << metrics.target10_continuation_entries << ",\n"
        << "  \"target11_calls\": " << metrics.target11_calls << ",\n"
        << "  \"target11_steps\": " << metrics.target11_steps << ",\n"
        << "  \"target11_rts_exits\": " << metrics.target11_rts_exits << ",\n"
        << "  \"target11_transfer_exits\": " << metrics.target11_transfer_exits << ",\n"
        << "  \"target12_calls\": " << metrics.target12_calls << ",\n"
        << "  \"target12_steps\": " << metrics.target12_steps << ",\n"
        << "  \"target12_rts_exits\": " << metrics.target12_rts_exits << ",\n"
        << "  \"target12_loopback_exits\": " << metrics.target12_loopback_exits << ",\n"
        << "  \"rom_child_return_completions\": " << metrics.rom_child_return_completions << ",\n"
        << "  \"native_continuation_entries\": " << metrics.native_continuation_entries << ",\n"
        << "  \"contract_faults\": " << metrics.contract_faults << "\n"
        << "}\n";
}

} // namespace oasis::hybrid
