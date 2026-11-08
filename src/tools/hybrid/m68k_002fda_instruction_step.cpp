#include "tools/hybrid/m68k_002fda_instruction_step.hpp"
#include "tools/hybrid/m68k_002fda_step_ops.hpp"

#include <array>

namespace oasis::hybrid {

namespace {

using namespace step_002fda;

bool active = false;
bool interrupted = false;
unsigned resume_pc = 0U;

constexpr std::array<std::uint16_t, 3> kRomWords = {{
    0x47F9U, 0x00FFU, 0x134CU // lea ($FF134C).L, a3
}};

bool verify_rom(const M68k9D70StepApi& api) noexcept {
    for (std::size_t i = 0; i < kRomWords.size(); ++i) {
        unsigned word = 0U;
        if (!read_be(api, kStartPc + static_cast<unsigned>(i) * 2U, 2, word) ||
            word != kRomWords[i]) {
            return false;
        }
    }
    return true;
}

} // namespace

void reset_002fda_state() noexcept {
    active = false;
    interrupted = false;
    resume_pc = 0U;
}

M68k9D70StepResult step_m68k_002fda(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    if (!enabled) return M68k9D70StepResult::NotHandled;

    auto& metrics = mutable_002fda_step_metrics();

    if (pc == kStartPc && active) {
        active = false;
        interrupted = false;
    }

    if (!active) {
        if (pc != kStartPc) return M68k9D70StepResult::NotHandled;
        if (!verify_rom(api)) return M68k9D70StepResult::NotHandled;

        metrics.target_entries++;
        active = true;
        interrupted = false;
        resume_pc = pc;
        metrics.native_calls++;
    } else if (interrupted) {
        if (pc != resume_pc) return M68k9D70StepResult::NotHandled;
        interrupted = false;
    }

    if (active && pc != resume_pc) {
        interrupted = true;
        return M68k9D70StepResult::NotHandled;
    }

    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x002FDAU: { // lea ($FF134C).L, a3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        api.write_register(api.context, kRegA3, 0x00FF134CU);
        if (!complete_step(api, 0x002FE0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002FE0U: { // adda.w d6, a3
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a3 = api.read_register(api.context, kRegA3);
        const auto d6 = api.read_register(api.context, kRegD6);
        const auto d6_signed = static_cast<std::int32_t>(static_cast<std::int16_t>(d6 & 0xFFFFU));
        api.write_register(api.context, kRegA3, static_cast<std::uint32_t>(static_cast<std::int32_t>(a3) + d6_signed) & kMask24);
        if (!complete_step(api, 0x002FE2U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002FE2U: { // andi.w #7, d5
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d5 = api.read_register(api.context, kRegD5);
        const auto res_w = static_cast<std::uint16_t>((d5 & 0xFFFFU) & 0x0007U);
        api.write_register(api.context, kRegD5, (d5 & 0xFFFF0000U) | res_w);
        set_ccr(api, ccr_logic16(res_w, ccr(api)));
        if (!complete_step(api, 0x002FE6U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002FE6U: { // move.w (a6)+, d4
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto a6 = api.read_register(api.context, kRegA6) & kMask24;
        unsigned word = 0U;
        if (!read_be(api, a6, 2, word)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA6, a6 + 2U);
        const auto d4 = api.read_register(api.context, kRegD4);
        api.write_register(api.context, kRegD4, (d4 & 0xFFFF0000U) | (word & 0xFFFFU));
        set_ccr(api, ccr_logic16(static_cast<std::uint16_t>(word & 0xFFFFU), ccr(api)));
        if (!complete_step(api, 0x002FE8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002FE8U: { // move.w d4, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4);
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto val_w = static_cast<std::uint16_t>(d4 & 0xFFFFU);
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | val_w);
        set_ccr(api, ccr_logic16(val_w, ccr(api)));
        if (!complete_step(api, 0x002FEAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002FEAU: { // andi.w #$0E, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto res_w = static_cast<std::uint16_t>((d0 & 0xFFFFU) & 0x000EU);
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | res_w);
        set_ccr(api, ccr_logic16(res_w, ccr(api)));
        if (!complete_step(api, 0x002FEEU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002FEEU: { // mulu.w d5, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0_w = static_cast<std::uint32_t>(api.read_register(api.context, kRegD0) & 0xFFFFU);
        const auto d5_w = static_cast<std::uint32_t>(api.read_register(api.context, kRegD5) & 0xFFFFU);
        const auto res = d0_w * d5_w;
        api.write_register(api.context, kRegD0, res);
        const auto old_ccr = ccr(api) & 0x10U;
        const auto n = (res & 0x80000000U) ? 0x08U : 0U;
        const auto z = (res == 0U) ? 0x04U : 0U;
        set_ccr(api, static_cast<std::uint16_t>(old_ccr | n | z));
        api.add_cycles(api.context, static_cast<int>(calc_mulu_cycles(static_cast<std::uint16_t>(d5_w))));
        if (!complete_step(api, 0x002FF0U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002FF0U: { // divu.w #7, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto quot = static_cast<std::uint32_t>((d0 / 7U) & 0xFFFFU);
        const auto rem = static_cast<std::uint32_t>((d0 % 7U) & 0xFFFFU);
        const auto res = (rem << 16U) | quot;
        api.write_register(api.context, kRegD0, res);
        const auto old_ccr = ccr(api) & 0x10U;
        const auto n = (quot & 0x8000U) ? 0x08U : 0U;
        const auto z = (quot == 0U) ? 0x04U : 0U;
        set_ccr(api, static_cast<std::uint16_t>(old_ccr | n | z));
        api.add_cycles(api.context, static_cast<int>(calc_divu_cycles(d0, 7U)));
        if (api.skip_bus_refresh) api.skip_bus_refresh(api.context);
        if (!complete_step(api, 0x002FF4U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002FF4U: { // andi.w #$0E, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto res_w = static_cast<std::uint16_t>((d0 & 0xFFFFU) & 0x000EU);
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | res_w);
        set_ccr(api, ccr_logic16(res_w, ccr(api)));
        if (!complete_step(api, 0x002FF8U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002FF8U: { // move.w d0, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto val_w = static_cast<std::uint16_t>(d0 & 0xFFFFU);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | val_w);
        set_ccr(api, ccr_logic16(val_w, ccr(api)));
        if (!complete_step(api, 0x002FFAU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002FFAU: { // move.w d4, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4);
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto val_w = static_cast<std::uint16_t>(d4 & 0xFFFFU);
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | val_w);
        set_ccr(api, ccr_logic16(val_w, ccr(api)));
        if (!complete_step(api, 0x002FFCU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x002FFCU: { // andi.w #$E0, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto res_w = static_cast<std::uint16_t>((d0 & 0xFFFFU) & 0x00E0U);
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | res_w);
        set_ccr(api, ccr_logic16(res_w, ccr(api)));
        if (!complete_step(api, 0x003000U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x003000U: { // mulu.w d5, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0_w = static_cast<std::uint32_t>(api.read_register(api.context, kRegD0) & 0xFFFFU);
        const auto d5_w = static_cast<std::uint32_t>(api.read_register(api.context, kRegD5) & 0xFFFFU);
        const auto res = d0_w * d5_w;
        api.write_register(api.context, kRegD0, res);
        const auto old_ccr = ccr(api) & 0x10U;
        const auto n = (res & 0x80000000U) ? 0x08U : 0U;
        const auto z = (res == 0U) ? 0x04U : 0U;
        set_ccr(api, static_cast<std::uint16_t>(old_ccr | n | z));
        api.add_cycles(api.context, static_cast<int>(calc_mulu_cycles(static_cast<std::uint16_t>(d5_w))));
        if (!complete_step(api, 0x003002U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x003002U: { // divu.w #7, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto quot = static_cast<std::uint32_t>((d0 / 7U) & 0xFFFFU);
        const auto rem = static_cast<std::uint32_t>((d0 % 7U) & 0xFFFFU);
        const auto res = (rem << 16U) | quot;
        api.write_register(api.context, kRegD0, res);
        const auto old_ccr = ccr(api) & 0x10U;
        const auto n = (quot & 0x8000U) ? 0x08U : 0U;
        const auto z = (quot == 0U) ? 0x04U : 0U;
        set_ccr(api, static_cast<std::uint16_t>(old_ccr | n | z));
        api.add_cycles(api.context, static_cast<int>(calc_divu_cycles(d0, 7U)));
        if (api.skip_bus_refresh) api.skip_bus_refresh(api.context);
        if (!complete_step(api, 0x003006U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x003006U: { // andi.w #$E0, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto res_w = static_cast<std::uint16_t>((d0 & 0xFFFFU) & 0x00E0U);
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | res_w);
        set_ccr(api, ccr_logic16(res_w, ccr(api)));
        if (!complete_step(api, 0x00300AU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00300AU: { // or.w d0, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto res_w = static_cast<std::uint16_t>((d1 | d0) & 0xFFFFU);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res_w);
        set_ccr(api, ccr_logic16(res_w, ccr(api)));
        if (!complete_step(api, 0x00300CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00300CU: { // move.w d4, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d4 = api.read_register(api.context, kRegD4);
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto val_w = static_cast<std::uint16_t>(d4 & 0xFFFFU);
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | val_w);
        set_ccr(api, ccr_logic16(val_w, ccr(api)));
        if (!complete_step(api, 0x00300EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00300EU: { // andi.w #$0E00, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto res_w = static_cast<std::uint16_t>((d0 & 0xFFFFU) & 0x0E00U);
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | res_w);
        set_ccr(api, ccr_logic16(res_w, ccr(api)));
        if (!complete_step(api, 0x003012U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x003012U: { // mulu.w d5, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0_w = static_cast<std::uint32_t>(api.read_register(api.context, kRegD0) & 0xFFFFU);
        const auto d5_w = static_cast<std::uint32_t>(api.read_register(api.context, kRegD5) & 0xFFFFU);
        const auto res = d0_w * d5_w;
        api.write_register(api.context, kRegD0, res);
        const auto old_ccr = ccr(api) & 0x10U;
        const auto n = (res & 0x80000000U) ? 0x08U : 0U;
        const auto z = (res == 0U) ? 0x04U : 0U;
        set_ccr(api, static_cast<std::uint16_t>(old_ccr | n | z));
        api.add_cycles(api.context, static_cast<int>(calc_mulu_cycles(static_cast<std::uint16_t>(d5_w))));
        if (!complete_step(api, 0x003014U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x003014U: { // divu.w #7, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto quot = static_cast<std::uint32_t>((d0 / 7U) & 0xFFFFU);
        const auto rem = static_cast<std::uint32_t>((d0 % 7U) & 0xFFFFU);
        const auto res = (rem << 16U) | quot;
        api.write_register(api.context, kRegD0, res);
        const auto old_ccr = ccr(api) & 0x10U;
        const auto n = (quot & 0x8000U) ? 0x08U : 0U;
        const auto z = (quot == 0U) ? 0x04U : 0U;
        set_ccr(api, static_cast<std::uint16_t>(old_ccr | n | z));
        api.add_cycles(api.context, static_cast<int>(calc_divu_cycles(d0, 7U)));
        if (api.skip_bus_refresh) api.skip_bus_refresh(api.context);
        if (!complete_step(api, 0x003018U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x003018U: { // andi.w #$0E00, d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto res_w = static_cast<std::uint16_t>((d0 & 0xFFFFU) & 0x0E00U);
        api.write_register(api.context, kRegD0, (d0 & 0xFFFF0000U) | res_w);
        set_ccr(api, ccr_logic16(res_w, ccr(api)));
        if (!complete_step(api, 0x00301CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00301CU: { // or.w d0, d1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d0 = api.read_register(api.context, kRegD0);
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto res_w = static_cast<std::uint16_t>((d1 | d0) & 0xFFFFU);
        api.write_register(api.context, kRegD1, (d1 & 0xFFFF0000U) | res_w);
        set_ccr(api, ccr_logic16(res_w, ccr(api)));
        if (!complete_step(api, 0x00301EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00301EU: { // move.w d1, (a3)+
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        const auto d1 = api.read_register(api.context, kRegD1);
        const auto a3 = api.read_register(api.context, kRegA3) & kMask24;
        write_be(api, a3, 2, d1 & 0xFFFFU);
        api.write_register(api.context, kRegA3, a3 + 2U);
        set_ccr(api, ccr_logic16(static_cast<std::uint16_t>(d1 & 0xFFFFU), ccr(api)));
        metrics.colors_scaled++;
        if (!complete_step(api, 0x003020U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x003020U: { // dbf d7, $002FE6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto d7 = api.read_register(api.context, kRegD7);
        auto count = static_cast<std::int16_t>(d7 & 0xFFFFU);
        count = static_cast<std::int16_t>(count - 1);
        api.write_register(api.context, kRegD7, (d7 & 0xFFFF0000U) | static_cast<std::uint16_t>(count));
        const bool taken = (count != -1);
        const unsigned target = taken ? 0x002FE6U : 0x003024U;
        const int adj = taken ? -14 : 14;
        if (!complete_step(api, target, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x003024U: { // bset.b #0, ($FF164D).L
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        unsigned byte = 0U;
        if (!read_be(api, 0x00FF164DU, 1, byte)) return M68k9D70StepResult::ContractFault;
        const bool bit0_was_set = (byte & 0x01U) != 0U;
        write_be(api, 0x00FF164DU, 1, byte | 0x01U);
        const auto cur_ccr = ccr(api);
        set_ccr(api, (cur_ccr & ~0x04U) | (!bit0_was_set ? 0x04U : 0U));
        if (!complete_step(api, 0x00302CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00302CU: { // rts
        metrics.stepped_instructions++;
        metrics.completed_calls++;
        active = false;
        return step_rts(api, pc, resume_pc);
    }
    default:
        break;
    }

    return M68k9D70StepResult::NotHandled;
}

} // namespace oasis::hybrid
