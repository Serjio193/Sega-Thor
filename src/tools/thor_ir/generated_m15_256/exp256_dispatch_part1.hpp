#pragma once
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x0112CC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x48E7U; constexpr std::uint32_t kPc = 0x0112CCU;
  std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(1)); if (bus.faulted()) return false;
  bus.set_a(7, sp);
  out_pc = 0x0112D0U; return bus.end_insn_mc(out_pc, kOp, 16, 56, true);
}
template <typename Bus> inline bool step_ir_0x0112D0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4BF9U; constexpr std::uint32_t kPc = 0x0112D0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(5, 0xFF1CD8U);
  out_pc = 0x0112D6U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x0112D6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7210U; constexpr std::uint32_t kPc = 0x0112D6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000010U;
  bus.set_d(1, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x0112D8U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x0112D8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xB06DU; constexpr std::uint32_t kPc = 0x0112D8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_s = (bus.a(5) + static_cast<std::uint32_t>(static_cast<std::int32_t>(0))) & kBusAddressMask;
  if (!bus.check_align(addr_s, IrOpSize::Word)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.read_m(addr_s, IrOpSize::Word));
  if (bus.faulted()) return false;
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x0112DCU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x0112DC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6710U; constexpr std::uint32_t kPc = 0x0112DCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x0112EEU : 0x0112DEU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x0112DE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4BEDU; constexpr std::uint32_t kPc = 0x0112DEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t lea_a = (bus.a(5) + static_cast<std::uint32_t>(static_cast<std::int32_t>(188))) & kBusAddressMask;
  bus.set_a(5, lea_a); out_pc = 0x0112E2U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x0112E2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x51C9U; constexpr std::uint32_t kPc = 0x0112E2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t nw = static_cast<std::uint16_t>((bus.d(1) & 0xFFFFU) - 1U);
  write_dn(bus, 1, nw, 2);
  const bool taken = (nw != 0xFFFFU); out_pc = taken ? 0x0112D8U : 0x0112E6U;
  return bus.end_insn_mc(out_pc, kOp, taken ? 10 : 14, taken ? -14 : 14, false);
}
template <typename Bus> inline bool step_ir_0x0112E6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x70FFU; constexpr std::uint32_t kPc = 0x0112E6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0xFFFFFFFFU;
  bus.set_d(0, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x0112E8U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x0112E8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4CDFU; constexpr std::uint32_t kPc = 0x0112E8U;
  std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t mv_1 = bus.read_m(sp, IrOpSize::Long); if (bus.faulted()) return false; sp = (sp + 4U) & kBusAddressMask;
  bus.set_d(1, mv_1);
  bus.set_a(7, sp);
  out_pc = 0x0112ECU; return bus.end_insn_mc(out_pc, kOp, 20, 56, true);
}
template <typename Bus> inline bool step_ir_0x0112EC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x0112ECU;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x0141CA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C45U; constexpr std::uint32_t kPc = 0x0141CAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x50U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(5));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x0141CEU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x0141CE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x642EU; constexpr std::uint32_t kPc = 0x0141CEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 4);
  out_pc = taken ? 0x0141FEU : 0x0141D0U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x0141D0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C41U; constexpr std::uint32_t kPc = 0x0141D0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x68U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x0141D4U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x0141D4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6D00U; constexpr std::uint32_t kPc = 0x0141D4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 13);
  out_pc = taken ? 0x0141FEU : 0x0141D8U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x0141D8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C41U; constexpr std::uint32_t kPc = 0x0141D8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x1C0U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x0141DCU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x0141DC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6C00U; constexpr std::uint32_t kPc = 0x0141DCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 12);
  out_pc = taken ? 0x0141FEU : 0x0141E0U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x0141E0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C42U; constexpr std::uint32_t kPc = 0x0141E0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x70U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(2));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x0141E4U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x0141E4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6D00U; constexpr std::uint32_t kPc = 0x0141E4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 13);
  out_pc = taken ? 0x0141FEU : 0x0141E8U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x0141E8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C42U; constexpr std::uint32_t kPc = 0x0141E8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x160U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(2));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x0141ECU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x0141EC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6C00U; constexpr std::uint32_t kPc = 0x0141ECU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 12);
  out_pc = taken ? 0x0141FEU : 0x0141F0U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x0141F0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5245U; constexpr std::uint32_t kPc = 0x0141F0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x1U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 5, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x0141F2U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x0141F2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3AC2U; constexpr std::uint32_t kPc = 0x0141F2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t val = static_cast<std::uint16_t>(bus.d(2));
  const std::uint32_t addr_dst = bus.a(5) & kBusAddressMask;
  if (!bus.check_align(addr_dst, IrOpSize::Word)) return false;
  bus.write_m(addr_dst, IrOpSize::Word, val);
  bus.set_a(5, (addr_dst + 2U) & kBusAddressMask);
  if (bus.faulted()) return false;
  const std::uint16_t old_ccr = bus.ccr();
  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);
  if ((val & static_cast<std::uint16_t>(1ULL << (15))) != 0U) new_ccr |= 0x08U;
  if (val == 0U) new_ccr |= 0x04U;
  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);
  bus.set_ccr(new_ccr);
  out_pc = 0x0141F4U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x0141F4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3005U; constexpr std::uint32_t kPc = 0x0141F4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(5));
  write_dn(bus, 0, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x0141F6U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x0141F6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD044U; constexpr std::uint32_t kPc = 0x0141F6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(4));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 0, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x0141F8U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x0141F8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3AC0U; constexpr std::uint32_t kPc = 0x0141F8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t val = static_cast<std::uint16_t>(bus.d(0));
  const std::uint32_t addr_dst = bus.a(5) & kBusAddressMask;
  if (!bus.check_align(addr_dst, IrOpSize::Word)) return false;
  bus.write_m(addr_dst, IrOpSize::Word, val);
  bus.set_a(5, (addr_dst + 2U) & kBusAddressMask);
  if (bus.faulted()) return false;
  const std::uint16_t old_ccr = bus.ccr();
  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);
  if ((val & static_cast<std::uint16_t>(1ULL << (15))) != 0U) new_ccr |= 0x08U;
  if (val == 0U) new_ccr |= 0x04U;
  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);
  bus.set_ccr(new_ccr);
  out_pc = 0x0141FAU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x0141FA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3AC3U; constexpr std::uint32_t kPc = 0x0141FAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t val = static_cast<std::uint16_t>(bus.d(3));
  const std::uint32_t addr_dst = bus.a(5) & kBusAddressMask;
  if (!bus.check_align(addr_dst, IrOpSize::Word)) return false;
  bus.write_m(addr_dst, IrOpSize::Word, val);
  bus.set_a(5, (addr_dst + 2U) & kBusAddressMask);
  if (bus.faulted()) return false;
  const std::uint16_t old_ccr = bus.ccr();
  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);
  if ((val & static_cast<std::uint16_t>(1ULL << (15))) != 0U) new_ccr |= 0x08U;
  if (val == 0U) new_ccr |= 0x04U;
  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);
  bus.set_ccr(new_ccr);
  out_pc = 0x0141FCU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x0141FC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3AC1U; constexpr std::uint32_t kPc = 0x0141FCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t val = static_cast<std::uint16_t>(bus.d(1));
  const std::uint32_t addr_dst = bus.a(5) & kBusAddressMask;
  if (!bus.check_align(addr_dst, IrOpSize::Word)) return false;
  bus.write_m(addr_dst, IrOpSize::Word, val);
  bus.set_a(5, (addr_dst + 2U) & kBusAddressMask);
  if (bus.faulted()) return false;
  const std::uint16_t old_ccr = bus.ccr();
  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);
  if ((val & static_cast<std::uint16_t>(1ULL << (15))) != 0U) new_ccr |= 0x08U;
  if (val == 0U) new_ccr |= 0x04U;
  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);
  bus.set_ccr(new_ccr);
  out_pc = 0x0141FEU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x0141FE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x0141FEU;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x0302F4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x41F9U; constexpr std::uint32_t kPc = 0x0302F4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, 0xFF19E8U);
  out_pc = 0x0302FAU; return bus.end_insn(out_pc, kOp, 12, 0);
}
} // namespace oasis::recompiler::ir
