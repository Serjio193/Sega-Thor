#pragma once
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_part0.hpp"
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_part1.hpp"
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_part2.hpp"
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_part3.hpp"
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_part4.hpp"
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_part5.hpp"
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_part6.hpp"
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_part7.hpp"
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_part8.hpp"
#include "tools/thor_ir/generated_m15_248/exp248_dispatch_part9.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_exp248_router_p2(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x00FD20U: return step_ir_0x00FD20(bus, out_pc);
    case 0x00FD22U: return step_ir_0x00FD22(bus, out_pc);
    case 0x00FD26U: return step_ir_0x00FD26(bus, out_pc);
    case 0x00FD2AU: return step_ir_0x00FD2A(bus, out_pc);
    case 0x00FD2CU: return step_ir_0x00FD2C(bus, out_pc);
    case 0x00FD30U: return step_ir_0x00FD30(bus, out_pc);
    case 0x00FD34U: return step_ir_0x00FD34(bus, out_pc);
    case 0x00FD38U: return step_ir_0x00FD38(bus, out_pc);
    case 0x00FD3CU: return step_ir_0x00FD3C(bus, out_pc);
    case 0x00FD3EU: return step_ir_0x00FD3E(bus, out_pc);
    case 0x00FD42U: return step_ir_0x00FD42(bus, out_pc);
    case 0x00FD46U: return step_ir_0x00FD46(bus, out_pc);
    case 0x00FD4AU: return step_ir_0x00FD4A(bus, out_pc);
    case 0x00FD4EU: return step_ir_0x00FD4E(bus, out_pc);
    case 0x00FD52U: return step_ir_0x00FD52(bus, out_pc);
    case 0x00FD56U: return step_ir_0x00FD56(bus, out_pc);
    case 0x00FD5AU: return step_ir_0x00FD5A(bus, out_pc);
    case 0x00FD5CU: return step_ir_0x00FD5C(bus, out_pc);
    case 0x01A184U: return step_ir_0x01A184(bus, out_pc);
    case 0x01A188U: return step_ir_0x01A188(bus, out_pc);
    case 0x01A18CU: return step_ir_0x01A18C(bus, out_pc);
    case 0x01A18EU: return step_ir_0x01A18E(bus, out_pc);
    case 0x01A192U: return step_ir_0x01A192(bus, out_pc);
    case 0x01A194U: return step_ir_0x01A194(bus, out_pc);
    case 0x01A198U: return step_ir_0x01A198(bus, out_pc);
    case 0x01A19AU: return step_ir_0x01A19A(bus, out_pc);
    case 0x01A19EU: return step_ir_0x01A19E(bus, out_pc);
    case 0x01A1A2U: return step_ir_0x01A1A2(bus, out_pc);
    case 0x01A1A6U: return step_ir_0x01A1A6(bus, out_pc);
    case 0x01A1AAU: return step_ir_0x01A1AA(bus, out_pc);
    case 0x01A1AEU: return step_ir_0x01A1AE(bus, out_pc);
    case 0x01A1B2U: return step_ir_0x01A1B2(bus, out_pc);
    case 0x01A1B6U: return step_ir_0x01A1B6(bus, out_pc);
    case 0x01A1BAU: return step_ir_0x01A1BA(bus, out_pc);
    case 0x01A1BEU: return step_ir_0x01A1BE(bus, out_pc);
    case 0x01A1C2U: return step_ir_0x01A1C2(bus, out_pc);
    case 0x01A1C6U: return step_ir_0x01A1C6(bus, out_pc);
    case 0x01A1CAU: return step_ir_0x01A1CA(bus, out_pc);
    case 0x01A1CEU: return step_ir_0x01A1CE(bus, out_pc);
    case 0x01A1D4U: return step_ir_0x01A1D4(bus, out_pc);
    default: return false;
  }
}
inline constexpr bool is_compiled_exp248_p2(std::uint32_t pc) noexcept {
  switch (pc) {
    case 0x00FD20U: return true;
    case 0x00FD22U: return true;
    case 0x00FD26U: return true;
    case 0x00FD2AU: return true;
    case 0x00FD2CU: return true;
    case 0x00FD30U: return true;
    case 0x00FD34U: return true;
    case 0x00FD38U: return true;
    case 0x00FD3CU: return true;
    case 0x00FD3EU: return true;
    case 0x00FD42U: return true;
    case 0x00FD46U: return true;
    case 0x00FD4AU: return true;
    case 0x00FD4EU: return true;
    case 0x00FD52U: return true;
    case 0x00FD56U: return true;
    case 0x00FD5AU: return true;
    case 0x00FD5CU: return true;
    case 0x01A184U: return true;
    case 0x01A188U: return true;
    case 0x01A18CU: return true;
    case 0x01A18EU: return true;
    case 0x01A192U: return true;
    case 0x01A194U: return true;
    case 0x01A198U: return true;
    case 0x01A19AU: return true;
    case 0x01A19EU: return true;
    case 0x01A1A2U: return true;
    case 0x01A1A6U: return true;
    case 0x01A1AAU: return true;
    case 0x01A1AEU: return true;
    case 0x01A1B2U: return true;
    case 0x01A1B6U: return true;
    case 0x01A1BAU: return true;
    case 0x01A1BEU: return true;
    case 0x01A1C2U: return true;
    case 0x01A1C6U: return true;
    case 0x01A1CAU: return true;
    case 0x01A1CEU: return true;
    case 0x01A1D4U: return true;
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
