#include "tools/hybrid/m68k_00ad76_step_ops.hpp"

namespace oasis::hybrid::step_00ad76 {

namespace {
constexpr unsigned kRamStart = 0xFF0000U;
constexpr unsigned kRamEnd = 0x1000000U;
} // namespace

bool read_be(const M68k9D70StepApi& api, unsigned addr, int width, unsigned& out) noexcept {
    out = 0;
    if (addr > 0xFFFFFFU || width <= 0 || width > 4 || addr + static_cast<unsigned>(width) > 0x1000000U)
        return false;
    for (int i = 0; i < width; ++i) {
        const int byte = api.peek(api.context, (addr + static_cast<unsigned>(i)) & kMask24);
        if (byte < 0 || byte > 255) return false;
        out = (out << 8U) | static_cast<unsigned>(byte);
    }
    return true;
}

void write_be(const M68k9D70StepApi& api, unsigned addr, int width, unsigned val) noexcept {
    api.write(api.context, addr & kMask24, width, val);
}

bool in_work_ram(unsigned addr, unsigned size) noexcept {
    return size != 0U && addr >= kRamStart && addr <= kRamEnd - size;
}

std::uint16_t ccr(const M68k9D70StepApi& api) noexcept {
    return static_cast<std::uint16_t>(api.read_register(api.context, 17U));
}

void set_ccr(const M68k9D70StepApi& api, std::uint16_t value) noexcept {
    const auto sr = api.read_register(api.context, 17U);
    api.write_register(api.context, 17U, (sr & 0xFFE0U) | (value & 0x1FU));
}

bool prepare_step(const M68k9D70StepApi& api, unsigned pc, std::uint16_t& opcode) noexcept {
    unsigned raw{};
    if (!read_be(api, pc, 2, raw)) return false;
    opcode = static_cast<std::uint16_t>(raw);
    api.observe_instruction(api.context, pc, opcode);
    api.begin_instruction(api.context, opcode);
    return true;
}

bool complete_step(const M68k9D70StepApi& api, unsigned next_pc, std::uint16_t opcode,
                   int cycle_adjustment, unsigned& resume_pc) noexcept {
    unsigned word{};
    if (!read_be(api, next_pc, 2, word)) return false;
    api.set_return_state(api.context, next_pc, next_pc + 2U, word);
    if (cycle_adjustment != 0) api.add_cycles(api.context, cycle_adjustment);
    api.finish_instruction(api.context, opcode);
    resume_pc = next_pc;
    return true;
}

M68k9D70StepResult step_bcc_w(const M68k9D70StepApi& api, unsigned pc, bool taken,
                              unsigned target_pc, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const unsigned target = taken ? target_pc : next_pc;
    const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 14);
    if (!complete_step(api, target, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_bcc_s(const M68k9D70StepApi& api, unsigned pc, bool taken,
                              unsigned target_pc, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const unsigned target = taken ? target_pc : next_pc;
    const int adj = taken ? 0 : (api.byte_not_taken_adjustment ? api.byte_not_taken_adjustment(api.context) : -14);
    if (!complete_step(api, target, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_bra_w(const M68k9D70StepApi& api, unsigned pc, unsigned target_pc,
                              unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    if (!complete_step(api, target_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_dbf_d5(const M68k9D70StepApi& api, unsigned pc, unsigned loop_pc,
                               unsigned exit_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto orig = api.read_register(api.context, kRegD5);
    auto count = static_cast<std::int16_t>(orig & 0xFFFFU);
    count = static_cast<std::int16_t>(count - 1);
    api.write_register(api.context, kRegD5, (orig & 0xFFFF0000U) | static_cast<std::uint16_t>(count));
    const bool taken = (count != -1);
    const unsigned target = taken ? loop_pc : exit_pc;
    const int adj = taken ? -14 : 14;
    if (!complete_step(api, target, opcode, adj, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_movem_push_d7(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto old_sp = api.read_register(api.context, kRegSP);
    const auto sp = (old_sp - 4U) & kMask24;
    const auto d7 = api.read_register(api.context, kRegD7);
    api.write(api.context, sp, 4, d7);
    api.write_register(api.context, kRegSP, old_sp - 4U);
    api.add_cycles(api.context, 56);
    if (api.skip_bus_refresh) api.skip_bus_refresh(api.context);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_movem_pop_d7(const M68k9D70StepApi& api, unsigned pc,
                                     unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context);
    const auto old_sp = api.read_register(api.context, kRegSP);
    const auto sp = old_sp & kMask24;
    unsigned d7{};
    if (!read_be(api, sp, 4, d7)) return M68k9D70StepResult::ContractFault;
    api.write_register(api.context, kRegD7, d7);
    const auto new_sp = (sp + 4U) & kMask24;
    api.write_register(api.context, kRegSP, old_sp + 4U);
    (void)api.read(api.context, new_sp, 2);
    api.add_cycles(api.context, 56);
    if (api.skip_bus_refresh) api.skip_bus_refresh(api.context);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

M68k9D70StepResult step_rts(const M68k9D70StepApi& api, unsigned pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    const auto raw_sp = api.read_register(api.context, kRegSP);
    const auto sp = raw_sp & kMask24;
    unsigned ret_pc = 0U;
    if (!in_work_ram(sp, 4U) || !read_be(api, sp, 4, ret_pc)) return M68k9D70StepResult::ContractFault;
    api.write_register(api.context, kRegSP, raw_sp + 4U);
    unsigned next_word{};
    if (!read_be(api, ret_pc & kMask24, 2, next_word)) return M68k9D70StepResult::ContractFault;
    api.set_return_state(api.context, ret_pc & kMask24, (ret_pc + 2U) & kMask24, next_word);
    api.finish_instruction(api.context, opcode);
    resume_pc = ret_pc & kMask24;
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::hybrid::step_00ad76
