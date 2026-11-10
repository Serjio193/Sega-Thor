#pragma once
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x02ED54(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x674AU; constexpr std::uint32_t kPc = 0x02ED54U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x02EDA0U : 0x02ED56U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x02ED56(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5041U; constexpr std::uint32_t kPc = 0x02ED56U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x8U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 1, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x02ED58U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02ED58(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C40U; constexpr std::uint32_t kPc = 0x02ED58U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x3AU);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x02ED5CU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x02ED5C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6742U; constexpr std::uint32_t kPc = 0x02ED5CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x02EDA0U : 0x02ED5EU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x02ED5E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5041U; constexpr std::uint32_t kPc = 0x02ED5EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x8U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 1, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x02ED60U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02ED60(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C40U; constexpr std::uint32_t kPc = 0x02ED60U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x37U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x02ED64U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x02ED64(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x673AU; constexpr std::uint32_t kPc = 0x02ED64U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x02EDA0U : 0x02ED66U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x02ED66(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5041U; constexpr std::uint32_t kPc = 0x02ED66U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x8U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 1, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x02ED68U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02ED68(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C40U; constexpr std::uint32_t kPc = 0x02ED68U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x34U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x02ED6CU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x02ED6C(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6732U; constexpr std::uint32_t kPc = 0x02ED6CU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x02EDA0U : 0x02ED6EU;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x02ED6E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5041U; constexpr std::uint32_t kPc = 0x02ED6EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x8U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 1, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x02ED70U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02ED70(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C40U; constexpr std::uint32_t kPc = 0x02ED70U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x33U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x02ED74U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x02ED74(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x672AU; constexpr std::uint32_t kPc = 0x02ED74U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x02EDA0U : 0x02ED76U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x02ED76(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x323CU; constexpr std::uint32_t kPc = 0x02ED76U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(0x8U);
  write_dn(bus, 1, v, 2);
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x02ED7AU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x02ED7A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C40U; constexpr std::uint32_t kPc = 0x02ED7AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x6CU);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x02ED7EU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x02ED7E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6720U; constexpr std::uint32_t kPc = 0x02ED7EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x02EDA0U : 0x02ED80U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x02ED80(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5041U; constexpr std::uint32_t kPc = 0x02ED80U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x8U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 1, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x02ED82U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02ED82(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C40U; constexpr std::uint32_t kPc = 0x02ED82U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x6FU);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x02ED86U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x02ED86(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6718U; constexpr std::uint32_t kPc = 0x02ED86U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x02EDA0U : 0x02ED88U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x02ED88(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5041U; constexpr std::uint32_t kPc = 0x02ED88U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x8U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 1, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x02ED8AU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02ED8A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C40U; constexpr std::uint32_t kPc = 0x02ED8AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x72U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x02ED8EU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x02ED8E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6710U; constexpr std::uint32_t kPc = 0x02ED8EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x02EDA0U : 0x02ED90U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x02ED90(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5041U; constexpr std::uint32_t kPc = 0x02ED90U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x8U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 1, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x02ED92U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02ED92(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C40U; constexpr std::uint32_t kPc = 0x02ED92U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x75U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x02ED96U; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x02ED96(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6708U; constexpr std::uint32_t kPc = 0x02ED96U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 7);
  out_pc = taken ? 0x02EDA0U : 0x02ED98U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x02ED98(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x5041U; constexpr std::uint32_t kPc = 0x02ED98U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x8U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(1));
  const std::uint16_t r = static_cast<std::uint16_t>(d + s);
  write_dn(bus, 1, r, 2);
  bus.set_ccr(ccr_add<std::uint16_t>(d, s, bus.ccr(), true)); out_pc = 0x02ED9AU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02ED9A(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x0C40U; constexpr std::uint32_t kPc = 0x02ED9AU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t s = static_cast<std::uint16_t>(0x78U);
  const std::uint16_t d = static_cast<std::uint16_t>(bus.d(0));
  bus.set_ccr(ccr_sub<std::uint16_t>(d, s, bus.ccr(), false)); out_pc = 0x02ED9EU; return bus.end_insn(out_pc, kOp, 8, 0);
}
template <typename Bus> inline bool step_ir_0x02ED9E(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6604U; constexpr std::uint32_t kPc = 0x02ED9EU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const bool taken = eval_cond(bus.ccr(), 6);
  out_pc = taken ? 0x02EDA4U : 0x02EDA0U;
  return bus.end_insn(out_pc, kOp, 10, taken ? 0 : -1);
}
template <typename Bus> inline bool step_ir_0x02EDA0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D41U; constexpr std::uint32_t kPc = 0x02EDA0U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint16_t v = static_cast<std::uint16_t>(bus.d(1));
  const std::uint32_t da = (bus.a(6) + static_cast<std::uint32_t>(static_cast<std::int32_t>(16))) & kBusAddressMask;
  if (!bus.check_align(da, IrOpSize::Word)) return false; bus.write_m(da, IrOpSize::Word, v); if (bus.faulted()) return false;
  bus.set_ccr(ccr_logic<std::uint16_t>(v, bus.ccr())); out_pc = 0x02EDA4U; return bus.end_insn(out_pc, kOp, 12, 0);
}
template <typename Bus> inline bool step_ir_0x02EDA4(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x284EU; constexpr std::uint32_t kPc = 0x02EDA4U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = static_cast<std::uint32_t>(bus.a(6));
  bus.set_a(4, v); out_pc = 0x02EDA6U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EDA6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x72E4U; constexpr std::uint32_t kPc = 0x02EDA6U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0xFFFFFFE4U;
  bus.set_d(1, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EDA8U; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EDA8(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x7402U; constexpr std::uint32_t kPc = 0x02EDA8U;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x00000002U;
  bus.set_d(2, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EDAAU; return bus.end_insn(out_pc, kOp, 4, 0);
}
template <typename Bus> inline bool step_ir_0x02EDAA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x6100U; constexpr std::uint32_t kPc = 0x02EDAAU;
  const std::uint32_t new_sp = (bus.a(7) - 4U) & kBusAddressMask;
  if ((new_sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  bus.set_a(7, new_sp); bus.write_m(new_sp, IrOpSize::Long, 0x02EDAEU); if (bus.faulted()) return false;
  out_pc = 0x02EDCAU; return bus.end_insn(out_pc, kOp, 18, 0);
}
template <typename Bus> inline bool step_ir_0x02EDAE(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x721CU; constexpr std::uint32_t kPc = 0x02EDAEU;
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t v = 0x0000001CU;
  bus.set_d(1, v); bus.set_ccr(ccr_logic<std::uint32_t>(v, bus.ccr()));
  out_pc = 0x02EDB0U; return bus.end_insn(out_pc, kOp, 4, 0);
}
} // namespace oasis::recompiler::ir
