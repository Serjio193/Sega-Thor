#pragma once

#include "tools/re_slice_decoder.hpp"

#include <span>

namespace oasis::tools {

struct CfgClosure {
    std::vector<DecodedInstruction> instructions;
    std::vector<std::uint32_t> seeds;
    std::vector<std::string> blockers;
    std::size_t decoded_bytes{};
    std::size_t connected_components{};
    bool closed{};
};

// Repeatedly decodes exact entry seeds and newly discovered backward targets.
// Bounds constrain exploration; they are never returned as object extents.
[[nodiscard]] CfgClosure decode_closed_cfg(
    std::span<const std::uint8_t> rom, std::uint32_t range_start,
    std::uint32_t range_end, std::span<const std::uint32_t> exact_seeds,
    std::span<const std::uint32_t> exact_known_targets = {});

} // namespace oasis::tools
