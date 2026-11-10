#pragma once
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_common.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_router.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool run_exp250_block_BB_60_0x02E8EE(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02E8EE(bus, next_pc)) return false;
  if (!step_ir_0x02E8F4(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_61_0x02E8F6(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02E8F6(bus, next_pc)) return false;
  if (!step_ir_0x02E8FC(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_62_0x02E8FE(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02E8FE(bus, next_pc)) return false;
  if (!step_ir_0x02E902(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_63_0x02E904(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02E904(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_64_0x02E908(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02E908(bus, next_pc)) return false;
  if (!step_ir_0x02E90C(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_65_0x02EC18(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02EC18(bus, next_pc)) return false;
  if (!step_ir_0x02EC20(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_66_0x02EC24(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02EC24(bus, next_pc)) return false;
  if (!step_ir_0x02EC2C(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_67_0x02EC30(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02EC30(bus, next_pc)) return false;
  if (!step_ir_0x02EC38(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_68_0x02EC3C(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02EC3C(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_69_0x02EC40(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02EC40(bus, next_pc)) return false;
  if (!step_ir_0x02EC48(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_70_0x02EC4C(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02EC4C(bus, next_pc)) return false;
  if (!step_ir_0x02EC54(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_mode_b_chunk_4(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x02E8EEU: return run_exp250_block_BB_60_0x02E8EE(bus, out_pc);
    case 0x02E8F6U: return run_exp250_block_BB_61_0x02E8F6(bus, out_pc);
    case 0x02E8FEU: return run_exp250_block_BB_62_0x02E8FE(bus, out_pc);
    case 0x02E904U: return run_exp250_block_BB_63_0x02E904(bus, out_pc);
    case 0x02E908U: return run_exp250_block_BB_64_0x02E908(bus, out_pc);
    case 0x02EC18U: return run_exp250_block_BB_65_0x02EC18(bus, out_pc);
    case 0x02EC24U: return run_exp250_block_BB_66_0x02EC24(bus, out_pc);
    case 0x02EC30U: return run_exp250_block_BB_67_0x02EC30(bus, out_pc);
    case 0x02EC3CU: return run_exp250_block_BB_68_0x02EC3C(bus, out_pc);
    case 0x02EC40U: return run_exp250_block_BB_69_0x02EC40(bus, out_pc);
    case 0x02EC4CU: return run_exp250_block_BB_70_0x02EC4C(bus, out_pc);
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
