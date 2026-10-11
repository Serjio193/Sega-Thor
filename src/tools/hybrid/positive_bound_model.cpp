#include "tools/hybrid/positive_bound_model.hpp"

namespace oasis::hybrid {

PositiveBoundResult apply_positive_bound(
    const PositiveBoundInput& input) noexcept {
    const auto coordinate_sum = static_cast<std::uint16_t>(
        input.axis_value + input.record_offset);
    const auto limit_base = static_cast<std::uint16_t>(
        static_cast<std::uint32_t>(input.dimension_units) * 8U - 1U);
    const auto maximum = static_cast<std::uint16_t>(limit_base - coordinate_sum);
    const auto clamped = input.coordinate > maximum;
    const auto coordinate = clamped ? maximum : input.coordinate;
    const auto flags_written = clamped && coordinate == 0U;
    const auto flags = static_cast<std::uint8_t>(
        flags_written ? input.flags | 0x48U : input.flags);
    return {coordinate, maximum, coordinate_sum, flags, flags_written,
            limit_base < coordinate_sum};
}

} // namespace oasis::hybrid
