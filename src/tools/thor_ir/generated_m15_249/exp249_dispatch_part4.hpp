#pragma once
#include "tools/thor_ir/generated_m15_249/exp249_dispatch_common.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_ir_0x0255CA(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7BU; constexpr std::uint32_t kPc = 0x0255CAU;
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
  out_pc = 0x0255D0U; return bus.end_insn(out_pc, kOp, 22, 0);
}
template <typename Bus> inline bool step_ir_0x0255D0(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x3D7BU; constexpr std::uint32_t kPc = 0x0255D0U;
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
  out_pc = 0x0255D6U; return bus.end_insn(out_pc, kOp, 22, 0);
}
template <typename Bus> inline bool step_ir_0x0255D6(Bus& bus, std::uint32_t& out_pc) noexcept {
  constexpr std::uint16_t kOp = 0x4E75U; constexpr std::uint32_t kPc = 0x0255D6U;
  const std::uint32_t sp = bus.a(7) & kBusAddressMask;
  if ((sp & 1U) != 0U) { bus.fault_addr(); return false; }
  if (!bus.begin_insn(kPc, kOp)) return false;
  const std::uint32_t ret = bus.read_m(sp, IrOpSize::Long) & kBusAddressMask; if (bus.faulted()) return false;
  bus.set_a(7, (sp + 4U) & kBusAddressMask); out_pc = ret; return bus.end_insn(out_pc, kOp, 16, 0);
}
} // namespace oasis::recompiler::ir
