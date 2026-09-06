#pragma once

#include "game/graphics_decompress.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace oasis::tools {

struct M68kState {
    std::array<std::uint32_t, 8> d{};
    std::array<std::uint32_t, 8> a{};
    std::uint16_t ccr{};
};

struct MemoryWrite {
    std::uint32_t address{};
    std::uint8_t width{};
    std::uint32_t value{};
};

class BoundedMemory {
public:
    BoundedMemory(std::uint32_t base, std::span<std::uint8_t> bytes);

    [[nodiscard]] std::uint32_t read_u16(std::uint32_t address) const;
    [[nodiscard]] std::uint32_t read_u32(std::uint32_t address) const;
    void write_u16(std::uint32_t address, std::uint16_t value);
    void write_u32(std::uint32_t address, std::uint32_t value);
    [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept { return bytes_; }
    [[nodiscard]] std::uint32_t base() const noexcept { return base_; }
    [[nodiscard]] const std::vector<MemoryWrite>& writes() const noexcept { return writes_; }

private:
    [[nodiscard]] std::size_t offset(std::uint32_t address, std::size_t width) const;

    std::uint32_t base_{};
    std::span<std::uint8_t> bytes_{};
    std::vector<MemoryWrite> writes_;
};

enum class TranslationStatus { verified, unsupported };

struct TranslationRun {
    TranslationStatus status{TranslationStatus::verified};
    std::size_t instructions_executed{};
    std::string detail;
};

struct StateDiff {
    bool equal{};
    std::string first_divergence;
};

[[nodiscard]] StateDiff compare_state(const M68kState& expected,
                                      const M68kState& actual,
                                      const BoundedMemory& expected_memory,
                                      const BoundedMemory& actual_memory);

// Mechanical output for the two verified 0x3820 stream formats. This is a
// normal compiled function; it never fetches or decodes 68000 instructions.
[[nodiscard]] oasis::game::DecompressResult mechanical_3820(
    std::span<const std::uint8_t> source, std::span<std::uint8_t> destination);

// 0xA8DA: bounded arithmetic leaf selected from current mass verification.
[[nodiscard]] TranslationRun mechanical_A8DA(M68kState& state);

// 0x62CC: bounded RAM-state leaf selected from current mass verification.
[[nodiscard]] TranslationRun mechanical_62CC(M68kState& state, BoundedMemory& memory);

[[nodiscard]] TranslationRun unsupported_opcode(std::uint16_t opcode);

} // namespace oasis::tools
