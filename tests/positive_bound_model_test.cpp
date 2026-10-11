#include "tools/hybrid/positive_bound_model.hpp"

#include <array>
#include <cstdint>
#include <iostream>

namespace {
struct Sample {
    std::uint16_t coordinate, axis, offset, dimension;
    std::uint16_t result, maximum, sum;
};
bool check(bool value, const char* message) {
    if (!value) std::cerr << message << '\n';
    return value;
}
}

int main() {
    using oasis::hybrid::PositiveBoundInput;
    using oasis::hybrid::apply_positive_bound;
    constexpr std::array<Sample, 24> natural{{
        {33,168,6,64,33,337,174},{33,168,6,64,33,337,174},
        {8,201,6,64,8,304,207},{8,201,6,64,8,304,207},
        {7,209,6,64,7,296,215},{7,209,6,64,7,296,215},
        {8,216,6,64,8,289,222},{8,216,6,64,8,289,222},
        {7,224,6,64,7,281,230},{7,224,6,64,7,281,230},
        {8,231,6,64,8,274,237},{8,231,6,64,8,274,237},
        {7,239,6,64,7,266,245},{7,239,6,64,7,266,245},
        {8,246,6,64,8,259,252},{8,246,6,64,8,259,252},
        {7,254,6,64,7,251,260},{7,254,6,64,7,251,260},
        {8,261,6,64,8,244,267},{8,261,6,64,8,244,267},
        {7,269,6,64,7,236,275},{7,269,6,64,7,236,275},
        {8,276,6,64,8,229,282},{8,276,6,64,8,229,282},
    }};
    for (const auto& sample : natural) {
        const auto result = apply_positive_bound({sample.coordinate, sample.axis,
            sample.offset, sample.dimension, 0U});
        if (!check(result.coordinate == sample.result &&
                   result.maximum == sample.maximum &&
                   result.coordinate_sum == sample.sum &&
                   !result.flags_written,
                   "natural 0x9D92 replay vector mismatch")) return 1;
    }
    const auto inside = apply_positive_bound({10,20,2,64,0x05});
    const auto clamp = apply_positive_bound({500,20,2,64,0x05});
    const auto zero = apply_positive_bound({1,7,0,1,0x21});
    const auto wrapped = apply_positive_bound({0xFFFF,0x8000,0,1,0x10});
    return check(inside.coordinate == 10 && inside.maximum == 489 &&
                 !inside.flags_written, "in-range predicate mismatch") &&
           check(clamp.coordinate == 489 && clamp.maximum == 489 &&
                 !clamp.flags_written, "nonzero clamp mismatch") &&
           check(zero.coordinate == 0 && zero.maximum == 0 &&
                 zero.flags == 0x69 && zero.flags_written,
                 "zero clamp flag effect mismatch") &&
           check(wrapped.coordinate == 0x8007 && wrapped.maximum == 0x8007 &&
                 wrapped.coordinate_sum == 0x8000 && wrapped.extend_flag,
                 "16-bit wrapped subtraction mismatch") ? 0 : 1;
}
