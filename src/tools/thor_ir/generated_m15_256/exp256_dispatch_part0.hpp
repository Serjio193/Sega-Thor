#pragma once
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x006B12(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xB641U; constexpr std::uint32_t kPc = 0x006B12U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(3));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x006B14U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x006B14(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6D00U; constexpr std::uint32_t kPc = 0x006B14U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 13);
  out_pc = taken ? 0x006B24U : 0x006B18U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x006B18(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x006B18U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x006B28U : 0x006B1CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x006B1C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5343U; constexpr std::uint32_t kPc = 0x006B1CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x1U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(3));
  const std::uint16_t r = static_cast<std::uint16_t>(d - s);
  write_dn(bus, 3, r, 2);
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x006B1EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x006B1E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5205U; constexpr std::uint32_t kPc = 0x006B1EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(5));
  const std::uint8_t r = static_cast<std::uint8_t>(d + s);
  write_dn(bus, 5, r, 1);
  bus.set_ccr(ccr_add<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x006B20U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x006B20(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6000U; constexpr std::uint32_t kPc = 0x006B20U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x006B28U; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x006B24(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5243U; constexpr std::uint32_t kPc = 0x006B24U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x1U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(3));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 3, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x006B26U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x006B26(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5205U; constexpr std::uint32_t kPc = 0x006B26U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(5));
  const std::uint8_t r = static_cast<std::uint8_t>(d + s);
  write_dn(bus, 5, r, 1);
  bus.set_ccr(ccr_add<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x006B28U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x006B28(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x006B28U;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x010F48(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x322DU; constexpr std::uint32_t kPc = 0x010F48U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(5) + static_cast<std::uint32_t>(static_cast<std::int32_t>(8))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 1, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x010F4CU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x010F4C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x342DU; constexpr std::uint32_t kPc = 0x010F4CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(5) + static_cast<std::uint32_t>(static_cast<std::int32_t>(12))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 2, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x010F50U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x010F50(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x926EU; constexpr std::uint32_t kPc = 0x010F50U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_s = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(8))) & kBusAddressMask;
  if (!bus.check_align(addr_s, IrOpSize::Word)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.read_m(addr_s, IrOpSize::Word));
  if (bus.faulted()) return false;
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t r = static_cast<std::uint16_t>(d - s);
  write_dn(bus, 1, r, 2);
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x010F54U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x010F54(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x946EU; constexpr std::uint32_t kPc = 0x010F54U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_s = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(12))) & kBusAddressMask;
  if (!bus.check_align(addr_s, IrOpSize::Word)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.read_m(addr_s, IrOpSize::Word));
  if (bus.faulted()) return false;
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(2));
  const std::uint16_t r = static_cast<std::uint16_t>(d - s);
  write_dn(bus, 2, r, 2);
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x010F58U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x010F58(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3601U; constexpr std::uint32_t kPc = 0x010F58U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(1));
  write_dn(bus, 3, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x010F5AU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x010F5A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6A02U; constexpr std::uint32_t kPc = 0x010F5AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 10);
  out_pc = taken ? 0x010F5EU : 0x010F5CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x010F5C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4443U; constexpr std::uint32_t kPc = 0x010F5CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(3)); const std::uint16_t v = static_cast<std::uint16_t>(0U - s);
  write_dn(bus, 3, v, 2); bus.set_ccr(ccr_sub<std::uint16_t>(0U, s, bus.ccr(), true));
  out_pc = 0x010F5EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x010F5E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3802U; constexpr std::uint32_t kPc = 0x010F5EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(2));
  write_dn(bus, 4, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x010F60U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x010F60(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6A02U; constexpr std::uint32_t kPc = 0x010F60U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 10);
  out_pc = taken ? 0x010F64U : 0x010F62U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x010F62(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4444U; constexpr std::uint32_t kPc = 0x010F62U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(4)); const std::uint16_t v = static_cast<std::uint16_t>(0U - s);
  write_dn(bus, 4, v, 2); bus.set_ccr(ccr_sub<std::uint16_t>(0U, s, bus.ccr(), true));
  out_pc = 0x010F64U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x010F64(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xB644U; constexpr std::uint32_t kPc = 0x010F64U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(4));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(3));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x010F66U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x010F66(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x630CU; constexpr std::uint32_t kPc = 0x010F66U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 3);
  out_pc = taken ? 0x010F74U : 0x010F68U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x010F68(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4A41U; constexpr std::uint32_t kPc = 0x010F68U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(1));
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x010F6AU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x010F6A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6B04U; constexpr std::uint32_t kPc = 0x010F6AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 11);
  out_pc = taken ? 0x010F70U : 0x010F6CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x010F6C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7001U; constexpr std::uint32_t kPc = 0x010F6CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000001U;
  bus.set_d(0, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x010F6EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x010F6E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x600EU; constexpr std::uint32_t kPc = 0x010F6EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x010F7EU; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x010F70(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7003U; constexpr std::uint32_t kPc = 0x010F70U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000003U;
  bus.set_d(0, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x010F72U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x010F72(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x600AU; constexpr std::uint32_t kPc = 0x010F72U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x010F7EU; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x010F74(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4A42U; constexpr std::uint32_t kPc = 0x010F74U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(2));
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x010F76U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x010F76(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6B04U; constexpr std::uint32_t kPc = 0x010F76U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 11);
  out_pc = taken ? 0x010F7CU : 0x010F78U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x010F78(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7002U; constexpr std::uint32_t kPc = 0x010F78U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000002U;
  bus.set_d(0, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x010F7AU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x010F7A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6002U; constexpr std::uint32_t kPc = 0x010F7AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x010F7EU; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x010F7C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7000U; constexpr std::uint32_t kPc = 0x010F7CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000000U;
  bus.set_d(0, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x010F7EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x010F7E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D40U; constexpr std::uint32_t kPc = 0x010F7EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(0));
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(22))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x010F82U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x010F82(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x010F82U;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
} // namespace oasis::recompiler::ir
