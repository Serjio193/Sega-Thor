#pragma once

#include "tools/re_slice_decoder.hpp"

namespace oasis::tools {
[[nodiscard]] std::string exact_instruction_asm(const DecodedInstruction& instruction);
[[nodiscard]] std::string slice_asm(const DecodedSlice& slice);
[[nodiscard]] std::string exact_slice_json(const DecodedSlice& slice);

struct ByteDifference {
    std::size_t slice_offset{};
    std::uint32_t rom_offset{};
    std::optional<std::uint8_t> expected;
    std::optional<std::uint8_t> actual;
};
[[nodiscard]] std::optional<ByteDifference> first_byte_difference(
    std::span<const std::uint8_t> rom, std::span<const std::uint8_t> rebuilt,
    std::uint32_t start, std::uint32_t end);
[[nodiscard]] std::string difference_text(const std::optional<ByteDifference>& difference,
                                         const DecodedSlice* slice = nullptr);
} // namespace oasis::tools
