#pragma once
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x02EDB0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7402U; constexpr std::uint32_t kPc = 0x02EDB0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000002U;
  bus.set_d(2, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EDB2U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EDB2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x02EDB2U;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x02EDB6U); if (bus.faulted()) return false;
  out_pc = 0x02EDCAU; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x02EDB6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x72F4U; constexpr std::uint32_t kPc = 0x02EDB6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0xFFFFFFF4U;
  bus.set_d(1, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EDB8U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EDB8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x74ECU; constexpr std::uint32_t kPc = 0x02EDB8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0xFFFFFFECU;
  bus.set_d(2, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EDBAU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EDBA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x02EDBAU;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x02EDBEU); if (bus.faulted()) return false;
  out_pc = 0x02EDCAU; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x02EDBE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x720CU; constexpr std::uint32_t kPc = 0x02EDBEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x0000000CU;
  bus.set_d(1, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EDC0U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EDC0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x74ECU; constexpr std::uint32_t kPc = 0x02EDC0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0xFFFFFFECU;
  bus.set_d(2, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EDC2U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EDC2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x02EDC2U;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x02EDC6U); if (bus.faulted()) return false;
  out_pc = 0x02EDCAU; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x02EDC6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2C4CU; constexpr std::uint32_t kPc = 0x02EDC6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.a(4));
  bus.set_a(6, v); out_pc = 0x02EDC8U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EDC8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x601CU; constexpr std::uint32_t kPc = 0x02EDC8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x02EDE6U; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x02EDE6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x49F9U; constexpr std::uint32_t kPc = 0x02EDE6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(4, 0xFF1CD8U);
  out_pc = 0x02EDECU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x02EDEC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4DF9U; constexpr std::uint32_t kPc = 0x02EDECU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(6, 0xFF2084U);
  out_pc = 0x02EDF2U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x02EDF2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C6EU; constexpr std::uint32_t kPc = 0x02EDF2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x30U);
  const std::uint32_t addr_d = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(0))) & kBusAddressMask;
  if (!bus.check_align(addr_d, IrOpSize::Word)) return false;
  const std::uint16_t d = static_cast<std::uint16_t>(bus.read_m(addr_d, IrOpSize::Word));
  if (bus.faulted()) return false;
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x02EDF8U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EDF8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6600U; constexpr std::uint32_t kPc = 0x02EDF8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x02EE9EU : 0x02EDFCU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x02EDFC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C6EU; constexpr std::uint32_t kPc = 0x02EDFCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x12U);
  const std::uint32_t addr_d = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(4))) & kBusAddressMask;
  if (!bus.check_align(addr_d, IrOpSize::Word)) return false;
  const std::uint16_t d = static_cast<std::uint16_t>(bus.read_m(addr_d, IrOpSize::Word));
  if (bus.faulted()) return false;
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x02EE02U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE02(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x02EE02U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x02EE9EU : 0x02EE06U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x02EE06(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x202EU; constexpr std::uint32_t kPc = 0x02EE06U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(16))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x02EE0AU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE0A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x222EU; constexpr std::uint32_t kPc = 0x02EE0AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(8))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  write_dn(bus, 1, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x02EE0EU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE0E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x242EU; constexpr std::uint32_t kPc = 0x02EE0EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(12))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  write_dn(bus, 2, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x02EE12U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE12(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2A2CU; constexpr std::uint32_t kPc = 0x02EE12U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(4) + static_cast<std::uint32_t>(static_cast<std::int32_t>(16))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  write_dn(bus, 5, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x02EE16U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE16(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x262CU; constexpr std::uint32_t kPc = 0x02EE16U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(4) + static_cast<std::uint32_t>(static_cast<std::int32_t>(8))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  write_dn(bus, 3, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x02EE1AU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE1A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x282CU; constexpr std::uint32_t kPc = 0x02EE1AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(4) + static_cast<std::uint32_t>(static_cast<std::int32_t>(12))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  write_dn(bus, 4, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x02EE1EU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE1E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0483U; constexpr std::uint32_t kPc = 0x02EE1EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(0x100000U);
  const std::uint32_t d = static_cast<std::uint32_t>(bus.d(3));
  const std::uint32_t r = static_cast<std::uint32_t>(d - s);
  write_dn(bus, 3, r, 4);
  bus.set_ccr(ccr_sub<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x02EE24U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE24(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0684U; constexpr std::uint32_t kPc = 0x02EE24U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(0xC0000U);
  const std::uint32_t d = static_cast<std::uint32_t>(bus.d(4));
  const std::uint32_t r = static_cast<std::uint32_t>(d + s);
  write_dn(bus, 4, r, 4);
  bus.set_ccr(ccr_add<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x02EE2AU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE2A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4A6EU; constexpr std::uint32_t kPc = 0x02EE2AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(138))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x02EE2EU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x02EE2E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6600U; constexpr std::uint32_t kPc = 0x02EE2EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x02EE52U : 0x02EE32U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x02EE32(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x49F9U; constexpr std::uint32_t kPc = 0x02EE32U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(4, 0xFF2374U);
  out_pc = 0x02EE38U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x02EE38(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x262CU; constexpr std::uint32_t kPc = 0x02EE38U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(4) + static_cast<std::uint32_t>(static_cast<std::int32_t>(8))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  write_dn(bus, 3, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x02EE3CU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE3C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x282CU; constexpr std::uint32_t kPc = 0x02EE3CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(4) + static_cast<std::uint32_t>(static_cast<std::int32_t>(12))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  write_dn(bus, 4, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x02EE40U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE40(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2A2CU; constexpr std::uint32_t kPc = 0x02EE40U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(4) + static_cast<std::uint32_t>(static_cast<std::int32_t>(16))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  write_dn(bus, 5, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x02EE44U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x02EE44(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0485U; constexpr std::uint32_t kPc = 0x02EE44U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(0x8000U);
  const std::uint32_t d = static_cast<std::uint32_t>(bus.d(5));
  const std::uint32_t r = static_cast<std::uint32_t>(d - s);
  write_dn(bus, 5, r, 4);
  bus.set_ccr(ccr_sub<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x02EE4AU; return bus.end_insn(out_pc, kOp, 16, 0);
}
} // namespace oasis::recompiler::ir
