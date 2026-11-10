#pragma once
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_part0.hpp"
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_part1.hpp"
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_part2.hpp"
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_part3.hpp"
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_part4.hpp"
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_part5.hpp"
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_part6.hpp"
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_part7.hpp"
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_part8.hpp"
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_part9.hpp"
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_part10.hpp"
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_part11.hpp"
#include "tools/thor_ir/generated_m15_246/exp246_dispatch_part12.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_exp246_chunk_2(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc & kBusAddressMask) {
  case 0x02EE78U: return step_ir_0x02EE78(bus, out_pc);
  case 0x02EE7CU: return step_ir_0x02EE7C(bus, out_pc);
  case 0x02EE7EU: return step_ir_0x02EE7E(bus, out_pc);
  case 0x02EE80U: return step_ir_0x02EE80(bus, out_pc);
  case 0x02EE82U: return step_ir_0x02EE82(bus, out_pc);
  case 0x02EE86U: return step_ir_0x02EE86(bus, out_pc);
  case 0x02EE88U: return step_ir_0x02EE88(bus, out_pc);
  case 0x02EE8AU: return step_ir_0x02EE8A(bus, out_pc);
  case 0x02EE8EU: return step_ir_0x02EE8E(bus, out_pc);
  case 0x02EE90U: return step_ir_0x02EE90(bus, out_pc);
  case 0x02EE92U: return step_ir_0x02EE92(bus, out_pc);
  case 0x02EE96U: return step_ir_0x02EE96(bus, out_pc);
  case 0x02EE98U: return step_ir_0x02EE98(bus, out_pc);
  case 0x02EE9AU: return step_ir_0x02EE9A(bus, out_pc);
  case 0x02EE9EU: return step_ir_0x02EE9E(bus, out_pc);
  default: return false; }
}
} // namespace oasis::recompiler::ir
