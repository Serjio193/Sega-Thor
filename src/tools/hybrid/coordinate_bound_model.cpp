#include "tools/hybrid/coordinate_bound_model.hpp"

namespace oasis::hybrid {

CoordinateBoundResult apply_coordinate_bound(
    const CoordinateBoundInput& input) noexcept {
    const auto sum = static_cast<std::uint16_t>(input.input_origin + input.record_offset);
    const auto scaled = static_cast<std::uint16_t>(input.row_units << 3U);
    const auto maximum = static_cast<std::uint16_t>(scaled - 1U - sum);
    if (input.coordinate <= maximum)
        return {input.coordinate, sum, maximum, input.flags, false};

    const bool mark_zero_boundary = maximum == 0U;
    const auto flags = static_cast<std::uint8_t>(
        mark_zero_boundary ? input.flags | 0x48U : input.flags);
    return {maximum, sum, maximum, flags, mark_zero_boundary};
}

} // namespace oasis::hybrid
