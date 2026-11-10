#pragma once
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x002E4A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x08F9U; constexpr std::uint32_t kPc = 0x002E4AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x2U);
  const std::uint32_t addr_cur = 0xFF164DU;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.write_m(addr_cur, IrOpSize::Byte, static_cast<std::uint8_t>(cur | mask)); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x002E52U; return bus.end_insn(out_pc, kOp, 24, 0);
}
template <typename Bus> inline bool step_ir_0x002E52(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0839U; constexpr std::uint32_t kPc = 0x002E52U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x2U);
  const std::uint32_t addr_cur = 0xFF164DU;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x002E5AU; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x002E5A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x66F6U; constexpr std::uint32_t kPc = 0x002E5AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x002E52U : 0x002E5CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x002E5C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x41F9U; constexpr std::uint32_t kPc = 0x002E5CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, 0xFF134CU);
  out_pc = 0x002E62U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x002E62(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4DF9U; constexpr std::uint32_t kPc = 0x002E62U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(6, 0xFF18A6U);
  out_pc = 0x002E68U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x002E68(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3E3CU; constexpr std::uint32_t kPc = 0x002E68U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x1FU);
  write_dn(bus, 7, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x002E6CU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x002E6C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x20DEU; constexpr std::uint32_t kPc = 0x002E6CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_val = bus.a(6) & kBusAddressMask;
  if (!bus.check_align(addr_val, IrOpSize::Long)) return false;
  const std::uint32_t val = static_cast<std::uint32_t>(bus.read_m(addr_val, IrOpSize::Long));
  bus.set_a(6, (addr_val + 4U) & kBusAddressMask);
  if (bus.faulted()) return false;
  const std::uint32_t addr_dst = bus.a(0) & kBusAddressMask;
  if (!bus.check_align(addr_dst, IrOpSize::Long)) return false;
  bus.write_m(addr_dst, IrOpSize::Long, val);
  bus.set_a(0, (addr_dst + 4U) & kBusAddressMask);
  if (bus.faulted()) return false;
  const std::uint16_t old_ccr = bus.ccr();
  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);
  if ((val & static_cast<std::uint32_t>(1ULL << (31))) != 0U) new_ccr |= 0x08U;
  if (val == 0U) new_ccr |= 0x04U;
  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);
  bus.set_ccr(new_ccr);
  out_pc = 0x002E6EU; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x002E6E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x51CFU; constexpr std::uint32_t kPc = 0x002E6EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t nw = static_cast<std::uint16_t>((bus.d(7) & 0xFFFFU) - 1U);
  write_dn(bus, 7, nw, 2);
  const bool taken = (nw != 0xFFFFU); out_pc = taken ? 0x002E6CU : 0x002E72U;
  return bus.end_insn_mc(out_pc, kOp, taken ? 10 : 14, taken ? 0 : 28, false);
}
template <typename Bus> inline bool step_ir_0x002E72(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4CDFU; constexpr std::uint32_t kPc = 0x002E72U;
  std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t mv_0 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_1 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_2 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_3 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_4 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_5 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_6 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_7 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_8 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_9 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_10 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_11 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_12 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_13 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_14 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  bus.set_d(0, mv_0);
  bus.set_d(1, mv_1);
  bus.set_d(2, mv_2);
  bus.set_d(3, mv_3);
  bus.set_d(4, mv_4);
  bus.set_d(5, mv_5);
  bus.set_d(6, mv_6);
  bus.set_d(7, mv_7);
  bus.set_a(0, mv_8);
  bus.set_a(1, mv_9);
  bus.set_a(2, mv_10);
  bus.set_a(3, mv_11);
  bus.set_a(4, mv_12);
  bus.set_a(5, mv_13);
  bus.set_a(6, mv_14);
  bus.set_a(7, sp);
  out_pc = 0x002E76U; return bus.end_insn_mc(out_pc, kOp, 132, 840, true);
}
template <typename Bus> inline bool step_ir_0x002E76(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x002E76U;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x002E78(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x48E7U; constexpr std::uint32_t kPc = 0x002E78U;
  std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(6)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(5)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(4)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(3)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(2)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(1)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(0)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(7)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(6)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(5)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(4)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(3)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(2)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(1)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(0)); if (bus.faulted()) return false;
  bus.set_a(7, sp);
  out_pc = 0x002E7CU; return bus.end_insn_mc(out_pc, kOp, 128, 840, true);
}
template <typename Bus> inline bool step_ir_0x002E7C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0839U; constexpr std::uint32_t kPc = 0x002E7CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x4U);
  const std::uint32_t addr_cur = 0xFF164DU;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x002E84U; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x002E84(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x66F6U; constexpr std::uint32_t kPc = 0x002E84U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x002E7CU : 0x002E86U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x002E86(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x51F9U; constexpr std::uint32_t kPc = 0x002E86U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t da = 0xFF1926U;
  bus.write_m(da, IrOpSize::Byte, 0U); if (bus.faulted()) return false;
  out_pc = 0x002E8CU; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x002E8C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x41F9U; constexpr std::uint32_t kPc = 0x002E8CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, 0xFF134CU);
  out_pc = 0x002E92U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x002E92(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4DF9U; constexpr std::uint32_t kPc = 0x002E92U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(6, 0xFF18A6U);
  out_pc = 0x002E98U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x002E98(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2C0EU; constexpr std::uint32_t kPc = 0x002E98U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.a(6));
  write_dn(bus, 6, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x002E9AU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x002E9A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3E3CU; constexpr std::uint32_t kPc = 0x002E9AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x1FU);
  write_dn(bus, 7, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x002E9EU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x002E9E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2CD8U; constexpr std::uint32_t kPc = 0x002E9EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_val = bus.a(0) & kBusAddressMask;
  if (!bus.check_align(addr_val, IrOpSize::Long)) return false;
  const std::uint32_t val = static_cast<std::uint32_t>(bus.read_m(addr_val, IrOpSize::Long));
  bus.set_a(0, (addr_val + 4U) & kBusAddressMask);
  if (bus.faulted()) return false;
  const std::uint32_t addr_dst = bus.a(6) & kBusAddressMask;
  if (!bus.check_align(addr_dst, IrOpSize::Long)) return false;
  bus.write_m(addr_dst, IrOpSize::Long, val);
  bus.set_a(6, (addr_dst + 4U) & kBusAddressMask);
  if (bus.faulted()) return false;
  const std::uint16_t old_ccr = bus.ccr();
  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);
  if ((val & static_cast<std::uint32_t>(1ULL << (31))) != 0U) new_ccr |= 0x08U;
  if (val == 0U) new_ccr |= 0x04U;
  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);
  bus.set_ccr(new_ccr);
  out_pc = 0x002EA0U; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x002EA0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x51CFU; constexpr std::uint32_t kPc = 0x002EA0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t nw = static_cast<std::uint16_t>((bus.d(7) & 0xFFFFU) - 1U);
  write_dn(bus, 7, nw, 2);
  const bool taken = (nw != 0xFFFFU); out_pc = taken ? 0x002E9EU : 0x002EA4U;
  return bus.end_insn_mc(out_pc, kOp, taken ? 10 : 14, taken ? 0 : 28, false);
}
template <typename Bus> inline bool step_ir_0x002EA4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x23C6U; constexpr std::uint32_t kPc = 0x002EA4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.d(6));
  const std::uint32_t da = 0xFF189CU;
  if (!bus.check_align(da, IrOpSize::Long)) return false; bus.write_m(da, IrOpSize::Long, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x002EAAU; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x002EAA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x33FCU; constexpr std::uint32_t kPc = 0x002EAAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x7U);
  const std::uint32_t da = 0xFF189AU;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x002EB2U; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x002EB2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x33F9U; constexpr std::uint32_t kPc = 0x002EB2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = 0xFF1896U;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  const std::uint32_t da = 0xFF1898U;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x002EBCU; return bus.end_insn(out_pc, kOp, 28, 0);
}
} // namespace oasis::recompiler::ir
