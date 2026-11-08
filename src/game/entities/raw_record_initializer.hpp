#pragma once

#include <cstdint>
#include <span>

namespace oasis::game::entities {

// Neutral translation of ROM helper 0x8EF8 for its proven record window.
[[nodiscard]] bool initialize_raw_record_fields(
    std::span<std::uint8_t> record, std::uint16_t input_word) noexcept;

} // namespace oasis::game::entities
