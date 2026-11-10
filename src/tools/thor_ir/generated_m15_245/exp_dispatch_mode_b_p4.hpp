#pragma once
#include "tools/thor_ir/generated_m15_245/exp_dispatch_router.hpp"
namespace oasis::recompiler::ir {
inline bool run_exp_mb_chunk_4(PortableIrBus& bus, std::uint32_t pc, std::uint32_t stop_pc, std::uint32_t& next_pc) noexcept {
  switch (pc) {
  case 0x007E78U: {
    if (!step_ir_0x007E78(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007E7E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007E84(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007E8A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007E90(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x007E96U: {
    if (!step_ir_0x007E96(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x007E9C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  default: return false; }
}
} // namespace oasis::recompiler::ir
