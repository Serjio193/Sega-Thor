#pragma once
#include "tools/thor_ir/generated_m15_249/exp249_dispatch_part0.hpp"
#include "tools/thor_ir/generated_m15_249/exp249_dispatch_part1.hpp"
#include "tools/thor_ir/generated_m15_249/exp249_dispatch_part2.hpp"
#include "tools/thor_ir/generated_m15_249/exp249_dispatch_part3.hpp"
#include "tools/thor_ir/generated_m15_249/exp249_dispatch_part4.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_exp249_router_p1(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x02557EU: return step_ir_0x02557E(bus, out_pc);
    case 0x025584U: return step_ir_0x025584(bus, out_pc);
    case 0x02558AU: return step_ir_0x02558A(bus, out_pc);
    case 0x025590U: return step_ir_0x025590(bus, out_pc);
    case 0x025596U: return step_ir_0x025596(bus, out_pc);
    case 0x025598U: return step_ir_0x025598(bus, out_pc);
    case 0x02559EU: return step_ir_0x02559E(bus, out_pc);
    case 0x0255A4U: return step_ir_0x0255A4(bus, out_pc);
    case 0x0255AAU: return step_ir_0x0255AA(bus, out_pc);
    case 0x0255AEU: return step_ir_0x0255AE(bus, out_pc);
    case 0x0255B4U: return step_ir_0x0255B4(bus, out_pc);
    case 0x0255BAU: return step_ir_0x0255BA(bus, out_pc);
    case 0x0255BEU: return step_ir_0x0255BE(bus, out_pc);
    case 0x0255C4U: return step_ir_0x0255C4(bus, out_pc);
    case 0x0255C8U: return step_ir_0x0255C8(bus, out_pc);
    case 0x0255CAU: return step_ir_0x0255CA(bus, out_pc);
    case 0x0255D0U: return step_ir_0x0255D0(bus, out_pc);
    case 0x0255D6U: return step_ir_0x0255D6(bus, out_pc);
    default: return false;
  }
}
inline constexpr bool is_compiled_exp249_p1(std::uint32_t pc) noexcept {
  switch (pc) {
    case 0x02557EU: return true;
    case 0x025584U: return true;
    case 0x02558AU: return true;
    case 0x025590U: return true;
    case 0x025596U: return true;
    case 0x025598U: return true;
    case 0x02559EU: return true;
    case 0x0255A4U: return true;
    case 0x0255AAU: return true;
    case 0x0255AEU: return true;
    case 0x0255B4U: return true;
    case 0x0255BAU: return true;
    case 0x0255BEU: return true;
    case 0x0255C4U: return true;
    case 0x0255C8U: return true;
    case 0x0255CAU: return true;
    case 0x0255D0U: return true;
    case 0x0255D6U: return true;
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
