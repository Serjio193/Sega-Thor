#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace oasis::game::world {

struct ByteGridAggregate {
    std::uint8_t any_bits{};
    std::uint8_t common_bits{0xFFU};

    [[nodiscard]] std::uint8_t common_terrain_code() const noexcept {
        return static_cast<std::uint8_t>(common_bits & 0x0FU);
    }
};

class ByteGridView {
public:
    ByteGridView(std::span<const std::uint8_t> bytes,
                 std::uint16_t row_stride,
                 std::uint8_t row_shift) noexcept;

    [[nodiscard]] std::uint16_t row_stride() const noexcept { return row_stride_; }
    [[nodiscard]] std::uint8_t row_shift() const noexcept { return row_shift_; }

    [[nodiscard]] std::optional<std::uint8_t> sample_cell(
        std::int32_t cell_x,
        std::int32_t cell_y) const noexcept;

    [[nodiscard]] std::optional<std::uint8_t> sample_world(
        std::int32_t world_x,
        std::int32_t world_y) const noexcept;

    [[nodiscard]] std::optional<std::uint8_t> terrain_code_world(
        std::int32_t world_x,
        std::int32_t world_y) const noexcept;

    [[nodiscard]] std::optional<ByteGridAggregate> aggregate_world_square(
        std::int32_t center_x,
        std::int32_t center_y,
        std::uint16_t radius) const noexcept;

private:
    std::span<const std::uint8_t> bytes_;
    std::uint16_t row_stride_{};
    std::uint8_t row_shift_{};
};

} // namespace oasis::game::world
