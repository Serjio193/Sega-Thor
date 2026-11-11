#pragma once
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x0302FA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7214U; constexpr std::uint32_t kPc = 0x0302FAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000014U;
  bus.set_d(1, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x0302FCU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x0302FC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4A68U; constexpr std::uint32_t kPc = 0x0302FCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(0))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x030300U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x030300(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6F00U; constexpr std::uint32_t kPc = 0x030300U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 15);
  out_pc = taken ? 0x03031AU : 0x030304U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x030304(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x08A8U; constexpr std::uint32_t kPc = 0x030304U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x1U);
  const std::uint32_t addr_cur = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(57))) & kBusAddressMask;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.write_m(addr_cur, IrOpSize::Byte, static_cast<std::uint8_t>(cur & ~mask)); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x03030AU; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x03030A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0828U; constexpr std::uint32_t kPc = 0x03030AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x5U);
  const std::uint32_t addr_cur = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(110))) & kBusAddressMask;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x030310U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x030310(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x030310U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x03031AU : 0x030314U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x030314(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x08E8U; constexpr std::uint32_t kPc = 0x030314U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x1U);
  const std::uint32_t addr_cur = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(57))) & kBusAddressMask;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.write_m(addr_cur, IrOpSize::Byte, static_cast<std::uint8_t>(cur | mask)); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x03031AU; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x03031A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x41E8U; constexpr std::uint32_t kPc = 0x03031AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t lea_a = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(188))) & kBusAddressMask;
  bus.set_a(0, lea_a); out_pc = 0x03031EU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x03031E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x51C9U; constexpr std::uint32_t kPc = 0x03031EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t nw = static_cast<std::uint16_t>((bus.d(1) & 0xFFFFU) - 1U);
  write_dn(bus, 1, nw, 2);
  const bool taken = (nw != 0xFFFFU); out_pc = taken ? 0x0302FCU : 0x030322U;
  return bus.end_insn_mc(out_pc, kOp, taken ? 10 : 14, taken ? -14 : 14, false);
}
template <typename Bus> inline bool step_ir_0x030322(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x030322U;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
} // namespace oasis::recompiler::ir
