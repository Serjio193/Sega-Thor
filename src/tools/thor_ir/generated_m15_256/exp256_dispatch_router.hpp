#pragma once
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_router_p0.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_one_exp256_dispatch_ir(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  if (step_one_exp256_dispatch_ir_p0(bus, pc, out_pc)) return true;
  return false;
}
inline bool is_compiled_exp256_dispatch_pc(std::uint32_t pc) noexcept {
  if (is_compiled_exp256_dispatch_pc_p0(pc)) return true;
  return false;
}
} // namespace oasis::recompiler::ir
