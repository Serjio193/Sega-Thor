#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace oasis::game::render {

struct SatShadowRecordResult {
    std::size_t final_a1_offset{};
    std::uint16_t d2{};
    std::size_t records_consumed{};
    std::size_t records_published{};
};

// Bounded data-effect model for ROM helper 0xB730 on caller 0x003B448.
// Count-derived input and the maximum possible published destination span are
// validated before reproducing the ROM's ordered, including partial, stores.
[[nodiscard]] std::optional<SatShadowRecordResult> build_sat_shadow_records(
    std::span<const std::uint8_t> input,
    std::span<std::uint8_t> sat_shadow,
    std::size_t a1_offset,
    std::uint16_t d1,
    std::uint16_t d2,
    std::uint16_t d3,
    std::uint16_t d4) noexcept;

} // namespace oasis::game::render
