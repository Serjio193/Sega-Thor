#pragma once
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x02EE4A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6A02U; constexpr std::uint32_t kPc = 0x02EE4AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 10);
  out_pc = taken ? 0x02EE4EU : 0x02EE4CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x02EE4C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4285U; constexpr std::uint32_t kPc = 0x02EE4CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  write_dn(bus, 5, 0U, 4);
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x02EE4EU; return bus.end_insn(out_pc, kOp, 6, 0);
}
template <typename Bus> inline bool step_ir_0x02EE4E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2945U; constexpr std::uint32_t kPc = 0x02EE4EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.d(5));
  const std::uint32_t da = (bus.a(4) + static_cast<std::uint32_t>(static_cast<std::int32_t>(16))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Long)) return false; bus.write_m(da, IrOpSize::Long, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x02EE52U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE52(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x9085U; constexpr std::uint32_t kPc = 0x02EE52U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(bus.d(5));
  const std::uint32_t d = static_cast<std::uint32_t>(bus.d(0));
  const std::uint32_t r = static_cast<std::uint32_t>(d - s);
  write_dn(bus, 0, r, 4);
  bus.set_ccr(ccr_sub<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x02EE54U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x02EE54(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x9283U; constexpr std::uint32_t kPc = 0x02EE54U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(bus.d(3));
  const std::uint32_t d = static_cast<std::uint32_t>(bus.d(1));
  const std::uint32_t r = static_cast<std::uint32_t>(d - s);
  write_dn(bus, 1, r, 4);
  bus.set_ccr(ccr_sub<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x02EE56U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x02EE56(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x9484U; constexpr std::uint32_t kPc = 0x02EE56U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(bus.d(4));
  const std::uint32_t d = static_cast<std::uint32_t>(bus.d(2));
  const std::uint32_t r = static_cast<std::uint32_t>(d - s);
  write_dn(bus, 2, r, 4);
  bus.set_ccr(ccr_sub<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x02EE58U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x02EE58(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x81FCU; constexpr std::uint32_t kPc = 0x02EE58U;
  const std::uint16_t s16 = static_cast<std::uint16_t>(0x280U);
  const std::int16_t s = static_cast<std::int16_t>(s16);
  if (s == 0) { bus.fault_div0(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::int32_t dvd = static_cast<std::int32_t>(bus.d(0));
  const std::int64_t q64 = static_cast<std::int64_t>(dvd) / static_cast<std::int64_t>(s);
  const int mc = m15246_calc_divs_cycles(dvd, s);
  if (q64 < -32768 || q64 > 32767) { bus.set_ccr((bus.ccr() & kFlagX) | kFlagN | kFlagV); }
  else { const std::uint16_t q = static_cast<std::uint16_t>(static_cast<std::int16_t>(q64));
    const std::uint16_t r = static_cast<std::uint16_t>(static_cast<std::int16_t>(dvd % s));
    bus.set_d(0, (static_cast<std::uint32_t>(r) << 16U) | q); bus.set_ccr(ccr_logic<std::uint16_t>(q, bus.ccr())); }
  out_pc = 0x02EE5CU; return bus.end_insn_mc(out_pc, kOp, (mc + 28) / 7, mc, true);
}
template <typename Bus> inline bool step_ir_0x02EE5C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x83FCU; constexpr std::uint32_t kPc = 0x02EE5CU;
  const std::uint16_t s16 = static_cast<std::uint16_t>(0x280U);
  const std::int16_t s = static_cast<std::int16_t>(s16);
  if (s == 0) { bus.fault_div0(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::int32_t dvd = static_cast<std::int32_t>(bus.d(1));
  const std::int64_t q64 = static_cast<std::int64_t>(dvd) / static_cast<std::int64_t>(s);
  const int mc = m15246_calc_divs_cycles(dvd, s);
  if (q64 < -32768 || q64 > 32767) { bus.set_ccr((bus.ccr() & kFlagX) | kFlagN | kFlagV); }
  else { const std::uint16_t q = static_cast<std::uint16_t>(static_cast<std::int16_t>(q64));
    const std::uint16_t r = static_cast<std::uint16_t>(static_cast<std::int16_t>(dvd % s));
    bus.set_d(1, (static_cast<std::uint32_t>(r) << 16U) | q); bus.set_ccr(ccr_logic<std::uint16_t>(q, bus.ccr())); }
  out_pc = 0x02EE60U; return bus.end_insn_mc(out_pc, kOp, (mc + 28) / 7, mc, true);
}
template <typename Bus> inline bool step_ir_0x02EE60(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x85FCU; constexpr std::uint32_t kPc = 0x02EE60U;
  const std::uint16_t s16 = static_cast<std::uint16_t>(0x280U);
  const std::int16_t s = static_cast<std::int16_t>(s16);
  if (s == 0) { bus.fault_div0(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::int32_t dvd = static_cast<std::int32_t>(bus.d(2));
  const std::int64_t q64 = static_cast<std::int64_t>(dvd) / static_cast<std::int64_t>(s);
  const int mc = m15246_calc_divs_cycles(dvd, s);
  if (q64 < -32768 || q64 > 32767) { bus.set_ccr((bus.ccr() & kFlagX) | kFlagN | kFlagV); }
  else { const std::uint16_t q = static_cast<std::uint16_t>(static_cast<std::int16_t>(q64));
    const std::uint16_t r = static_cast<std::uint16_t>(static_cast<std::int16_t>(dvd % s));
    bus.set_d(2, (static_cast<std::uint32_t>(r) << 16U) | q); bus.set_ccr(ccr_logic<std::uint16_t>(q, bus.ccr())); }
  out_pc = 0x02EE64U; return bus.end_insn_mc(out_pc, kOp, (mc + 28) / 7, mc, true);
}
template <typename Bus> inline bool step_ir_0x02EE64(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x48C0U; constexpr std::uint32_t kPc = 0x02EE64U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(bus.d(0) & 0xFFFFU)));
  bus.set_d(0, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EE66U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EE66(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x48C1U; constexpr std::uint32_t kPc = 0x02EE66U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(bus.d(1) & 0xFFFFU)));
  bus.set_d(1, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EE68U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EE68(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x48C2U; constexpr std::uint32_t kPc = 0x02EE68U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(bus.d(2) & 0xFFFFU)));
  bus.set_d(2, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EE6AU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EE6A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xEF80U; constexpr std::uint32_t kPc = 0x02EE6AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  m15246_shift_imm<std::uint32_t, 4, 0>(bus, 0, 7);
  out_pc = 0x02EE6CU; return bus.end_insn_mc(out_pc, kOp, 22, 98, false);
}
template <typename Bus> inline bool step_ir_0x02EE6C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xEF81U; constexpr std::uint32_t kPc = 0x02EE6CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  m15246_shift_imm<std::uint32_t, 4, 0>(bus, 1, 7);
  out_pc = 0x02EE6EU; return bus.end_insn_mc(out_pc, kOp, 22, 98, false);
}
template <typename Bus> inline bool step_ir_0x02EE6E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xEF82U; constexpr std::uint32_t kPc = 0x02EE6EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  m15246_shift_imm<std::uint32_t, 4, 0>(bus, 2, 7);
  out_pc = 0x02EE70U; return bus.end_insn_mc(out_pc, kOp, 22, 98, false);
}
template <typename Bus> inline bool step_ir_0x02EE70(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x262EU; constexpr std::uint32_t kPc = 0x02EE70U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(8))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  write_dn(bus, 3, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x02EE74U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE74(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x282EU; constexpr std::uint32_t kPc = 0x02EE74U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(12))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  write_dn(bus, 4, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x02EE78U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE78(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2C2EU; constexpr std::uint32_t kPc = 0x02EE78U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(16))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  write_dn(bus, 6, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x02EE7CU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE7C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x204EU; constexpr std::uint32_t kPc = 0x02EE7CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.a(6));
  bus.set_a(0, v); out_pc = 0x02EE7EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EE7E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7A08U; constexpr std::uint32_t kPc = 0x02EE7EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000008U;
  bus.set_d(5, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EE80U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EE80(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7E02U; constexpr std::uint32_t kPc = 0x02EE80U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000002U;
  bus.set_d(7, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EE82U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EE82(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x02EE82U;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x02EE86U); if (bus.faulted()) return false;
  out_pc = 0x02EEA0U; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x02EE86(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7A0AU; constexpr std::uint32_t kPc = 0x02EE86U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x0000000AU;
  bus.set_d(5, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EE88U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EE88(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7E04U; constexpr std::uint32_t kPc = 0x02EE88U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000004U;
  bus.set_d(7, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EE8AU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EE8A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x02EE8AU;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x02EE8EU); if (bus.faulted()) return false;
  out_pc = 0x02EEA0U; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x02EE8E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7A08U; constexpr std::uint32_t kPc = 0x02EE8EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000008U;
  bus.set_d(5, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EE90U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EE90(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7E06U; constexpr std::uint32_t kPc = 0x02EE90U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000006U;
  bus.set_d(7, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EE92U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EE92(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x02EE92U;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x02EE96U); if (bus.faulted()) return false;
  out_pc = 0x02EEA0U; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x02EE96(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7A04U; constexpr std::uint32_t kPc = 0x02EE96U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000004U;
  bus.set_d(5, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EE98U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EE98(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7E08U; constexpr std::uint32_t kPc = 0x02EE98U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000008U;
  bus.set_d(7, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EE9AU; return bus.end_insn(out_pc, kOp, 4, 0);
}
} // namespace oasis::recompiler::ir
