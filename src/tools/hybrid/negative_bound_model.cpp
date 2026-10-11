#include "tools/hybrid/negative_bound_model.hpp"

namespace oasis::hybrid {

NegativeBoundResult apply_negative_bound(
    const NegativeBoundInput& input) noexcept {
    const auto relative = static_cast<std::uint16_t>(
        input.axis_value - input.record_offset);
    const bool extend = input.axis_value < input.record_offset;
    const bool negative = (relative & 0x8000U) != 0U;
    if (negative) {
        return {0U, relative,
                static_cast<std::uint8_t>(input.flags | 0x48U), true, extend};
    }

    if (input.coordinate <= relative)
        return {input.coordinate, relative, input.flags, false, extend};

    const bool mark_zero = relative == 0U;
    const auto flags = static_cast<std::uint8_t>(
        mark_zero ? input.flags | 0x48U : input.flags);
    return {relative, relative, flags, mark_zero, extend};
}

} // namespace oasis::hybrid
