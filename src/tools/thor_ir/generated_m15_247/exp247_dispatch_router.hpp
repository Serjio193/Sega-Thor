#pragma once
#include "tools/thor_ir/generated_m15_247/exp247_dispatch_router_p0.hpp"
namespace oasis::recompiler::ir {
inline constexpr bool is_compiled_exp247_dispatch_pc(std::uint32_t pc) noexcept {
  return is_compiled_exp247_p0(pc);
}
template <typename Bus>
inline bool step_one_exp247_dispatch_ir(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  if (is_compiled_exp247_p0(pc)) return step_exp247_router_p0(bus, pc, out_pc);
  return false;
}
} // namespace oasis::recompiler::ir
