#include "game/entities/entity_pool.hpp"

namespace oasis::game::entities {

std::optional<std::uint16_t> EntityRecordView::read_u16(
    std::size_t offset) const noexcept {
    if (offset > bytes_.size() || bytes_.size() - offset < 2U) {
        return std::nullopt;
    }
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes_[offset]) << 8U) |
        static_cast<std::uint16_t>(bytes_[offset + 1U]));
}

std::optional<std::uint32_t> EntityRecordView::read_u32(
    std::size_t offset) const noexcept {
    if (offset > bytes_.size() || bytes_.size() - offset < 4U) {
        return std::nullopt;
    }
    return (static_cast<std::uint32_t>(bytes_[offset]) << 24U) |
           (static_cast<std::uint32_t>(bytes_[offset + 1U]) << 16U) |
           (static_cast<std::uint32_t>(bytes_[offset + 2U]) << 8U) |
           static_cast<std::uint32_t>(bytes_[offset + 3U]);
}

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

std::optional<EntityRecordView> EntityPoolView::record_view(
    std::size_t index) const noexcept {
    const auto entry = record(index);
    if (!entry) {
        return std::nullopt;
    }
    return EntityRecordView(*entry);
}

bool EntityPoolView::active(std::size_t index) const noexcept {
    const auto entry = record_view(index);
    if (!entry) {
        return false;
    }
    const auto value = entry->read_u16(kEntityFieldType);
    if (!value) {
        return false;
    }
    return *value != 0 && (*value & 0x8000U) == 0;
}

} // namespace oasis::game::entities
