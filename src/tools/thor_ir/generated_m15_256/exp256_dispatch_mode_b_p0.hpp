#pragma once
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_common.hpp"
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_router.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool run_exp256_block_BB_00_0x006B12(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x006B12(bus, next_pc)) return false;
  if (!step_ir_0x006B14(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_01_0x006B18(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x006B18(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_02_0x006B1C(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x006B1C(bus, next_pc)) return false;
  if (!step_ir_0x006B1E(bus, next_pc)) return false;
  if (!step_ir_0x006B20(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_03_0x006B24(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x006B24(bus, next_pc)) return false;
  if (!step_ir_0x006B26(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_04_0x006B28(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x006B28(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_05_0x010F48(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x010F48(bus, next_pc)) return false;
  if (!step_ir_0x010F4C(bus, next_pc)) return false;
  if (!step_ir_0x010F50(bus, next_pc)) return false;
  if (!step_ir_0x010F54(bus, next_pc)) return false;
  if (!step_ir_0x010F58(bus, next_pc)) return false;
  if (!step_ir_0x010F5A(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_06_0x010F5C(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x010F5C(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_07_0x010F5E(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x010F5E(bus, next_pc)) return false;
  if (!step_ir_0x010F60(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_08_0x010F62(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x010F62(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_09_0x010F64(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x010F64(bus, next_pc)) return false;
  if (!step_ir_0x010F66(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_10_0x010F68(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x010F68(bus, next_pc)) return false;
  if (!step_ir_0x010F6A(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_11_0x010F6C(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x010F6C(bus, next_pc)) return false;
  if (!step_ir_0x010F6E(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_12_0x010F70(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x010F70(bus, next_pc)) return false;
  if (!step_ir_0x010F72(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_13_0x010F74(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x010F74(bus, next_pc)) return false;
  if (!step_ir_0x010F76(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_14_0x010F78(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x010F78(bus, next_pc)) return false;
  if (!step_ir_0x010F7A(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_mode_b_chunk_0(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x006B12U: return run_exp256_block_BB_00_0x006B12(bus, out_pc);
    case 0x006B18U: return run_exp256_block_BB_01_0x006B18(bus, out_pc);
    case 0x006B1CU: return run_exp256_block_BB_02_0x006B1C(bus, out_pc);
    case 0x006B24U: return run_exp256_block_BB_03_0x006B24(bus, out_pc);
    case 0x006B28U: return run_exp256_block_BB_04_0x006B28(bus, out_pc);
    case 0x010F48U: return run_exp256_block_BB_05_0x010F48(bus, out_pc);
    case 0x010F5CU: return run_exp256_block_BB_06_0x010F5C(bus, out_pc);
    case 0x010F5EU: return run_exp256_block_BB_07_0x010F5E(bus, out_pc);
    case 0x010F62U: return run_exp256_block_BB_08_0x010F62(bus, out_pc);
    case 0x010F64U: return run_exp256_block_BB_09_0x010F64(bus, out_pc);
    case 0x010F68U: return run_exp256_block_BB_10_0x010F68(bus, out_pc);
    case 0x010F6CU: return run_exp256_block_BB_11_0x010F6C(bus, out_pc);
    case 0x010F70U: return run_exp256_block_BB_12_0x010F70(bus, out_pc);
    case 0x010F74U: return run_exp256_block_BB_13_0x010F74(bus, out_pc);
    case 0x010F78U: return run_exp256_block_BB_14_0x010F78(bus, out_pc);
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
