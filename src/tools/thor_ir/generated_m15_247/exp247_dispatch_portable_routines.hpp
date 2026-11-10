#pragma once
#include "tools/thor_ir/generated_m15_247/exp247_dispatch_mode_b.hpp"
namespace oasis::recompiler::ir {
inline PortableExecutionResult run_exp247_dispatch_mode_a(PortableGuestEnvironment& env, std::uint32_t entry_pc, std::uint32_t stop_pc) {
  PortableIrBus247 bus{env}; env.pc = entry_pc & kBusAddressMask; std::uint32_t next_pc = env.pc; int guard = 0;
  while (!bus.faulted() && !env.yielded && env.pc != stop_pc && ++guard < 8192) {
    if (!is_compiled_exp247_dispatch_pc(env.pc)) break;
    if (!step_one_exp247_dispatch_ir(bus, env.pc, next_pc)) break;
  }
  return finalize_exp247_res(bus, stop_pc);
}
} // namespace oasis::recompiler::ir
