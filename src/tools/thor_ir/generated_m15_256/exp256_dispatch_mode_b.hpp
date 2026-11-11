#pragma once
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_mode_b_p0.hpp"
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_mode_b_p1.hpp"
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_mode_b_p2.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool run_exp256_mode_b_all(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  if (run_exp256_mode_b_chunk_0(bus, pc, out_pc)) return true;
  if (run_exp256_mode_b_chunk_1(bus, pc, out_pc)) return true;
  if (run_exp256_mode_b_chunk_2(bus, pc, out_pc)) return true;
  return false;
}
} // namespace oasis::recompiler::ir
