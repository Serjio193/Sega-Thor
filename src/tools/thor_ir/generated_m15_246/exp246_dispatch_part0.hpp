#pragma once
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x006DD2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7200U; constexpr std::uint32_t kPc = 0x006DD2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000000U;
  bus.set_d(1, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x006DD4U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x006DD4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2401U; constexpr std::uint32_t kPc = 0x006DD4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.d(1));
  write_dn(bus, 2, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x006DD6U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x006DD6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4A39U; constexpr std::uint32_t kPc = 0x006DD6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = 0xFF17BBU;
  if (!bus.check_align(addr_v, IrOpSize::Byte)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.read_m(addr_v, IrOpSize::Byte));
  if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x006DDCU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x006DDC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6604U; constexpr std::uint32_t kPc = 0x006DDCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x006DE2U : 0x006DDEU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x006DDE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x006DDEU;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x006DE2U); if (bus.faulted()) return false;
  out_pc = 0x0085E2U; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x006DE2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C6EU; constexpr std::uint32_t kPc = 0x006DE2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x4CU);
  const std::uint32_t addr_d = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(42))) & kBusAddressMask;
  if (!bus.check_align(addr_d, IrOpSize::Word)) return false;
  const std::uint16_t d = static_cast<std::uint16_t>(bus.read_m(addr_d, IrOpSize::Word));
  if (bus.faulted()) return false;
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x006DE8U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x006DE8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x006DE8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x006E4AU : 0x006DECU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x006DEC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD2AEU; constexpr std::uint32_t kPc = 0x006DECU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_s = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(78))) & kBusAddressMask;
  if (!bus.check_align(addr_s, IrOpSize::Long)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(bus.read_m(addr_s, IrOpSize::Long));
  if (bus.faulted()) return false;
  const std::uint32_t d = static_cast<std::uint32_t>(bus.d(1));
  const std::uint32_t r = static_cast<std::uint32_t>(d + s);
  write_dn(bus, 1, r, 4);
  bus.set_ccr(ccr_add<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x006DF0U; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x006DF0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD4AEU; constexpr std::uint32_t kPc = 0x006DF0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_s = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(82))) & kBusAddressMask;
  if (!bus.check_align(addr_s, IrOpSize::Long)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(bus.read_m(addr_s, IrOpSize::Long));
  if (bus.faulted()) return false;
  const std::uint32_t d = static_cast<std::uint32_t>(bus.d(2));
  const std::uint32_t r = static_cast<std::uint32_t>(d + s);
  write_dn(bus, 2, r, 4);
  bus.set_ccr(ccr_add<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x006DF4U; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x006DF4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x202EU; constexpr std::uint32_t kPc = 0x006DF4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(86))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x006DF8U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x006DF8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6B30U; constexpr std::uint32_t kPc = 0x006DF8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 11);
  out_pc = taken ? 0x006E2AU : 0x006DFAU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x006DFA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x006DFAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x006E4AU : 0x006DFEU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x006DFE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0480U; constexpr std::uint32_t kPc = 0x006DFEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(0xC000U);
  const std::uint32_t d = static_cast<std::uint32_t>(bus.d(0));
  const std::uint32_t r = static_cast<std::uint32_t>(d - s);
  write_dn(bus, 0, r, 4);
  bus.set_ccr(ccr_sub<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x006E04U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x006E04(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6E0EU; constexpr std::uint32_t kPc = 0x006E04U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 14);
  out_pc = taken ? 0x006E14U : 0x006E06U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x006E06(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6602U; constexpr std::uint32_t kPc = 0x006E06U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x006E0AU : 0x006E08U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x006E08(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5380U; constexpr std::uint32_t kPc = 0x006E08U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(0x1U);
  const std::uint32_t d = static_cast<std::uint32_t>(bus.d(0));
  const std::uint32_t r = static_cast<std::uint32_t>(d - s);
  write_dn(bus, 0, r, 4);
  bus.set_ccr(ccr_sub<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x006E0AU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x006E0A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7CU; constexpr std::uint32_t kPc = 0x006E0AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x4U);
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(48))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x006E10U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x006E10(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x426EU; constexpr std::uint32_t kPc = 0x006E10U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(46))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, 0U); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x006E14U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x006E14(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2D40U; constexpr std::uint32_t kPc = 0x006E14U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.d(0));
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(86))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Long)) return false; bus.write_m(da, IrOpSize::Long, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x006E18U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x006E18(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD1AEU; constexpr std::uint32_t kPc = 0x006E18U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(bus.d(0));
  const std::uint32_t addr_d = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(16))) & kBusAddressMask;
  if (!bus.check_align(addr_d, IrOpSize::Long)) return false;
  const std::uint32_t d = static_cast<std::uint32_t>(bus.read_m(addr_d, IrOpSize::Long));
  if (bus.faulted()) return false;
  const std::uint32_t r = static_cast<std::uint32_t>(d + s);
  bus.write_m(addr_d, IrOpSize::Long, r); if (bus.faulted()) return false;
  bus.set_ccr(ccr_add<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x006E1CU; return bus.end_insn(out_pc, kOp, 24, 0);
}
template <typename Bus> inline bool step_ir_0x006E1C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD3AEU; constexpr std::uint32_t kPc = 0x006E1CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(bus.d(1));
  const std::uint32_t addr_d = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(114))) & kBusAddressMask;
  if (!bus.check_align(addr_d, IrOpSize::Long)) return false;
  const std::uint32_t d = static_cast<std::uint32_t>(bus.read_m(addr_d, IrOpSize::Long));
  if (bus.faulted()) return false;
  const std::uint32_t r = static_cast<std::uint32_t>(d + s);
  bus.write_m(addr_d, IrOpSize::Long, r); if (bus.faulted()) return false;
  bus.set_ccr(ccr_add<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x006E20U; return bus.end_insn(out_pc, kOp, 24, 0);
}
template <typename Bus> inline bool step_ir_0x006E20(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD5AEU; constexpr std::uint32_t kPc = 0x006E20U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(bus.d(2));
  const std::uint32_t addr_d = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(118))) & kBusAddressMask;
  if (!bus.check_align(addr_d, IrOpSize::Long)) return false;
  const std::uint32_t d = static_cast<std::uint32_t>(bus.read_m(addr_d, IrOpSize::Long));
  if (bus.faulted()) return false;
  const std::uint32_t r = static_cast<std::uint32_t>(d + s);
  bus.write_m(addr_d, IrOpSize::Long, r); if (bus.faulted()) return false;
  bus.set_ccr(ccr_add<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x006E24U; return bus.end_insn(out_pc, kOp, 24, 0);
}
template <typename Bus> inline bool step_ir_0x006E24(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x006E24U;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x006E28U); if (bus.faulted()) return false;
  out_pc = 0x006E7AU; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x00BCBA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x23FCU; constexpr std::uint32_t kPc = 0x00BCBAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(0xBE98U);
  const std::uint32_t da = 0xFF1940U;
  if (!bus.check_align(da, IrOpSize::Long)) return false; bus.write_m(da, IrOpSize::Long, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x00BCC4U; return bus.end_insn(out_pc, kOp, 28, 0);
}
template <typename Bus> inline bool step_ir_0x00BCC4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x13FCU; constexpr std::uint32_t kPc = 0x00BCC4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(0x2U);
  const std::uint32_t da = 0xFF198BU;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x00BCCCU; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x00BCCC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x41F9U; constexpr std::uint32_t kPc = 0x00BCCCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, 0xFF1CD8U);
  out_pc = 0x00BCD2U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BCD2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7E10U; constexpr std::uint32_t kPc = 0x00BCD2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000010U;
  bus.set_d(7, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x00BCD4U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BCD4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6000U; constexpr std::uint32_t kPc = 0x00BCD4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x00BCEAU; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x00BCD8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x23FCU; constexpr std::uint32_t kPc = 0x00BCD8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(0xBEBAU);
  const std::uint32_t da = 0xFF1940U;
  if (!bus.check_align(da, IrOpSize::Long)) return false; bus.write_m(da, IrOpSize::Long, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x00BCE2U; return bus.end_insn(out_pc, kOp, 28, 0);
}
template <typename Bus> inline bool step_ir_0x00BCE2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x41F9U; constexpr std::uint32_t kPc = 0x00BCE2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, 0xFF19E8U);
  out_pc = 0x00BCE8U; return bus.end_insn(out_pc, kOp, 12, 0);
}
} // namespace oasis::recompiler::ir
