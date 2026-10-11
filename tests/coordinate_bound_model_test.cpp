#include "tools/hybrid/coordinate_bound_model.hpp"

#include <array>
#include <cstdint>
#include <iostream>

namespace {
struct NaturalCase {
    std::uint16_t d0, d1, offset, expected_d0, expected_d3, expected_d7;
};

bool check(bool condition, const char* message) {
    if (!condition) std::cerr << message << '\n';
    return condition;
}
}

int main() {
    using oasis::hybrid::CoordinateBoundInput;
    using oasis::hybrid::apply_coordinate_bound;
    constexpr std::array inputs{
        CoordinateBoundInput{3, 160, 10, 64, 0x05},
        CoordinateBoundInput{600, 1, 10, 64, 0x05},
        CoordinateBoundInput{1, 502, 9, 64, 0x21},
        CoordinateBoundInput{0xFFFF, 0, 1, 0, 0x80},
    };
    const auto inside = apply_coordinate_bound(inputs[0]);
    const auto clipped = apply_coordinate_bound(inputs[1]);
    const auto zero = apply_coordinate_bound(inputs[2]);
    const auto wrapped = apply_coordinate_bound(inputs[3]);
    constexpr std::array<NaturalCase, 24> natural{{
        {3,160,10,3,341,170}, {1,229,9,1,273,238},
        {1,230,9,1,272,239}, {1,231,9,1,271,240},
        {1,232,9,1,270,241}, {1,233,9,1,269,242},
        {1,234,9,1,268,243}, {1,235,9,1,267,244},
        {1,236,9,1,266,245}, {1,237,9,1,265,246},
        {1,238,9,1,264,247}, {1,239,9,1,263,248},
        {1,240,9,1,262,249}, {2,241,9,2,261,250},
        {3,256,10,3,245,266}, {1,168,9,1,334,177},
        {2,169,9,2,333,178}, {1,171,9,1,331,180},
        {2,172,9,2,330,181}, {1,174,9,1,328,183},
        {2,175,9,2,327,184}, {1,177,9,1,325,186},
        {2,178,9,2,324,187}, {1,180,9,1,322,189},
    }};
    for (const auto& sample : natural) {
        const auto result = apply_coordinate_bound(
            {sample.d0, sample.d1, sample.offset, 64, 0});
        if (!check(result.coordinate == sample.expected_d0 &&
                       result.maximum == sample.expected_d3 &&
                       result.coordinate_sum == sample.expected_d7 &&
                       !result.flags_written,
                   "natural ROM capture differential mismatch"))
            return 1;
    }
    return check(inside.coordinate == 3 && inside.maximum == 341 &&
                     !inside.flags_written,
                 "in-range coordinate changed") &&
           check(clipped.coordinate == 500 && clipped.flags == 0x05 &&
                     !clipped.flags_written,
                 "nonzero boundary clamp mismatch") &&
           check(zero.coordinate == 0 && zero.flags == 0x69 &&
                     zero.flags_written,
                 "zero boundary flag write mismatch") &&
           check(wrapped.coordinate == 0xFFFE && wrapped.maximum == 0xFFFE &&
                     !wrapped.flags_written,
                 "16-bit arithmetic wrap mismatch")
               ? 0
               : 1;
}
