#pragma once

#include "tools/re_slice_decoder.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::tools {

[[nodiscard]] bool set_absolute_long_control_target(
    std::span<const std::uint8_t> rom, std::uint32_t pc,
    std::uint32_t range_end, bool is_call, DecodedInstruction& instruction);

[[nodiscard]] bool set_direct_target_from_effective_address(
    DecodedInstruction& instruction, unsigned mode, unsigned reg, bool is_call);

} // namespace oasis::tools
