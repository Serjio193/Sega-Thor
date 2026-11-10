#pragma once
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x001E62(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x129AU; constexpr std::uint32_t kPc = 0x001E62U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_val = bus.a(2) & kBusAddressMask;
  if (!bus.check_align(addr_val, IrOpSize::Byte)) return false;
  const std::uint8_t val = static_cast<std::uint8_t>(bus.read_m(addr_val, IrOpSize::Byte));
  bus.set_a(2, (addr_val + 1U) & kBusAddressMask);
  if (bus.faulted()) return false;
  const std::uint32_t addr_dst = bus.a(1) & kBusAddressMask;
  if (!bus.check_align(addr_dst, IrOpSize::Byte)) return false;
  bus.write_m(addr_dst, IrOpSize::Byte, val);
  if (bus.faulted()) return false;
  const std::uint16_t old_ccr = bus.ccr();
  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);
  if ((val & static_cast<std::uint8_t>(1ULL << (7))) != 0U) new_ccr |= 0x08U;
  if (val == 0U) new_ccr |= 0x04U;
  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);
  bus.set_ccr(new_ccr);
  out_pc = 0x001E64U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001E64(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5489U; constexpr std::uint32_t kPc = 0x001E64U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(1, bus.a(1) + 0x2U);
  out_pc = 0x001E66U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x001E66(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x51C9U; constexpr std::uint32_t kPc = 0x001E66U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t nw = static_cast<std::uint16_t>((bus.d(1) & 0xFFFFU) - 1U);
  write_dn(bus, 1, nw, 2);
  const bool taken = (nw != 0xFFFFU); out_pc = taken ? 0x001E62U : 0x001E6AU;
  return bus.end_insn_mc(out_pc, kOp, taken ? 10 : 14, taken ? 0 : 28, false);
}
template <typename Bus> inline bool step_ir_0x001E6A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x323CU; constexpr std::uint32_t kPc = 0x001E6AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x81U);
  write_dn(bus, 1, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x001E6EU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x001E6E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4240U; constexpr std::uint32_t kPc = 0x001E6EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  write_dn(bus, 0, 0U, 2);
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x001E70U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x001E70(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4242U; constexpr std::uint32_t kPc = 0x001E70U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  write_dn(bus, 2, 0U, 2);
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x001E72U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x001E72(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1018U; constexpr std::uint32_t kPc = 0x001E72U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_val = bus.a(0) & kBusAddressMask;
  if (!bus.check_align(addr_val, IrOpSize::Byte)) return false;
  const std::uint8_t val = static_cast<std::uint8_t>(bus.read_m(addr_val, IrOpSize::Byte));
  bus.set_a(0, (addr_val + 1U) & kBusAddressMask);
  if (bus.faulted()) return false;
  bus.set_d(0, (bus.d(0) & 0xFFFFFF00U) | (val & 0xFFU));
  const std::uint16_t old_ccr = bus.ccr();
  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);
  if ((val & static_cast<std::uint8_t>(1ULL << (7))) != 0U) new_ccr |= 0x08U;
  if (val == 0U) new_ccr |= 0x04U;
  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);
  bus.set_ccr(new_ccr);
  out_pc = 0x001E74U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x001E74(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1280U; constexpr std::uint32_t kPc = 0x001E74U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.d(0));
  const std::uint32_t da = bus.a(1) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x001E76U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x001E76(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD440U; constexpr std::uint32_t kPc = 0x001E76U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(0));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(2));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 2, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x001E78U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x001E78(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5489U; constexpr std::uint32_t kPc = 0x001E78U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(1, bus.a(1) + 0x2U);
  out_pc = 0x001E7AU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x001E7A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x51C9U; constexpr std::uint32_t kPc = 0x001E7AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t nw = static_cast<std::uint16_t>((bus.d(1) & 0xFFFFU) - 1U);
  write_dn(bus, 1, nw, 2);
  const bool taken = (nw != 0xFFFFU); out_pc = taken ? 0x001E72U : 0x001E7EU;
  return bus.end_insn_mc(out_pc, kOp, taken ? 10 : 14, taken ? 0 : 28, false);
}
template <typename Bus> inline bool step_ir_0x001E7E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1342U; constexpr std::uint32_t kPc = 0x001E7EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.d(2));
  const std::uint32_t da = (bus.a(1) + static_cast<std::uint32_t>(static_cast<std::int32_t>(2))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x001E82U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001E82(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xE04AU; constexpr std::uint32_t kPc = 0x001E82U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  m15246_shift_imm<std::uint16_t, 2, 2>(bus, 2, 8);
  out_pc = 0x001E84U; return bus.end_insn_mc(out_pc, kOp, 22, 112, false);
}
template <typename Bus> inline bool step_ir_0x001E84(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1282U; constexpr std::uint32_t kPc = 0x001E84U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.d(2));
  const std::uint32_t da = bus.a(1) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x001E86U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x001E86(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4BF9U; constexpr std::uint32_t kPc = 0x001E86U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(5, 0xA130F1U);
  out_pc = 0x001E8CU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001E8C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1ABCU; constexpr std::uint32_t kPc = 0x001E8CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(0x2U);
  const std::uint32_t da = bus.a(5) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x001E90U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001E90(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2A4EU; constexpr std::uint32_t kPc = 0x001E90U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.a(6));
  bus.set_a(5, v); out_pc = 0x001E92U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x001E92(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4CDFU; constexpr std::uint32_t kPc = 0x001E92U;
  std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t mv_0 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_1 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_2 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_8 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_9 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_10 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_13 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  const std::uint32_t mv_14 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  bus.set_d(0, mv_0);
  bus.set_d(1, mv_1);
  bus.set_d(2, mv_2);
  bus.set_a(0, mv_8);
  bus.set_a(1, mv_9);
  bus.set_a(2, mv_10);
  bus.set_a(5, mv_13);
  bus.set_a(6, mv_14);
  bus.set_a(7, sp);
  out_pc = 0x001E96U; return bus.end_insn_mc(out_pc, kOp, 76, 448, true);
}
template <typename Bus> inline bool step_ir_0x001E96(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x001E96U;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x001E98(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x48E7U; constexpr std::uint32_t kPc = 0x001E98U;
  std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(5)); if (bus.faulted()) return false;
  bus.set_a(7, sp);
  out_pc = 0x001E9CU; return bus.end_insn_mc(out_pc, kOp, 16, 56, true);
}
template <typename Bus> inline bool step_ir_0x001E9C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4BF9U; constexpr std::uint32_t kPc = 0x001E9CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(5, 0xA130F1U);
  out_pc = 0x001EA2U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001EA2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1ABCU; constexpr std::uint32_t kPc = 0x001EA2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(0x1U);
  const std::uint32_t da = bus.a(5) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x001EA6U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001EA6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2A4EU; constexpr std::uint32_t kPc = 0x001EA6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.a(6));
  bus.set_a(5, v); out_pc = 0x001EA8U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x001EA8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4BF9U; constexpr std::uint32_t kPc = 0x001EA8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(5, 0x2025CDU);
  out_pc = 0x001EAEU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001EAE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1ABCU; constexpr std::uint32_t kPc = 0x001EAEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(0x0U);
  const std::uint32_t da = bus.a(5) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x001EB2U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001EB2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4BF9U; constexpr std::uint32_t kPc = 0x001EB2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(5, 0x2026E1U);
  out_pc = 0x001EB8U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001EB8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1ABCU; constexpr std::uint32_t kPc = 0x001EB8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(0x0U);
  const std::uint32_t da = bus.a(5) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x001EBCU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001EBC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4BF9U; constexpr std::uint32_t kPc = 0x001EBCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(5, 0xA130F1U);
  out_pc = 0x001EC2U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001EC2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1ABCU; constexpr std::uint32_t kPc = 0x001EC2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(0x2U);
  const std::uint32_t da = bus.a(5) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x001EC6U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001EC6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2A4EU; constexpr std::uint32_t kPc = 0x001EC6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.a(6));
  bus.set_a(5, v); out_pc = 0x001EC8U; return bus.end_insn(out_pc, kOp, 4, 0);
}
} // namespace oasis::recompiler::ir
