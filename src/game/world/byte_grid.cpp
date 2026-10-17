#include "game/world/byte_grid.hpp"

#include <limits>

namespace oasis::game::world {

ByteGridView::ByteGridView(std::span<const std::uint8_t> bytes,
                           std::uint16_t width,
                           std::uint16_t height,
                           std::uint16_t row_stride,
                           std::uint8_t row_shift) noexcept
    : bytes_(bytes), width_(width), height_(height),
      row_stride_(row_stride), row_shift_(row_shift) {}

std::optional<ByteGridView> ByteGridView::from_parent_output(
    std::span<const std::uint8_t> parent,
    std::size_t offset,
    std::uint16_t width,
    std::uint16_t height,
    std::uint16_t row_stride,
    std::uint8_t row_shift) noexcept {
    if (width == 0U || height == 0U || row_stride < width || row_shift >= 16U ||
        (std::uint32_t{1U} << row_shift) != row_stride) {
        return std::nullopt;
    }
    const auto byte_count = static_cast<std::size_t>(row_stride) * height;
    if (offset > parent.size() || byte_count > parent.size() - offset) {
        return std::nullopt;
    }
    ByteGridView view(parent.subspan(offset, byte_count), width, height,
                      row_stride, row_shift);
    return view.valid() ? std::optional<ByteGridView>(view) : std::nullopt;
}

bool ByteGridView::valid() const noexcept {
    if (width_ == 0U || height_ == 0U || row_stride_ < width_ ||
        row_shift_ >= 16U || (std::uint32_t{1U} << row_shift_) != row_stride_) {
        return false;
    }
    return bytes_.size() == static_cast<std::size_t>(row_stride_) * height_;
}

std::optional<std::uint8_t> ByteGridView::sample_cell(
    std::int32_t cell_x,
    std::int32_t cell_y) const noexcept {
    if (!valid() || cell_x < 0 || cell_y < 0) {
        return std::nullopt;
    }

    const auto x = static_cast<std::uint32_t>(cell_x);
    const auto y = static_cast<std::uint32_t>(cell_y);
    if (x >= width_ || y >= height_) {
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

std::optional<ByteGridAggregate> ByteGridView::aggregate_cell_column_world_y(
    std::int32_t cell_x,
    std::int32_t center_y,
    std::uint16_t radius) const noexcept {
    const auto r = static_cast<std::int64_t>(radius);
    const auto min_y = static_cast<std::int64_t>(center_y) - r;
    const auto max_y = static_cast<std::int64_t>(center_y) + r;
    if (!valid() || cell_x < 0 || static_cast<std::uint32_t>(cell_x) >= width_ ||
        min_y < 0 || max_y > std::numeric_limits<std::int32_t>::max()) {
        return std::nullopt;
    }

    const auto first_y = static_cast<std::int32_t>(min_y) / 8;
    const auto last_y = static_cast<std::int32_t>(max_y) / 8;
    if (last_y >= height_) return std::nullopt;

    ByteGridAggregate result{};
    for (auto y = first_y; y <= last_y; ++y) {
        const auto value = sample_cell(cell_x, y);
        if (!value) return std::nullopt;
        result.any_bits = static_cast<std::uint8_t>(result.any_bits | *value);
        result.common_bits = static_cast<std::uint8_t>(result.common_bits & *value);
    }
    return result;
}

} // namespace oasis::game::world
