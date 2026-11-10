#pragma once
#include "tools/thor_ir/generated_m15_249/exp249_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x0171D2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x0171D2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x0171D4U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x0171D4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x0171D4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x017258U : 0x0171D8U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x0171D8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1D7CU; constexpr std::uint32_t kPc = 0x0171D8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(0x1U);
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(102))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x0171DEU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x0171DE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x302EU; constexpr std::uint32_t kPc = 0x0171DEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(160))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x0171E2U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x0171E2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D40U; constexpr std::uint32_t kPc = 0x0171E2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(0));
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(2))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x0171E6U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x0171E6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D6EU; constexpr std::uint32_t kPc = 0x0171E6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(140))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(16))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x0171ECU; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x0171EC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D6EU; constexpr std::uint32_t kPc = 0x0171ECU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(8))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(166))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x0171F2U; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x0171F2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x426EU; constexpr std::uint32_t kPc = 0x0171F2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(170))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, 0U); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x0171F6U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x0171F6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x0171F6U;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x0254EA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x102EU; constexpr std::uint32_t kPc = 0x0254EAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(102))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Byte)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.read_m(addr_v, IrOpSize::Byte));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 1);
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x0254EEU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x0254EE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x0254EEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x0256B8U : 0x0254F2U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x0254F2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x0254F2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x0254F4U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x0254F4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x0254F4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x02550EU : 0x0254F8U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x0254F8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x0254F8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x0254FAU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x0254FA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x0254FAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x025538U : 0x0254FEU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x0254FE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x0254FEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x025500U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x025500(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x025500U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x025556U : 0x025504U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x025504(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x025504U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x025506U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x025506(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x025506U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x025578U : 0x02550AU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x02550A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6000U; constexpr std::uint32_t kPc = 0x02550AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x025598U; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x02550E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x51EEU; constexpr std::uint32_t kPc = 0x02550EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(102))) & kBusAddressMask;
  bus.write_m(da, IrOpSize::Byte, 0U); if (bus.faulted()) return false;
  out_pc = 0x025512U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x025512(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x08AEU; constexpr std::uint32_t kPc = 0x025512U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x7U);
  const std::uint32_t addr_cur = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(150))) & kBusAddressMask;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.write_m(addr_cur, IrOpSize::Byte, static_cast<std::uint8_t>(cur & ~mask)); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x025518U; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x025518(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x025518U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x025522U : 0x02551CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x02551C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x08EEU; constexpr std::uint32_t kPc = 0x02551CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x5U);
  const std::uint32_t addr_cur = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(56))) & kBusAddressMask;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.write_m(addr_cur, IrOpSize::Byte, static_cast<std::uint8_t>(cur | mask)); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x025522U; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x025522(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x302EU; constexpr std::uint32_t kPc = 0x025522U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(150))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x025526U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x025526(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD040U; constexpr std::uint32_t kPc = 0x025526U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(0));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 0, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x025528U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x025528(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7BU; constexpr std::uint32_t kPc = 0x025528U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_val = (0x025568U + static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(bus.d(0) & 0xFFFFU)))) & kBusAddressMask;
  if (!bus.check_align(addr_val, IrOpSize::Word)) return false;
  const std::uint16_t val = static_cast<std::uint16_t>(bus.read_m(addr_val, IrOpSize::Word));
  if (bus.faulted()) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(144))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false;
  bus.write_m(da, IrOpSize::Word, val);
  if (bus.faulted()) return false;
  const std::uint16_t old_ccr = bus.ccr();
  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);
  if ((val & static_cast<std::uint16_t>(1ULL << (15))) != 0U) new_ccr |= 0x08U;
  if (val == 0U) new_ccr |= 0x04U;
  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);
  bus.set_ccr(new_ccr);
  out_pc = 0x02552EU; return bus.end_insn(out_pc, kOp, 22, 0);
}
} // namespace oasis::recompiler::ir
