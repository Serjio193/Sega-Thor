#pragma once
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_router_p0.hpp"
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_router_p1.hpp"
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_router_p2.hpp"
namespace oasis::recompiler::ir {
inline constexpr bool is_compiled_exp248_dispatch_pc(std::uint32_t pc) noexcept {
  return is_compiled_exp248_p0(pc) || is_compiled_exp248_p1(pc) || is_compiled_exp248_p2(pc);
}
template <typename Bus>
inline bool step_one_exp248_dispatch_ir(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  if (is_compiled_exp248_p0(pc)) return step_exp248_router_p0(bus, pc, out_pc);
  if (is_compiled_exp248_p1(pc)) return step_exp248_router_p1(bus, pc, out_pc);
  if (is_compiled_exp248_p2(pc)) return step_exp248_router_p2(bus, pc, out_pc);
  return false;
}
} // namespace oasis::recompiler::ir
