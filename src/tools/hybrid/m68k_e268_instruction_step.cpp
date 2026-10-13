#include "tools/hybrid/m68k_e268_instruction_step.hpp"

#include <array>
#include <fstream>

namespace oasis::hybrid {
namespace {
constexpr unsigned kStart = 0x00E268U;
constexpr unsigned kRts = 0x00E2A0U;
constexpr unsigned kEnd = 0x00E2A2U;
constexpr unsigned kCaller = 0x00359AU;
constexpr unsigned kReturn = 0x0035A0U;
constexpr unsigned kState = 0x00FF1928U;
constexpr unsigned kRamStart = 0x00FF0000U;
constexpr unsigned kRamEnd = 0x01000000U;
constexpr unsigned kMask24 = 0x00FFFFFFU;
constexpr unsigned kX = 0x10U, kN = 0x08U, kZ = 0x04U, kV = 0x02U, kC = 0x01U;
constexpr std::array<std::uint16_t, 29> kRomWords{
    0x3F01U, 0x3039U, 0x00FFU, 0x1928U, 0x3200U, 0xE559U, 0xB141U,
    0xE249U, 0xD140U, 0x3200U, 0xE559U, 0xB141U, 0xE249U, 0xD140U,
    0x3200U, 0xE559U, 0xB141U, 0xE249U, 0xD140U, 0x3200U, 0xE559U,
    0xB141U, 0xE249U, 0xD140U, 0x33C0U, 0x00FFU, 0x1928U, 0x321FU,
    0x4E75U};
bool active{}, interrupted{}, pending_scheduler_resume{}, call_saw_refresh{};
unsigned resume_pc{};
E268StepMetrics metrics{};
unsigned last_contract_fault{};
unsigned last_fault_pc{};
unsigned last_expected_pc{};

bool read_be(const M68k9D70StepApi& api, unsigned address, unsigned width,
             unsigned& value) noexcept {
    value = 0;
    if (address > kMask24 || width == 0U || width > 4U ||
        address + width > 0x01000000U) return false;
    for (unsigned i = 0; i < width; ++i) {
        const int byte = api.peek(api.context, address + i);
        if (byte < 0 || byte > 255) return false;
        value = (value << 8U) | static_cast<unsigned>(byte);
    }
    return true;
}

bool in_work_ram(unsigned address, unsigned size) noexcept {
    return size != 0U && address >= kRamStart && address <= kRamEnd - size;
}

bool caller_instruction_matches(const M68k9D70StepApi& api) noexcept {
    unsigned opcode{}, destination{};
    return read_be(api, kCaller, 2U, opcode) && opcode == 0x4EB9U &&
        read_be(api, kCaller + 2U, 4U, destination) && destination == kStart;
}

bool entry_stack_matches(const M68k9D70StepApi& api) noexcept {
    const unsigned sp = api.read_register(api.context, 15U) & kMask24;
    unsigned return_address{};
    return in_work_ram(sp, 4U) && read_be(api, sp, 4U, return_address) &&
        (return_address & kMask24) == kReturn;
}

bool validate_rom(const M68k9D70StepApi& api) noexcept {
    for (std::size_t i = 0; i < kRomWords.size(); ++i) {
        unsigned word{};
        if (!read_be(api, kStart + static_cast<unsigned>(i) * 2U, 2U, word) ||
            word != kRomWords[i]) return false;
    }
    return true;
}

bool interrupt_frame_for_resume(const M68k9D70StepApi& api) noexcept {
    const unsigned sp = api.read_register(api.context, 15U) & kMask24;
    unsigned saved_sr{}, saved_pc{};
    if (!in_work_ram(sp, 6U) || !read_be(api, sp, 2U, saved_sr) ||
        !read_be(api, sp + 2U, 4U, saved_pc)) return false;
    const unsigned sr = api.read_register(api.context, 17U) & 0xFFFFU;
    return (saved_pc & kMask24) == resume_pc && (saved_sr & 0x2000U) != 0U &&
        (sr & 0x2000U) != 0U && (sr & 0x0700U) > (saved_sr & 0x0700U);
}

std::uint16_t flags(const M68k9D70StepApi& api) noexcept {
    return static_cast<std::uint16_t>(api.read_register(api.context, 17U));
}

void set_flags(const M68k9D70StepApi& api, std::uint16_t value) noexcept {
    const unsigned sr = api.read_register(api.context, 17U);
    api.write_register(api.context, 17U, (sr & 0xFFE0U) | (value & 0x1FU));
}

std::uint16_t logic_flags(std::uint16_t value, std::uint16_t old) noexcept {
    return static_cast<std::uint16_t>((old & kX) | (value == 0U ? kZ : 0U) |
        ((value & 0x8000U) != 0U ? kN : 0U));
}

bool fetch_extension(const M68k9D70StepApi& api, std::uint16_t expected) noexcept {
    return (api.fetch16(api.context) & 0xFFFFU) == expected;
}

std::uint16_t expected_opcode(unsigned pc) noexcept {
    if (pc < kStart || pc >= kRts || ((pc - kStart) & 1U) != 0U) return 0U;
    const unsigned index = (pc - kStart) / 2U;
    return index < kRomWords.size() ? kRomWords[index] : 0U;
}

void fail_call(bool contract) noexcept {
    ++metrics.rom_fallback_calls;
    if (contract) ++metrics.contract_rejects;
    else ++metrics.precondition_rejects;
}
} // namespace

M68k9D70StepResult step_m68k_e268(unsigned pc, const M68k9D70StepApi& api,
                                  bool enabled) noexcept {
    pc &= kMask24;
    if (!api.read_register || !api.write_register || !api.peek || !api.read ||
        !api.write || !api.fetch16 || !api.begin_instruction ||
        !api.finish_instruction || !api.add_cycles || !api.observe_instruction ||
        !api.read_scheduler_field || !api.read_transfer_source_pc)
        return M68k9D70StepResult::ContractFault;

    if (!active) {
        if (pc != kStart || !enabled) return M68k9D70StepResult::NotHandled;
        ++metrics.target_entries;
        if (api.read_transfer_source_pc(api.context) != kCaller || !caller_instruction_matches(api)) {
            ++metrics.provenance_rejects;
            ++metrics.rom_fallback_calls;
            return M68k9D70StepResult::NotHandled;
        }
        const unsigned sp = api.read_register(api.context, 15U) & kMask24;
        if (!entry_stack_matches(api) || !in_work_ram(sp, 2U)) {
            fail_call(false);
            return M68k9D70StepResult::NotHandled;
        }
        if (!validate_rom(api)) {
            fail_call(true);
            return M68k9D70StepResult::NotHandled;
        }
        active = true;
        interrupted = false;
        pending_scheduler_resume = false;
        call_saw_refresh = false;
        resume_pc = kStart;
        ++metrics.native_calls;
    }

    if (interrupted) {
        if (pc != resume_pc) return M68k9D70StepResult::NotHandled;
        interrupted = false;
        ++metrics.irq_resume_count;
    } else if (pc != resume_pc) {
        if (!interrupt_frame_for_resume(api)) {
            ++metrics.contract_rejects;
            last_contract_fault = 1U; last_fault_pc = pc; last_expected_pc = resume_pc;
            return M68k9D70StepResult::ContractFault;
        }
        interrupted = true;
        ++metrics.irq_pause_count;
        return M68k9D70StepResult::NotHandled;
    }
    if (pending_scheduler_resume && pc == resume_pc) {
        ++metrics.scheduler_resume_count;
        pending_scheduler_resume = false;
    }

    if (pc == kRts) {
        active = false;
        interrupted = false;
        ++metrics.completed_calls;
        return M68k9D70StepResult::NotHandled;
    }
    const auto opcode = expected_opcode(pc);
    unsigned raw{};
    if (opcode == 0U || !read_be(api, pc, 2U, raw) || raw != opcode) {
        ++metrics.contract_rejects;
        last_contract_fault = 2U; last_fault_pc = pc; last_expected_pc = resume_pc;
        return M68k9D70StepResult::ContractFault;
    }
    api.observe_instruction(api.context, pc, opcode);
    if ((api.fetch16(api.context) & 0xFFFFU) != opcode) {
        ++metrics.contract_rejects;
        last_contract_fault = 3U; last_fault_pc = pc; last_expected_pc = resume_pc;
        return M68k9D70StepResult::ContractFault;
    }

    const auto cycles_before = static_cast<std::uint32_t>(api.read_scheduler_field(api.context, 0U));
    const auto refresh_before = static_cast<std::uint32_t>(api.read_scheduler_field(api.context, 2U));
    const bool refresh_now = cycles_before >= refresh_before;
    if (refresh_now) {
        ++metrics.refresh_events;
        if (!call_saw_refresh) ++metrics.refresh_variant_calls;
        call_saw_refresh = true;
    }
    api.begin_instruction(api.context, opcode);

    unsigned dynamic_cycles{};
    if (pc == 0xE268U && opcode == 0x3F01U) {
        unsigned sp = api.read_register(api.context, 15U) & kMask24;
        if (sp < kRamStart + 2U) { ++metrics.contract_rejects; return M68k9D70StepResult::ContractFault; }
        sp -= 2U;
        api.write_register(api.context, 15U, sp);
        set_flags(api, logic_flags(static_cast<std::uint16_t>(api.read_register(api.context, 1U)), flags(api)));
        api.write(api.context, sp, 2, api.read_register(api.context, 1U) & 0xFFFFU);
    } else if (pc == 0xE26AU && opcode == 0x3039U) {
        if (!fetch_extension(api, 0x00FFU) || !fetch_extension(api, 0x1928U)) {
            ++metrics.contract_rejects; return M68k9D70StepResult::ContractFault;
        }
        const auto value = static_cast<std::uint16_t>(api.read(api.context, kState, 2));
        const auto d0 = api.read_register(api.context, 0U);
        api.write_register(api.context, 0U, (d0 & 0xFFFF0000U) | value);
        set_flags(api, logic_flags(value, flags(api)));
    } else if (opcode == 0x3200U && pc >= 0xE270U && pc <= 0xE296U &&
               ((pc - 0xE270U) % 10U) == 0U) {
        const auto d1 = api.read_register(api.context, 1U);
        const auto d0 = api.read_register(api.context, 0U);
        api.write_register(api.context, 1U, (d1 & 0xFFFF0000U) | (d0 & 0xFFFFU));
        set_flags(api, logic_flags(static_cast<std::uint16_t>(d0), flags(api)));
    } else if (opcode == 0xE559U && pc >= 0xE270U && pc <= 0xE296U &&
               ((pc - 0xE270U) % 10U) == 2U) {
        const auto old = static_cast<std::uint16_t>(api.read_register(api.context, 1U));
        const auto value = static_cast<std::uint16_t>((old << 2U) | (old >> 14U));
        const auto carry = (old >> 14U) & 1U;
        api.write_register(api.context, 1U,
            (api.read_register(api.context, 1U) & 0xFFFF0000U) | value);
        set_flags(api, static_cast<std::uint16_t>((flags(api) & kX) |
            (value == 0U ? kZ : 0U) | ((value & 0x8000U) ? kN : 0U) | carry));
        dynamic_cycles = 28U;
    } else if (opcode == 0xB141U && pc >= 0xE270U && pc <= 0xE296U &&
               ((pc - 0xE270U) % 10U) == 4U) {
        const auto d1 = api.read_register(api.context, 1U);
        const auto value = static_cast<std::uint16_t>(d1 ^ api.read_register(api.context, 0U));
        api.write_register(api.context, 1U, (d1 & 0xFFFF0000U) | value);
        set_flags(api, logic_flags(value, flags(api)));
    } else if (opcode == 0xE249U && pc >= 0xE270U && pc <= 0xE296U &&
               ((pc - 0xE270U) % 10U) == 6U) {
        const auto old = static_cast<std::uint16_t>(api.read_register(api.context, 1U));
        const auto value = static_cast<std::uint16_t>(old >> 1U);
        const auto carry = old & 1U;
        const auto x = carry ? kX : 0U;
        api.write_register(api.context, 1U,
            (api.read_register(api.context, 1U) & 0xFFFF0000U) | value);
        set_flags(api, static_cast<std::uint16_t>(x | (value == 0U ? kZ : 0U) | carry));
        dynamic_cycles = 14U;
    } else if (opcode == 0xD140U && pc >= 0xE270U && pc <= 0xE296U &&
               ((pc - 0xE270U) % 10U) == 8U) {
        const auto d0 = api.read_register(api.context, 0U);
        const auto lhs = static_cast<std::uint16_t>(d0);
        const auto x = (flags(api) & kX) != 0U ? 1U : 0U;
        const unsigned wide = static_cast<unsigned>(lhs) + lhs + x;
        const auto value = static_cast<std::uint16_t>(wide);
        const bool overflow = ((~(lhs ^ lhs) & (lhs ^ value) & 0x8000U) != 0U);
        const bool old_zero = (flags(api) & kZ) != 0U;
        const auto d0_out = (d0 & 0xFFFF0000U) | value;
        api.write_register(api.context, 0U, d0_out);
        set_flags(api, static_cast<std::uint16_t>(((wide > 0xFFFFU) ? (kX | kC) : 0U) |
            (overflow ? kV : 0U) | ((value & 0x8000U) ? kN : 0U) |
            ((old_zero && value == 0U) ? kZ : 0U)));
    } else if (pc == 0xE298U && opcode == 0x33C0U) {
        if (!fetch_extension(api, 0x00FFU) || !fetch_extension(api, 0x1928U)) {
            ++metrics.contract_rejects; return M68k9D70StepResult::ContractFault;
        }
        const auto value = static_cast<std::uint16_t>(api.read_register(api.context, 0U));
        set_flags(api, logic_flags(value, flags(api)));
        api.write(api.context, kState, 2, value);
    } else if (pc == 0xE29EU && opcode == 0x321FU) {
        const unsigned sp = api.read_register(api.context, 15U) & kMask24;
        if (!in_work_ram(sp, 2U)) { ++metrics.contract_rejects; return M68k9D70StepResult::ContractFault; }
        api.write_register(api.context, 15U, sp + 2U);
        const auto value = static_cast<std::uint16_t>(api.read(api.context, sp, 2));
        const auto d1 = api.read_register(api.context, 1U);
        api.write_register(api.context, 1U, (d1 & 0xFFFF0000U) | value);
        set_flags(api, logic_flags(value, flags(api)));
    } else {
        ++metrics.contract_rejects;
        return M68k9D70StepResult::ContractFault;
    }

    api.finish_instruction(api.context, opcode);
    if (dynamic_cycles != 0U) api.add_cycles(api.context, static_cast<int>(dynamic_cycles));
    const auto cycles_after = static_cast<std::uint32_t>(api.read_scheduler_field(api.context, 0U));
    const auto cycle_end = static_cast<std::uint32_t>(api.read_scheduler_field(api.context, 1U));
    pending_scheduler_resume = cycles_after >= cycle_end;
    resume_pc = api.read_register(api.context, 16U) & kMask24;
    if (resume_pc <= pc || resume_pc >= kEnd) {
        ++metrics.contract_rejects;
        last_contract_fault = 6U; last_fault_pc = pc; last_expected_pc = resume_pc;
        return M68k9D70StepResult::ContractFault;
    }
    ++metrics.stepped_instructions;
    return M68k9D70StepResult::InstructionStepped;
}

E268StepMetrics e268_step_metrics() noexcept { return metrics; }

void write_e268_step_metrics(const std::filesystem::path& path) {
    std::ofstream out(path, std::ios::trunc);
    out << "{\"schema\":\"oasis.m68k.e268-step-metrics.v1\",\"target_entries\":"
        << metrics.target_entries << ",\"native_calls\":" << metrics.native_calls
        << ",\"rom_fallback_calls\":" << metrics.rom_fallback_calls
        << ",\"provenance_rejects\":" << metrics.provenance_rejects
        << ",\"precondition_rejects\":" << metrics.precondition_rejects
        << ",\"contract_rejects\":" << metrics.contract_rejects
        << ",\"completed_calls\":" << metrics.completed_calls
        << ",\"stepped_instructions\":" << metrics.stepped_instructions
        << ",\"scheduler_resume_count\":" << metrics.scheduler_resume_count
        << ",\"irq_pause_count\":" << metrics.irq_pause_count
        << ",\"irq_resume_count\":" << metrics.irq_resume_count
        << ",\"refresh_events\":" << metrics.refresh_events
        << ",\"refresh_variant_calls\":" << metrics.refresh_variant_calls
        << ",\"last_contract_fault\":" << last_contract_fault
        << ",\"last_fault_pc\":" << last_fault_pc
        << ",\"last_expected_pc\":" << last_expected_pc << "}\n";
}

} // namespace oasis::hybrid
