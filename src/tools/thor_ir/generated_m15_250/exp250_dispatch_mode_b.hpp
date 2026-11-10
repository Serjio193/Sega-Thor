#pragma once
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_mode_b_p0.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_mode_b_p1.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_mode_b_p2.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_mode_b_p3.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_mode_b_p4.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool run_exp250_mode_b_all(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  if (run_exp250_mode_b_chunk_0(bus, pc, out_pc)) return true;
  if (run_exp250_mode_b_chunk_1(bus, pc, out_pc)) return true;
  if (run_exp250_mode_b_chunk_2(bus, pc, out_pc)) return true;
  if (run_exp250_mode_b_chunk_3(bus, pc, out_pc)) return true;
  if (run_exp250_mode_b_chunk_4(bus, pc, out_pc)) return true;
  return false;
}
} // namespace oasis::recompiler::ir
