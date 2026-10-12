#include "tools/hybrid/native_island_runtime.hpp"

#include "tools/hybrid/subtraction_bound_model.hpp"

#include <cstdint>

namespace oasis::hybrid::native_island {
namespace {
constexpr unsigned target_9d70 = 0x009D70U;
constexpr unsigned caller_9d70 = 0x009144U;
constexpr unsigned return_9d70 = 0x009148U;
constexpr unsigned keep_exit_9d70 = 0x009D86U;
constexpr unsigned write_exit_9d70 = 0x009D90U;
constexpr unsigned bsr_word_opcode = 0x6100U;
constexpr unsigned ram_start = 0xFF0000U;
constexpr unsigned ram_end = 0x1000000U;
constexpr unsigned contract_version = 1U;
constexpr std::string_view contract_id_9d70 = "M68K_009D70_CALLER_009144_V1";

bool in_ram(unsigned address, unsigned size) noexcept {
    return address >= ram_start && size <= ram_end - address;
}

bool read_byte(const Access& access, unsigned address, std::uint8_t& value) noexcept {
    if (!access.peek || !in_ram(address, 1U)) return false;
    const auto read = access.peek(address);
    if (read < 0 || read > 0xFF) return false;
    value = static_cast<std::uint8_t>(read);
    return true;
}

bool read_word(const Access& access, unsigned address, std::uint16_t& value) noexcept {
    std::uint8_t high{}, low{};
    if (!read_byte(access, address, high) || !read_byte(access, address + 1U, low))
        return false;
    value = static_cast<std::uint16_t>((static_cast<unsigned>(high) << 8U) | low);
    return true;
}

bool exact_bsr(unsigned source, unsigned opcode, unsigned target) noexcept {
    if ((opcode >> 16U) != bsr_word_opcode) return false;
    const auto displacement = static_cast<std::int16_t>(opcode & 0xFFFFU);
    return ((source + 2U + static_cast<unsigned>(displacement)) & 0xFFFFFFU) == target;
}
} // namespace

bool supported_helper(std::string_view id) noexcept {
    return id == "9d70";
}

Result route_9d70(bool island_enabled, bool helper_enabled, bool canonical_rom,
                  unsigned target_pc, unsigned source_pc, unsigned source_opcode,
                  unsigned stack_return, std::string_view contract_id,
                  const CpuState& cpu, const Access& access,
                  Counters& counters) noexcept {
    if (target_pc != target_9d70) return Result::NotTarget;
    ++counters.total_transfers;

    const bool provenance_ok = source_pc == caller_9d70 &&
        exact_bsr(source_pc, source_opcode, target_9d70) &&
        stack_return == return_9d70;
    if (!provenance_ok) {
        ++counters.provenance_rejects;
        ++counters.rom_fallbacks;
        return Result::RomFallback;
    }
    if (!island_enabled || !helper_enabled) {
        ++counters.rom_fallbacks;
        return Result::RomFallback;
    }
    if (!canonical_rom) {
        ++counters.contract_rejects;
        ++counters.rom_fallbacks;
        return Result::RomFallback;
    }
    ++counters.eligible_calls;
    if (contract_id != contract_id_9d70 || contract_version != 1U ||
        !access.peek || !access.write || !access.set_register) {
        ++counters.contract_rejects;
        ++counters.rom_fallbacks;
        return Result::RomFallback;
    }

    const auto object = cpu.a[6] & 0xFFFFFFU;
    std::uint16_t limit{};
    std::uint8_t marker{};
    if (!in_ram(object, 0x48U) || !read_word(access, object + 0x46U, limit) ||
        !read_byte(access, object + 0x40U, marker)) {
        ++counters.precondition_rejects;
        ++counters.rom_fallbacks;
        return Result::RomFallback;
    }

    // Stage every effect before touching guest state. The accepted contract has
    // exactly one optional byte store and D0/D3/SR plus a return-path choice.
    const auto effect = apply_subtraction_bound({
        cpu.d[0], cpu.d[1], cpu.d[3], limit, marker, cpu.sr});
    if (effect.marker_written) {
        if (!access.write(object + 0x40U, 1, effect.marker)) {
            ++counters.precondition_rejects;
            ++counters.rom_fallbacks;
            return Result::RomFallback;
        }
        std::uint8_t committed{};
        if (!read_byte(access, object + 0x40U, committed) || committed != effect.marker) {
            const bool restored = access.write(object + 0x40U, 1, marker) &&
                read_byte(access, object + 0x40U, committed) && committed == marker;
            if (!restored) {
                ++counters.mismatches;
                ++counters.contract_rejects;
                return Result::FatalAbort;
            }
            ++counters.contract_rejects;
            ++counters.rom_fallbacks;
            return Result::RomFallback;
        }
    }

    access.set_register(0U, effect.d0);
    access.set_register(3U, effect.d3);
    access.set_register(17U, effect.sr);
    access.set_register(16U, effect.marker_written ? write_exit_9d70 : keep_exit_9d70);
    ++counters.native_calls;
    return Result::NativeRouted;
}

} // namespace oasis::hybrid::native_island
