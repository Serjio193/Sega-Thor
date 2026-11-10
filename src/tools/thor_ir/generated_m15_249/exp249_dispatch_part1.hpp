#pragma once
#include "tools/thor_ir/generated_m15_249/exp249_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x00F9AC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3039U; constexpr std::uint32_t kPc = 0x00F9ACU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = 0xFF19F0U;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00F9B2U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00F9B2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x602EU; constexpr std::uint32_t kPc = 0x00F9B2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x00F9E2U; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x00F9B4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3039U; constexpr std::uint32_t kPc = 0x00F9B4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = 0xFF19F4U;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00F9BAU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00F9BA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6026U; constexpr std::uint32_t kPc = 0x00F9BAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x00F9E2U; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x00F9BC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C79U; constexpr std::uint32_t kPc = 0x00F9BCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x1AU);
  const std::uint32_t addr_d = 0xFF1AA4U;
  if (!bus.check_align(addr_d, IrOpSize::Word)) return false;
  const std::uint16_t d = static_cast<std::uint16_t>(bus.read_m(addr_d, IrOpSize::Word));
  if (bus.faulted()) return false;
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00F9C4U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00F9C4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6600U; constexpr std::uint32_t kPc = 0x00F9C4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x00FA2CU : 0x00F9C8U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00F9C8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3039U; constexpr std::uint32_t kPc = 0x00F9C8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = 0xFF1AACU;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00F9CEU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00F9CE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6012U; constexpr std::uint32_t kPc = 0x00F9CEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x00F9E2U; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x016D5C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x102EU; constexpr std::uint32_t kPc = 0x016D5CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(102))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Byte)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.read_m(addr_v, IrOpSize::Byte));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 1);
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x016D60U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x016D60(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6600U; constexpr std::uint32_t kPc = 0x016D60U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x016D76U : 0x016D64U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x016D64(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x522EU; constexpr std::uint32_t kPc = 0x016D64U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint32_t addr_d = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(102))) & kBusAddressMask;
  if (!bus.check_align(addr_d, IrOpSize::Byte)) return false;
  const std::uint8_t d = static_cast<std::uint8_t>(bus.read_m(addr_d, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t r = static_cast<std::uint8_t>(d + s);
  bus.write_m(addr_d, IrOpSize::Byte, r); if (bus.faulted()) return false;
  bus.set_ccr(ccr_add<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x016D68U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x016D68(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x302EU; constexpr std::uint32_t kPc = 0x016D68U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(160))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x016D6CU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x016D6C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D40U; constexpr std::uint32_t kPc = 0x016D6CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(0));
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(138))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x016D70U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x016D70(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D40U; constexpr std::uint32_t kPc = 0x016D70U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(0));
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(136))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x016D74U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x016D74(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x016D74U;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x0170E0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x102EU; constexpr std::uint32_t kPc = 0x0170E0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(102))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Byte)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.read_m(addr_v, IrOpSize::Byte));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 1);
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x0170E4U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x0170E4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x0170E4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x0170F4U : 0x0170E8U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x0170E8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x0170E8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x0170EAU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x0170EA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x0170EAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x01710AU : 0x0170EEU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x0170EE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x0170EEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x0170F0U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x0170F0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x0170F0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x01716CU : 0x0170F4U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x0170F4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1D7CU; constexpr std::uint32_t kPc = 0x0170F4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(0x1U);
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(102))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x0170FAU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x0170FA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x302EU; constexpr std::uint32_t kPc = 0x0170FAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(160))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x0170FEU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x0170FE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D40U; constexpr std::uint32_t kPc = 0x0170FEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(0));
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(2))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x017102U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x017102(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D6EU; constexpr std::uint32_t kPc = 0x017102U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(140))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(16))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x017108U; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x017108(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x017108U;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x0171C4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x102EU; constexpr std::uint32_t kPc = 0x0171C4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(102))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Byte)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.read_m(addr_v, IrOpSize::Byte));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 1);
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x0171C8U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x0171C8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x0171C8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x0171D8U : 0x0171CCU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x0171CC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x0171CCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x0171CEU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x0171CE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x0171CEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x0171F8U : 0x0171D2U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
} // namespace oasis::recompiler::ir
