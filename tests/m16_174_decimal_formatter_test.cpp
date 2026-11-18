#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

#include "game/hud/decimal_formatter.hpp"

using namespace oasis::game::hud;

namespace {

void test_constants() {
    static_assert(DecimalFormatterConfig::kFormatDecFixed5Addr == 0x00C0BEU);
    static_assert(DecimalFormatterConfig::kFormatDecSpacesAddr == 0x00C114U);
    static_assert(DecimalFormatterConfig::kExitRtsFixed5Addr == 0x00C112U);
    static_assert(DecimalFormatterConfig::kExitRtsSpacesAddr == 0x00C178U);
    static_assert(DecimalFormatterConfig::kRangeEndAddr == 0x00C17AU);
    static_assert(DecimalFormatterConfig::kRoutineSizeBytes == 188U);

    static_assert(DecimalFormatterConfig::kDecimalBufferRam == 0x00FF192AU);
    static_assert(DecimalFormatterConfig::kBufferLen == 5U);

    static_assert(DecimalFormatterConfig::kDivisor10000 == 10000U);
    static_assert(DecimalFormatterConfig::kDivisor1000 == 1000U);
    static_assert(DecimalFormatterConfig::kDivisor100 == 100U);
    static_assert(DecimalFormatterConfig::kDivisor10 == 10U);

    static_assert(DecimalFormatterConfig::kZeroChar == '0');
    static_assert(DecimalFormatterConfig::kSpaceChar == ' ');

    std::cout << "[PASS] test_constants\n";
}

bool str_match(const DecimalString5& arr, std::string_view expected) {
    if (expected.size() != arr.size()) {
        return false;
    }
    for (std::size_t i = 0; i < arr.size(); ++i) {
        if (arr[i] != expected[i]) {
            return false;
        }
    }
    return true;
}

void test_fixed5_values() {
    assert(str_match(format_dec_fixed5(0), "00000"));
    assert(str_match(format_dec_fixed5(1), "00001"));
    assert(str_match(format_dec_fixed5(9), "00009"));
    assert(str_match(format_dec_fixed5(10), "00010"));
    assert(str_match(format_dec_fixed5(42), "00042"));
    assert(str_match(format_dec_fixed5(99), "00099"));
    assert(str_match(format_dec_fixed5(100), "00100"));
    assert(str_match(format_dec_fixed5(999), "00999"));
    assert(str_match(format_dec_fixed5(1000), "01000"));
    assert(str_match(format_dec_fixed5(9999), "09999"));
    assert(str_match(format_dec_fixed5(10000), "10000"));
    assert(str_match(format_dec_fixed5(65535), "65535"));

    // Interspersed zeros
    assert(str_match(format_dec_fixed5(105), "00105"));
    assert(str_match(format_dec_fixed5(1005), "01005"));
    assert(str_match(format_dec_fixed5(10005), "10005"));
    assert(str_match(format_dec_fixed5(50000), "50000"));

    std::cout << "[PASS] test_fixed5_values\n";
}

void test_spaces_values() {
    assert(str_match(format_dec_spaces(0), "    0"));
    assert(str_match(format_dec_spaces(1), "    1"));
    assert(str_match(format_dec_spaces(9), "    9"));
    assert(str_match(format_dec_spaces(10), "   10"));
    assert(str_match(format_dec_spaces(42), "   42"));
    assert(str_match(format_dec_spaces(99), "   99"));
    assert(str_match(format_dec_spaces(100), "  100"));
    assert(str_match(format_dec_spaces(999), "  999"));
    assert(str_match(format_dec_spaces(1000), " 1000"));
    assert(str_match(format_dec_spaces(9999), " 9999"));
    assert(str_match(format_dec_spaces(10000), "10000"));
    assert(str_match(format_dec_spaces(65535), "65535"));

    // Interspersed zeros (zeros after first non-zero are NOT suppressed)
    assert(str_match(format_dec_spaces(105), "  105"));
    assert(str_match(format_dec_spaces(1005), " 1005"));
    assert(str_match(format_dec_spaces(10005), "10005"));
    assert(str_match(format_dec_spaces(50000), "50000"));

    std::cout << "[PASS] test_spaces_values\n";
}

void test_decimal_formatter_class() {
    assert(str_match(DecimalFormatter::format_fixed5(7), "00007"));
    assert(str_match(DecimalFormatter::format_fixed5(12345), "12345"));
    assert(str_match(DecimalFormatter::format_spaces(7), "    7"));
    assert(str_match(DecimalFormatter::format_spaces(12345), "12345"));

    std::cout << "[PASS] test_decimal_formatter_class\n";
}

void test_raw_memory_integration() {
    std::vector<std::uint8_t> ram(0x10000, 0x00);

    const std::size_t off = static_cast<std::size_t>(0x00FF192AU - 0x00FF0000U);

    // Initial check: RAM is zero
    // Format fixed5(123)
    format_dec_fixed5_raw(ram, 123);
    assert(read_decimal_buffer_raw(ram) == DecimalString5({'0', '0', '1', '2', '3'}));
    assert(str_match(read_decimal_buffer_raw(ram), "00123"));

    // Format spaces(123) -> "  123"
    format_dec_spaces_raw(ram, 123);
    assert(str_match(read_decimal_buffer_raw(ram), "  123"));

    // Format spaces(0) -> "    0"
    format_dec_spaces_raw(ram, 0);
    assert(str_match(read_decimal_buffer_raw(ram), "    0"));

    // Format fixed5(65535) -> "65535"
    format_dec_fixed5_raw(ram, 65535);
    assert(str_match(read_decimal_buffer_raw(ram), "65535"));

    // Verify raw bytes directly at off..off+4
    assert(ram[off + 0] == '6');
    assert(ram[off + 1] == '5');
    assert(ram[off + 2] == '5');
    assert(ram[off + 3] == '3');
    assert(ram[off + 4] == '5');

    std::cout << "[PASS] test_raw_memory_integration\n";
}

} // namespace

int main() {
    test_constants();
    test_fixed5_values();
    test_spaces_values();
    test_decimal_formatter_class();
    test_raw_memory_integration();

    std::cout << "\nAll M16.174 Decimal Formatter tests passed successfully!\n";
    return 0;
}
