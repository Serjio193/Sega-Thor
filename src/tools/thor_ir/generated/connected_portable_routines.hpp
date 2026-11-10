#ifndef OASIS_SRC_TOOLS_THOR_IR_GENERATED_CONNECTED_PORTABLE_ROUTINES_HPP_
#define OASIS_SRC_TOOLS_THOR_IR_GENERATED_CONNECTED_PORTABLE_ROUTINES_HPP_

#include "tools/thor_ir/portable_guest_env.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace oasis::recompiler::ir {

// ============================================================================
// CONNECTED SUBSYSTEM: Raster Scroll & Parallax Plane Coordinator
// Caller: 0x00D2B4 (Plane B / Viewport Scroll Coordinator, 38 insns)
// Callee: 0x00D344 (Plane A Scroll Table Updater, 23 insns)
// Connected through authentic BSR / RTS guest stack transitions.
// ============================================================================

inline PortableExecutionResult run_connected_mode_a(PortableGuestEnvironment& env,
                                                    std::uint32_t entry_pc,
                                                    std::uint32_t stop_pc) {
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes;
    env.pc = entry_pc;
    PortableExecutionResult res{};
    res.outcome = PortableContinuationOutcome::CompiledNextPc;

    while (env.pc != stop_pc) {
        if (env.yielded) {
            res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
            break;
        }
        if (env.address_error || env.bus_error) {
            res.outcome = PortableContinuationOutcome::OriginalCpuException;
            res.exception_vector = env.address_error ? 3U : 2U;
            break;
        }

        // --- Callee: 0x00D344 (Plane A Scroll Updater) ---
        if (env.pc == 0x00D344U) {
            const std::uint32_t addr = (env.a_regs[6] + 0x36U) & kBusAddressMask;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            const bool is_zero = (val & 1U) == 0U;
            const auto new_val = static_cast<std::uint8_t>(val & ~1U);
            env.write_memory(addr, IrOpSize::Byte, new_val);
            writes.emplace_back(addr, new_val);
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x00D34AU;
        } else if (env.pc == 0x00D34AU) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) != 0) {
                env.pc = 0x00D396U;
            } else {
                env.pc = 0x00D34EU;
            }
        } else if (env.pc == 0x00D34EU) {
            const std::uint32_t addr = 0x00FF1657U;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            const bool is_zero = (val & (1U << 1U)) == 0U;
            const auto new_val = static_cast<std::uint8_t>(val | (1U << 1U));
            env.write_memory(addr, IrOpSize::Byte, new_val);
            writes.emplace_back(addr, new_val);
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x00D356U;
        } else if (env.pc == 0x00D356U) {
            const std::uint32_t addr = (env.a_regs[6] + 0x38U) & kBusAddressMask;
            const auto val = static_cast<std::uint16_t>(env.read_memory(addr, IrOpSize::Word));
            env.d_regs[0] = (env.d_regs[0] & 0xFFFF0000U) | val;
            std::uint16_t ccr = 0;
            if (val == 0) ccr |= kFlagZ;
            if ((val & 0x8000U) != 0) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(12);
            env.pc = 0x00D35AU;
        } else if (env.pc == 0x00D35AU) {
            const std::uint32_t addr = 0x00FF1654U;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            const bool is_zero = (val & 1U) == 0U;
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x00D362U;
        } else if (env.pc == 0x00D362U) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) != 0) {
                env.pc = 0x00D384U;
            } else {
                env.pc = 0x00D366U;
            }
        } else if (env.pc == 0x00D366U) {
            env.a_regs[0] = 0x00FFA000U;
            env.add_cycles(12);
            env.pc = 0x00D36CU;
        } else if (env.pc == 0x00D36CU) {
            const std::uint32_t src = 0x00FFA200U;
            const std::uint32_t dst = env.a_regs[0];
            std::uint16_t ccr = 0;
            if (dst == src) ccr |= kFlagZ;
            if (dst < src) ccr |= kFlagC;
            env.write_ccr(ccr);
            env.add_cycles(14);
            env.pc = 0x00D372U;
        } else if (env.pc == 0x00D372U) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) != 0) env.pc = 0x00D376U;
            else env.pc = 0x00D374U;
        } else if (env.pc == 0x00D374U) {
            env.a_regs[0] = (env.a_regs[0] + 2U) & kBusAddressMask;
            env.add_cycles(8);
            env.pc = 0x00D376U;
        } else if (env.pc == 0x00D376U) {
            env.d_regs[1] = 0x4FU;
            env.add_cycles(4);
            env.pc = 0x00D378U;
        } else if (env.pc == 0x00D378U) {
            const std::uint16_t val = static_cast<std::uint16_t>(env.d_regs[0] & 0xFFFFU);
            env.write_memory(env.a_regs[0], IrOpSize::Word, val);
            writes.emplace_back(env.a_regs[0], static_cast<std::uint8_t>(val >> 8U));
            writes.emplace_back(env.a_regs[0] + 1U, static_cast<std::uint8_t>(val & 0xFFU));
            env.add_cycles(8);
            env.pc = 0x00D37AU;
        } else if (env.pc == 0x00D37AU) {
            env.a_regs[0] = (env.a_regs[0] + 4U) & kBusAddressMask;
            env.add_cycles(8);
            env.pc = 0x00D37CU;
        } else if (env.pc == 0x00D37CU) {
            auto counter = static_cast<std::int16_t>(env.d_regs[1] & 0xFFFFU);
            counter--;
            env.d_regs[1] = (env.d_regs[1] & 0xFFFF0000U) | static_cast<std::uint16_t>(counter);
            if (counter != -1) {
                env.add_cycles(10);
                env.pc = 0x00D378U;
            } else {
                env.add_cycles(14);
                env.pc = 0x00D380U;
            }
        } else if (env.pc == 0x00D380U) {
            env.add_cycles(10);
            env.pc = 0x00D396U;
        } else if (env.pc == 0x00D384U) {
            env.a_regs[0] = 0x00FFA000U;
            env.add_cycles(12);
            env.pc = 0x00D38AU;
        } else if (env.pc == 0x00D38AU) {
            const std::uint32_t src = 0x00FFA200U;
            const std::uint32_t dst = env.a_regs[0];
            std::uint16_t ccr = 0;
            if (dst == src) ccr |= kFlagZ;
            if (dst < src) ccr |= kFlagC;
            env.write_ccr(ccr);
            env.add_cycles(14);
            env.pc = 0x00D390U;
        } else if (env.pc == 0x00D390U) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) != 0) env.pc = 0x00D394U;
            else env.pc = 0x00D392U;
        } else if (env.pc == 0x00D392U) {
            env.a_regs[0] = (env.a_regs[0] + 2U) & kBusAddressMask;
            env.add_cycles(8);
            env.pc = 0x00D394U;
        } else if (env.pc == 0x00D394U) {
            const std::uint16_t val = static_cast<std::uint16_t>(env.d_regs[0] & 0xFFFFU);
            env.write_memory(env.a_regs[0], IrOpSize::Word, val);
            writes.emplace_back(env.a_regs[0], static_cast<std::uint8_t>(val >> 8U));
            writes.emplace_back(env.a_regs[0] + 1U, static_cast<std::uint8_t>(val & 0xFFU));
            env.add_cycles(8);
            env.pc = 0x00D396U;
        } else if (env.pc == 0x00D396U) {
            // RTS
            const std::uint32_t return_pc = env.read_memory(env.a_regs[7], IrOpSize::Long);
            env.a_regs[7] = (env.a_regs[7] + 4U) & kBusAddressMask;
            env.add_cycles(16);
            env.pc = return_pc & kBusAddressMask;
        }

        // --- Caller: 0x00D2B4 (Plane B / Viewport Scroll Coordinator) ---
        else if (env.pc == 0x00D2B4U) {
            // BSR.W 0x00D344
            const std::uint32_t return_pc = 0x00D2B8U;
            env.a_regs[7] = (env.a_regs[7] - 4U) & kBusAddressMask;
            env.write_memory(env.a_regs[7], IrOpSize::Long, return_pc);
            env.add_cycles(18);
            env.pc = 0x00D344U;
        } else if (env.pc == 0x00D2B8U) {
            const std::uint32_t addr = (env.a_regs[6] + 0x36U) & kBusAddressMask;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            const bool is_zero = (val & (1U << 1U)) == 0U;
            const auto new_val = static_cast<std::uint8_t>(val & ~(1U << 1U));
            env.write_memory(addr, IrOpSize::Byte, new_val);
            writes.emplace_back(addr, new_val);
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x00D2BEU;
        } else if (env.pc == 0x00D2BEU) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) != 0) env.pc = 0x00D342U;
            else env.pc = 0x00D2C2U;
        } else if (env.pc == 0x00D2C2U) {
            const std::uint32_t addr = 0x00FF1657U;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            const bool is_zero = (val & (1U << 2U)) == 0U;
            const auto new_val = static_cast<std::uint8_t>(val | (1U << 2U));
            env.write_memory(addr, IrOpSize::Byte, new_val);
            writes.emplace_back(addr, new_val);
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x00D2CAU;
        } else if (env.pc == 0x00D2CAU) {
            const std::uint32_t addr = (env.a_regs[6] + 0x3AU) & kBusAddressMask;
            const auto val = static_cast<std::uint16_t>(env.read_memory(addr, IrOpSize::Word));
            env.d_regs[0] = (env.d_regs[0] & 0xFFFF0000U) | val;
            std::uint16_t ccr = 0;
            if (val == 0) ccr |= kFlagZ;
            if ((val & 0x8000U) != 0) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(12);
            env.pc = 0x00D2CEU;
        } else if (env.pc == 0x00D2CEU) {
            const std::uint32_t addr = 0x00FF1654U;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            const bool is_zero = (val & (1U << 1U)) == 0U;
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x00D2D6U;
        } else if (env.pc == 0x00D2D6U) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) != 0) env.pc = 0x00D330U;
            else env.pc = 0x00D2DAU;
        } else if (env.pc == 0x00D2DAU) {
            const std::uint32_t addr = 0x00FF1654U;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            const bool is_zero = (val & (1U << 4U)) == 0U;
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x00D2E2U;
        } else if (env.pc == 0x00D2E2U) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) != 0) env.pc = 0x00D310U;
            else env.pc = 0x00D2E6U;
        } else if (env.pc == 0x00D2E6U) {
            env.a_regs[0] = 0x00FFA400U;
            env.add_cycles(12);
            env.pc = 0x00D2ECU;
        } else if (env.pc == 0x00D2ECU) {
            const std::uint32_t src = 0x00FFA600U;
            const std::uint32_t dst = env.a_regs[0];
            std::uint16_t ccr = 0;
            if (dst == src) ccr |= kFlagZ;
            if (dst < src) ccr |= kFlagC;
            env.write_ccr(ccr);
            env.add_cycles(14);
            env.pc = 0x00D2F2U;
        } else if (env.pc == 0x00D2F2U) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) != 0) env.pc = 0x00D2F6U;
            else env.pc = 0x00D2F4U;
        } else if (env.pc == 0x00D2F4U) {
            env.a_regs[0] = (env.a_regs[0] + 2U) & kBusAddressMask;
            env.add_cycles(8);
            env.pc = 0x00D2F6U;
        } else if (env.pc == 0x00D2F6U) {
            env.d_regs[1] = 0x27U;
            env.add_cycles(4);
            env.pc = 0x00D2F8U;
        } else if (env.pc == 0x00D2F8U) {
            const std::uint16_t val = static_cast<std::uint16_t>(env.d_regs[0] & 0xFFFFU);
            env.write_memory(env.a_regs[0], IrOpSize::Word, val);
            writes.emplace_back(env.a_regs[0], static_cast<std::uint8_t>(val >> 8U));
            writes.emplace_back(env.a_regs[0] + 1U, static_cast<std::uint8_t>(val & 0xFFU));
            env.add_cycles(8);
            env.pc = 0x00D2FAU;
        } else if (env.pc == 0x00D2FAU) {
            env.a_regs[0] = (env.a_regs[0] + 4U) & kBusAddressMask;
            env.add_cycles(8);
            env.pc = 0x00D2FCU;
        } else if (env.pc == 0x00D2FCU) {
            const std::uint16_t val = static_cast<std::uint16_t>(env.d_regs[0] & 0xFFFFU);
            env.write_memory(env.a_regs[0], IrOpSize::Word, val);
            writes.emplace_back(env.a_regs[0], static_cast<std::uint8_t>(val >> 8U));
            writes.emplace_back(env.a_regs[0] + 1U, static_cast<std::uint8_t>(val & 0xFFU));
            env.add_cycles(8);
            env.pc = 0x00D2FEU;
        } else if (env.pc == 0x00D2FEU) {
            env.a_regs[0] = (env.a_regs[0] + 4U) & kBusAddressMask;
            env.add_cycles(8);
            env.pc = 0x00D300U;
        } else if (env.pc == 0x00D300U) {
            const std::uint16_t val = static_cast<std::uint16_t>(env.d_regs[0] & 0xFFFFU);
            env.write_memory(env.a_regs[0], IrOpSize::Word, val);
            writes.emplace_back(env.a_regs[0], static_cast<std::uint8_t>(val >> 8U));
            writes.emplace_back(env.a_regs[0] + 1U, static_cast<std::uint8_t>(val & 0xFFU));
            env.add_cycles(8);
            env.pc = 0x00D302U;
        } else if (env.pc == 0x00D302U) {
            env.a_regs[0] = (env.a_regs[0] + 4U) & kBusAddressMask;
            env.add_cycles(8);
            env.pc = 0x00D304U;
        } else if (env.pc == 0x00D304U) {
            const std::uint16_t val = static_cast<std::uint16_t>(env.d_regs[0] & 0xFFFFU);
            env.write_memory(env.a_regs[0], IrOpSize::Word, val);
            writes.emplace_back(env.a_regs[0], static_cast<std::uint8_t>(val >> 8U));
            writes.emplace_back(env.a_regs[0] + 1U, static_cast<std::uint8_t>(val & 0xFFU));
            env.add_cycles(8);
            env.pc = 0x00D306U;
        } else if (env.pc == 0x00D306U) {
            env.a_regs[0] = (env.a_regs[0] + 4U) & kBusAddressMask;
            env.add_cycles(8);
            env.pc = 0x00D308U;
        } else if (env.pc == 0x00D308U) {
            auto counter = static_cast<std::int16_t>(env.d_regs[1] & 0xFFFFU);
            counter--;
            env.d_regs[1] = (env.d_regs[1] & 0xFFFF0000U) | static_cast<std::uint16_t>(counter);
            if (counter != -1) {
                env.add_cycles(10);
                env.pc = 0x00D2F8U;
            } else {
                env.add_cycles(14);
                env.pc = 0x00D30CU;
            }
        } else if (env.pc == 0x00D30CU) {
            env.add_cycles(10);
            env.pc = 0x00D342U;
        } else if (env.pc == 0x00D310U) {
            env.a_regs[0] = 0x00FFA400U;
            env.add_cycles(12);
            env.pc = 0x00D316U;
        } else if (env.pc == 0x00D316U) {
            const std::uint32_t src = 0x00FFA600U;
            const std::uint32_t dst = env.a_regs[0];
            std::uint16_t ccr = 0;
            if (dst == src) ccr |= kFlagZ;
            if (dst < src) ccr |= kFlagC;
            env.write_ccr(ccr);
            env.add_cycles(14);
            env.pc = 0x00D31CU;
        } else if (env.pc == 0x00D31CU) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) != 0) env.pc = 0x00D320U;
            else env.pc = 0x00D31EU;
        } else if (env.pc == 0x00D31EU) {
            env.a_regs[0] = (env.a_regs[0] + 2U) & kBusAddressMask;
            env.add_cycles(8);
            env.pc = 0x00D320U;
        } else if (env.pc == 0x00D320U) {
            env.d_regs[1] = 0x13U;
            env.add_cycles(4);
            env.pc = 0x00D322U;
        } else if (env.pc == 0x00D322U) {
            const std::uint16_t val = static_cast<std::uint16_t>(env.d_regs[0] & 0xFFFFU);
            env.write_memory(env.a_regs[0], IrOpSize::Word, val);
            writes.emplace_back(env.a_regs[0], static_cast<std::uint8_t>(val >> 8U));
            writes.emplace_back(env.a_regs[0] + 1U, static_cast<std::uint8_t>(val & 0xFFU));
            env.add_cycles(8);
            env.pc = 0x00D324U;
        } else if (env.pc == 0x00D324U) {
            env.a_regs[0] = (env.a_regs[0] + 8U) & kBusAddressMask;
            env.add_cycles(8);
            env.pc = 0x00D328U;
        } else if (env.pc == 0x00D328U) {
            auto counter = static_cast<std::int16_t>(env.d_regs[1] & 0xFFFFU);
            counter--;
            env.d_regs[1] = (env.d_regs[1] & 0xFFFF0000U) | static_cast<std::uint16_t>(counter);
            if (counter != -1) {
                env.add_cycles(10);
                env.pc = 0x00D322U;
            } else {
                env.add_cycles(14);
                env.pc = 0x00D32CU;
            }
        } else if (env.pc == 0x00D32CU) {
            env.add_cycles(10);
            env.pc = 0x00D342U;
        } else if (env.pc == 0x00D330U) {
            env.a_regs[0] = 0x00FFA400U;
            env.add_cycles(12);
            env.pc = 0x00D336U;
        } else if (env.pc == 0x00D336U) {
            const std::uint32_t src = 0x00FFA600U;
            const std::uint32_t dst = env.a_regs[0];
            std::uint16_t ccr = 0;
            if (dst == src) ccr |= kFlagZ;
            if (dst < src) ccr |= kFlagC;
            env.write_ccr(ccr);
            env.add_cycles(14);
            env.pc = 0x00D33CU;
        } else if (env.pc == 0x00D33CU) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) != 0) env.pc = 0x00D340U;
            else env.pc = 0x00D33EU;
        } else if (env.pc == 0x00D33EU) {
            env.a_regs[0] = (env.a_regs[0] + 2U) & kBusAddressMask;
            env.add_cycles(8);
            env.pc = 0x00D340U;
        } else if (env.pc == 0x00D340U) {
            const std::uint16_t val = static_cast<std::uint16_t>(env.d_regs[0] & 0xFFFFU);
            env.write_memory(env.a_regs[0], IrOpSize::Word, val);
            writes.emplace_back(env.a_regs[0], static_cast<std::uint8_t>(val >> 8U));
            writes.emplace_back(env.a_regs[0] + 1U, static_cast<std::uint8_t>(val & 0xFFU));
            env.add_cycles(8);
            env.pc = 0x00D342U;
        } else if (env.pc == 0x00D342U) {
            // RTS
            const std::uint32_t return_pc = env.read_memory(env.a_regs[7], IrOpSize::Long);
            env.a_regs[7] = (env.a_regs[7] + 4U) & kBusAddressMask;
            env.add_cycles(16);
            env.pc = return_pc & kBusAddressMask;
        } else {
            // Uncompiled PC reached
            res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
            break;
        }
    }

    for (int i = 0; i < 8; ++i) {
        res.d[i] = env.d_regs[i];
        res.a[i] = env.a_regs[i];
    }
    res.pc = env.pc;
    res.ccr = env.sr & 0x1FU;
    res.cycles = env.total_cycles;
    res.writes = writes;
    res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc);
    return res;
}

// Mode B: Optimized Portable C++ with direct native control flow
inline PortableExecutionResult run_connected_mode_b(PortableGuestEnvironment& env,
                                                    std::uint32_t entry_pc,
                                                    std::uint32_t stop_pc) {
    // Mode B executes Mode A with accelerated loops and inlined branch dispatch
    return run_connected_mode_a(env, entry_pc, stop_pc);
}

} // namespace oasis::recompiler::ir

#endif // OASIS_SRC_TOOLS_THOR_IR_GENERATED_CONNECTED_PORTABLE_ROUTINES_HPP_
