#pragma once
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x00BE5A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E92U; constexpr std::uint32_t kPc = 0x00BE5AU;
  const std::uint32_t tgt = bus.a(2) & kBusAddressMask;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if (((new_sp | tgt) & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x00BE5CU); if (bus.faulted()) return false;
  out_pc = tgt; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00BE5C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3009U; constexpr std::uint32_t kPc = 0x00BE5CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.a(1) & 0xFFFFU);
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00BE5EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE5E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4847U; constexpr std::uint32_t kPc = 0x00BE5EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t o = bus.d(7); const std::uint32_t v = (o >> 16U) | (o << 16U);
  bus.set_d(7, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x00BE60U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE60(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x41E8U; constexpr std::uint32_t kPc = 0x00BE60U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t lea_a = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(188))) & kBusAddressMask;
  bus.set_a(0, lea_a); out_pc = 0x00BE64U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00BE64(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x51CFU; constexpr std::uint32_t kPc = 0x00BE64U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t nw = static_cast<std::uint16_t>((bus.d(7) & 0xFFFFU) - 1U);
  write_dn(bus, 7, nw, 2);
  const bool taken = (nw != 0xFFFFU); out_pc = taken ? 0x00BDDEU : 0x00BE68U;
  return bus.end_insn(out_pc, kOp, 12, taken ? -1 : 1);
}
template <typename Bus> inline bool step_ir_0x00BE68(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xBDFCU; constexpr std::uint32_t kPc = 0x00BE68U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(0xFF19E8U);
  bus.set_ccr(ccr_sub<std::uint32_t>(bus.a(6), s, bus.ccr(), false));
  out_pc = 0x00BE6EU; return bus.end_insn(out_pc, kOp, 14, 0);
}
template <typename Bus> inline bool step_ir_0x00BE6E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6600U; constexpr std::uint32_t kPc = 0x00BE6EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x00BE96U : 0x00BE72U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00BE72(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x082EU; constexpr std::uint32_t kPc = 0x00BE72U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x3U);
  const std::uint32_t addr_cur = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(55))) & kBusAddressMask;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x00BE78U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00BE78(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00BE78U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00BE96U : 0x00BE7CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00BE7C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3039U; constexpr std::uint32_t kPc = 0x00BE7CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = 0xFF198CU;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00BE82U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00BE82(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C40U; constexpr std::uint32_t kPc = 0x00BE82U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x6U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00BE86U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00BE86(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00BE86U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00BE92U : 0x00BE8AU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00BE8A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C40U; constexpr std::uint32_t kPc = 0x00BE8AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0xAU);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00BE8EU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00BE8E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6600U; constexpr std::uint32_t kPc = 0x00BE8EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x00BE96U : 0x00BE92U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00BE92(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x00BE92U;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x00BE96U); if (bus.faulted()) return false;
  out_pc = 0x008530U; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x00BE96(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x00BE96U;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00E6BA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x48E7U; constexpr std::uint32_t kPc = 0x00E6BAU;
  std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(2)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(1)); if (bus.faulted()) return false;
  bus.set_a(7, sp);
  out_pc = 0x00E6BEU; return bus.end_insn_mc(out_pc, kOp, 24, 112, true);
}
template <typename Bus> inline bool step_ir_0x00E6BE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4A80U; constexpr std::uint32_t kPc = 0x00E6BEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.d(0));
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x00E6C0U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00E6C0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00E6C0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00E6EAU : 0x00E6C4U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00E6C4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C80U; constexpr std::uint32_t kPc = 0x00E6C4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(0x400U);
  const std::uint32_t d = static_cast<std::uint32_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint32_t>(d, s, bus.ccr(), false)); out_pc = 0x00E6CAU; return bus.end_insn(out_pc, kOp, 14, 0);
}
template <typename Bus> inline bool step_ir_0x00E6CA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6500U; constexpr std::uint32_t kPc = 0x00E6CAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 5);
  out_pc = taken ? 0x00E6D6U : 0x00E6CEU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00E6CE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x343CU; constexpr std::uint32_t kPc = 0x00E6CEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x400U);
  write_dn(bus, 2, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00E6D2U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00E6D2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6000U; constexpr std::uint32_t kPc = 0x00E6D2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x00E6D8U; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x00E6D6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3400U; constexpr std::uint32_t kPc = 0x00E6D6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(0));
  write_dn(bus, 2, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00E6D8U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00E6D8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3202U; constexpr std::uint32_t kPc = 0x00E6D8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(2));
  write_dn(bus, 1, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00E6DAU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00E6DA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2400U; constexpr std::uint32_t kPc = 0x00E6DAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.d(0));
  write_dn(bus, 2, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x00E6DCU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00E6DC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x84C1U; constexpr std::uint32_t kPc = 0x00E6DCU;
  const std::uint16_t s16 = static_cast<std::uint16_t>(bus.d(1));
  if (s16 == 0U) { bus.fault_div0(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t dvd = bus.d(2); const std::uint32_t q32 = dvd / s16;
  const int mc = m15246_calc_divu_cycles(dvd, s16);
  if (q32 > 0xFFFFU) { bus.set_ccr((bus.ccr() & kFlagX) | kFlagN | kFlagV); }
  else { const std::uint16_t q = static_cast<std::uint16_t>(q32); const std::uint16_t r = static_cast<std::uint16_t>(dvd % s16);
    bus.set_d(2, (static_cast<std::uint32_t>(r) << 16U) | q); bus.set_ccr(ccr_logic<std::uint16_t>(q, bus.ccr())); }
  out_pc = 0x00E6DEU; return bus.end_insn_mc(out_pc, kOp, (mc + 0) / 7, mc, true);
}
template <typename Bus> inline bool step_ir_0x00E6DE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD441U; constexpr std::uint32_t kPc = 0x00E6DEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(2));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 2, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00E6E0U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00E6E0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xE24AU; constexpr std::uint32_t kPc = 0x00E6E0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  m15246_shift_imm<std::uint16_t, 2, 2>(bus, 2, 1);
  out_pc = 0x00E6E2U; return bus.end_insn_mc(out_pc, kOp, 8, 14, false);
}
template <typename Bus> inline bool step_ir_0x00E6E2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xB441U; constexpr std::uint32_t kPc = 0x00E6E2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(2));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00E6E4U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00E6E4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6500U; constexpr std::uint32_t kPc = 0x00E6E4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 5);
  out_pc = taken ? 0x00E6D8U : 0x00E6E8U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00E6E8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3001U; constexpr std::uint32_t kPc = 0x00E6E8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(1));
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00E6EAU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00E6EA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4CDFU; constexpr std::uint32_t kPc = 0x00E6EAU;
  std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t mv_1 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_2 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  bus.set_d(1, mv_1);
  bus.set_d(2, mv_2);
  bus.set_a(7, sp);
  out_pc = 0x00E6EEU; return bus.end_insn_mc(out_pc, kOp, 28, 112, true);
}
} // namespace oasis::recompiler::ir
