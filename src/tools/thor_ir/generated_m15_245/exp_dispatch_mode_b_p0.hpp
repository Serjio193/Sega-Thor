#pragma once
#include "tools/thor_ir/generated_m15_245/exp_dispatch_router.hpp"
namespace oasis::recompiler::ir {
inline bool run_exp_mb_chunk_0(PortableIrBus& bus, std::uint32_t pc, std::uint32_t stop_pc, std::uint32_t& next_pc) noexcept {
  switch (pc) {
  case 0x005E78U: {
    if (!step_ir_0x005E78(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x005E7A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x005E7E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x005E82(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x005E86U: {
    if (!step_ir_0x005E86(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x005E8A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x005E8EU: {
    if (!step_ir_0x005E8E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x005E92U: {
    if (!step_ir_0x005E92(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x005E96U: {
    if (!step_ir_0x005E96(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x005E9C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x005EA2(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x005EA8(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x005EAE(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x005EB4(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x005EB8(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x005EBE(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x005EC4(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x005ECC(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x005ED2(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x005ED8(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x005EDAU: {
    if (!step_ir_0x005EDA(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x005EE0U: {
    if (!step_ir_0x005EE0(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x005EE6U: {
    if (!step_ir_0x005EE6(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0061F6U: {
    if (!step_ir_0x0061F6(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0061FAU: {
    if (!step_ir_0x0061FA(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0061FEU: {
    if (!step_ir_0x0061FE(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x006202U: {
    if (!step_ir_0x006202(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006208(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00620CU: {
    if (!step_ir_0x00620C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006212(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006216(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006218(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00621C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00621EU: {
    if (!step_ir_0x00621E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006222(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x006224U: {
    if (!step_ir_0x006224(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006228(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00622AU: {
    if (!step_ir_0x00622A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00622E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x006230U: {
    if (!step_ir_0x006230(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x006234U: {
    if (!step_ir_0x006234(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x006238U: {
    if (!step_ir_0x006238(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00623C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006240(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006244(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006248(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00624C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006252(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006258(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00625E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x006260U: {
    if (!step_ir_0x006260(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006266(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00626CU: {
    if (!step_ir_0x00626C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00626EU: {
    if (!step_ir_0x00626E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006270(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006272(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006274(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006276(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006278(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00627A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00627E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006282(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006286(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00628A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00628E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006294(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00629A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0062A0(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0062A6(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0062AE(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0062B0U: {
    if (!step_ir_0x0062B0(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0062B6U: {
    if (!step_ir_0x0062B6(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0062BC(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0062BEU: {
    if (!step_ir_0x0062BE(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0062C4U: {
    if (!step_ir_0x0062C4(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0062CAU: {
    if (!step_ir_0x0062CA(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x006338U: {
    if (!step_ir_0x006338(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00633C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00633EU: {
    if (!step_ir_0x00633E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x006342U: {
    if (!step_ir_0x006342(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x006346U: {
    if (!step_ir_0x006346(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00634AU: {
    if (!step_ir_0x00634A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006350(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006354(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006356(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006358(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00635A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00635EU: {
    if (!step_ir_0x00635E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x006362U: {
    if (!step_ir_0x006362(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006366(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00636A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00636E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006372(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006376(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x006378U: {
    if (!step_ir_0x006378(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00637CU: {
    if (!step_ir_0x00637C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00637EU: {
    if (!step_ir_0x00637E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006384(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00638A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x006464U: {
    if (!step_ir_0x006464(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006468(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00646A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00646E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x006472U: {
    if (!step_ir_0x006472(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006476(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x006478U: {
    if (!step_ir_0x006478(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00647C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006482(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006486(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00648A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00648CU: {
    if (!step_ir_0x00648C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00648E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006492(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x006494U: {
    if (!step_ir_0x006494(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x006496(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00649A(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x00649CU: {
    if (!step_ir_0x00649C(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x00649E(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0064A2(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0064A4U: {
    if (!step_ir_0x0064A4(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0064A6(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0064A8U: {
    if (!step_ir_0x0064A8(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0064AA(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0064ACU: {
    if (!step_ir_0x0064AC(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0064B0(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0064B6U: {
    if (!step_ir_0x0064B6(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0064BC(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0064C2U: {
    if (!step_ir_0x0064C2(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0066AAU: {
    if (!step_ir_0x0066AA(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0066AC(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0066B0U: {
    if (!step_ir_0x0066B0(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0066B6(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0066B8U: {
    if (!step_ir_0x0066B8(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0066C0(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  case 0x0066C2U: {
    if (!step_ir_0x0066C2(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    if (!step_ir_0x0066C8(bus, next_pc)) return true; if (next_pc == stop_pc) return true;
    return true; }
  default: return false; }
}
} // namespace oasis::recompiler::ir
