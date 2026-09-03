#include "game/world/byte_grid.hpp"

#include <limits>

namespace oasis::game::world {

ByteGridView::ByteGridView(std::span<const std::uint8_t> bytes,
                           std::uint16_t row_stride,
                           std::uint8_t row_shift) noexcept
    : bytes_(bytes), row_stride_(row_stride), row_shift_(row_shift) {}

std::optional<std::uint8_t> ByteGridView::sample_cell(
    std::int32_t cell_x,
    std::int32_t cell_y) const noexcept {
    if (cell_x < 0 || cell_y < 0 || row_stride_ == 0 || row_shift_ >= 31) {
        return std::nullopt;
    }

    const auto x = static_cast<std::uint32_t>(cell_x);
    const auto y = static_cast<std::uint32_t>(cell_y);
    if (x >= row_stride_) {
        return std::nullopt;
    }

    const auto shifted_y = static_cast<std::uint64_t>(y) << row_shift_;
    const auto index = shifted_y + x;
    if (index >= bytes_.size()) {
        return std::nullopt;
    }
    return bytes_[static_cast<std::size_t>(index)];
}

std::optional<std::uint8_t> ByteGridView::sample_world(
    std::int32_t world_x,
    std::int32_t world_y) const noexcept {
    if (world_x < 0 || world_y < 0) {
        return std::nullopt;
    }
    return sample_cell(world_x / 8, world_y / 8);
}

std::optional<std::uint8_t> ByteGridView::terrain_code_world(
    std::int32_t world_x,
    std::int32_t world_y) const noexcept {
    const auto value = sample_world(world_x, world_y);
    if (!value) {
        return std::nullopt;
    }
    return static_cast<std::uint8_t>(*value & 0x0FU);
}

std::optional<ByteGridAggregate> ByteGridView::aggregate_world_square(
    std::int32_t center_x,
    std::int32_t center_y,
    std::uint16_t radius) const noexcept {
    const auto r = static_cast<std::int64_t>(radius);
    const auto min_x = static_cast<std::int64_t>(center_x) - r;
    const auto max_x = static_cast<std::int64_t>(center_x) + r;
    const auto min_y = static_cast<std::int64_t>(center_y) - r;
    const auto max_y = static_cast<std::int64_t>(center_y) + r;
    if (min_x < 0 || min_y < 0 || max_x > std::numeric_limits<std::int32_t>::max() ||
        max_y > std::numeric_limits<std::int32_t>::max()) {
        return std::nullopt;
    }

    const auto x0 = static_cast<std::int32_t>(min_x) / 8;
    const auto x1 = static_cast<std::int32_t>(max_x) / 8;
    const auto y0 = static_cast<std::int32_t>(min_y) / 8;
    const auto y1 = static_cast<std::int32_t>(max_y) / 8;

    ByteGridAggregate result{};
    for (auto y = y0; y <= y1; ++y) {
        for (auto x = x0; x <= x1; ++x) {
            const auto value = sample_cell(x, y);
            if (!value) {
                return std::nullopt;
            }
            result.any_bits = static_cast<std::uint8_t>(result.any_bits | *value);
            result.common_bits = static_cast<std::uint8_t>(result.common_bits & *value);
        }
    }
    return result;
}

} // namespace oasis::game::world
