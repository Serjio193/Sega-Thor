#ifndef OASIS_SRC_TOOLS_THOR_IR_GENERATED_PORTABLE_ROUTINES_HPP_
#define OASIS_SRC_TOOLS_THOR_IR_GENERATED_PORTABLE_ROUTINES_HPP_

#include "tools/thor_ir/portable_guest_env.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace oasis::recompiler::ir {

struct PortableExecutionResult {
    std::uint32_t d[8]{};
    std::uint32_t a[8]{};
    std::uint32_t pc{0};
    std::uint16_t ccr{0};
    int cycles{0};
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes;
    bool completed{false};
};

inline PortableExecutionResult extract_result(const PortableGuestEnvironment& env,
                                             const std::vector<std::pair<std::uint32_t, std::uint8_t>>& writes) {
    PortableExecutionResult res{};
    for (int i = 0; i < 8; ++i) {
        res.d[i] = env.d_regs[i];
        res.a[i] = env.a_regs[i];
    }
    res.pc = env.pc;
    res.ccr = env.sr & 0x1FU;
    res.cycles = env.total_cycles;
    res.writes = writes;
    return res;
}

// ============================================================================
// ROUTINE 1: 0x00E682 Camera Viewport Coordinator
// ============================================================================

inline PortableExecutionResult run_mode_a_00e682(PortableGuestEnvironment& env) {
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes;
    env.pc = 0x00E682U;
    env.total_cycles = 0;

    while (env.pc < 0x00E6AAU) {
        env.on_sync_boundary(env.pc);
        if (env.pc == 0x00E682U) {
            const std::uint32_t addr = (env.a_regs[6] + 0x36U) & kBusAddressMask;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            const bool is_zero = (val & (1U << 6U)) == 0U;
            const auto new_val = static_cast<std::uint8_t>(val & ~(1U << 6U));
            env.write_memory(addr, IrOpSize::Byte, new_val);
            writes.emplace_back(addr, new_val);
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x00E688U;
        } else if (env.pc == 0x00E688U) {
            const std::uint32_t addr = (env.a_regs[6] + 0x9DU) & kBusAddressMask;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            env.d_regs[0] = (env.d_regs[0] & 0xFFFFFF00U) | val;
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (val == 0) ccr |= kFlagZ;
            if (val & 0x80U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(8);
            env.pc = 0x00E68CU;
        } else if (env.pc == 0x00E68CU) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) == 0U) {
                env.pc = 0x00E6AAU;
                break;
            }
            env.pc = 0x00E690U;
        } else if (env.pc == 0x00E690U) {
            const std::uint32_t addr = (env.a_regs[6] + 0x82U) & kBusAddressMask;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            env.d_regs[0] = (env.d_regs[0] & 0xFFFFFF00U) | val;
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (val == 0) ccr |= kFlagZ;
            if (val & 0x80U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(8);
            env.pc = 0x00E694U;
        } else if (env.pc == 0x00E694U) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) == 0U) {
                env.pc = 0x00E6A8U;
            } else {
                env.pc = 0x00E698U;
            }
        } else if (env.pc == 0x00E698U) {
            const auto val = static_cast<std::uint8_t>(env.d_regs[2] & 0xFFU);
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (val == 0) ccr |= kFlagZ;
            if (val & 0x80U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(4);
            env.pc = 0x00E69AU;
        } else if (env.pc == 0x00E69AU) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) == 0U) {
                env.pc = 0x00E6A8U;
            } else {
                env.pc = 0x00E69EU;
            }
        } else if (env.pc == 0x00E69EU) {
            const auto val = static_cast<std::uint8_t>(env.d_regs[0] & 0xFFU);
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (val == 0) ccr |= kFlagZ;
            if (val & 0x80U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(4);
            env.pc = 0x00E6A0U;
        } else if (env.pc == 0x00E6A0U) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) == 0U) {
                env.pc = 0x00E6A8U;
            } else {
                env.pc = 0x00E6A4U;
            }
        } else if (env.pc == 0x00E6A4U) {
            const std::uint32_t addr = (env.a_regs[6] + 0x36U) & kBusAddressMask;
            const auto val = static_cast<std::uint8_t>(env.read_memory(addr, IrOpSize::Byte));
            const bool is_zero = (val & (1U << 0U)) == 0U;
            const auto new_val = static_cast<std::uint8_t>(val | (1U << 0U));
            env.write_memory(addr, IrOpSize::Byte, new_val);
            writes.emplace_back(addr, new_val);
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x00E6A8U;
        } else if (env.pc == 0x00E6A8U) {
            env.add_cycles(16); // RTS
            break;
        }
    }
    return extract_result(env, writes);
}

inline PortableExecutionResult run_mode_b_00e682(PortableGuestEnvironment& env) {
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes;
    env.pc = 0x00E682U;
    env.total_cycles = 0;

    const std::uint32_t addr_36 = (env.a_regs[6] + 0x36U) & kBusAddressMask;
    auto val_36 = static_cast<std::uint8_t>(env.read_memory(addr_36, IrOpSize::Byte));
    const bool z_bclr = (val_36 & (1U << 6U)) == 0U;
    val_36 &= ~(1U << 6U);
    env.write_memory(addr_36, IrOpSize::Byte, val_36);
    writes.emplace_back(addr_36, val_36);
    env.write_ccr(z_bclr ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));

    const std::uint32_t addr_9d = (env.a_regs[6] + 0x9DU) & kBusAddressMask;
    const auto val_9d = static_cast<std::uint8_t>(env.read_memory(addr_9d, IrOpSize::Byte));
    env.d_regs[0] = (env.d_regs[0] & 0xFFFFFF00U) | val_9d;
    std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
    if (val_9d == 0) ccr |= kFlagZ;
    if (val_9d & 0x80U) ccr |= kFlagN;
    env.write_ccr(ccr);

    if ((env.read_ccr() & kFlagZ) == 0U) {
        env.pc = 0x00E6AAU;
        env.add_cycles(12 + 8 + 10);
    } else {
        const std::uint32_t addr_82 = (env.a_regs[6] + 0x82U) & kBusAddressMask;
        const auto val_82 = static_cast<std::uint8_t>(env.read_memory(addr_82, IrOpSize::Byte));
        env.d_regs[0] = (env.d_regs[0] & 0xFFFFFF00U) | val_82;
        ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
        if (val_82 == 0) ccr |= kFlagZ;
        if (val_82 & 0x80U) ccr |= kFlagN;
        env.write_ccr(ccr);

        if ((env.read_ccr() & kFlagZ) == 0U) {
            env.pc = 0x00E6A8U;
            env.add_cycles(12 + 8 + 10 + 8 + 10 + 16);
        } else {
            const auto val_d2 = static_cast<std::uint8_t>(env.d_regs[2] & 0xFFU);
            ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (val_d2 == 0) ccr |= kFlagZ;
            if (val_d2 & 0x80U) ccr |= kFlagN;
            env.write_ccr(ccr);

            if ((env.read_ccr() & kFlagZ) == 0U) {
                env.pc = 0x00E6A8U;
                env.add_cycles(12 + 8 + 10 + 8 + 10 + 4 + 10 + 16);
            } else {
                const auto val_d0 = static_cast<std::uint8_t>(env.d_regs[0] & 0xFFU);
                ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
                if (val_d0 == 0) ccr |= kFlagZ;
                if (val_d0 & 0x80U) ccr |= kFlagN;
                env.write_ccr(ccr);

                if ((env.read_ccr() & kFlagZ) == 0U) {
                    env.pc = 0x00E6A8U;
                    env.add_cycles(12 + 8 + 10 + 8 + 10 + 4 + 10 + 4 + 10 + 16);
                } else {
                    const bool z_bset = (val_36 & (1U << 0U)) == 0U;
                    val_36 |= (1U << 0U);
                    env.write_memory(addr_36, IrOpSize::Byte, val_36);
                    writes.emplace_back(addr_36, val_36);
                    env.write_ccr(z_bset ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
                    env.pc = 0x00E6A8U;
                    env.add_cycles(12 + 8 + 10 + 8 + 10 + 4 + 10 + 4 + 10 + 12 + 16);
                }
            }
        }
    }
    return extract_result(env, writes);
}

// ============================================================================
// ROUTINE 2: 0x016294 Entity AI Patrol Timer
// ============================================================================

inline PortableExecutionResult run_mode_a_016294(PortableGuestEnvironment& env) {
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes;
    env.pc = 0x016294U;
    env.total_cycles = 0;

    while (env.pc < 0x0162D0U) {
        env.on_sync_boundary(env.pc);
        if (env.pc == 0x016294U) {
            const auto val = static_cast<std::uint8_t>(env.read_memory(0x00FF0E1DU, IrOpSize::Byte));
            const bool is_zero = (val & 1U) == 0U;
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x01629CU;
        } else if (env.pc == 0x01629CU) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagZ) == 0U) {
                env.pc = 0x0163A4U;
                break;
            }
            env.pc = 0x0162A0U;
        } else if (env.pc == 0x0162A0U) {
            const std::uint32_t addr = (env.a_regs[6] + 0xB0U) & kBusAddressMask;
            const auto val = static_cast<std::uint16_t>(env.read_memory(addr, IrOpSize::Word));
            env.d_regs[0] = (env.d_regs[0] & 0xFFFF0000U) | val;
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (val == 0) ccr |= kFlagZ;
            if (val & 0x8000U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(8);
            env.pc = 0x0162A4U;
        } else if (env.pc == 0x0162A4U) {
            const auto old_d0 = static_cast<std::uint16_t>(env.d_regs[0] & 0xFFFFU);
            const auto new_d0 = static_cast<std::uint16_t>(old_d0 + 1U);
            env.d_regs[0] = (env.d_regs[0] & 0xFFFF0000U) | new_d0;
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC | kFlagX);
            if (new_d0 == 0) ccr |= (kFlagZ | kFlagC | kFlagX);
            if (new_d0 & 0x8000U) ccr |= kFlagN;
            if (old_d0 == 0x7FFFU) ccr |= kFlagV;
            env.write_ccr(ccr);
            env.add_cycles(4);
            env.pc = 0x0162A6U;
        } else if (env.pc == 0x0162A6U) {
            const std::uint32_t addr = (env.a_regs[6] + 0xB0U) & kBusAddressMask;
            const auto val = static_cast<std::uint16_t>(env.d_regs[0] & 0xFFFFU);
            env.write_memory(addr, IrOpSize::Word, val);
            writes.emplace_back(addr, static_cast<std::uint8_t>((val >> 8) & 0xFF));
            writes.emplace_back(addr + 1U, static_cast<std::uint8_t>(val & 0xFF));
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if (val == 0) ccr |= kFlagZ;
            if (val & 0x8000U) ccr |= kFlagN;
            env.write_ccr(ccr);
            env.add_cycles(12);
            env.pc = 0x0162AAU;
        } else if (env.pc == 0x0162AAU) {
            const auto cur_d0 = static_cast<std::uint16_t>(env.d_regs[0] & 0xFFFFU);
            const std::uint32_t res = static_cast<std::uint32_t>(cur_d0) - 90U;
            std::uint16_t ccr = env.read_ccr() & ~(kFlagN | kFlagZ | kFlagV | kFlagC);
            if ((res & 0xFFFFU) == 0) ccr |= kFlagZ;
            if (res & 0x8000U) ccr |= kFlagN;
            if (cur_d0 < 90U) ccr |= kFlagC;
            env.write_ccr(ccr);
            env.add_cycles(8);
            env.pc = 0x0162AEU;
        } else if (env.pc == 0x0162AEU) {
            env.add_cycles(10);
            if ((env.read_ccr() & kFlagC) != 0U) {
                env.pc = 0x015AFCU;
                break;
            }
            break;
        }
    }
    return extract_result(env, writes);
}

inline PortableExecutionResult run_mode_b_016294(PortableGuestEnvironment& env) {
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes;
    env.pc = 0x016294U;
    env.total_cycles = 0;

    const auto val_flag = static_cast<std::uint8_t>(env.read_memory(0x00FF0E1DU, IrOpSize::Byte));
    const bool is_zero = (val_flag & 1U) == 0U;
    env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));

    if (!is_zero) {
        env.pc = 0x0163A4U;
        env.add_cycles(12 + 10);
    } else {
        const std::uint32_t addr = (env.a_regs[6] + 0xB0U) & kBusAddressMask;
        const auto val_b0 = static_cast<std::uint16_t>(env.read_memory(addr, IrOpSize::Word));
        const auto new_d0 = static_cast<std::uint16_t>(val_b0 + 1U);
        env.d_regs[0] = (env.d_regs[0] & 0xFFFF0000U) | new_d0;
        env.write_memory(addr, IrOpSize::Word, new_d0);
        writes.emplace_back(addr, static_cast<std::uint8_t>((new_d0 >> 8) & 0xFF));
        writes.emplace_back(addr + 1U, static_cast<std::uint8_t>(new_d0 & 0xFF));

        const std::uint32_t res = static_cast<std::uint32_t>(new_d0) - 90U;
        std::uint16_t ccr = 0;
        if ((res & 0xFFFFU) == 0) ccr |= kFlagZ;
        if (res & 0x8000U) ccr |= kFlagN;
        if (new_d0 < 90U) ccr |= kFlagC;
        env.write_ccr(ccr);

        if (new_d0 < 90U) {
            env.pc = 0x015AFCU;
            env.add_cycles(12 + 10 + 8 + 4 + 12 + 8 + 10);
        }
    }
    return extract_result(env, writes);
}

// ============================================================================
// ROUTINE 3: 0x006E7A Event Flag Bit Tester
// ============================================================================

inline PortableExecutionResult run_mode_a_006e7a(PortableGuestEnvironment& env) {
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes;
    env.pc = 0x006E7AU;
    env.total_cycles = 0;

    while (env.pc < 0x006E88U) {
        env.on_sync_boundary(env.pc);
        if (env.pc == 0x006E7AU) {
            const auto val = static_cast<std::uint8_t>(env.read_memory(0x00FF165FU, IrOpSize::Byte));
            const bool is_zero = (val & (1U << 4U)) == 0U;
            env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));
            env.add_cycles(12);
            env.pc = 0x006E82U;
        } else if (env.pc == 0x006E82U) {
            if ((env.read_ccr() & kFlagZ) == 0U) {
                env.pc = 0x006E86U;
                env.add_cycles(10);
                break;
            } else {
                env.pc = 0x006E84U;
                env.add_cycles(8);
            }
        } else if (env.pc == 0x006E84U) {
            env.add_cycles(16); // RTS
            break;
        }
    }
    return extract_result(env, writes);
}

inline PortableExecutionResult run_mode_b_006e7a(PortableGuestEnvironment& env) {
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes;
    env.pc = 0x006E7AU;
    env.total_cycles = 0;

    const auto val = static_cast<std::uint8_t>(env.read_memory(0x00FF165FU, IrOpSize::Byte));
    const bool is_zero = (val & (1U << 4U)) == 0U;
    env.write_ccr(is_zero ? (env.read_ccr() | kFlagZ) : (env.read_ccr() & ~kFlagZ));

    if (!is_zero) {
        env.pc = 0x006E86U;
        env.add_cycles(12 + 10);
    } else {
        env.pc = 0x006E84U;
        env.add_cycles(12 + 8 + 16);
    }
    return extract_result(env, writes);
}

} // namespace oasis::recompiler::ir

#endif // OASIS_SRC_TOOLS_THOR_IR_GENERATED_PORTABLE_ROUTINES_HPP_
