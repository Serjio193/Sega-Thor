#pragma once
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x00BDFC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xDA68U; constexpr std::uint32_t kPc = 0x00BDFCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_s = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(20))) & kBusAddressMask;
  if (!bus.check_align(addr_s, IrOpSize::Word)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.read_m(addr_s, IrOpSize::Word));
  if (bus.faulted()) return false;
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 5, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BE00U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BE00(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3C05U; constexpr std::uint32_t kPc = 0x00BE00U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(5));
  write_dn(bus, 6, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00BE02U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE02(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xDC68U; constexpr std::uint32_t kPc = 0x00BE02U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_s = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(74))) & kBusAddressMask;
  if (!bus.check_align(addr_s, IrOpSize::Word)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.read_m(addr_s, IrOpSize::Word));
  if (bus.faulted()) return false;
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(6));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 6, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BE06U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BE06(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xBE45U; constexpr std::uint32_t kPc = 0x00BE06U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(7));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00BE08U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE08(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6D00U; constexpr std::uint32_t kPc = 0x00BE08U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 13);
  out_pc = taken ? 0x00BE5EU : 0x00BE0CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00BE0C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xB046U; constexpr std::uint32_t kPc = 0x00BE0CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(6));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00BE0EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE0E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6E00U; constexpr std::uint32_t kPc = 0x00BE0EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 14);
  out_pc = taken ? 0x00BE5EU : 0x00BE12U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00BE12(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3240U; constexpr std::uint32_t kPc = 0x00BE12U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(0));
  bus.set_a(1, static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(v)))); out_pc = 0x00BE14U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE14(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3028U; constexpr std::uint32_t kPc = 0x00BE14U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(66))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00BE18U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BE18(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3A28U; constexpr std::uint32_t kPc = 0x00BE18U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(8))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 5, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00BE1CU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BE1C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3C05U; constexpr std::uint32_t kPc = 0x00BE1CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(5));
  write_dn(bus, 6, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00BE1EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE1E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x9A40U; constexpr std::uint32_t kPc = 0x00BE1EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(0));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t r = static_cast<std::uint16_t>(d - s);
  write_dn(bus, 5, r, 2);
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BE20U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE20(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xDC40U; constexpr std::uint32_t kPc = 0x00BE20U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(0));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(6));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 6, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BE22U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE22(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xB246U; constexpr std::uint32_t kPc = 0x00BE22U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(6));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00BE24U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE24(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6E00U; constexpr std::uint32_t kPc = 0x00BE24U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 14);
  out_pc = taken ? 0x00BE5CU : 0x00BE28U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00BE28(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xB645U; constexpr std::uint32_t kPc = 0x00BE28U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(3));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00BE2AU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE2A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6D00U; constexpr std::uint32_t kPc = 0x00BE2AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 13);
  out_pc = taken ? 0x00BE5CU : 0x00BE2EU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00BE2E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3028U; constexpr std::uint32_t kPc = 0x00BE2EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(68))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00BE32U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BE32(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3A28U; constexpr std::uint32_t kPc = 0x00BE32U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(12))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 5, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00BE36U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BE36(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xDA68U; constexpr std::uint32_t kPc = 0x00BE36U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_s = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(20))) & kBusAddressMask;
  if (!bus.check_align(addr_s, IrOpSize::Word)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.read_m(addr_s, IrOpSize::Word));
  if (bus.faulted()) return false;
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 5, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BE3AU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BE3A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3C05U; constexpr std::uint32_t kPc = 0x00BE3AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(5));
  write_dn(bus, 6, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00BE3CU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE3C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x9A40U; constexpr std::uint32_t kPc = 0x00BE3CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(0));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t r = static_cast<std::uint16_t>(d - s);
  write_dn(bus, 5, r, 2);
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BE3EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE3E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xDC40U; constexpr std::uint32_t kPc = 0x00BE3EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(0));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(6));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 6, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BE40U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE40(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xB446U; constexpr std::uint32_t kPc = 0x00BE40U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(6));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(2));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00BE42U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE42(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6E00U; constexpr std::uint32_t kPc = 0x00BE42U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 14);
  out_pc = taken ? 0x00BE5CU : 0x00BE46U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00BE46(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xB845U; constexpr std::uint32_t kPc = 0x00BE46U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(4));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00BE48U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BE48(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6D00U; constexpr std::uint32_t kPc = 0x00BE48U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 13);
  out_pc = taken ? 0x00BE5CU : 0x00BE4CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00BE4C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4A28U; constexpr std::uint32_t kPc = 0x00BE4CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(130))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Byte)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.read_m(addr_v, IrOpSize::Byte));
  if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x00BE50U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BE50(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6600U; constexpr std::uint32_t kPc = 0x00BE50U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x00BE5CU : 0x00BE54U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00BE54(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2479U; constexpr std::uint32_t kPc = 0x00BE54U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = 0xFF1940U;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  bus.set_a(2, v); out_pc = 0x00BE5AU; return bus.end_insn(out_pc, kOp, 20, 0);
}
} // namespace oasis::recompiler::ir
