#include "tools/hybrid/m68k_jump_exception_types.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed at " << __FILE__ << ":" << __LINE__ << ": " #cond << "\n"; \
            std::exit(1); \
        } \
    } while (false)

using namespace oasis::hybrid;

int main() {
    std::cout << "[M15.240] Starting Dynamic Jump and Exception Semantics Audit...\n";

    // 1. Odd address -> Address Error Exception (Vector 3)
    {
        const auto out1 = classify_jump_target(0x001235U);
        TEST_ASSERT(out1.classification == JumpTargetClassification::OriginalM68000Exception);
        TEST_ASSERT(out1.exception_vector == M68kExceptionVector::AddressError);
        TEST_ASSERT(!out1.yield_to_interpreter);

        // 32-bit odd address masked to 24-bit
        const auto out2 = classify_jump_target(0xFF001235U);
        TEST_ASSERT(out2.classification == JumpTargetClassification::OriginalM68000Exception);
        TEST_ASSERT(out2.exception_vector == M68kExceptionVector::AddressError);
        TEST_ASSERT(out2.resolved_pc == 0x00001235U);
        std::cout << "  [PASS] Odd address -> Address Error (Vector 3)\n";
    }

    // 2. Unmapped address -> Bus Error Exception (Vector 2)
    {
        // Address between ROM end (0x00400000) and Hardware space (0x00A00000)
        const auto out1 = classify_jump_target(0x00500000U);
        TEST_ASSERT(out1.classification == JumpTargetClassification::OriginalM68000Exception);
        TEST_ASSERT(out1.exception_vector == M68kExceptionVector::BusError);
        TEST_ASSERT(!out1.yield_to_interpreter);
        std::cout << "  [PASS] Unmapped address -> Bus Error (Vector 2)\n";
    }

    // 3. Hardware space (VDP / IO) -> UnsupportedBusMapping
    {
        const auto out1 = classify_jump_target(0x00C00004U);
        TEST_ASSERT(out1.classification == JumpTargetClassification::UnsupportedBusMapping);
        TEST_ASSERT(out1.exception_vector == M68kExceptionVector::BusError);
        TEST_ASSERT(!out1.yield_to_interpreter);
        std::cout << "  [PASS] Hardware MMIO execution -> UnsupportedBusMapping\n";
    }

    // 4. Known compiled targets
    {
        const auto out1 = classify_jump_target(0x00E682U);
        TEST_ASSERT(out1.classification == JumpTargetClassification::KnownCompiledTarget);
        TEST_ASSERT(out1.exception_vector == M68kExceptionVector::None);
        TEST_ASSERT(!out1.yield_to_interpreter);

        const auto out2 = classify_jump_target(0x0059A8U);
        TEST_ASSERT(out2.classification == JumpTargetClassification::KnownCompiledTarget);
        std::cout << "  [PASS] Known compiled targets correctly identified\n";
    }

    // 5. Valid uncompiled targets (safe continuation yield)
    {
        // Valid address in ROM (e.g. 0x007852)
        const auto out1 = classify_jump_target(0x007852U);
        TEST_ASSERT(out1.classification == JumpTargetClassification::ValidUncompiledTarget);
        TEST_ASSERT(out1.exception_vector == M68kExceptionVector::None);
        TEST_ASSERT(out1.yield_to_interpreter);
        TEST_ASSERT(out1.resolved_pc == 0x007852U);

        // Valid address in Work RAM (e.g. 0x00FF1000)
        const auto out2 = classify_jump_target(0x00FF1000U);
        TEST_ASSERT(out2.classification == JumpTargetClassification::ValidUncompiledTarget);
        TEST_ASSERT(out2.yield_to_interpreter);
        TEST_ASSERT(out2.resolved_pc == 0x00FF1000U);
        std::cout << "  [PASS] Valid uncompiled targets safely yield to interpreter without ContractFault\n";
    }

    std::cout << "[M15.240] Jump and Exception Semantics Audit: ALL TESTS PASSED!\n";
    return 0;
}
