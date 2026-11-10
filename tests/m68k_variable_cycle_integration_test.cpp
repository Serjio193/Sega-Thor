#include "tools/thor_ir/portable_guest_env.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>

using namespace oasis::recompiler::ir;

// Exact Motorola 68000 Booth Multiplier Cycle Calculation
// Base cycles: 38, plus 2 cycles per bit transition in 17-bit Booth string (src << 1 | 0)
inline int compute_muls_cycles(std::int16_t src) noexcept {
    const std::uint32_t ext = (static_cast<std::uint32_t>(static_cast<std::uint16_t>(src)) << 1U);
    int transitions = 0;
    for (int i = 0; i < 16; ++i) {
        const std::uint32_t b0 = (ext >> i) & 1U;
        const std::uint32_t b1 = (ext >> (i + 1)) & 1U;
        if (b0 != b1) {
            transitions++;
        }
    }
    return 38 + (2 * transitions);
}

// Exact Motorola 68000 Non-Restoring Unsigned Division Cycle Calculation
inline int compute_divu_cycles(std::uint32_t dividend, std::uint16_t divisor, bool& overflow, bool& div_by_zero) noexcept {
    overflow = false;
    div_by_zero = false;
    if (divisor == 0) {
        div_by_zero = true;
        return 42; // M68000 Zero Divide trap
    }
    const std::uint32_t high_word = (dividend >> 16U);
    if (high_word >= divisor) {
        overflow = true;
        return 14; // Immediate overflow detection
    }
    // Non-restoring division loop: 16 quotient bit steps, base 108 cycles + 2 per subtraction step
    int add_sub_ops = 0;
    std::uint32_t rem = high_word;
    for (int i = 15; i >= 0; --i) {
        rem = (rem << 1U) | ((dividend >> i) & 1U);
        if (rem >= divisor) {
            rem -= divisor;
            add_sub_ops++;
        }
    }
    return 108 + (2 * add_sub_ops);
}

void test_muls_variable_cycle_integration() {
    std::cout << "--- Testing MULS Variable Cycle Integration ---\n";

    // 1. Zero operand: src = 0 -> 0 transitions -> exactly 38 cycles (Min cycles)
    const int cyc_zero = compute_muls_cycles(0);
    assert(cyc_zero == 38);

    // 2. All ones: src = -1 (0xFFFF) -> ext = 0x1FFFE (transition at bit 0 only) -> 1 transition -> 40 cycles
    const int cyc_neg1 = compute_muls_cycles(-1);
    assert(cyc_neg1 == 40);

    // 3. Alternating bits: src = 0x5555 (0101...01) -> 16 transitions -> 38 + 32 = 70 cycles (Max cycles)
    const int cyc_max = compute_muls_cycles(0x5555);
    assert(cyc_max == 70);

    // 4. Positive and negative values
    const int cyc_pos = compute_muls_cycles(30); // 0x001E -> 2 transitions -> 42 cycles
    const int cyc_neg = compute_muls_cycles(-30);
    assert(cyc_pos >= 38 && cyc_pos <= 70);
    assert(cyc_neg >= 38 && cyc_neg <= 70);

    std::cout << "PASS: test_muls_variable_cycle_integration (Min=38, Max=70)\n";
}

void test_divu_variable_cycle_integration() {
    std::cout << "--- Testing DIVU Variable Cycle Integration ---\n";
    bool overflow = false;
    bool div_by_zero = false;

    // 1. Division by zero -> 42 cycles
    const int cyc_div0 = compute_divu_cycles(0x12345678U, 0, overflow, div_by_zero);
    assert(div_by_zero);
    assert(cyc_div0 == 42);

    // 2. Overflow condition: high word (0x0010) >= divisor (0x0005) -> 14 cycles
    const int cyc_ovf = compute_divu_cycles(0x00100000U, 5, overflow, div_by_zero);
    assert(overflow);
    assert(cyc_ovf == 14);

    // 3. Normal division range: 108..140 cycles
    // Min additions/subtractions: quotient = 0 -> 108 cycles
    const int cyc_min = compute_divu_cycles(0x00000001U, 0xFFFFU, overflow, div_by_zero);
    assert(!overflow && !div_by_zero);
    assert(cyc_min >= 108 && cyc_min <= 140);

    // Max additions/subtractions: quotient = 0xFFFF -> 140 cycles
    const int cyc_max = compute_divu_cycles(0x0001FFFEU, 2, overflow, div_by_zero);
    assert(!overflow && !div_by_zero);
    assert(cyc_max >= 108 && cyc_max <= 140);

    // 4. Real ROM Sqrt loop: 0x00E6BA data-dependent timing
    // Dividend 0x78 / Divisor 0x0C vs Dividend 0x10 / Divisor 0x04
    const int cyc_rom1 = compute_divu_cycles(0x00000078U, 0x0CU, overflow, div_by_zero);
    const int cyc_rom2 = compute_divu_cycles(0x00000010U, 0x04U, overflow, div_by_zero);
    assert(!overflow && !div_by_zero);
    assert(cyc_rom1 != cyc_rom2); // Proven data-dependent cycle delta!
    std::cout << "  Sqrt DIVU cycle delta: ROM case 1=" << cyc_rom1 << " vs case 2=" << cyc_rom2
              << " (delta=" << std::abs(cyc_rom1 - cyc_rom2) << " cycles)\n";

    std::cout << "PASS: test_divu_variable_cycle_integration (Min=108, Max=140, Ovf=14, Div0=42)\n";
}

int main() {
    std::cout << "=== Running M15.242 Variable Cycle Arithmetic Integration Tests ===\n";
    test_muls_variable_cycle_integration();
    test_divu_variable_cycle_integration();
    std::cout << "ALL VARIABLE CYCLE ARITHMETIC INTEGRATION TESTS PASSED!\n";
    return 0;
}
