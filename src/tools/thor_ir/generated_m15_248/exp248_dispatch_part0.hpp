#pragma once
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x001112(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x48E7U; constexpr std::uint32_t kPc = 0x001112U;
  std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(0)); if (bus.faulted()) return false;
  bus.set_a(7, sp);
  out_pc = 0x001116U; return bus.end_insn_mc(out_pc, kOp, 16, 56, true);
}
template <typename Bus> inline bool step_ir_0x001116(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x41F9U; constexpr std::uint32_t kPc = 0x001116U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, 0x200009U);
  out_pc = 0x00111CU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00111C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3039U; constexpr std::uint32_t kPc = 0x00111CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = 0xFF10A8U;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x001122U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x001122(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x001122U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x001144U : 0x001126U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x001126(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x41F9U; constexpr std::uint32_t kPc = 0x001126U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, 0x2004C1U);
  out_pc = 0x00112CU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00112C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x00112CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x00112EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00112E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00112EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x001144U : 0x001132U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x001132(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x41F9U; constexpr std::uint32_t kPc = 0x001132U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, 0x200979U);
  out_pc = 0x001138U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001138(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x001138U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x00113AU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00113A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00113AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x001144U : 0x00113EU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00113E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x41F9U; constexpr std::uint32_t kPc = 0x00113EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, 0x200E31U);
  out_pc = 0x001144U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001144(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4CDFU; constexpr std::uint32_t kPc = 0x001144U;
  std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t mv_0 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  bus.set_d(0, mv_0);
  bus.set_a(7, sp);
  out_pc = 0x001148U; return bus.end_insn_mc(out_pc, kOp, 20, 56, true);
}
template <typename Bus> inline bool step_ir_0x001148(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x001148U;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00114A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x48E7U; constexpr std::uint32_t kPc = 0x00114AU;
  std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(0)); if (bus.faulted()) return false;
  bus.set_a(7, sp);
  out_pc = 0x00114EU; return bus.end_insn_mc(out_pc, kOp, 16, 56, true);
}
template <typename Bus> inline bool step_ir_0x00114E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x41F9U; constexpr std::uint32_t kPc = 0x00114EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, 0x2012E9U);
  out_pc = 0x001154U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001154(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3039U; constexpr std::uint32_t kPc = 0x001154U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = 0xFF10A8U;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00115AU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00115A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00115AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00117CU : 0x00115EU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00115E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x41F9U; constexpr std::uint32_t kPc = 0x00115EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, 0x2017A1U);
  out_pc = 0x001164U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001164(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x001164U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x001166U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x001166(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x001166U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00117CU : 0x00116AU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00116A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x41F9U; constexpr std::uint32_t kPc = 0x00116AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, 0x201C59U);
  out_pc = 0x001170U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001170(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x001170U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x001172U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x001172(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x001172U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00117CU : 0x001176U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x001176(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x41F9U; constexpr std::uint32_t kPc = 0x001176U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, 0x202111U);
  out_pc = 0x00117CU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00117C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4CDFU; constexpr std::uint32_t kPc = 0x00117CU;
  std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t mv_0 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  bus.set_d(0, mv_0);
  bus.set_a(7, sp);
  out_pc = 0x001180U; return bus.end_insn_mc(out_pc, kOp, 20, 56, true);
}
template <typename Bus> inline bool step_ir_0x001180(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x001180U;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x001E46(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x48E7U; constexpr std::uint32_t kPc = 0x001E46U;
  std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(6)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(5)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(2)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(1)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(0)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(2)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(1)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(0)); if (bus.faulted()) return false;
  bus.set_a(7, sp);
  out_pc = 0x001E4AU; return bus.end_insn_mc(out_pc, kOp, 72, 448, true);
}
template <typename Bus> inline bool step_ir_0x001E4A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4BF9U; constexpr std::uint32_t kPc = 0x001E4AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(5, 0xA130F1U);
  out_pc = 0x001E50U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001E50(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1ABCU; constexpr std::uint32_t kPc = 0x001E50U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(0x1U);
  const std::uint32_t da = bus.a(5) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x001E54U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001E54(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2A4EU; constexpr std::uint32_t kPc = 0x001E54U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.a(6));
  bus.set_a(5, v); out_pc = 0x001E56U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x001E56(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x43F9U; constexpr std::uint32_t kPc = 0x001E56U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(1, 0x2025CDU);
  out_pc = 0x001E5CU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x001E5C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x45FAU; constexpr std::uint32_t kPc = 0x001E5CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(2, 0x0004BAU);
  out_pc = 0x001E60U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x001E60(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7205U; constexpr std::uint32_t kPc = 0x001E60U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000005U;
  bus.set_d(1, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x001E62U; return bus.end_insn(out_pc, kOp, 4, 0);
}
} // namespace oasis::recompiler::ir
