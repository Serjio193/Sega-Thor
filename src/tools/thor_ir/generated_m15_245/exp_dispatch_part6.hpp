#pragma once
#include "tools/thor_ir/generated_m15_245/exp_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x006774(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4EB9U; constexpr std::uint32_t kPc = 0x006774U;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x00677AU); if (bus.faulted()) return false;
  out_pc = 0x060004U; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x00677A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C6EU; constexpr std::uint32_t kPc = 0x00677AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x1CU);
  const std::uint32_t addr_d = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(48))) & kBusAddressMask;
  if (!bus.check_align(addr_d, IrOpSize::Word)) return false;
  const std::uint16_t d = static_cast<std::uint16_t>(bus.read_m(addr_d, IrOpSize::Word));
  if (bus.faulted()) return false;
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x006780U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x006780(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6418U; constexpr std::uint32_t kPc = 0x006780U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 4);
  out_pc = taken ? 0x00679AU : 0x006782U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x006782(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x102EU; constexpr std::uint32_t kPc = 0x006782U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(64))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Byte)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.read_m(addr_v, IrOpSize::Byte));
  if (bus.faulted()) return false;
  write_dn(bus, 0, v, 1);
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x006786U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x006786(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x802EU; constexpr std::uint32_t kPc = 0x006786U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_s = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(65))) & kBusAddressMask;
  if (!bus.check_align(addr_s, IrOpSize::Byte)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(bus.read_m(addr_s, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  const std::uint8_t r = static_cast<std::uint8_t>(d | s);
  write_dn(bus, 0, r, 1);
  bus.set_ccr(ccr_logic<std::uint8_t>(r, bus.ccr())); out_pc = 0x00678AU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00678A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0800U; constexpr std::uint32_t kPc = 0x00678AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x6U);
  const std::uint32_t cur = bus.d(0);
  const std::uint32_t mask = 1U << (bit_s & 31U); const bool z = (cur & mask) == 0U;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x00678EU; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x00678E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x670AU; constexpr std::uint32_t kPc = 0x00678EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00679AU : 0x006790U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x006790(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7CU; constexpr std::uint32_t kPc = 0x006790U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x18U);
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(48))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x006796U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x006796(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x426EU; constexpr std::uint32_t kPc = 0x006796U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(46))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, 0U); if (bus.faulted()) return false;
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x00679AU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00679A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x203CU; constexpr std::uint32_t kPc = 0x00679AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(0x8000U);
  write_dn(bus, 0, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x0067A0U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x0067A0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x222EU; constexpr std::uint32_t kPc = 0x0067A0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(78))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  write_dn(bus, 1, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x0067A4U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x0067A4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6B08U; constexpr std::uint32_t kPc = 0x0067A4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 11);
  out_pc = taken ? 0x0067AEU : 0x0067A6U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x0067A6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x9280U; constexpr std::uint32_t kPc = 0x0067A6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(bus.d(0));
  const std::uint32_t d = static_cast<std::uint32_t>(bus.d(1));
  const std::uint32_t r = static_cast<std::uint32_t>(d - s);
  write_dn(bus, 1, r, 4);
  bus.set_ccr(ccr_sub<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x0067A8U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x0067A8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6A0AU; constexpr std::uint32_t kPc = 0x0067A8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 10);
  out_pc = taken ? 0x0067B4U : 0x0067AAU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x0067AA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4281U; constexpr std::uint32_t kPc = 0x0067AAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  write_dn(bus, 1, 0U, 4);
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x0067ACU; return bus.end_insn(out_pc, kOp, 6, 0);
}
template <typename Bus> inline bool step_ir_0x0067AC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6006U; constexpr std::uint32_t kPc = 0x0067ACU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x0067B4U; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x0067AE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD280U; constexpr std::uint32_t kPc = 0x0067AEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(bus.d(0));
  const std::uint32_t d = static_cast<std::uint32_t>(bus.d(1));
  const std::uint32_t r = static_cast<std::uint32_t>(d + s);
  write_dn(bus, 1, r, 4);
  bus.set_ccr(ccr_add<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x0067B0U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x0067B0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6B02U; constexpr std::uint32_t kPc = 0x0067B0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 11);
  out_pc = taken ? 0x0067B4U : 0x0067B2U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x0067B2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4281U; constexpr std::uint32_t kPc = 0x0067B2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  write_dn(bus, 1, 0U, 4);
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x0067B4U; return bus.end_insn(out_pc, kOp, 6, 0);
}
template <typename Bus> inline bool step_ir_0x0067B4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x242EU; constexpr std::uint32_t kPc = 0x0067B4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(82))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  write_dn(bus, 2, v, 4);
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x0067B8U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x0067B8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6B08U; constexpr std::uint32_t kPc = 0x0067B8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 11);
  out_pc = taken ? 0x0067C2U : 0x0067BAU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x0067BA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x9480U; constexpr std::uint32_t kPc = 0x0067BAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(bus.d(0));
  const std::uint32_t d = static_cast<std::uint32_t>(bus.d(2));
  const std::uint32_t r = static_cast<std::uint32_t>(d - s);
  write_dn(bus, 2, r, 4);
  bus.set_ccr(ccr_sub<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x0067BCU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x0067BC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6A0AU; constexpr std::uint32_t kPc = 0x0067BCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 10);
  out_pc = taken ? 0x0067C8U : 0x0067BEU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x0067BE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4282U; constexpr std::uint32_t kPc = 0x0067BEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  write_dn(bus, 2, 0U, 4);
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x0067C0U; return bus.end_insn(out_pc, kOp, 6, 0);
}
template <typename Bus> inline bool step_ir_0x0067C0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6006U; constexpr std::uint32_t kPc = 0x0067C0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x0067C8U; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x0067C2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD480U; constexpr std::uint32_t kPc = 0x0067C2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(bus.d(0));
  const std::uint32_t d = static_cast<std::uint32_t>(bus.d(2));
  const std::uint32_t r = static_cast<std::uint32_t>(d + s);
  write_dn(bus, 2, r, 4);
  bus.set_ccr(ccr_add<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x0067C4U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x0067C4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6B02U; constexpr std::uint32_t kPc = 0x0067C4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 11);
  out_pc = taken ? 0x0067C8U : 0x0067C6U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x0067C6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4282U; constexpr std::uint32_t kPc = 0x0067C6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  write_dn(bus, 2, 0U, 4);
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x0067C8U; return bus.end_insn(out_pc, kOp, 6, 0);
}
template <typename Bus> inline bool step_ir_0x0067C8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2D41U; constexpr std::uint32_t kPc = 0x0067C8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.d(1));
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(78))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Long)) return false; bus.write_m(da, IrOpSize::Long, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x0067CCU; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x0067CC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2D42U; constexpr std::uint32_t kPc = 0x0067CCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.d(2));
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(82))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Long)) return false; bus.write_m(da, IrOpSize::Long, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x0067D0U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x0067D0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x0067D0U;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x0067D4U); if (bus.faulted()) return false;
  out_pc = 0x0064C4U; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x0067D4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD3AEU; constexpr std::uint32_t kPc = 0x0067D4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(bus.d(1));
  const std::uint32_t addr_d = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(114))) & kBusAddressMask;
  if (!bus.check_align(addr_d, IrOpSize::Long)) return false;
  const std::uint32_t d = static_cast<std::uint32_t>(bus.read_m(addr_d, IrOpSize::Long));
  if (bus.faulted()) return false;
  const std::uint32_t r = static_cast<std::uint32_t>(d + s);
  bus.write_m(addr_d, IrOpSize::Long, r); if (bus.faulted()) return false;
  bus.set_ccr(ccr_add<std::uint32_t>(d, s, bus.ccr(), true)); out_pc = 0x0067D8U; return bus.end_insn(out_pc, kOp, 24, 0);
}
} // namespace oasis::recompiler::ir
