#pragma once
#include "tools/thor_ir/generated_m15_249/exp249_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x00E0B8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x00E0B8U;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00EA04(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x302EU; constexpr std::uint32_t kPc = 0x00EA04U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(42))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00EA08U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00EA08(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00EA08U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00EA16U : 0x00EA0CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00EA0C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C40U; constexpr std::uint32_t kPc = 0x00EA0CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x6U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00EA10U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00EA10(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6300U; constexpr std::uint32_t kPc = 0x00EA10U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 3);
  out_pc = taken ? 0x00EA76U : 0x00EA14U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00EA14(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x00EA14U;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00F96A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5488U; constexpr std::uint32_t kPc = 0x00F96AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, bus.a(0) + 0x2U);
  out_pc = 0x00F96CU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00F96C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1018U; constexpr std::uint32_t kPc = 0x00F96CU;
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
  out_pc = 0x00F96EU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00F96E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1218U; constexpr std::uint32_t kPc = 0x00F96EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_val = bus.a(0) & kBusAddressMask;
  if (!bus.check_align(addr_val, IrOpSize::Byte)) return false;
  const std::uint8_t val = static_cast<std::uint8_t>(bus.read_m(addr_val, IrOpSize::Byte));
  bus.set_a(0, (addr_val + 1U) & kBusAddressMask);
  if (bus.faulted()) return false;
  bus.set_d(1, (bus.d(1) & 0xFFFFFF00U) | (val & 0xFFU));
  const std::uint16_t old_ccr = bus.ccr();
  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);
  if ((val & static_cast<std::uint8_t>(1ULL << (7))) != 0U) new_ccr |= 0x08U;
  if (val == 0U) new_ccr |= 0x04U;
  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);
  bus.set_ccr(new_ccr);
  out_pc = 0x00F970U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00F970(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3418U; constexpr std::uint32_t kPc = 0x00F970U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_val = bus.a(0) & kBusAddressMask;
  if (!bus.check_align(addr_val, IrOpSize::Word)) return false;
  const std::uint16_t val = static_cast<std::uint16_t>(bus.read_m(addr_val, IrOpSize::Word));
  bus.set_a(0, (addr_val + 2U) & kBusAddressMask);
  if (bus.faulted()) return false;
  bus.set_d(2, (bus.d(2) & 0xFFFF0000U) | (val & 0xFFFFU));
  const std::uint16_t old_ccr = bus.ccr();
  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);
  if ((val & static_cast<std::uint16_t>(1ULL << (15))) != 0U) new_ccr |= 0x08U;
  if (val == 0U) new_ccr |= 0x04U;
  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);
  bus.set_ccr(new_ccr);
  out_pc = 0x00F972U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00F972(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4A00U; constexpr std::uint32_t kPc = 0x00F972U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.d(0));
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x00F974U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00F974(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00F974U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00F9A6U : 0x00F978U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00F978(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x00F978U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x00F97AU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00F97A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00F97AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00FA36U : 0x00F97EU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00F97E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x00F97EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x00F980U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00F980(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00F980U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00FA5CU : 0x00F984U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00F984(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x00F984U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x00F986U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00F986(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00F986U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00FA82U : 0x00F98AU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00F98A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x00F98AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x00F98CU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00F98C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00F98CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00F9ACU : 0x00F990U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00F990(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x00F990U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x00F992U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00F992(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00F992U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00F9B4U : 0x00F996U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00F996(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x00F996U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x00F998U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00F998(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00F998U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00F9BCU : 0x00F99CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00F99C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x00F99CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x00F99EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00F99E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00F99EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00F9D0U : 0x00F9A2U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00F9A2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4240U; constexpr std::uint32_t kPc = 0x00F9A2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  write_dn(bus, 0, 0U, 2);
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x00F9A4U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00F9A4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x603CU; constexpr std::uint32_t kPc = 0x00F9A4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x00F9E2U; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x00F9A6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x00F9A6U;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x00F9AAU); if (bus.faulted()) return false;
  out_pc = 0x0101ECU; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x00F9AA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6036U; constexpr std::uint32_t kPc = 0x00F9AAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x00F9E2U; return bus.end_insn(out_pc, kOp, 10, 0);
}
} // namespace oasis::recompiler::ir
