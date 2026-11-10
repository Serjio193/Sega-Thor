#pragma once
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_router_p0.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_router_p1.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_router_p2.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_one_exp250_dispatch_ir(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  if (step_one_exp250_dispatch_ir_p0(bus, pc, out_pc)) return true;
  if (step_one_exp250_dispatch_ir_p1(bus, pc, out_pc)) return true;
  if (step_one_exp250_dispatch_ir_p2(bus, pc, out_pc)) return true;
  return false;
}
inline bool is_compiled_exp250_dispatch_pc(std::uint32_t pc) noexcept {
  if (is_compiled_exp250_dispatch_pc_p0(pc)) return true;
  if (is_compiled_exp250_dispatch_pc_p1(pc)) return true;
  if (is_compiled_exp250_dispatch_pc_p2(pc)) return true;
  return false;
}
} // namespace oasis::recompiler::ir
