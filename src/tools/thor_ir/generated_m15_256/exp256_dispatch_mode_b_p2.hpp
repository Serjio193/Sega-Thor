#pragma once
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_common.hpp"
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_router.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool run_exp256_block_BB_30_0x030304(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x030304(bus, next_pc)) return false;
  if (!step_ir_0x03030A(bus, next_pc)) return false;
  if (!step_ir_0x030310(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_31_0x030314(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x030314(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_32_0x03031A(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x03031A(bus, next_pc)) return false;
  if (!step_ir_0x03031E(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_33_0x030322(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x030322(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_mode_b_chunk_2(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x030304U: return run_exp256_block_BB_30_0x030304(bus, out_pc);
    case 0x030314U: return run_exp256_block_BB_31_0x030314(bus, out_pc);
    case 0x03031AU: return run_exp256_block_BB_32_0x03031A(bus, out_pc);
    case 0x030322U: return run_exp256_block_BB_33_0x030322(bus, out_pc);
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
