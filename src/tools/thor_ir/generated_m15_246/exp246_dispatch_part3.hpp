#pragma once
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x00BDA6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4C99U; constexpr std::uint32_t kPc = 0x00BDA6U;
  std::uint32_t sp = bus.a(1) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t mv_0 = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(bus.read_m(sp, IrOpSize::Word) & 0xFFFFU))); if (bus.faulted()) return false; sp = (sp + 2U) & kBusAddressMask;
  const std::uint32_t mv_10 = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(bus.read_m(sp, IrOpSize::Word) & 0xFFFFU))); if (bus.faulted()) return false; sp = (sp + 2U) & kBusAddressMask;
  bus.set_d(0, mv_0);
  bus.set_a(2, mv_10);
  bus.set_a(1, sp);
  out_pc = 0x00BDAAU; return bus.end_insn_mc(out_pc, kOp, 20, 56, false);
}
template <typename Bus> inline bool step_ir_0x00BDAA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4C99U; constexpr std::uint32_t kPc = 0x00BDAAU;
  std::uint32_t sp = bus.a(1) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t mv_1 = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(bus.read_m(sp, IrOpSize::Word) & 0xFFFFU))); if (bus.faulted()) return false; sp = (sp + 2U) & kBusAddressMask;
  const std::uint32_t mv_2 = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(bus.read_m(sp, IrOpSize::Word) & 0xFFFFU))); if (bus.faulted()) return false; sp = (sp + 2U) & kBusAddressMask;
  const std::uint32_t mv_3 = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(bus.read_m(sp, IrOpSize::Word) & 0xFFFFU))); if (bus.faulted()) return false; sp = (sp + 2U) & kBusAddressMask;
  const std::uint32_t mv_4 = static_cast<std::uint32_t>(static_cast<std::int32_t>(static_cast<std::int16_t>(bus.read_m(sp, IrOpSize::Word) & 0xFFFFU))); if (bus.faulted()) return false; sp = (sp + 2U) & kBusAddressMask;
  bus.set_d(1, mv_1);
  bus.set_d(2, mv_2);
  bus.set_d(3, mv_3);
  bus.set_d(4, mv_4);
  bus.set_a(1, sp);
  out_pc = 0x00BDAEU; return bus.end_insn_mc(out_pc, kOp, 28, 112, false);
}
template <typename Bus> inline bool step_ir_0x00BDAE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3E0AU; constexpr std::uint32_t kPc = 0x00BDAEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.a(2) & 0xFFFFU);
  write_dn(bus, 7, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00BDB0U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BDB0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4A05U; constexpr std::uint32_t kPc = 0x00BDB0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t v = static_cast<std::uint8_t>(bus.d(5));
  bus.set_ccr(ccr_logic<std::uint8_t>(v, bus.ccr())); out_pc = 0x00BDB2U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BDB2(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00BDB2U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00BDBCU : 0x00BDB6U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00BDB6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4441U; constexpr std::uint32_t kPc = 0x00BDB6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(1)); const std::uint16_t v = static_cast<std::uint16_t>(0U - s);
  write_dn(bus, 1, v, 2); bus.set_ccr(ccr_sub<std::uint16_t>(0U, s, bus.ccr(), true));
  out_pc = 0x00BDB8U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BDB8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4443U; constexpr std::uint32_t kPc = 0x00BDB8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(3)); const std::uint16_t v = static_cast<std::uint16_t>(0U - s);
  write_dn(bus, 3, v, 2); bus.set_ccr(ccr_sub<std::uint16_t>(0U, s, bus.ccr(), true));
  out_pc = 0x00BDBAU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BDBA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xC343U; constexpr std::uint32_t kPc = 0x00BDBAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t vx = bus.d(1); bus.set_d(1, bus.d(3)); bus.set_d(3, vx);
  out_pc = 0x00BDBCU; return bus.end_insn(out_pc, kOp, 6, 0);
}
template <typename Bus> inline bool step_ir_0x00BDBC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3A2EU; constexpr std::uint32_t kPc = 0x00BDBCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(16))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 5, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00BDC0U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BDC0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xDA6EU; constexpr std::uint32_t kPc = 0x00BDC0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_s = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(20))) & kBusAddressMask;
  if (!bus.check_align(addr_s, IrOpSize::Word)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.read_m(addr_s, IrOpSize::Word));
  if (bus.faulted()) return false;
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 5, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BDC4U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BDC4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD045U; constexpr std::uint32_t kPc = 0x00BDC4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 0, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BDC6U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BDC6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xDE45U; constexpr std::uint32_t kPc = 0x00BDC6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(7));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 7, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BDC8U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BDC8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3A2EU; constexpr std::uint32_t kPc = 0x00BDC8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(8))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 5, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00BDCCU; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BDCC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD245U; constexpr std::uint32_t kPc = 0x00BDCCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 1, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BDCEU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BDCE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD645U; constexpr std::uint32_t kPc = 0x00BDCEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(3));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 3, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BDD0U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BDD0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3A2EU; constexpr std::uint32_t kPc = 0x00BDD0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(12))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 5, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00BDD4U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BDD4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xDA6EU; constexpr std::uint32_t kPc = 0x00BDD4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_s = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(20))) & kBusAddressMask;
  if (!bus.check_align(addr_s, IrOpSize::Word)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.read_m(addr_s, IrOpSize::Word));
  if (bus.faulted()) return false;
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 5, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BDD8U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BDD8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD445U; constexpr std::uint32_t kPc = 0x00BDD8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(2));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 2, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BDDAU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BDDA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xD845U; constexpr std::uint32_t kPc = 0x00BDDAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(bus.d(5));
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(4));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 4, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x00BDDCU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BDDC(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4847U; constexpr std::uint32_t kPc = 0x00BDDCU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t o = bus.d(7); const std::uint32_t v = (o >> 16U) | (o << 16U);
  bus.set_d(7, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x00BDDEU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BDDE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4847U; constexpr std::uint32_t kPc = 0x00BDDEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t o = bus.d(7); const std::uint32_t v = (o >> 16U) | (o << 16U);
  bus.set_d(7, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x00BDE0U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x00BDE0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4A68U; constexpr std::uint32_t kPc = 0x00BDE0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(0))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00BDE4U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x00BDE4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6F00U; constexpr std::uint32_t kPc = 0x00BDE4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 15);
  out_pc = taken ? 0x00BE5EU : 0x00BDE8U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00BDE8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0xBDC8U; constexpr std::uint32_t kPc = 0x00BDE8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t s = static_cast<std::uint32_t>(bus.a(0));
  bus.set_ccr(ccr_sub<std::uint32_t>(bus.a(6), s, bus.ccr(), false));
  out_pc = 0x00BDEAU; return bus.end_insn(out_pc, kOp, 6, 0);
}
template <typename Bus> inline bool step_ir_0x00BDEA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6700U; constexpr std::uint32_t kPc = 0x00BDEAU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x00BE5EU : 0x00BDEEU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00BDEE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0828U; constexpr std::uint32_t kPc = 0x00BDEEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint8_t bit_s = static_cast<std::uint8_t>(0x3U);
  const std::uint32_t addr_cur = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(56))) & kBusAddressMask;
  if (!bus.check_align(addr_cur, IrOpSize::Byte)) return false;
  const std::uint8_t cur = static_cast<std::uint8_t>(bus.read_m(addr_cur, IrOpSize::Byte));
  if (bus.faulted()) return false;
  const std::uint8_t mask = static_cast<std::uint8_t>(1U << (bit_s & 7U)); const bool z = (cur & mask) == 0U;
  bus.set_ccr((bus.ccr() & ~kFlagZ) | (z ? kFlagZ : 0U)); out_pc = 0x00BDF4U; return bus.end_insn(out_pc, kOp, 16, 0);
}
template <typename Bus> inline bool step_ir_0x00BDF4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6600U; constexpr std::uint32_t kPc = 0x00BDF4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x00BE5EU : 0x00BDF8U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : 1);
}
template <typename Bus> inline bool step_ir_0x00BDF8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3A28U; constexpr std::uint32_t kPc = 0x00BDF8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t addr_v = (bus.a(0) + static_cast<std::uint32_t>(static_cast<std::int32_t>(16))) & kBusAddressMask;
  if (!bus.check_align(addr_v, IrOpSize::Word)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.read_m(addr_v, IrOpSize::Word));
  if (bus.faulted()) return false;
  write_dn(bus, 5, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x00BDFCU; return bus.end_insn(out_pc, kOp, 12, 0);
}
} // namespace oasis::recompiler::ir
