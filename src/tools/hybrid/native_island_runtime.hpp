#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace oasis::hybrid::native_island {

struct CpuState {
    std::array<std::uint32_t, 8> d{};
    std::array<std::uint32_t, 8> a{};
    std::uint32_t pc{};
    std::uint16_t sr{};
};

struct Access {
    int (*peek)(unsigned address){};
    bool (*write)(unsigned address, int width, unsigned value){};
    void (*set_register)(unsigned index, unsigned value){};
};

struct Counters {
    std::uint64_t total_transfers{};
    std::uint64_t eligible_calls{};
    std::uint64_t native_calls{};
    std::uint64_t rom_fallbacks{};
    std::uint64_t provenance_rejects{};
    std::uint64_t precondition_rejects{};
    std::uint64_t contract_rejects{};
    std::uint64_t mismatches{};
};

enum class Result { NotTarget, RomFallback, NativeRouted, FatalAbort };

// Developer replay only. The sole executable adapter is deliberately limited
// to verified 0x9D70 caller 0x009144 / contract V1.
[[nodiscard]] Result route_9d70(
    bool island_enabled, bool helper_enabled, bool canonical_rom,
    unsigned target_pc, unsigned source_pc, unsigned source_opcode,
    unsigned stack_return, std::string_view contract_id,
    const CpuState& cpu, const Access& access,
    Counters& counters) noexcept;

[[nodiscard]] bool supported_helper(std::string_view id) noexcept;

} // namespace oasis::hybrid::native_island
