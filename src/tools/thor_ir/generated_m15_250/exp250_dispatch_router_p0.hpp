#pragma once
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_common.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part0.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part1.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part2.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part3.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part4.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_part5.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool step_one_exp250_dispatch_ir_p0(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x008778U: return step_ir_0x008778(bus, out_pc);
    case 0x00877EU: return step_ir_0x00877E(bus, out_pc);
    case 0x008784U: return step_ir_0x008784(bus, out_pc);
    case 0x00878AU: return step_ir_0x00878A(bus, out_pc);
    case 0x00878CU: return step_ir_0x00878C(bus, out_pc);
    case 0x008790U: return step_ir_0x008790(bus, out_pc);
    case 0x008794U: return step_ir_0x008794(bus, out_pc);
    case 0x008798U: return step_ir_0x008798(bus, out_pc);
    case 0x00879CU: return step_ir_0x00879C(bus, out_pc);
    case 0x0087A0U: return step_ir_0x0087A0(bus, out_pc);
    case 0x0087A2U: return step_ir_0x0087A2(bus, out_pc);
    case 0x0087A6U: return step_ir_0x0087A6(bus, out_pc);
    case 0x0087AAU: return step_ir_0x0087AA(bus, out_pc);
    case 0x0087ACU: return step_ir_0x0087AC(bus, out_pc);
    case 0x0087B0U: return step_ir_0x0087B0(bus, out_pc);
    case 0x0087B2U: return step_ir_0x0087B2(bus, out_pc);
    case 0x0087B6U: return step_ir_0x0087B6(bus, out_pc);
    case 0x0087B8U: return step_ir_0x0087B8(bus, out_pc);
    case 0x0087BCU: return step_ir_0x0087BC(bus, out_pc);
    case 0x0087BEU: return step_ir_0x0087BE(bus, out_pc);
    case 0x0087C2U: return step_ir_0x0087C2(bus, out_pc);
    case 0x0087C6U: return step_ir_0x0087C6(bus, out_pc);
    case 0x0087CAU: return step_ir_0x0087CA(bus, out_pc);
    case 0x0087CCU: return step_ir_0x0087CC(bus, out_pc);
    case 0x0087D0U: return step_ir_0x0087D0(bus, out_pc);
    case 0x0087D2U: return step_ir_0x0087D2(bus, out_pc);
    case 0x0087D4U: return step_ir_0x0087D4(bus, out_pc);
    case 0x0087D8U: return step_ir_0x0087D8(bus, out_pc);
    case 0x0087DAU: return step_ir_0x0087DA(bus, out_pc);
    case 0x0087DEU: return step_ir_0x0087DE(bus, out_pc);
    case 0x00A4C8U: return step_ir_0x00A4C8(bus, out_pc);
    case 0x00A4D0U: return step_ir_0x00A4D0(bus, out_pc);
    case 0x00A4D6U: return step_ir_0x00A4D6(bus, out_pc);
    case 0x00A4DCU: return step_ir_0x00A4DC(bus, out_pc);
    case 0x00A4E0U: return step_ir_0x00A4E0(bus, out_pc);
    case 0x00A4E8U: return step_ir_0x00A4E8(bus, out_pc);
    case 0x00A4EEU: return step_ir_0x00A4EE(bus, out_pc);
    case 0x00A4F2U: return step_ir_0x00A4F2(bus, out_pc);
    case 0x00A4F8U: return step_ir_0x00A4F8(bus, out_pc);
    case 0x00A4FAU: return step_ir_0x00A4FA(bus, out_pc);
    case 0x00A4FEU: return step_ir_0x00A4FE(bus, out_pc);
    case 0x00A504U: return step_ir_0x00A504(bus, out_pc);
    case 0x00A50AU: return step_ir_0x00A50A(bus, out_pc);
    case 0x00A50EU: return step_ir_0x00A50E(bus, out_pc);
    case 0x00A514U: return step_ir_0x00A514(bus, out_pc);
    case 0x00A550U: return step_ir_0x00A550(bus, out_pc);
    case 0x00A556U: return step_ir_0x00A556(bus, out_pc);
    case 0x00A55CU: return step_ir_0x00A55C(bus, out_pc);
    case 0x00A55EU: return step_ir_0x00A55E(bus, out_pc);
    case 0x00A560U: return step_ir_0x00A560(bus, out_pc);
    case 0x00A562U: return step_ir_0x00A562(bus, out_pc);
    case 0x00A564U: return step_ir_0x00A564(bus, out_pc);
    case 0x00A568U: return step_ir_0x00A568(bus, out_pc);
    case 0x00A56CU: return step_ir_0x00A56C(bus, out_pc);
    case 0x00A570U: return step_ir_0x00A570(bus, out_pc);
    case 0x00A574U: return step_ir_0x00A574(bus, out_pc);
    case 0x00A576U: return step_ir_0x00A576(bus, out_pc);
    case 0x00A578U: return step_ir_0x00A578(bus, out_pc);
    case 0x00A57CU: return step_ir_0x00A57C(bus, out_pc);
    case 0x00A580U: return step_ir_0x00A580(bus, out_pc);
    case 0x00A582U: return step_ir_0x00A582(bus, out_pc);
    case 0x00A586U: return step_ir_0x00A586(bus, out_pc);
    case 0x00A588U: return step_ir_0x00A588(bus, out_pc);
    case 0x00EA16U: return step_ir_0x00EA16(bus, out_pc);
    case 0x00EA1CU: return step_ir_0x00EA1C(bus, out_pc);
    case 0x00EA20U: return step_ir_0x00EA20(bus, out_pc);
    case 0x00EA24U: return step_ir_0x00EA24(bus, out_pc);
    case 0x00EA28U: return step_ir_0x00EA28(bus, out_pc);
    case 0x00EA2AU: return step_ir_0x00EA2A(bus, out_pc);
    case 0x00EA2EU: return step_ir_0x00EA2E(bus, out_pc);
    case 0x00EA32U: return step_ir_0x00EA32(bus, out_pc);
    case 0x00EA36U: return step_ir_0x00EA36(bus, out_pc);
    case 0x00EA3AU: return step_ir_0x00EA3A(bus, out_pc);
    case 0x00EA3EU: return step_ir_0x00EA3E(bus, out_pc);
    case 0x00EA42U: return step_ir_0x00EA42(bus, out_pc);
    case 0x00EA46U: return step_ir_0x00EA46(bus, out_pc);
    case 0x00EA4AU: return step_ir_0x00EA4A(bus, out_pc);
    case 0x00EA4EU: return step_ir_0x00EA4E(bus, out_pc);
    case 0x00EA52U: return step_ir_0x00EA52(bus, out_pc);
    case 0x00EA56U: return step_ir_0x00EA56(bus, out_pc);
    default: return false;
  }
}
inline bool is_compiled_exp250_dispatch_pc_p0(std::uint32_t pc) noexcept {
  switch (pc) {
    case 0x008778U:
    case 0x00877EU:
    case 0x008784U:
    case 0x00878AU:
    case 0x00878CU:
    case 0x008790U:
    case 0x008794U:
    case 0x008798U:
    case 0x00879CU:
    case 0x0087A0U:
    case 0x0087A2U:
    case 0x0087A6U:
    case 0x0087AAU:
    case 0x0087ACU:
    case 0x0087B0U:
    case 0x0087B2U:
    case 0x0087B6U:
    case 0x0087B8U:
    case 0x0087BCU:
    case 0x0087BEU:
    case 0x0087C2U:
    case 0x0087C6U:
    case 0x0087CAU:
    case 0x0087CCU:
    case 0x0087D0U:
    case 0x0087D2U:
    case 0x0087D4U:
    case 0x0087D8U:
    case 0x0087DAU:
    case 0x0087DEU:
    case 0x00A4C8U:
    case 0x00A4D0U:
    case 0x00A4D6U:
    case 0x00A4DCU:
    case 0x00A4E0U:
    case 0x00A4E8U:
    case 0x00A4EEU:
    case 0x00A4F2U:
    case 0x00A4F8U:
    case 0x00A4FAU:
    case 0x00A4FEU:
    case 0x00A504U:
    case 0x00A50AU:
    case 0x00A50EU:
    case 0x00A514U:
    case 0x00A550U:
    case 0x00A556U:
    case 0x00A55CU:
    case 0x00A55EU:
    case 0x00A560U:
    case 0x00A562U:
    case 0x00A564U:
    case 0x00A568U:
    case 0x00A56CU:
    case 0x00A570U:
    case 0x00A574U:
    case 0x00A576U:
    case 0x00A578U:
    case 0x00A57CU:
    case 0x00A580U:
    case 0x00A582U:
    case 0x00A586U:
    case 0x00A588U:
    case 0x00EA16U:
    case 0x00EA1CU:
    case 0x00EA20U:
    case 0x00EA24U:
    case 0x00EA28U:
    case 0x00EA2AU:
    case 0x00EA2EU:
    case 0x00EA32U:
    case 0x00EA36U:
    case 0x00EA3AU:
    case 0x00EA3EU:
    case 0x00EA42U:
    case 0x00EA46U:
    case 0x00EA4AU:
    case 0x00EA4EU:
    case 0x00EA52U:
    case 0x00EA56U:
      return true;
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
