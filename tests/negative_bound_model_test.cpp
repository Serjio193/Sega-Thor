#include "tools/hybrid/negative_bound_model.hpp"

#include <array>
#include <cstdint>
#include <iostream>

namespace {
struct Sample {
    std::uint16_t coordinate, axis, offset, expected_coordinate, expected_relative;
};
bool check(bool value, const char* message) {
    if (!value) std::cerr << message << '\n';
    return value;
}
}

int main() {
    using oasis::hybrid::NegativeBoundInput;
    using oasis::hybrid::apply_negative_bound;
    constexpr std::array<Sample, 24> natural{{
        {3,296,10,3,286},{3,293,10,3,283},{5,290,10,5,280},
        {4,285,10,4,275},{5,281,10,5,271},{4,276,10,4,266},
        {4,272,10,4,262},{4,268,10,4,258},{3,264,10,3,254},
        {2,261,10,2,251},{2,259,10,2,249},{2,257,10,2,247},
        {1,255,10,1,245},{3,254,10,3,244},{3,251,10,3,241},
        {3,248,10,3,238},{3,245,10,3,235},{3,242,10,3,232},
        {3,239,10,3,229},{3,236,10,3,226},{3,233,10,3,223},
        {3,230,10,3,220},{3,227,10,3,217},{3,224,10,3,214},
    }};
    for (const auto& sample : natural) {
        const auto result = apply_negative_bound(
            {sample.coordinate, sample.axis, sample.offset, 0U});
        if (!check(result.coordinate == sample.expected_coordinate &&
                   result.relative_value == sample.expected_relative &&
                   !result.flags_written && !result.extend_flag,
                   "natural 0x9DB4 replay vector mismatch")) return 1;
    }
    const auto in_range = apply_negative_bound({7,20,10,0x05});
    const auto clamp = apply_negative_bound({30,20,10,0x05});
    const auto zero = apply_negative_bound({1,10,10,0x21});
    const auto negative = apply_negative_bound({5,3,10,0x21});
    const auto signed_negative_without_borrow = apply_negative_bound({5,0xFFFF,1,0x21});
    return check(in_range.coordinate == 7 && in_range.relative_value == 10 &&
                 !in_range.flags_written, "in-range path mismatch") &&
           check(clamp.coordinate == 10 && clamp.relative_value == 10 &&
                 !clamp.flags_written, "nonzero clamp mismatch") &&
           check(zero.coordinate == 0 && zero.relative_value == 0 &&
                 zero.flags == 0x69 && zero.flags_written,
                 "zero clamp flag effect mismatch") &&
           check(negative.coordinate == 0 && negative.relative_value == 0xFFF9 &&
                 negative.flags == 0x69 && negative.flags_written &&
                 negative.extend_flag,
                 "negative bound path mismatch") &&
           check(signed_negative_without_borrow.coordinate == 0 &&
                 signed_negative_without_borrow.relative_value == 0xFFFE &&
                 signed_negative_without_borrow.flags == 0x69 &&
                 signed_negative_without_borrow.flags_written &&
                 !signed_negative_without_borrow.extend_flag,
                 "signed-negative non-borrow X preservation mismatch") ? 0 : 1;
}
