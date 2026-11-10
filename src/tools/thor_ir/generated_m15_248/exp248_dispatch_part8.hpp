#pragma once
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x00FD38(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1D58U; constexpr std::uint32_t kPc = 0x00FD38U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_val = bus.a(0) & kBusAddressMask;
  if (!bus.check_align(addr_val, IrOpSize::Byte)) return false;
  const std::uint8_t val = static_cast<std::uint8_t>(bus.read_m(addr_val, IrOpSize::Byte));
  bus.set_a(0, (addr_val + 1U) & kBusAddressMask);
  if (bus.faulted()) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(102))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, val); if (bus.faulted()) return false;
  const std::uint16_t old_ccr = bus.ccr();
  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);
  if ((val & static_cast<std::uint8_t>(1ULL << (7))) != 0U) new_ccr |= 0x08U;
  if (val == 0U) new_ccr |= 0x04U;
  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);
  bus.set_ccr(new_ccr);
  out_pc = 0x00FD3CU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00FD3C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5688U; constexpr std::uint32_t kPc = 0x00FD3CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, bus.a(0) + 0x3U);
  out_pc = 0x00FD3EU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00FD3E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x00FD3EU;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x00FD42U); if (bus.faulted()) return false;
  out_pc = 0x009D00U; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x00FD42(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C47U; constexpr std::uint32_t kPc = 0x00FD42U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x2U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(7));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x00FD46U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00FD46(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6600U; constexpr std::uint32_t kPc = 0x00FD46U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x00FD5EU : 0x00FD4AU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00FD4A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x102EU; constexpr std::uint32_t kPc = 0x00FD4AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(110))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Byte)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.read_m(addr_v, IrOpSize::Byte));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 1);
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x00FD4EU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00FD4E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0200U; constexpr std::uint32_t kPc = 0x00FD4EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0xFU);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d & s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_logic<std::uint8_t>(r, bus.ccr())); out_pc = 0x00FD52U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00FD52(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6600U; constexpr std::uint32_t kPc = 0x00FD52U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x00FD5EU : 0x00FD56U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00FD56(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x426EU; constexpr std::uint32_t kPc = 0x00FD56U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(0))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, 0U); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x00FD5AU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00FD5A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x588FU; constexpr std::uint32_t kPc = 0x00FD5AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, bus.a(7) + 0x4U);
  out_pc = 0x00FD5CU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00FD5C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x00FD5CU;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x01A184(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x102EU; constexpr std::uint32_t kPc = 0x01A184U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(102))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Byte)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.read_m(addr_v, IrOpSize::Byte));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 1);
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x01A188U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x01A188(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x01A188U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x01A54EU : 0x01A18CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x01A18C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x01A18CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x01A18EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x01A18E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x01A18EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x01A1A2U : 0x01A192U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x01A192(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x01A192U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x01A194U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x01A194(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x01A194U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x01A1B6U : 0x01A198U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x01A198(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5300U; constexpr std::uint32_t kPc = 0x01A198U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x1U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d - s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), true)); out_pc = 0x01A19AU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x01A19A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x01A19AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x01A1D6U : 0x01A19EU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x01A19E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6000U; constexpr std::uint32_t kPc = 0x01A19EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x01A1F2U; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x01A1A2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x422EU; constexpr std::uint32_t kPc = 0x01A1A2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(102))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, 0U); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x01A1A6U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x01A1A6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x01A1A6U;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x01A1AAU); if (bus.faulted()) return false;
  out_pc = 0x01A21EU; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x01A1AA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x302EU; constexpr std::uint32_t kPc = 0x01A1AAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(22))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x01A1AEU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x01A1AE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1D40U; constexpr std::uint32_t kPc = 0x01A1AEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.d(0));
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(166))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x01A1B2U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x01A1B2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6000U; constexpr std::uint32_t kPc = 0x01A1B2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x01A3ECU; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x01A1B6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x422EU; constexpr std::uint32_t kPc = 0x01A1B6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(102))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, 0U); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x01A1BAU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x01A1BA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x01A1BAU;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x01A1BEU); if (bus.faulted()) return false;
  out_pc = 0x01A21EU; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x01A1BE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x302EU; constexpr std::uint32_t kPc = 0x01A1BEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(22))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x01A1C2U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x01A1C2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1D40U; constexpr std::uint32_t kPc = 0x01A1C2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.d(0));
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(166))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Byte)) return false; bus.write_m(da, IrOpSize::Byte, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x01A1C6U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x01A1C6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x303CU; constexpr std::uint32_t kPc = 0x01A1C6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x0U);
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x01A1CAU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x01A1CA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x01A1CAU;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x01A1CEU); if (bus.faulted()) return false;
  out_pc = 0x01A244U; return bus.end_insn(out_pc, kOp, 18, 0);
}
} // namespace oasis::recompiler::ir
