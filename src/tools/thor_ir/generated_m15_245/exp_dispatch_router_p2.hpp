#pragma once
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part0.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part1.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part2.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part3.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part4.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part5.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part6.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part7.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part8.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part9.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part10.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part11.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part12.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part13.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part14.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part15.hpp"
#include "tools/thor_ir/generated_m15_245/exp_dispatch_part16.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_exp_chunk_2(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc & kBusAddressMask) {
  case 0x0076F2U: return step_ir_0x0076F2(bus, out_pc);
  case 0x0076F6U: return step_ir_0x0076F6(bus, out_pc);
  case 0x0076F8U: return step_ir_0x0076F8(bus, out_pc);
  case 0x0076FEU: return step_ir_0x0076FE(bus, out_pc);
  case 0x007700U: return step_ir_0x007700(bus, out_pc);
  case 0x007706U: return step_ir_0x007706(bus, out_pc);
  case 0x00770AU: return step_ir_0x00770A(bus, out_pc);
  case 0x00770CU: return step_ir_0x00770C(bus, out_pc);
  case 0x007712U: return step_ir_0x007712(bus, out_pc);
  case 0x007718U: return step_ir_0x007718(bus, out_pc);
  case 0x007720U: return step_ir_0x007720(bus, out_pc);
  case 0x007726U: return step_ir_0x007726(bus, out_pc);
  case 0x00772EU: return step_ir_0x00772E(bus, out_pc);
  case 0x007734U: return step_ir_0x007734(bus, out_pc);
  case 0x00773AU: return step_ir_0x00773A(bus, out_pc);
  case 0x007740U: return step_ir_0x007740(bus, out_pc);
  case 0x007742U: return step_ir_0x007742(bus, out_pc);
  case 0x007748U: return step_ir_0x007748(bus, out_pc);
  case 0x00774EU: return step_ir_0x00774E(bus, out_pc);
  case 0x007756U: return step_ir_0x007756(bus, out_pc);
  case 0x00775CU: return step_ir_0x00775C(bus, out_pc);
  case 0x007762U: return step_ir_0x007762(bus, out_pc);
  case 0x00776AU: return step_ir_0x00776A(bus, out_pc);
  case 0x00776CU: return step_ir_0x00776C(bus, out_pc);
  case 0x007772U: return step_ir_0x007772(bus, out_pc);
  case 0x007778U: return step_ir_0x007778(bus, out_pc);
  case 0x00777EU: return step_ir_0x00777E(bus, out_pc);
  case 0x007786U: return step_ir_0x007786(bus, out_pc);
  case 0x00778AU: return step_ir_0x00778A(bus, out_pc);
  case 0x00778EU: return step_ir_0x00778E(bus, out_pc);
  case 0x007792U: return step_ir_0x007792(bus, out_pc);
  case 0x007798U: return step_ir_0x007798(bus, out_pc);
  case 0x00779CU: return step_ir_0x00779C(bus, out_pc);
  case 0x0077A0U: return step_ir_0x0077A0(bus, out_pc);
  case 0x0077A6U: return step_ir_0x0077A6(bus, out_pc);
  case 0x0077ACU: return step_ir_0x0077AC(bus, out_pc);
  case 0x0077AEU: return step_ir_0x0077AE(bus, out_pc);
  case 0x0077B4U: return step_ir_0x0077B4(bus, out_pc);
  case 0x0077BAU: return step_ir_0x0077BA(bus, out_pc);
  case 0x0077C0U: return step_ir_0x0077C0(bus, out_pc);
  case 0x0077C6U: return step_ir_0x0077C6(bus, out_pc);
  case 0x0077CCU: return step_ir_0x0077CC(bus, out_pc);
  case 0x0077CEU: return step_ir_0x0077CE(bus, out_pc);
  case 0x0077D4U: return step_ir_0x0077D4(bus, out_pc);
  case 0x0077DAU: return step_ir_0x0077DA(bus, out_pc);
  case 0x0077DEU: return step_ir_0x0077DE(bus, out_pc);
  case 0x0077E2U: return step_ir_0x0077E2(bus, out_pc);
  case 0x0077E6U: return step_ir_0x0077E6(bus, out_pc);
  case 0x0077ECU: return step_ir_0x0077EC(bus, out_pc);
  case 0x0077F0U: return step_ir_0x0077F0(bus, out_pc);
  case 0x0077F4U: return step_ir_0x0077F4(bus, out_pc);
  case 0x0077FAU: return step_ir_0x0077FA(bus, out_pc);
  case 0x007800U: return step_ir_0x007800(bus, out_pc);
  case 0x007806U: return step_ir_0x007806(bus, out_pc);
  case 0x00780CU: return step_ir_0x00780C(bus, out_pc);
  case 0x007852U: return step_ir_0x007852(bus, out_pc);
  case 0x007854U: return step_ir_0x007854(bus, out_pc);
  case 0x007858U: return step_ir_0x007858(bus, out_pc);
  case 0x00785EU: return step_ir_0x00785E(bus, out_pc);
  case 0x007862U: return step_ir_0x007862(bus, out_pc);
  case 0x007864U: return step_ir_0x007864(bus, out_pc);
  case 0x007868U: return step_ir_0x007868(bus, out_pc);
  case 0x00786CU: return step_ir_0x00786C(bus, out_pc);
  case 0x007870U: return step_ir_0x007870(bus, out_pc);
  case 0x007874U: return step_ir_0x007874(bus, out_pc);
  case 0x007878U: return step_ir_0x007878(bus, out_pc);
  case 0x00787EU: return step_ir_0x00787E(bus, out_pc);
  case 0x007882U: return step_ir_0x007882(bus, out_pc);
  case 0x007888U: return step_ir_0x007888(bus, out_pc);
  case 0x00788CU: return step_ir_0x00788C(bus, out_pc);
  case 0x007892U: return step_ir_0x007892(bus, out_pc);
  case 0x007896U: return step_ir_0x007896(bus, out_pc);
  case 0x00789CU: return step_ir_0x00789C(bus, out_pc);
  case 0x0078A0U: return step_ir_0x0078A0(bus, out_pc);
  case 0x0078A6U: return step_ir_0x0078A6(bus, out_pc);
  case 0x0078AAU: return step_ir_0x0078AA(bus, out_pc);
  case 0x0078B2U: return step_ir_0x0078B2(bus, out_pc);
  case 0x0078B6U: return step_ir_0x0078B6(bus, out_pc);
  case 0x0078BCU: return step_ir_0x0078BC(bus, out_pc);
  case 0x0078C2U: return step_ir_0x0078C2(bus, out_pc);
  case 0x0078C8U: return step_ir_0x0078C8(bus, out_pc);
  case 0x0078CEU: return step_ir_0x0078CE(bus, out_pc);
  case 0x0078D4U: return step_ir_0x0078D4(bus, out_pc);
  case 0x0078D6U: return step_ir_0x0078D6(bus, out_pc);
  case 0x0078DEU: return step_ir_0x0078DE(bus, out_pc);
  case 0x0078E2U: return step_ir_0x0078E2(bus, out_pc);
  case 0x0078E6U: return step_ir_0x0078E6(bus, out_pc);
  case 0x0078EAU: return step_ir_0x0078EA(bus, out_pc);
  case 0x0078F0U: return step_ir_0x0078F0(bus, out_pc);
  case 0x0078F6U: return step_ir_0x0078F6(bus, out_pc);
  case 0x0078F8U: return step_ir_0x0078F8(bus, out_pc);
  case 0x0078FEU: return step_ir_0x0078FE(bus, out_pc);
  case 0x007904U: return step_ir_0x007904(bus, out_pc);
  case 0x007906U: return step_ir_0x007906(bus, out_pc);
  case 0x007910U: return step_ir_0x007910(bus, out_pc);
  case 0x007912U: return step_ir_0x007912(bus, out_pc);
  case 0x007914U: return step_ir_0x007914(bus, out_pc);
  case 0x007918U: return step_ir_0x007918(bus, out_pc);
  case 0x00791AU: return step_ir_0x00791A(bus, out_pc);
  case 0x00791EU: return step_ir_0x00791E(bus, out_pc);
  case 0x007924U: return step_ir_0x007924(bus, out_pc);
  case 0x007928U: return step_ir_0x007928(bus, out_pc);
  case 0x007DC8U: return step_ir_0x007DC8(bus, out_pc);
  case 0x007DCEU: return step_ir_0x007DCE(bus, out_pc);
  case 0x007DD2U: return step_ir_0x007DD2(bus, out_pc);
  case 0x007DD8U: return step_ir_0x007DD8(bus, out_pc);
  case 0x007DDCU: return step_ir_0x007DDC(bus, out_pc);
  case 0x007DE2U: return step_ir_0x007DE2(bus, out_pc);
  case 0x007DE6U: return step_ir_0x007DE6(bus, out_pc);
  case 0x007DECU: return step_ir_0x007DEC(bus, out_pc);
  case 0x007DF2U: return step_ir_0x007DF2(bus, out_pc);
  case 0x007DF8U: return step_ir_0x007DF8(bus, out_pc);
  case 0x007DFEU: return step_ir_0x007DFE(bus, out_pc);
  case 0x007E00U: return step_ir_0x007E00(bus, out_pc);
  case 0x007E04U: return step_ir_0x007E04(bus, out_pc);
  case 0x007E08U: return step_ir_0x007E08(bus, out_pc);
  case 0x007E0CU: return step_ir_0x007E0C(bus, out_pc);
  case 0x007E10U: return step_ir_0x007E10(bus, out_pc);
  case 0x007E12U: return step_ir_0x007E12(bus, out_pc);
  case 0x007E16U: return step_ir_0x007E16(bus, out_pc);
  case 0x007E18U: return step_ir_0x007E18(bus, out_pc);
  case 0x007E1CU: return step_ir_0x007E1C(bus, out_pc);
  case 0x007E20U: return step_ir_0x007E20(bus, out_pc);
  case 0x007E24U: return step_ir_0x007E24(bus, out_pc);
  case 0x007E28U: return step_ir_0x007E28(bus, out_pc);
  case 0x007E2CU: return step_ir_0x007E2C(bus, out_pc);
  case 0x007E30U: return step_ir_0x007E30(bus, out_pc);
  case 0x007E38U: return step_ir_0x007E38(bus, out_pc);
  case 0x007E3CU: return step_ir_0x007E3C(bus, out_pc);
  case 0x007E40U: return step_ir_0x007E40(bus, out_pc);
  case 0x007E44U: return step_ir_0x007E44(bus, out_pc);
  case 0x007E46U: return step_ir_0x007E46(bus, out_pc);
  case 0x007E4CU: return step_ir_0x007E4C(bus, out_pc);
  case 0x007E50U: return step_ir_0x007E50(bus, out_pc);
  case 0x007E58U: return step_ir_0x007E58(bus, out_pc);
  case 0x007E5CU: return step_ir_0x007E5C(bus, out_pc);
  case 0x007E62U: return step_ir_0x007E62(bus, out_pc);
  case 0x007E66U: return step_ir_0x007E66(bus, out_pc);
  case 0x007E6CU: return step_ir_0x007E6C(bus, out_pc);
  case 0x007E70U: return step_ir_0x007E70(bus, out_pc);
  case 0x007E74U: return step_ir_0x007E74(bus, out_pc);
  case 0x007E78U: return step_ir_0x007E78(bus, out_pc);
  case 0x007E7EU: return step_ir_0x007E7E(bus, out_pc);
  case 0x007E84U: return step_ir_0x007E84(bus, out_pc);
  case 0x007E8AU: return step_ir_0x007E8A(bus, out_pc);
  case 0x007E90U: return step_ir_0x007E90(bus, out_pc);
  case 0x007E96U: return step_ir_0x007E96(bus, out_pc);
  case 0x007E9CU: return step_ir_0x007E9C(bus, out_pc);
  default: return false; }
}
} // namespace oasis::recompiler::ir
