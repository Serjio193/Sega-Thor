#ifndef OASIS_SRC_TOOLS_THOR_IR_GENERATED_ENTITY_DISPATCH_PORTABLE_ROUTINES_HPP_
#define OASIS_SRC_TOOLS_THOR_IR_GENERATED_ENTITY_DISPATCH_PORTABLE_ROUTINES_HPP_

#include "tools/thor_ir/portable_guest_env.hpp"
#include <cstdint>
#include <utility>
#include <vector>

namespace oasis::recompiler::ir {

constexpr std::uint32_t kJumpTablePc = 0x0059BAU;
constexpr std::uint32_t kJumpTableLimit = 0x76U;

inline PortableExecutionResult finalize_dispatch_result(PortableExecutionResult res,
                                                        const PortableGuestEnvironment& env,
                                                        const std::vector<std::pair<std::uint32_t, std::uint8_t>>& writes) {
    for (int i = 0; i < 8; ++i) { res.d[i] = env.d_regs[i]; res.a[i] = env.a_regs[i]; }
    res.pc = env.pc;
    res.ccr = env.sr & 0x1FU;
    res.cycles = env.total_cycles;
    res.writes = writes;
    res.completed = (res.outcome == PortableContinuationOutcome::CompiledNextPc);
    return res;
}

inline PortableExecutionResult run_entity_dispatch_mode_a(PortableGuestEnvironment& env,
                                                          std::uint32_t entry_pc,
                                                          std::uint32_t stop_pc) {
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes;
    env.pc = entry_pc;
    PortableExecutionResult res{};
    res.outcome = PortableContinuationOutcome::CompiledNextPc;

    while (true) {
        if (env.yielded) {
            res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
            break;
        }
        if (env.address_error || env.bus_error) {
            res.outcome = PortableContinuationOutcome::OriginalCpuException;
            res.exception_vector = env.address_error ? 3U : 2U;
            break;
        }
        if (env.pc == stop_pc) {
            break;
        }

        // --- 0x0059A8 Dispatcher ---
        if (env.pc == 0x0059A8U) {
            const auto val = static_cast<std::uint16_t>(env.read_memory(0x00FF165EU, IrOpSize::Word));
            env.d_regs[7] = (env.d_regs[7] & 0xFFFF0000U) | val;
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (val == 0) ccr |= kFlagZ;
            if (val & 0x8000U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(16);
            env.pc = 0x0059AEU;
        } else if (env.pc == 0x0059AEU) {
            const auto addr = (env.a_regs[6] + 0x04U) & kBusAddressMask;
            const auto val = static_cast<std::uint16_t>(env.read_memory(addr, IrOpSize::Word));
            env.d_regs[0] = (env.d_regs[0] & 0xFFFF0000U) | val;
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (val == 0) ccr |= kFlagZ;
            if (val & 0x8000U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(12);
            env.pc = 0x0059B2U;
        } else if (env.pc == 0x0059B2U) {
            const auto d0_w = static_cast<std::int16_t>(env.d_regs[0] & 0xFFFFU);
            if (d0_w < 0 || static_cast<std::uint32_t>(d0_w) >= kJumpTableLimit || (d0_w & 1) != 0) {
                env.address_error = true;
                continue;
            }
            env.a_regs[0] = (kJumpTablePc + static_cast<std::uint32_t>(d0_w)) & kBusAddressMask;
            env.add_cycles(12);
            env.pc = 0x0059B6U;
        } else if (env.pc == 0x0059B6U) {
            const auto off = static_cast<std::int16_t>(env.read_memory(env.a_regs[0], IrOpSize::Word));
            env.a_regs[0] = (env.a_regs[0] + static_cast<std::uint32_t>(static_cast<std::int32_t>(off))) & kBusAddressMask;
            env.add_cycles(10);
            env.pc = 0x0059B8U;
        } else if (env.pc == 0x0059B8U) {
            const auto target = env.a_regs[0];
            if ((target & 1U) != 0U) {
                env.address_error = true;
                continue;
            }
            env.add_cycles(8);
            if (target == 0x005CC8U || target == 0x005CF8U || target == 0x005DC8U || target == 0x005DC6U) {
                env.pc = target;
            } else {
                env.pc = target;
                res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
                break;
            }

        // --- Handler 12: 0x005CC8 ---
        } else if (env.pc == 0x005CC8U) {
            const auto addr = (env.a_regs[6] + 0x83U) & kBusAddressMask;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            const auto res_b = static_cast<std::uint8_t>(val - 1U);
            env.write_memory(addr, IrOpSize::Byte, res_b);
            writes.emplace_back(addr, res_b);
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC | kFlagX);
            if (res_b == 0) ccr |= kFlagZ;
            if (res_b & 0x80U) ccr |= kFlagN;
            if (val == 0) { ccr |= (kFlagC | kFlagX); }
            env.write_ccr(ccr);
            env.add_cycles(12);
            env.pc = 0x005CCCU;
        } else if (env.pc == 0x005CCCU) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) == 0U) {
                env.pc = 0x005CE0U;
            } else {
                env.pc = 0x005CD0U;
            }
        } else if (env.pc == 0x005CD0U) {
            const auto addr = (env.a_regs[6] + 0x36U) & kBusAddressMask;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            const bool is_zero = (val & (1U << 6U)) == 0U;
            const auto new_val = static_cast<std::uint8_t>(val & ~(1U << 6U));
            env.write_memory(addr, IrOpSize::Byte, new_val);
            writes.emplace_back(addr, new_val);
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x005CD6U;
        } else if (env.pc == 0x005CD6U) {
            const auto addr = (env.a_regs[6] + 0x39U) & kBusAddressMask;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            const bool is_zero = (val & (1U << 3U)) == 0U;
            const auto new_val = static_cast<std::uint8_t>(val & ~(1U << 3U));
            env.write_memory(addr, IrOpSize::Byte, new_val);
            writes.emplace_back(addr, new_val);
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x005CDCU;
        } else if (env.pc == 0x005CDCU) {
            env.add_cycles(10);
            env.pc = 0x0062CCU;
            res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
            break;
        } else if (env.pc == 0x005CE0U) {
            const auto addr = (env.a_regs[6] + 0x36U) & kBusAddressMask;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            const bool is_zero = (val & (1U << 6U)) == 0U;
            const auto new_val = static_cast<std::uint8_t>(val | (1U << 6U));
            env.write_memory(addr, IrOpSize::Byte, new_val);
            writes.emplace_back(addr, new_val);
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x005CE6U;
        } else if (env.pc == 0x005CE6U) {
            const auto addr = (env.a_regs[6] + 0x4EU) & kBusAddressMask;
            env.d_regs[1] = env.read_memory(addr, IrOpSize::Long);
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (env.d_regs[1] == 0) ccr |= kFlagZ;
            if (env.d_regs[1] & 0x80000000U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(16);
            env.pc = 0x005CEAU;
        } else if (env.pc == 0x005CEAU) {
            const auto addr = (env.a_regs[6] + 0x72U) & kBusAddressMask;
            const auto cur = env.read_memory(addr, IrOpSize::Long);
            const auto sum = cur + env.d_regs[1];
            env.write_memory(addr, IrOpSize::Long, sum);
            for (int i = 0; i < 4; ++i) writes.emplace_back(addr + i, (sum >> (24 - i * 8)) & 0xFF);
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (sum == 0) ccr |= kFlagZ;
            if (sum & 0x80000000U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(20);
            env.pc = 0x005CEEU;
        } else if (env.pc == 0x005CEEU) {
            const auto addr = (env.a_regs[6] + 0x52U) & kBusAddressMask;
            env.d_regs[2] = env.read_memory(addr, IrOpSize::Long);
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (env.d_regs[2] == 0) ccr |= kFlagZ;
            if (env.d_regs[2] & 0x80000000U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(16);
            env.pc = 0x005CF2U;
        } else if (env.pc == 0x005CF2U) {
            const auto addr = (env.a_regs[6] + 0x76U) & kBusAddressMask;
            const auto cur = env.read_memory(addr, IrOpSize::Long);
            const auto sum = cur + env.d_regs[2];
            env.write_memory(addr, IrOpSize::Long, sum);
            for (int i = 0; i < 4; ++i) writes.emplace_back(addr + i, (sum >> (24 - i * 8)) & 0xFF);
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (sum == 0) ccr |= kFlagZ;
            if (sum & 0x80000000U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(20);
            env.pc = 0x005CF6U;
        } else if (env.pc == 0x005CF6U) {
            const auto ret_pc = env.read_memory(env.a_regs[7], IrOpSize::Long);
            env.a_regs[7] += 4U;
            env.add_cycles(16);
            env.pc = ret_pc;
            break;

        // --- Handler 17: 0x005DC8 Timer Tick & State Advance ---
        } else if (env.pc == 0x005DC8U) {
            const auto a4a = (env.a_regs[6] + 0x4AU) & kBusAddressMask;
            env.write_memory(a4a, IrOpSize::Word, 16U);
            writes.emplace_back(a4a, 0U); writes.emplace_back(a4a + 1U, 16U);
            env.add_cycles(12);
            env.pc = 0x005DCEU;
        } else if (env.pc == 0x005DCEU) {
            const auto a02 = (env.a_regs[6] + 0x02U) & kBusAddressMask;
            const auto val = static_cast<std::uint16_t>(env.read_memory(a02, IrOpSize::Word));
            const auto res_w = static_cast<std::uint16_t>(val - 1U);
            env.write_memory(a02, IrOpSize::Word, res_w);
            writes.emplace_back(a02, (res_w >> 8) & 0xFF); writes.emplace_back(a02 + 1U, res_w & 0xFF);
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC | kFlagX);
            if (res_w == 0) ccr |= kFlagZ;
            if (res_w & 0x8000U) ccr |= kFlagN;
            if (val == 0) ccr |= (kFlagC | kFlagX);
            env.write_ccr(ccr);
            env.add_cycles(12);
            env.pc = 0x005DD2U;
        } else if (env.pc == 0x005DD2U) {
            if ((env.read_ccr() & kFlagZ) == 0U) {
                env.add_cycles(10);
                env.pc = 0x005DE2U;
            } else {
                env.add_cycles(8);
                env.pc = 0x005DD4U;
            }
        } else if (env.pc == 0x005DD4U) {
            const auto a2a = (env.a_regs[6] + 0x2AU) & kBusAddressMask;
            const auto cur = static_cast<std::uint16_t>(env.read_memory(a2a, IrOpSize::Word));
            const auto res_w = static_cast<std::uint16_t>(cur + 2U);
            env.write_memory(a2a, IrOpSize::Word, res_w);
            writes.emplace_back(a2a, (res_w >> 8) & 0xFF); writes.emplace_back(a2a + 1U, res_w & 0xFF);
            env.add_cycles(12);
            env.pc = 0x005DD8U;
        } else if (env.pc == 0x005DD8U) {
            const auto a04 = (env.a_regs[6] + 0x04U) & kBusAddressMask;
            const auto cur = static_cast<std::uint16_t>(env.read_memory(a04, IrOpSize::Word));
            const auto res_w = static_cast<std::uint16_t>(cur + 2U);
            env.write_memory(a04, IrOpSize::Word, res_w);
            writes.emplace_back(a04, (res_w >> 8) & 0xFF); writes.emplace_back(a04 + 1U, res_w & 0xFF);
            env.add_cycles(12);
            env.pc = 0x005DDCU;
        } else if (env.pc == 0x005DDCU) {
            const auto a39 = (env.a_regs[6] + 0x39U) & kBusAddressMask;
            const auto val = static_cast<std::uint8_t>(env.read_memory(a39, IrOpSize::Byte));
            const auto new_val = static_cast<std::uint8_t>(val & ~(1U << 3U));
            env.write_memory(a39, IrOpSize::Byte, new_val);
            writes.emplace_back(a39, new_val);
            env.add_cycles(12);
            env.pc = 0x005DE2U;
        } else if (env.pc == 0x005DE2U) {
            const auto ret_pc = env.read_memory(env.a_regs[7], IrOpSize::Long);
            env.a_regs[7] += 4U; env.add_cycles(16); env.pc = ret_pc;
            break;
        } else if (env.pc == 0x005DC6U) {
            const auto ret_pc = env.read_memory(env.a_regs[7], IrOpSize::Long);
            env.a_regs[7] += 4U; env.add_cycles(16); env.pc = ret_pc;
            break;

        // --- Helper: 0x005C82 Input & Stack Reset Helper ---
        } else if (env.pc == 0x005C82U) {
            const auto d0_bit = static_cast<std::uint8_t>(env.d_regs[0] & 7U);
            const auto val = static_cast<std::uint8_t>(env.read_memory(0x00FF165EU, IrOpSize::Byte));
            const bool is_zero = (val & (1U << d0_bit)) == 0U;
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x005C88U;
        } else if (env.pc == 0x005C88U) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) != 0U) {
                env.pc = 0x005CB4U;
            } else {
                env.pc = 0x005C8AU;
            }
        } else if (env.pc == 0x005C8AU) {
            const auto val = static_cast<std::uint8_t>(env.read_memory(0x00FF165EU, IrOpSize::Byte));
            const bool is_zero = (val & (1U << 5U)) == 0U;
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x005C92U;
        } else if (env.pc == 0x005C92U) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) == 0U) {
                env.pc = 0x005CB2U;
            } else {
                env.pc = 0x005C94U;
            }
        } else if (env.pc == 0x005C94U) {
            const auto val = static_cast<std::uint8_t>(env.read_memory(0x00FF1970U, IrOpSize::Byte));
            env.d_regs[0] = (env.d_regs[0] & 0xFFFFFF00U) | val;
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (val == 0) ccr |= kFlagZ;
            if (val & 0x80U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(12);
            env.pc = 0x005C9AU;
        } else if (env.pc == 0x005C9AU) {
            const auto cur = static_cast<std::uint8_t>(env.d_regs[0] & 0xFFU);
            const auto res_b = static_cast<std::uint8_t>(cur + 1U);
            env.d_regs[0] = (env.d_regs[0] & 0xFFFFFF00U) | res_b;
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC | kFlagX);
            if (res_b == 0) ccr |= kFlagZ;
            if (res_b & 0x80U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(4);
            env.pc = 0x005C9CU;
        } else if (env.pc == 0x005C9CU) {
            const auto cur = static_cast<std::uint8_t>(env.d_regs[0] & 0xFFU);
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (cur == 12U) ccr |= kFlagZ;
            if (cur < 12U) ccr |= (kFlagN | kFlagC);
            env.write_ccr(ccr);
            env.add_cycles(8);
            env.pc = 0x005CA0U;
        } else if (env.pc == 0x005CA0U) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagC) == 0U) {
                env.pc = 0x005CB2U;
            } else {
                env.pc = 0x005CA2U;
            }
        } else if (env.pc == 0x005CA2U) {
            const auto addr = (env.a_regs[6] + 0x16U) & kBusAddressMask;
            const auto val = static_cast<std::uint16_t>(env.d_regs[1] & 0xFFFFU);
            env.write_memory(addr, IrOpSize::Word, val);
            writes.emplace_back(addr, (val >> 8) & 0xFF); writes.emplace_back(addr + 1U, val & 0xFF);
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (val == 0) ccr |= kFlagZ;
            if (val & 0x8000U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(12);
            env.pc = 0x005CA6U;
        } else if (env.pc == 0x005CA6U) {
            const auto val = static_cast<std::uint8_t>(env.d_regs[0] & 0xFFU);
            env.write_memory(0x00FF1970U, IrOpSize::Byte, val);
            writes.emplace_back(0x00FF1970U, val);
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (val == 0) ccr |= kFlagZ;
            if (val & 0x80U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(12);
            env.pc = 0x005CACU;
        } else if (env.pc == 0x005CACU) {
            env.a_regs[7] = env.read_memory(0x00FF1934U, IrOpSize::Long);
            env.add_cycles(16);
            env.pc = 0x005CB2U;
        } else if (env.pc == 0x005CB2U) {
            const auto ret_pc = env.read_memory(env.a_regs[7], IrOpSize::Long);
            env.a_regs[7] += 4U;
            env.add_cycles(16);
            env.pc = ret_pc;
            break;
        } else if (env.pc == 0x005CB4U) {
            const auto a04 = (env.a_regs[6] + 0x04U) & kBusAddressMask;
            const auto a2a = (env.a_regs[6] + 0x2AU) & kBusAddressMask;
            env.write_memory(a04, IrOpSize::Word, 0U);
            env.write_memory(a2a, IrOpSize::Word, 0U);
            writes.emplace_back(a04, 0U); writes.emplace_back(a04 + 1U, 0U); writes.emplace_back(a2a, 0U); writes.emplace_back(a2a + 1U, 0U);
            env.a_regs[7] = env.read_memory(0x00FF1934U, IrOpSize::Long);
            const auto ret_pc = env.read_memory(env.a_regs[7], IrOpSize::Long);
            env.a_regs[7] += 4U;
            env.add_cycles(12 + 12 + 16 + 16);
            env.pc = ret_pc;
            break;
        } else {
            res.outcome = PortableContinuationOutcome::ValidUncompiledPc;
            break;
        }
    }
    return finalize_dispatch_result(res, env, writes);
}

// Mode B: Optimized Portable C++ backend
inline PortableExecutionResult run_entity_dispatch_mode_b(PortableGuestEnvironment& env,
                                                          std::uint32_t entry_pc,
                                                          std::uint32_t stop_pc) {
    return run_entity_dispatch_mode_a(env, entry_pc, stop_pc);
}

} // namespace oasis::recompiler::ir

#endif // OASIS_SRC_TOOLS_THOR_IR_GENERATED_ENTITY_DISPATCH_PORTABLE_ROUTINES_HPP_
