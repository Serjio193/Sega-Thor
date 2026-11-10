#pragma once
#include "tools/thor_ir/portable_guest_env.hpp"
#include <cstdint>
#include <utility>
#include <vector>

namespace oasis::recompiler::ir {

constexpr std::uint32_t kJumpTableBase = 0x0059BAU;

template <typename T>
inline std::uint16_t ccr_logic(T val, std::uint16_t cur) noexcept {
    const std::uint64_t msb = 1ULL << (sizeof(T) * 8U - 1U);
    std::uint16_t r = cur & kFlagX;
    if (val == 0) r |= kFlagZ;
    if (static_cast<std::uint64_t>(val) & msb) r |= kFlagN;
    return r;
}

template <typename T>
inline std::uint16_t ccr_add(T d, T s, std::uint16_t cur, bool upd_x) noexcept {
    const std::uint64_t mask = (sizeof(T) == 1) ? 0xFFULL : ((sizeof(T) == 2) ? 0xFFFFULL : 0xFFFFFFFFULL);
    const std::uint64_t msb = 1ULL << (sizeof(T) * 8U - 1U);
    const std::uint64_t sum = static_cast<std::uint64_t>(d) + static_cast<std::uint64_t>(s);
    const std::uint64_t res = sum & mask;
    std::uint16_t r = upd_x ? 0U : (cur & kFlagX);
    if (res == 0) r |= kFlagZ;
    if (res & msb) r |= kFlagN;
    if (sum > mask) { r |= kFlagC; if (upd_x) r |= kFlagX; }
    if (((static_cast<std::uint64_t>(d) ^ res) & (static_cast<std::uint64_t>(s) ^ res) & msb) != 0ULL) r |= kFlagV;
    return r;
}

template <typename T>
inline std::uint16_t ccr_sub(T d, T s, std::uint16_t cur, bool upd_x) noexcept {
    const std::uint64_t mask = (sizeof(T) == 1) ? 0xFFULL : ((sizeof(T) == 2) ? 0xFFFFULL : 0xFFFFFFFFULL);
    const std::uint64_t msb = 1ULL << (sizeof(T) * 8U - 1U);
    const std::uint64_t diff = static_cast<std::uint64_t>(d) - static_cast<std::uint64_t>(s);
    const std::uint64_t res = diff & mask;
    std::uint16_t r = upd_x ? 0U : (cur & kFlagX);
    if (res == 0) r |= kFlagZ;
    if (res & msb) r |= kFlagN;
    if (static_cast<std::uint64_t>(s) > static_cast<std::uint64_t>(d)) { r |= kFlagC; if (upd_x) r |= kFlagX; }
    if (((static_cast<std::uint64_t>(d) ^ static_cast<std::uint64_t>(s)) & (static_cast<std::uint64_t>(d) ^ res) & msb) != 0ULL) r |= kFlagV;
    return r;
}

inline bool eval_cond(std::uint16_t ccr, int cond) noexcept {
    const bool c = (ccr & kFlagC) != 0, v = (ccr & kFlagV) != 0;
    const bool z = (ccr & kFlagZ) != 0, n = (ccr & kFlagN) != 0;
    switch (cond) {
    case 0: return true; case 2: return !c && !z; case 3: return c || z;
    case 4: return !c; case 5: return c; case 6: return !z; case 7: return z;
    case 10: return !n; case 11: return n; case 12: return n == v;
    case 13: return n != v; case 14: return !z && (n == v); case 15: return z || (n != v);
    default: return false;
    }
}

struct PortableIrBus {
    PortableGuestEnvironment& env;
    std::vector<std::pair<std::uint32_t, std::uint8_t>> writes;
    std::uint32_t d(int i) const noexcept { return env.d_regs[i]; }
    void set_d(int i, std::uint32_t v) noexcept { env.d_regs[i] = v; }
    std::uint32_t a(int i) const noexcept { return env.a_regs[i]; }
    void set_a(int i, std::uint32_t v) noexcept {
        if (i == 7) env.a_regs[7] = v & kBusAddressMask; else env.a_regs[i] = v;
    }
    std::uint16_t ccr() const noexcept { return env.read_ccr(); }
    void set_ccr(std::uint16_t c) noexcept { env.write_ccr(c); }
    bool check_align(std::uint32_t addr, IrOpSize sz) noexcept {
        if (sz != IrOpSize::Byte && (addr & 1U) != 0U) { env.address_error = true; return false; }
        return true;
    }
    std::uint32_t read_m(std::uint32_t addr, IrOpSize sz) noexcept { return env.read_memory(addr & kBusAddressMask, sz); }
    void write_m(std::uint32_t addr, IrOpSize sz, std::uint32_t val) noexcept {
        const auto m = addr & kBusAddressMask;
        env.write_memory(m, sz, val);
        if (env.address_error || env.bus_error) return;
        if (sz == IrOpSize::Byte) writes.emplace_back(m, val & 0xFF);
        else if (sz == IrOpSize::Word) { writes.emplace_back(m, (val >> 8) & 0xFF); writes.emplace_back(m + 1, val & 0xFF); }
        else for (int k = 0; k < 4; ++k) writes.emplace_back(m + k, (val >> (24 - k * 8)) & 0xFF);
    }
    bool faulted() const noexcept { return env.address_error || env.bus_error; }
    void fault_addr() noexcept { env.address_error = true; }
    bool begin_insn(std::uint32_t, std::uint16_t) noexcept { return true; }
    bool end_insn(std::uint32_t next_pc, std::uint16_t, int m68k_cyc, int branch_adj_sign) noexcept {
        env.pc = next_pc & kBusAddressMask;
        env.add_cycles(m68k_cyc + branch_adj_sign * 2);
        return true;
    }
};

template <typename Bus>
inline void write_dn(Bus& bus, int r, std::uint32_t val, int bytes) noexcept {
    if (bytes == 1) bus.set_d(r, (bus.d(r) & 0xFFFFFF00U) | (val & 0xFFU));
    else if (bytes == 2) bus.set_d(r, (bus.d(r) & 0xFFFF0000U) | (val & 0xFFFFU));
    else bus.set_d(r, val);
}

inline constexpr IrOpSize sz_enum(int b) noexcept {
    return b == 1 ? IrOpSize::Byte : (b == 2 ? IrOpSize::Word : IrOpSize::Long);
}
} // namespace oasis::recompiler::ir
