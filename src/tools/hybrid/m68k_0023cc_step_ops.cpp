#include "tools/hybrid/m68k_0023cc_step_ops.hpp"

namespace oasis::hybrid::step_0023cc {

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

void update_logic_ccr16(const M68k9D70StepApi& api, std::uint16_t val) noexcept {
    const std::uint16_t old = ccr(api) & 0x10U; // keep X
    const std::uint16_t n = (val & 0x8000U) ? 0x08U : 0x00U;
    const std::uint16_t z = (val == 0U) ? 0x04U : 0x00U;
    set_ccr(api, old | n | z);
}

void update_add_ccr16(const M68k9D70StepApi& api, std::uint32_t a, std::uint32_t b, std::uint32_t res) noexcept {
    const bool sm = (a & 0x8000U) != 0U;
    const bool dm = (b & 0x8000U) != 0U;
    const bool rm = (res & 0x8000U) != 0U;
    const std::uint16_t x = ((res & 0x10000U) != 0U) ? 0x10U : 0x00U;
    const std::uint16_t n = rm ? 0x08U : 0x00U;
    const std::uint16_t z = ((res & 0xFFFFU) == 0U) ? 0x04U : 0x00U;
    const std::uint16_t v = ((sm && dm && !rm) || (!sm && !dm && rm)) ? 0x02U : 0x00U;
    const std::uint16_t c = ((res & 0x10000U) != 0U) ? 0x01U : 0x00U;
    set_ccr(api, x | n | z | v | c);
}

void update_sub_ccr16(const M68k9D70StepApi& api, std::uint32_t src, std::uint32_t dst, std::uint32_t res) noexcept {
    const bool sm = (src & 0x8000U) != 0U;
    const bool dm = (dst & 0x8000U) != 0U;
    const bool rm = (res & 0x8000U) != 0U;
    const std::uint16_t x = ((res & 0x10000U) != 0U) ? 0x10U : 0x00U;
    const std::uint16_t n = rm ? 0x08U : 0x00U;
    const std::uint16_t z = ((res & 0xFFFFU) == 0U) ? 0x04U : 0x00U;
    const std::uint16_t v = ((!sm && dm && !rm) || (sm && !dm && rm)) ? 0x02U : 0x00U;
    const std::uint16_t c = ((res & 0x10000U) != 0U) ? 0x01U : 0x00U;
    set_ccr(api, x | n | z | v | c);
}

void update_cmp_ccr16(const M68k9D70StepApi& api, std::uint32_t src, std::uint32_t dst, std::uint32_t res) noexcept {
    const bool sm = (src & 0x8000U) != 0U;
    const bool dm = (dst & 0x8000U) != 0U;
    const bool rm = (res & 0x8000U) != 0U;
    const std::uint16_t x = ccr(api) & 0x10U; // preserved
    const std::uint16_t n = rm ? 0x08U : 0x00U;
    const std::uint16_t z = ((res & 0xFFFFU) == 0U) ? 0x04U : 0x00U;
    const std::uint16_t v = ((!sm && dm && !rm) || (sm && !dm && rm)) ? 0x02U : 0x00U;
    const std::uint16_t c = ((res & 0x10000U) != 0U) ? 0x01U : 0x00U;
    set_ccr(api, x | n | z | v | c);
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

} // namespace oasis::hybrid::step_0023cc
