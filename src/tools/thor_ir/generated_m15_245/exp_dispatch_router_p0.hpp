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
template <typename Bus> inline bool step_exp_chunk_0(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc & kBusAddressMask) {
  case 0x005E78U: return step_ir_0x005E78(bus, out_pc);
  case 0x005E7AU: return step_ir_0x005E7A(bus, out_pc);
  case 0x005E7EU: return step_ir_0x005E7E(bus, out_pc);
  case 0x005E82U: return step_ir_0x005E82(bus, out_pc);
  case 0x005E86U: return step_ir_0x005E86(bus, out_pc);
  case 0x005E8AU: return step_ir_0x005E8A(bus, out_pc);
  case 0x005E8EU: return step_ir_0x005E8E(bus, out_pc);
  case 0x005E92U: return step_ir_0x005E92(bus, out_pc);
  case 0x005E96U: return step_ir_0x005E96(bus, out_pc);
  case 0x005E9CU: return step_ir_0x005E9C(bus, out_pc);
  case 0x005EA2U: return step_ir_0x005EA2(bus, out_pc);
  case 0x005EA8U: return step_ir_0x005EA8(bus, out_pc);
  case 0x005EAEU: return step_ir_0x005EAE(bus, out_pc);
  case 0x005EB4U: return step_ir_0x005EB4(bus, out_pc);
  case 0x005EB8U: return step_ir_0x005EB8(bus, out_pc);
  case 0x005EBEU: return step_ir_0x005EBE(bus, out_pc);
  case 0x005EC4U: return step_ir_0x005EC4(bus, out_pc);
  case 0x005ECCU: return step_ir_0x005ECC(bus, out_pc);
  case 0x005ED2U: return step_ir_0x005ED2(bus, out_pc);
  case 0x005ED8U: return step_ir_0x005ED8(bus, out_pc);
  case 0x005EDAU: return step_ir_0x005EDA(bus, out_pc);
  case 0x005EE0U: return step_ir_0x005EE0(bus, out_pc);
  case 0x005EE6U: return step_ir_0x005EE6(bus, out_pc);
  case 0x0061F6U: return step_ir_0x0061F6(bus, out_pc);
  case 0x0061FAU: return step_ir_0x0061FA(bus, out_pc);
  case 0x0061FEU: return step_ir_0x0061FE(bus, out_pc);
  case 0x006202U: return step_ir_0x006202(bus, out_pc);
  case 0x006208U: return step_ir_0x006208(bus, out_pc);
  case 0x00620CU: return step_ir_0x00620C(bus, out_pc);
  case 0x006212U: return step_ir_0x006212(bus, out_pc);
  case 0x006216U: return step_ir_0x006216(bus, out_pc);
  case 0x006218U: return step_ir_0x006218(bus, out_pc);
  case 0x00621CU: return step_ir_0x00621C(bus, out_pc);
  case 0x00621EU: return step_ir_0x00621E(bus, out_pc);
  case 0x006222U: return step_ir_0x006222(bus, out_pc);
  case 0x006224U: return step_ir_0x006224(bus, out_pc);
  case 0x006228U: return step_ir_0x006228(bus, out_pc);
  case 0x00622AU: return step_ir_0x00622A(bus, out_pc);
  case 0x00622EU: return step_ir_0x00622E(bus, out_pc);
  case 0x006230U: return step_ir_0x006230(bus, out_pc);
  case 0x006234U: return step_ir_0x006234(bus, out_pc);
  case 0x006238U: return step_ir_0x006238(bus, out_pc);
  case 0x00623CU: return step_ir_0x00623C(bus, out_pc);
  case 0x006240U: return step_ir_0x006240(bus, out_pc);
  case 0x006244U: return step_ir_0x006244(bus, out_pc);
  case 0x006248U: return step_ir_0x006248(bus, out_pc);
  case 0x00624CU: return step_ir_0x00624C(bus, out_pc);
  case 0x006252U: return step_ir_0x006252(bus, out_pc);
  case 0x006258U: return step_ir_0x006258(bus, out_pc);
  case 0x00625EU: return step_ir_0x00625E(bus, out_pc);
  case 0x006260U: return step_ir_0x006260(bus, out_pc);
  case 0x006266U: return step_ir_0x006266(bus, out_pc);
  case 0x00626CU: return step_ir_0x00626C(bus, out_pc);
  case 0x00626EU: return step_ir_0x00626E(bus, out_pc);
  case 0x006270U: return step_ir_0x006270(bus, out_pc);
  case 0x006272U: return step_ir_0x006272(bus, out_pc);
  case 0x006274U: return step_ir_0x006274(bus, out_pc);
  case 0x006276U: return step_ir_0x006276(bus, out_pc);
  case 0x006278U: return step_ir_0x006278(bus, out_pc);
  case 0x00627AU: return step_ir_0x00627A(bus, out_pc);
  case 0x00627EU: return step_ir_0x00627E(bus, out_pc);
  case 0x006282U: return step_ir_0x006282(bus, out_pc);
  case 0x006286U: return step_ir_0x006286(bus, out_pc);
  case 0x00628AU: return step_ir_0x00628A(bus, out_pc);
  case 0x00628EU: return step_ir_0x00628E(bus, out_pc);
  case 0x006294U: return step_ir_0x006294(bus, out_pc);
  case 0x00629AU: return step_ir_0x00629A(bus, out_pc);
  case 0x0062A0U: return step_ir_0x0062A0(bus, out_pc);
  case 0x0062A6U: return step_ir_0x0062A6(bus, out_pc);
  case 0x0062AEU: return step_ir_0x0062AE(bus, out_pc);
  case 0x0062B0U: return step_ir_0x0062B0(bus, out_pc);
  case 0x0062B6U: return step_ir_0x0062B6(bus, out_pc);
  case 0x0062BCU: return step_ir_0x0062BC(bus, out_pc);
  case 0x0062BEU: return step_ir_0x0062BE(bus, out_pc);
  case 0x0062C4U: return step_ir_0x0062C4(bus, out_pc);
  case 0x0062CAU: return step_ir_0x0062CA(bus, out_pc);
  case 0x006338U: return step_ir_0x006338(bus, out_pc);
  case 0x00633CU: return step_ir_0x00633C(bus, out_pc);
  case 0x00633EU: return step_ir_0x00633E(bus, out_pc);
  case 0x006342U: return step_ir_0x006342(bus, out_pc);
  case 0x006346U: return step_ir_0x006346(bus, out_pc);
  case 0x00634AU: return step_ir_0x00634A(bus, out_pc);
  case 0x006350U: return step_ir_0x006350(bus, out_pc);
  case 0x006354U: return step_ir_0x006354(bus, out_pc);
  case 0x006356U: return step_ir_0x006356(bus, out_pc);
  case 0x006358U: return step_ir_0x006358(bus, out_pc);
  case 0x00635AU: return step_ir_0x00635A(bus, out_pc);
  case 0x00635EU: return step_ir_0x00635E(bus, out_pc);
  case 0x006362U: return step_ir_0x006362(bus, out_pc);
  case 0x006366U: return step_ir_0x006366(bus, out_pc);
  case 0x00636AU: return step_ir_0x00636A(bus, out_pc);
  case 0x00636EU: return step_ir_0x00636E(bus, out_pc);
  case 0x006372U: return step_ir_0x006372(bus, out_pc);
  case 0x006376U: return step_ir_0x006376(bus, out_pc);
  case 0x006378U: return step_ir_0x006378(bus, out_pc);
  case 0x00637CU: return step_ir_0x00637C(bus, out_pc);
  case 0x00637EU: return step_ir_0x00637E(bus, out_pc);
  case 0x006384U: return step_ir_0x006384(bus, out_pc);
  case 0x00638AU: return step_ir_0x00638A(bus, out_pc);
  case 0x006464U: return step_ir_0x006464(bus, out_pc);
  case 0x006468U: return step_ir_0x006468(bus, out_pc);
  case 0x00646AU: return step_ir_0x00646A(bus, out_pc);
  case 0x00646EU: return step_ir_0x00646E(bus, out_pc);
  case 0x006472U: return step_ir_0x006472(bus, out_pc);
  case 0x006476U: return step_ir_0x006476(bus, out_pc);
  case 0x006478U: return step_ir_0x006478(bus, out_pc);
  case 0x00647CU: return step_ir_0x00647C(bus, out_pc);
  case 0x006482U: return step_ir_0x006482(bus, out_pc);
  case 0x006486U: return step_ir_0x006486(bus, out_pc);
  case 0x00648AU: return step_ir_0x00648A(bus, out_pc);
  case 0x00648CU: return step_ir_0x00648C(bus, out_pc);
  case 0x00648EU: return step_ir_0x00648E(bus, out_pc);
  case 0x006492U: return step_ir_0x006492(bus, out_pc);
  case 0x006494U: return step_ir_0x006494(bus, out_pc);
  case 0x006496U: return step_ir_0x006496(bus, out_pc);
  case 0x00649AU: return step_ir_0x00649A(bus, out_pc);
  case 0x00649CU: return step_ir_0x00649C(bus, out_pc);
  case 0x00649EU: return step_ir_0x00649E(bus, out_pc);
  case 0x0064A2U: return step_ir_0x0064A2(bus, out_pc);
  case 0x0064A4U: return step_ir_0x0064A4(bus, out_pc);
  case 0x0064A6U: return step_ir_0x0064A6(bus, out_pc);
  case 0x0064A8U: return step_ir_0x0064A8(bus, out_pc);
  case 0x0064AAU: return step_ir_0x0064AA(bus, out_pc);
  case 0x0064ACU: return step_ir_0x0064AC(bus, out_pc);
  case 0x0064B0U: return step_ir_0x0064B0(bus, out_pc);
  case 0x0064B6U: return step_ir_0x0064B6(bus, out_pc);
  case 0x0064BCU: return step_ir_0x0064BC(bus, out_pc);
  case 0x0064C2U: return step_ir_0x0064C2(bus, out_pc);
  case 0x0066AAU: return step_ir_0x0066AA(bus, out_pc);
  case 0x0066ACU: return step_ir_0x0066AC(bus, out_pc);
  case 0x0066B0U: return step_ir_0x0066B0(bus, out_pc);
  case 0x0066B6U: return step_ir_0x0066B6(bus, out_pc);
  case 0x0066B8U: return step_ir_0x0066B8(bus, out_pc);
  case 0x0066C0U: return step_ir_0x0066C0(bus, out_pc);
  case 0x0066C2U: return step_ir_0x0066C2(bus, out_pc);
  case 0x0066C8U: return step_ir_0x0066C8(bus, out_pc);
  case 0x0066CEU: return step_ir_0x0066CE(bus, out_pc);
  case 0x0066D4U: return step_ir_0x0066D4(bus, out_pc);
  case 0x0066D6U: return step_ir_0x0066D6(bus, out_pc);
  case 0x0066DCU: return step_ir_0x0066DC(bus, out_pc);
  case 0x0066DEU: return step_ir_0x0066DE(bus, out_pc);
  case 0x0066E4U: return step_ir_0x0066E4(bus, out_pc);
  case 0x0066EAU: return step_ir_0x0066EA(bus, out_pc);
  case 0x0066F0U: return step_ir_0x0066F0(bus, out_pc);
  case 0x0066F4U: return step_ir_0x0066F4(bus, out_pc);
  case 0x0066F6U: return step_ir_0x0066F6(bus, out_pc);
  case 0x0066F8U: return step_ir_0x0066F8(bus, out_pc);
  case 0x0066FAU: return step_ir_0x0066FA(bus, out_pc);
  case 0x0066FCU: return step_ir_0x0066FC(bus, out_pc);
  case 0x0066FEU: return step_ir_0x0066FE(bus, out_pc);
  case 0x006700U: return step_ir_0x006700(bus, out_pc);
  case 0x006702U: return step_ir_0x006702(bus, out_pc);
  case 0x006704U: return step_ir_0x006704(bus, out_pc);
  case 0x006708U: return step_ir_0x006708(bus, out_pc);
  case 0x00670AU: return step_ir_0x00670A(bus, out_pc);
  case 0x00670CU: return step_ir_0x00670C(bus, out_pc);
  case 0x00670EU: return step_ir_0x00670E(bus, out_pc);
  case 0x006710U: return step_ir_0x006710(bus, out_pc);
  case 0x006712U: return step_ir_0x006712(bus, out_pc);
  case 0x006714U: return step_ir_0x006714(bus, out_pc);
  case 0x006716U: return step_ir_0x006716(bus, out_pc);
  case 0x006718U: return step_ir_0x006718(bus, out_pc);
  case 0x00671CU: return step_ir_0x00671C(bus, out_pc);
  case 0x006720U: return step_ir_0x006720(bus, out_pc);
  case 0x006724U: return step_ir_0x006724(bus, out_pc);
  case 0x006728U: return step_ir_0x006728(bus, out_pc);
  case 0x00672CU: return step_ir_0x00672C(bus, out_pc);
  case 0x006732U: return step_ir_0x006732(bus, out_pc);
  case 0x006736U: return step_ir_0x006736(bus, out_pc);
  case 0x006738U: return step_ir_0x006738(bus, out_pc);
  case 0x00673EU: return step_ir_0x00673E(bus, out_pc);
  case 0x006742U: return step_ir_0x006742(bus, out_pc);
  case 0x006744U: return step_ir_0x006744(bus, out_pc);
  case 0x00674AU: return step_ir_0x00674A(bus, out_pc);
  case 0x00674CU: return step_ir_0x00674C(bus, out_pc);
  case 0x00674EU: return step_ir_0x00674E(bus, out_pc);
  case 0x006752U: return step_ir_0x006752(bus, out_pc);
  case 0x006758U: return step_ir_0x006758(bus, out_pc);
  case 0x00675AU: return step_ir_0x00675A(bus, out_pc);
  case 0x00675CU: return step_ir_0x00675C(bus, out_pc);
  default: return false; }
}
} // namespace oasis::recompiler::ir
