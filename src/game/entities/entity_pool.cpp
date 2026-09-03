#include "game/entities/entity_pool.hpp"

namespace oasis::game::entities {

std::optional<std::span<const std::uint8_t>> EntityPoolView::record(
    std::size_t index) const noexcept {
    if (index >= spec_.record_count || spec_.record_stride == 0) {
        return std::nullopt;
    }
    const auto offset = index * static_cast<std::size_t>(spec_.record_stride);
    const auto end = offset + spec_.record_stride;
    if (end > storage_.size()) {
        return std::nullopt;
    }
    return storage_.subspan(offset, spec_.record_stride);
}

bool EntityPoolView::active(std::size_t index) const noexcept {
    const auto entry = record(index);
    if (!entry || entry->size() < 2) {
        return false;
    }
    const auto value = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>((*entry)[0]) << 8U) |
        static_cast<std::uint16_t>((*entry)[1]));
    return value != 0 && (value & 0x8000U) == 0;
}

} // namespace oasis::game::entities
