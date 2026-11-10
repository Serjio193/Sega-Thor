#pragma once
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x00BD52(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6532U; constexpr std::uint32_t kPc = 0x00BD52U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 5);
  out_pc = taken ? 0x00BD86U : 0x00BD54U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x00BD54(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5244U; constexpr std::uint32_t kPc = 0x00BD54U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x1U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(4));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 4, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BD56U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BD56(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C41U; constexpr std::uint32_t kPc = 0x00BD56U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x14U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00BD5AU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00BD5A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x652AU; constexpr std::uint32_t kPc = 0x00BD5AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 5);
  out_pc = taken ? 0x00BD86U : 0x00BD5CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x00BD5C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5244U; constexpr std::uint32_t kPc = 0x00BD5CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x1U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(4));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 4, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BD5EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BD5E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C41U; constexpr std::uint32_t kPc = 0x00BD5EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x1EU);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00BD62U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00BD62(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6522U; constexpr std::uint32_t kPc = 0x00BD62U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 5);
  out_pc = taken ? 0x00BD86U : 0x00BD64U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x00BD64(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5244U; constexpr std::uint32_t kPc = 0x00BD64U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x1U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(4));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 4, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BD66U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BD66(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C41U; constexpr std::uint32_t kPc = 0x00BD66U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x28U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00BD6AU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00BD6A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x651AU; constexpr std::uint32_t kPc = 0x00BD6AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 5);
  out_pc = taken ? 0x00BD86U : 0x00BD6CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x00BD6C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5244U; constexpr std::uint32_t kPc = 0x00BD6CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x1U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(4));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 4, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BD6EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BD6E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C41U; constexpr std::uint32_t kPc = 0x00BD6EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x32U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00BD72U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00BD72(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6512U; constexpr std::uint32_t kPc = 0x00BD72U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 5);
  out_pc = taken ? 0x00BD86U : 0x00BD74U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x00BD74(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5244U; constexpr std::uint32_t kPc = 0x00BD74U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x1U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(4));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 4, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BD76U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BD76(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C41U; constexpr std::uint32_t kPc = 0x00BD76U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x64U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00BD7AU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00BD7A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x650AU; constexpr std::uint32_t kPc = 0x00BD7AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 5);
  out_pc = taken ? 0x00BD86U : 0x00BD7CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x00BD7C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5444U; constexpr std::uint32_t kPc = 0x00BD7CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x2U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(4));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 4, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BD7EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BD7E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C41U; constexpr std::uint32_t kPc = 0x00BD7EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0xC8U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00BD82U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00BD82(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6502U; constexpr std::uint32_t kPc = 0x00BD82U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 5);
  out_pc = taken ? 0x00BD86U : 0x00BD84U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x00BD84(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5844U; constexpr std::uint32_t kPc = 0x00BD84U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x4U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(4));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 4, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BD86U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BD86(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1206U; constexpr std::uint32_t kPc = 0x00BD86U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.d(6));
  write_dn(bus, 1, v, 1);
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x00BD88U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BD88(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0206U; constexpr std::uint32_t kPc = 0x00BD88U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0xFU);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(6));
  const std::uint8_t r = static_cast<std::uint8_t>(d & s);
  write_dn(bus, 6, r, 1);
  bus.set_ccr(ccr_logic<std::uint8_t>(r, bus.ccr())); out_pc = 0x00BD8CU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00BD8C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5206U; constexpr std::uint32_t kPc = 0x00BD8CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(6));
  const std::uint8_t r = static_cast<std::uint8_t>(d + s);
  write_dn(bus, 6, r, 1);
  bus.set_ccr(ccr_add<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x00BD8EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BD8E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0241U; constexpr std::uint32_t kPc = 0x00BD8EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0xF0U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t r = static_cast<std::uint16_t>(d & s);
  write_dn(bus, 1, r, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(r, bus.ccr())); out_pc = 0x00BD92U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00BD92(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xE149U; constexpr std::uint32_t kPc = 0x00BD92U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  m15246_shift_imm<std::uint16_t, 2, 1>(bus, 1, 8);
  out_pc = 0x00BD94U; return bus.end_insn_mc(out_pc, kOp, 22, 112, false);
}
template <typename Bus> inline bool step_ir_0x00BD94(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x8841U; constexpr std::uint32_t kPc = 0x00BD94U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(4));
  const std::uint16_t r = static_cast<std::uint16_t>(d | s);
  write_dn(bus, 4, r, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(r, bus.ccr())); out_pc = 0x00BD96U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BD96(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD86EU; constexpr std::uint32_t kPc = 0x00BD96U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_s = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(144))) & kBusAddressMask;
  if (!bus.check_align(addr_s, IrOpSize::Word)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.read_m(addr_s, IrOpSize::Word));
  if (bus.faulted()) return false;
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(4));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 4, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BD9AU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BD9A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4844U; constexpr std::uint32_t kPc = 0x00BD9AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t o = bus.d(4); const std::uint32_t v = (o >> 16U) | (o << 16U);
  bus.set_d(4, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x00BD9CU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BD9C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1806U; constexpr std::uint32_t kPc = 0x00BD9CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.d(6));
  write_dn(bus, 4, v, 1);
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x00BD9EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BD9E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2844U; constexpr std::uint32_t kPc = 0x00BD9EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.d(4));
  bus.set_a(4, v); out_pc = 0x00BDA0U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BDA0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4847U; constexpr std::uint32_t kPc = 0x00BDA0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t o = bus.d(7); const std::uint32_t v = (o >> 16U) | (o << 16U);
  bus.set_d(7, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x00BDA2U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BDA2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x43F3U; constexpr std::uint32_t kPc = 0x00BDA2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t lea_a = (bus.a(3) + static_cast<std::uint32_t>(static_cast<std::int32_t>(8)) + static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(bus.d(0) & 0xFFFFU)))) & kBusAddressMask;
  bus.set_a(1, lea_a); out_pc = 0x00BDA6U; return bus.end_insn(out_pc, kOp, 12, 0);
}
} // namespace oasis::recompiler::ir
