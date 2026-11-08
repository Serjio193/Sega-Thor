#include "game/entities/sparse_word_store.hpp"

namespace oasis::game::entities {
namespace {
constexpr std::size_t record_stride = 0x00BCU;
constexpr std::size_t initial_offset = record_stride + 0x2AU;
constexpr std::size_t stride = record_stride;
constexpr std::size_t store_count = 4U;
constexpr std::uint16_t data_offset_register = 0x00BCU;

void write_big_endian_word(std::span<std::uint8_t> bytes,
                           std::size_t offset,
                           std::uint16_t value) noexcept {
    bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
    bytes[offset + 1U] = static_cast<std::uint8_t>(value);
}
} // namespace

std::optional<SparseWordStoreResult> store_sparse_raw_words(
    std::span<std::uint8_t> address_space,
    std::size_t base_offset,
    std::uint16_t value,
    std::uint16_t status_register) noexcept {
    if (base_offset > address_space.size() ||
        initial_offset > address_space.size() - base_offset) {
        return std::nullopt;
    }

    const auto first = base_offset + initial_offset;
    const auto last = first + (store_count - 1U) * stride;
    if (last > address_space.size() || 2U > address_space.size() - last) {
        return std::nullopt;
    }

    for (std::size_t index = 0; index < store_count; ++index) {
        write_big_endian_word(address_space, first + index * stride, value);
    }

    // Final MOVE.W D0,(A0) sets N/Z, clears V/C, and preserves X.
    auto final_sr = static_cast<std::uint16_t>(status_register & 0xFFF0U);
    if ((value & 0x8000U) != 0U) {
        final_sr = static_cast<std::uint16_t>(final_sr | 0x0008U);
    }
    if (value == 0U) {
        final_sr = static_cast<std::uint16_t>(final_sr | 0x0004U);
    }
    return SparseWordStoreResult{
        last,
        data_offset_register,
        final_sr};
}

} // namespace oasis::game::entities
