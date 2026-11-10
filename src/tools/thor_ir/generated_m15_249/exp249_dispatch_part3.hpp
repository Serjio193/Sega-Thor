#pragma once
#include "tools/thor_ir/generated_m15_249/exp249_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x02552E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7BU; constexpr std::uint32_t kPc = 0x02552EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_val = (0x025570U + static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(bus.d(0) & 0xFFFFU)))) & kBusAddressMask;
  if (!bus.check_align(addr_val, IrOpSize::Word)) return false;
  const std::uint16_t val = static_cast<std::uint16_t>(bus.read_m(addr_val, IrOpSize::Word));
  if (bus.faulted()) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(160))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false;
  bus.write_m(da, IrOpSize::Word, val);
  if (bus.faulted()) return false;
  const std::uint16_t old_ccr = bus.ccr();
  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);
  if ((val & static_cast<std::uint16_t>(1ULL << (15))) != 0U) new_ccr |= 0x08U;
  if (val == 0U) new_ccr |= 0x04U;
  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);
  bus.set_ccr(new_ccr);
  out_pc = 0x025534U; return bus.end_insn(out_pc, kOp, 22, 0);
}
template <typename Bus> inline bool step_ir_0x025534(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6000U; constexpr std::uint32_t kPc = 0x025534U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x0255D8U; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x025538(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7CU; constexpr std::uint32_t kPc = 0x025538U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x10U);
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(4))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x02553EU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02553E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7CU; constexpr std::uint32_t kPc = 0x02553EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x10U);
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(42))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x025544U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x025544(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x08EEU; constexpr std::uint32_t kPc = 0x025544U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x1U);
  const std::uint32_t addr_cur = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(54))) & kBusAddressMask;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.write_m(addr_cur, IrOpSize::Byte, static_cast<std::uint8_t>(cur | mask)); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x02554AU; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x02554A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x08EEU; constexpr std::uint32_t kPc = 0x02554AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x0U);
  const std::uint32_t addr_cur = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(55))) & kBusAddressMask;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.write_m(addr_cur, IrOpSize::Byte, static_cast<std::uint8_t>(cur | mask)); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x025550U; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x025550(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x426EU; constexpr std::uint32_t kPc = 0x025550U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(74))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, 0U); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x025554U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x025554(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6054U; constexpr std::uint32_t kPc = 0x025554U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x0255AAU; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x025556(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7CU; constexpr std::uint32_t kPc = 0x025556U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x12U);
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(4))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x02555CU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02555C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7CU; constexpr std::uint32_t kPc = 0x02555CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x10U);
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(42))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x025562U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x025562(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x426EU; constexpr std::uint32_t kPc = 0x025562U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(74))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, 0U); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x025566U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x025566(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6042U; constexpr std::uint32_t kPc = 0x025566U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x0255AAU; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x025578(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7CU; constexpr std::uint32_t kPc = 0x025578U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x14U);
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(4))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x02557EU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02557E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7CU; constexpr std::uint32_t kPc = 0x02557EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x12U);
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(42))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x025584U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x025584(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x08EEU; constexpr std::uint32_t kPc = 0x025584U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x1U);
  const std::uint32_t addr_cur = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(54))) & kBusAddressMask;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.write_m(addr_cur, IrOpSize::Byte, static_cast<std::uint8_t>(cur | mask)); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x02558AU; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x02558A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x08EEU; constexpr std::uint32_t kPc = 0x02558AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x0U);
  const std::uint32_t addr_cur = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(55))) & kBusAddressMask;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.write_m(addr_cur, IrOpSize::Byte, static_cast<std::uint8_t>(cur | mask)); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x025590U; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x025590(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7CU; constexpr std::uint32_t kPc = 0x025590U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x100U);
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(16))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x025596U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x025596(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6012U; constexpr std::uint32_t kPc = 0x025596U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x0255AAU; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x025598(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7CU; constexpr std::uint32_t kPc = 0x025598U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x16U);
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(4))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x02559EU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02559E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7CU; constexpr std::uint32_t kPc = 0x02559EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x12U);
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(42))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x0255A4U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x0255A4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7CU; constexpr std::uint32_t kPc = 0x0255A4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x100U);
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(16))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x0255AAU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x0255AA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x51EEU; constexpr std::uint32_t kPc = 0x0255AAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(102))) & kBusAddressMask;
  bus.write_m(da, IrOpSize::Byte, 0U); if (bus.faulted()) return false;
  out_pc = 0x0255AEU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x0255AE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x002EU; constexpr std::uint32_t kPc = 0x0255AEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0xCU);
  const std::uint32_t addr_d = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(56))) & kBusAddressMask;
  if (!bus.check_align(addr_d, IrOpSize::Byte)) return false;
  const std::uint8_t d = static_cast<std::uint8_t>(bus.read_m(addr_d, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t r = static_cast<std::uint8_t>(d | s);
  bus.write_m(addr_d, IrOpSize::Byte, r); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(r, bus.ccr())); out_pc = 0x0255B4U; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x0255B4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x08AEU; constexpr std::uint32_t kPc = 0x0255B4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x7U);
  const std::uint32_t addr_cur = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(150))) & kBusAddressMask;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.write_m(addr_cur, IrOpSize::Byte, static_cast<std::uint8_t>(cur & ~mask)); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x0255BAU; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x0255BA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x0255BAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x0255C4U : 0x0255BEU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x0255BE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x08EEU; constexpr std::uint32_t kPc = 0x0255BEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x5U);
  const std::uint32_t addr_cur = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(56))) & kBusAddressMask;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.write_m(addr_cur, IrOpSize::Byte, static_cast<std::uint8_t>(cur | mask)); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x0255C4U; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x0255C4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x302EU; constexpr std::uint32_t kPc = 0x0255C4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(150))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x0255C8U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x0255C8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD040U; constexpr std::uint32_t kPc = 0x0255C8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(0));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 0, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x0255CAU; return bus.end_insn(out_pc, kOp, 4, 0);
}
} // namespace oasis::recompiler::ir
