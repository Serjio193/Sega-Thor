#pragma once
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x00D9BC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x00D9BCU;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00F80E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x48E7U; constexpr std::uint32_t kPc = 0x00F80EU;
  std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(6)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(5)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(4)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(3)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(2)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(1)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.a(0)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(7)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(6)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(5)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(4)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(3)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(2)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(1)); if (bus.faulted()) return false;
  sp = (sp - 4U) & kBusAddressMask; bus.write_m(sp, IrOpSize::Long, bus.d(0)); if (bus.faulted()) return false;
  bus.set_a(7, sp);
  out_pc = 0x00F812U; return bus.end_insn_mc(out_pc, kOp, 128, 840, true);
}
template <typename Bus> inline bool step_ir_0x00F812(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2079U; constexpr std::uint32_t kPc = 0x00F812U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = 0xFF17B2U;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  if (bus.faulted()) return false;
  bus.set_a(0, v); out_pc = 0x00F818U; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x00F818(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4241U; constexpr std::uint32_t kPc = 0x00F818U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  write_dn(bus, 1, 0U, 2);
  bus.set_ccr((bus.ccr() & kFlagX) | kFlagZ); out_pc = 0x00F81AU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00F81A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x1210U; constexpr std::uint32_t kPc = 0x00F81AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = bus.a(0) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Byte)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.read_m(addr_v, IrOpSize::Byte));
  if (bus.faulted()) return false;
  write_dn(bus, 1, v, 1);
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x00F81CU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00F81C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C01U; constexpr std::uint32_t kPc = 0x00F81CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0xAU);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(1));
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), false)); out_pc = 0x00F820U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00F820(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00F820U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00F83CU : 0x00F824U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00F824(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C01U; constexpr std::uint32_t kPc = 0x00F824U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x12U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(1));
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), false)); out_pc = 0x00F828U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00F828(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00F828U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00F83CU : 0x00F82CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00F82C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C01U; constexpr std::uint32_t kPc = 0x00F82CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x13U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(1));
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), false)); out_pc = 0x00F830U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00F830(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00F830U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00F83CU : 0x00F834U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00F834(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C01U; constexpr std::uint32_t kPc = 0x00F834U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(0x14U);
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(1));
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), false)); out_pc = 0x00F838U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00F838(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6600U; constexpr std::uint32_t kPc = 0x00F838U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x00F84EU : 0x00F83CU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00F83C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD241U; constexpr std::uint32_t kPc = 0x00F83CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 1, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00F83EU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00F83E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x43FBU; constexpr std::uint32_t kPc = 0x00F83EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::int32_t idx = static_cast<std::int16_t>(bus.d(1) & 0xFFFFU);
  bus.set_a(1, (0x00F7D0U + static_cast<std::uint32_t>(idx)) & kBusAddressMask);
  out_pc = 0x00F842U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00F842(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD2D1U; constexpr std::uint32_t kPc = 0x00F842U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_s = bus.a(1) & kBusAddressMask;
  if (!bus.check_align(addr_s, IrOpSize::Word)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.read_m(addr_s, IrOpSize::Word));
  if (bus.faulted()) return false;
  bus.set_a(1, bus.a(1) + static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(s)))); out_pc = 0x00F844U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00F844(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E91U; constexpr std::uint32_t kPc = 0x00F844U;
  const std::uint32_t tgt = bus.a(1) & kBusAddressMask;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if (((new_sp | tgt) & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x00F846U); if (bus.faulted()) return false;
  out_pc = tgt; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00F846(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x23C8U; constexpr std::uint32_t kPc = 0x00F846U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.a(0));
  const std::uint32_t da = 0xFF17B2U;
  if (!bus.check_align(da, IrOpSize::Long)) return false; bus.write_m(da, IrOpSize::Long, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr())); out_pc = 0x00F84CU; return bus.end_insn(out_pc, kOp, 20, 0);
}
template <typename Bus> inline bool step_ir_0x00F84C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x60CAU; constexpr std::uint32_t kPc = 0x00F84CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  out_pc = 0x00F818U; return bus.end_insn(out_pc, kOp, 10, 0);
}
template <typename Bus> inline bool step_ir_0x00FCB8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5488U; constexpr std::uint32_t kPc = 0x00FCB8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(0, bus.a(0) + 0x2U);
  out_pc = 0x00FCBAU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00FCBA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x2C58U; constexpr std::uint32_t kPc = 0x00FCBAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = bus.a(0) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Long)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.read_m(addr_v, IrOpSize::Long));
  bus.set_a(0, (addr_v + 4U) & kBusAddressMask);
  if (bus.faulted()) return false;
  bus.set_a(6, v);
  out_pc = 0x00FCBCU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00FCBC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3C18U; constexpr std::uint32_t kPc = 0x00FCBCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_val = bus.a(0) & kBusAddressMask;
  if (!bus.check_align(addr_val, IrOpSize::Word)) return false;
  const std::uint16_t val = static_cast<std::uint16_t>(bus.read_m(addr_val, IrOpSize::Word));
  bus.set_a(0, (addr_val + 2U) & kBusAddressMask);
  if (bus.faulted()) return false;
  bus.set_d(6, (bus.d(6) & 0xFFFF0000U) | (val & 0xFFFFU));
  const std::uint16_t old_ccr = bus.ccr();
  std::uint16_t new_ccr = static_cast<std::uint16_t>(old_ccr & 0xFFE0U);
  if ((val & static_cast<std::uint16_t>(1ULL << (15))) != 0U) new_ccr |= 0x08U;
  if (val == 0U) new_ccr |= 0x04U;
  new_ccr |= static_cast<std::uint16_t>(old_ccr & 0x10U);
  bus.set_ccr(new_ccr);
  out_pc = 0x00FCBEU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x00FCBE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x00FCBEU;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x00FCC2U); if (bus.faulted()) return false;
  out_pc = 0x00D9A4U; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x00FCC2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x00FCC2U;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00FCCE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x00FCCEU;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x00FCD2U); if (bus.faulted()) return false;
  out_pc = 0x0101ECU; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x00FCD2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xB039U; constexpr std::uint32_t kPc = 0x00FCD2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_s = 0xFF17B7U;
  if (!bus.check_align(addr_s, IrOpSize::Byte)) return false;
  const std::uint8_t s = static_cast<std::uint8_t>(bus.read_m(addr_s, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t d = static_cast<std::uint8_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint8_t>(d, s, bus.ccr(), false)); out_pc = 0x00FCD8U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00FCD8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6400U; constexpr std::uint32_t kPc = 0x00FCD8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 4);
  out_pc = taken ? 0x00FD82U : 0x00FCDCU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00FCDC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x00FCDCU;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x00FCE0U); if (bus.faulted()) return false;
  out_pc = 0x00DA2AU; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x00FCE0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6500U; constexpr std::uint32_t kPc = 0x00FCE0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 5);
  out_pc = taken ? 0x00FD82U : 0x00FCE4U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
} // namespace oasis::recompiler::ir
