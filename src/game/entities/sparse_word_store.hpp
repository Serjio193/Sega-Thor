#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace oasis::game::entities {

struct SparseWordStoreResult {
    std::size_t final_address_offset{};
    std::uint16_t data_register_word{};
    std::uint16_t status_register{};
};

// Neutral view of the four fixed-stride stores performed by ROM helper 0x16A46.
[[nodiscard]] std::optional<SparseWordStoreResult> store_sparse_raw_words(
    std::span<std::uint8_t> address_space,
    std::size_t base_offset,
    std::uint16_t value,
    std::uint16_t status_register) noexcept;

} // namespace oasis::game::entities
