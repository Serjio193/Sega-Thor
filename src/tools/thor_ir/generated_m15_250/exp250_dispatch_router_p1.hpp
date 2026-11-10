#pragma once
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_common.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part0.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part1.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part2.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part3.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part4.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part5.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_one_exp250_dispatch_ir_p1(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x00EA5AU: return step_ir_0x00EA5A(bus, out_pc);
    case 0x00EA60U: return step_ir_0x00EA60(bus, out_pc);
    case 0x00EA64U: return step_ir_0x00EA64(bus, out_pc);
    case 0x00EA6AU: return step_ir_0x00EA6A(bus, out_pc);
    case 0x00EA6EU: return step_ir_0x00EA6E(bus, out_pc);
    case 0x00EA70U: return step_ir_0x00EA70(bus, out_pc);
    case 0x00EA74U: return step_ir_0x00EA74(bus, out_pc);
    case 0x017908U: return step_ir_0x017908(bus, out_pc);
    case 0x01790CU: return step_ir_0x01790C(bus, out_pc);
    case 0x017910U: return step_ir_0x017910(bus, out_pc);
    case 0x017912U: return step_ir_0x017912(bus, out_pc);
    case 0x017916U: return step_ir_0x017916(bus, out_pc);
    case 0x017918U: return step_ir_0x017918(bus, out_pc);
    case 0x01791CU: return step_ir_0x01791C(bus, out_pc);
    case 0x01791EU: return step_ir_0x01791E(bus, out_pc);
    case 0x0255D8U: return step_ir_0x0255D8(bus, out_pc);
    case 0x0255DCU: return step_ir_0x0255DC(bus, out_pc);
    case 0x0255DEU: return step_ir_0x0255DE(bus, out_pc);
    case 0x0255E4U: return step_ir_0x0255E4(bus, out_pc);
    case 0x0255E6U: return step_ir_0x0255E6(bus, out_pc);
    case 0x0255EEU: return step_ir_0x0255EE(bus, out_pc);
    case 0x0255F0U: return step_ir_0x0255F0(bus, out_pc);
    case 0x0255F6U: return step_ir_0x0255F6(bus, out_pc);
    case 0x0255FCU: return step_ir_0x0255FC(bus, out_pc);
    case 0x0255FEU: return step_ir_0x0255FE(bus, out_pc);
    case 0x025602U: return step_ir_0x025602(bus, out_pc);
    case 0x025604U: return step_ir_0x025604(bus, out_pc);
    case 0x025606U: return step_ir_0x025606(bus, out_pc);
    case 0x02560CU: return step_ir_0x02560C(bus, out_pc);
    case 0x02560EU: return step_ir_0x02560E(bus, out_pc);
    case 0x025612U: return step_ir_0x025612(bus, out_pc);
    case 0x025616U: return step_ir_0x025616(bus, out_pc);
    case 0x02561AU: return step_ir_0x02561A(bus, out_pc);
    case 0x02561CU: return step_ir_0x02561C(bus, out_pc);
    case 0x025622U: return step_ir_0x025622(bus, out_pc);
    case 0x025626U: return step_ir_0x025626(bus, out_pc);
    case 0x025628U: return step_ir_0x025628(bus, out_pc);
    case 0x02562CU: return step_ir_0x02562C(bus, out_pc);
    case 0x025630U: return step_ir_0x025630(bus, out_pc);
    case 0x025636U: return step_ir_0x025636(bus, out_pc);
    case 0x025638U: return step_ir_0x025638(bus, out_pc);
    case 0x02563AU: return step_ir_0x02563A(bus, out_pc);
    case 0x02563CU: return step_ir_0x02563C(bus, out_pc);
    case 0x025642U: return step_ir_0x025642(bus, out_pc);
    case 0x025646U: return step_ir_0x025646(bus, out_pc);
    case 0x02564AU: return step_ir_0x02564A(bus, out_pc);
    case 0x02564EU: return step_ir_0x02564E(bus, out_pc);
    case 0x025650U: return step_ir_0x025650(bus, out_pc);
    case 0x025656U: return step_ir_0x025656(bus, out_pc);
    case 0x02565AU: return step_ir_0x02565A(bus, out_pc);
    case 0x02565EU: return step_ir_0x02565E(bus, out_pc);
    case 0x025664U: return step_ir_0x025664(bus, out_pc);
    case 0x02566AU: return step_ir_0x02566A(bus, out_pc);
    case 0x02566CU: return step_ir_0x02566C(bus, out_pc);
    case 0x025672U: return step_ir_0x025672(bus, out_pc);
    case 0x025674U: return step_ir_0x025674(bus, out_pc);
    case 0x02567AU: return step_ir_0x02567A(bus, out_pc);
    case 0x02567EU: return step_ir_0x02567E(bus, out_pc);
    case 0x025684U: return step_ir_0x025684(bus, out_pc);
    case 0x02568AU: return step_ir_0x02568A(bus, out_pc);
    case 0x025690U: return step_ir_0x025690(bus, out_pc);
    case 0x025696U: return step_ir_0x025696(bus, out_pc);
    case 0x02569CU: return step_ir_0x02569C(bus, out_pc);
    case 0x02569EU: return step_ir_0x02569E(bus, out_pc);
    case 0x0256A4U: return step_ir_0x0256A4(bus, out_pc);
    case 0x0256A6U: return step_ir_0x0256A6(bus, out_pc);
    case 0x0256AAU: return step_ir_0x0256AA(bus, out_pc);
    case 0x0256B0U: return step_ir_0x0256B0(bus, out_pc);
    case 0x0256B6U: return step_ir_0x0256B6(bus, out_pc);
    case 0x02E8B4U: return step_ir_0x02E8B4(bus, out_pc);
    case 0x02E8BAU: return step_ir_0x02E8BA(bus, out_pc);
    case 0x02E8C0U: return step_ir_0x02E8C0(bus, out_pc);
    case 0x02E8C6U: return step_ir_0x02E8C6(bus, out_pc);
    case 0x02E8C8U: return step_ir_0x02E8C8(bus, out_pc);
    case 0x02E8CEU: return step_ir_0x02E8CE(bus, out_pc);
    case 0x02E8D4U: return step_ir_0x02E8D4(bus, out_pc);
    case 0x02E8D8U: return step_ir_0x02E8D8(bus, out_pc);
    case 0x02E8DEU: return step_ir_0x02E8DE(bus, out_pc);
    case 0x02E8E2U: return step_ir_0x02E8E2(bus, out_pc);
    case 0x02E8E8U: return step_ir_0x02E8E8(bus, out_pc);
    default: return false;
  }
}
inline bool is_compiled_exp250_dispatch_pc_p1(std::uint32_t pc) noexcept {
  switch (pc) {
    case 0x00EA5AU:
    case 0x00EA60U:
    case 0x00EA64U:
    case 0x00EA6AU:
    case 0x00EA6EU:
    case 0x00EA70U:
    case 0x00EA74U:
    case 0x017908U:
    case 0x01790CU:
    case 0x017910U:
    case 0x017912U:
    case 0x017916U:
    case 0x017918U:
    case 0x01791CU:
    case 0x01791EU:
    case 0x0255D8U:
    case 0x0255DCU:
    case 0x0255DEU:
    case 0x0255E4U:
    case 0x0255E6U:
    case 0x0255EEU:
    case 0x0255F0U:
    case 0x0255F6U:
    case 0x0255FCU:
    case 0x0255FEU:
    case 0x025602U:
    case 0x025604U:
    case 0x025606U:
    case 0x02560CU:
    case 0x02560EU:
    case 0x025612U:
    case 0x025616U:
    case 0x02561AU:
    case 0x02561CU:
    case 0x025622U:
    case 0x025626U:
    case 0x025628U:
    case 0x02562CU:
    case 0x025630U:
    case 0x025636U:
    case 0x025638U:
    case 0x02563AU:
    case 0x02563CU:
    case 0x025642U:
    case 0x025646U:
    case 0x02564AU:
    case 0x02564EU:
    case 0x025650U:
    case 0x025656U:
    case 0x02565AU:
    case 0x02565EU:
    case 0x025664U:
    case 0x02566AU:
    case 0x02566CU:
    case 0x025672U:
    case 0x025674U:
    case 0x02567AU:
    case 0x02567EU:
    case 0x025684U:
    case 0x02568AU:
    case 0x025690U:
    case 0x025696U:
    case 0x02569CU:
    case 0x02569EU:
    case 0x0256A4U:
    case 0x0256A6U:
    case 0x0256AAU:
    case 0x0256B0U:
    case 0x0256B6U:
    case 0x02E8B4U:
    case 0x02E8BAU:
    case 0x02E8C0U:
    case 0x02E8C6U:
    case 0x02E8C8U:
    case 0x02E8CEU:
    case 0x02E8D4U:
    case 0x02E8D8U:
    case 0x02E8DEU:
    case 0x02E8E2U:
    case 0x02E8E8U:
      return true;
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
