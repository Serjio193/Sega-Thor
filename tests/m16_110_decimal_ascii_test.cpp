#include "game/ui/decimal_ascii.hpp"

#include <cassert>
#include <string_view>

namespace {

using namespace oasis::game::ui;

[[nodiscard]] std::string_view as_sv(const std::array<char, kDecimalDigitCount>& buf) noexcept {
    return std::string_view(buf.data(), buf.size());
}

void test_zero_padded_formatter() {
    assert(as_sv(format_u16_dec_zero_padded(0U)) == "00000");
    assert(as_sv(format_u16_dec_zero_padded(105U)) == "00105");
    assert(as_sv(format_u16_dec_zero_padded(9999U)) == "09999");
    assert(as_sv(format_u16_dec_zero_padded(10000U)) == "10000");
    assert(as_sv(format_u16_dec_zero_padded(65535U)) == "65535");
}

void test_space_padded_formatter() {
    assert(as_sv(format_u16_dec_space_padded(0U)) == "    0");
    assert(as_sv(format_u16_dec_space_padded(105U)) == "  105");
    assert(as_sv(format_u16_dec_space_padded(9999U)) == " 9999");
    assert(as_sv(format_u16_dec_space_padded(10000U)) == "10000");
    assert(as_sv(format_u16_dec_space_padded(65535U)) == "65535");
}

} // namespace

int main() {
    test_zero_padded_formatter();
    test_space_padded_formatter();
    return 0;
}
