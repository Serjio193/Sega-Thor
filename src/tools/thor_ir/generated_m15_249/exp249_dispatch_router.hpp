#pragma once
#include "tools/thor_ir/generated_m15_249/exp249_dispatch_router_p0.hpp"
#include "tools/thor_ir/generated_m15_249/exp249_dispatch_router_p1.hpp"
namespace oasis::recompiler::ir {
inline constexpr bool is_compiled_exp249_dispatch_pc(std::uint32_t pc) noexcept {
  return is_compiled_exp249_p0(pc) || is_compiled_exp249_p1(pc);
}
template <typename Bus>
inline bool step_one_exp249_dispatch_ir(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  if (is_compiled_exp249_p0(pc)) return step_exp249_router_p0(bus, pc, out_pc);
  if (is_compiled_exp249_p1(pc)) return step_exp249_router_p1(bus, pc, out_pc);
  return false;
}
} // namespace oasis::recompiler::ir
