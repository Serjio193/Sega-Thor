#pragma once

#include "tools/re_slice_decoder.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace oasis::hybrid {

struct GeneratedBlock {
    std::uint32_t start{};
    std::uint32_t end{};
    std::vector<oasis::tools::DecodedInstruction> instructions;
};

[[nodiscard]] GeneratedBlock generate_block(std::span<const std::uint8_t> rom,
                                            std::uint32_t start,
                                            std::uint32_t end);

[[nodiscard]] std::string emit_translation_unit(
    const std::vector<GeneratedBlock>& blocks);

[[nodiscard]] std::string emit_registry_translation_unit(
    const std::vector<GeneratedBlock>& blocks);

} // namespace oasis::hybrid
