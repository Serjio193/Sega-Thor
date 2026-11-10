#pragma once
#include "tools/thor_ir/generated/entity_dispatch_common.hpp"
#include <cstdlib>

namespace oasis::recompiler::ir {

inline int m15246_calc_muls_cycles(std::int16_t src) noexcept {
  int mc = 38 * 7;
  std::uint32_t diff = static_cast<std::uint32_t>(((static_cast<std::int32_t>(src) << 1) ^ static_cast<std::int32_t>(src)) & 0xFFFF);
  while (diff != 0U) { if ((diff & 1U) != 0U) mc += 14; diff >>= 1U; }
  return mc;
}

inline int m15246_calc_divs_cycles(std::int32_t dst, std::int16_t src) noexcept {
  int mc = 12 * 7;
  if (src == 0) return mc;
  if (dst < 0) mc += 14;
  const std::int64_t ad = std::abs(static_cast<std::int64_t>(dst));
  const std::int64_t as = std::abs(static_cast<std::int64_t>(src));
  if ((ad >> 16) < as) {
    std::uint32_t q = static_cast<std::uint32_t>(ad / as);
    mc += 110 * 7;
    if (src >= 0) mc += (dst >= 0) ? -14 : 14;
    for (int i = 0; i < 15; ++i) { q >>= 1U; if ((q & 1U) == 0U) mc += 14; }
  } else {
    mc += 28;
  }
  return mc;
}

inline int m15246_calc_divu_cycles(std::uint32_t dst, std::uint32_t src) noexcept {
  if (src == 0U || (dst >> 16U) >= src) return 10 * 7;
  int mc = 76 * 7;
  src <<= 16U;
  for (int i = 0; i < 15; ++i) {
    if (static_cast<std::int32_t>(dst) < 0) { dst <<= 1U; dst -= src; }
    else { dst <<= 1U; mc += 28; if (dst >= src) { dst -= src; mc -= 14; } }
  }
  return mc;
}

template <typename T, int Bytes, int Kind, typename Bus>
inline void m15246_shift_imm(Bus& bus, int r, int cnt) noexcept {
  constexpr std::uint64_t kMask = (Bytes == 1) ? 0xFFULL : ((Bytes == 2) ? 0xFFFFULL : 0xFFFFFFFFULL);
  constexpr std::uint64_t kMsb = 1ULL << (Bytes * 8 - 1);
  std::uint64_t val = static_cast<T>(bus.d(r)) & kMask;
  bool carry = false, ovf = false;
  for (int i = 0; i < cnt; ++i) {
    if constexpr (Kind == 2) {  // LSR
      carry = (val & 1ULL) != 0ULL;
      val >>= 1U;
    } else {  // 0: ASL, 1: LSL
      const bool prev_msb = (val & kMsb) != 0ULL;
      carry = prev_msb;
      val = (val << 1U) & kMask;
      const bool new_msb = (val & kMsb) != 0ULL;
      if constexpr (Kind == 0) { if (prev_msb != new_msb) ovf = true; }
    }
  }
  write_dn(bus, r, static_cast<std::uint32_t>(val), Bytes);
  std::uint16_t nc = 0U;
  if (val == 0ULL) nc |= kFlagZ;
  if ((val & kMsb) != 0ULL) nc |= kFlagN;
  if (carry) nc |= (kFlagC | kFlagX);
  if (ovf) nc |= kFlagV;
  bus.set_ccr(nc);
}

struct PortableIrBus246 : PortableIrBus {
  explicit PortableIrBus246(PortableGuestEnvironment& e) noexcept : PortableIrBus{e, {}} {}
  bool div_zero{false};
  void fault_div0() noexcept { div_zero = true; }
  bool faulted() const noexcept { return div_zero || PortableIrBus::faulted(); }
  bool end_insn_mc(std::uint32_t next_pc, std::uint16_t, int m68k_cyc, int, bool) noexcept {
    return end_insn(next_pc, 0, m68k_cyc, 0);
  }
};

} // namespace oasis::recompiler::ir
