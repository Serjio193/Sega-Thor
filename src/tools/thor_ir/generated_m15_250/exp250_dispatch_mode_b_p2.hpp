#pragma once
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_common.hpp"
#include "tools/thor_ir/generated_m15_250/exp250_dispatch_router.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool run_exp250_block_BB_30_0x00EA70(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x00EA70(bus, next_pc)) return false;
  if (!step_ir_0x00EA74(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_31_0x017908(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x017908(bus, next_pc)) return false;
  if (!step_ir_0x01790C(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_32_0x017910(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x017910(bus, next_pc)) return false;
  if (!step_ir_0x017912(bus, next_pc)) return false;
  if (!step_ir_0x017916(bus, next_pc)) return false;
  if (!step_ir_0x017918(bus, next_pc)) return false;
  if (!step_ir_0x01791C(bus, next_pc)) return false;
  if (!step_ir_0x01791E(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_33_0x0255D8(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0255D8(bus, next_pc)) return false;
  if (!step_ir_0x0255DC(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_34_0x0255DE(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0255DE(bus, next_pc)) return false;
  if (!step_ir_0x0255E4(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_35_0x0255E6(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0255E6(bus, next_pc)) return false;
  if (!step_ir_0x0255EE(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_36_0x0255F0(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0255F0(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_37_0x0255F6(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0255F6(bus, next_pc)) return false;
  if (!step_ir_0x0255FC(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_38_0x0255FE(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0255FE(bus, next_pc)) return false;
  if (!step_ir_0x025602(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_39_0x025604(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x025604(bus, next_pc)) return false;
  if (!step_ir_0x025606(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_40_0x02560C(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02560C(bus, next_pc)) return false;
  if (!step_ir_0x02560E(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_41_0x025612(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x025612(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_42_0x025616(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x025616(bus, next_pc)) return false;
  if (!step_ir_0x02561A(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_43_0x02561C(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x02561C(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_block_BB_44_0x025622(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x025622(bus, next_pc)) return false;
  if (!step_ir_0x025626(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp250_mode_b_chunk_2(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x00EA70U: return run_exp250_block_BB_30_0x00EA70(bus, out_pc);
    case 0x017908U: return run_exp250_block_BB_31_0x017908(bus, out_pc);
    case 0x017910U: return run_exp250_block_BB_32_0x017910(bus, out_pc);
    case 0x0255D8U: return run_exp250_block_BB_33_0x0255D8(bus, out_pc);
    case 0x0255DEU: return run_exp250_block_BB_34_0x0255DE(bus, out_pc);
    case 0x0255E6U: return run_exp250_block_BB_35_0x0255E6(bus, out_pc);
    case 0x0255F0U: return run_exp250_block_BB_36_0x0255F0(bus, out_pc);
    case 0x0255F6U: return run_exp250_block_BB_37_0x0255F6(bus, out_pc);
    case 0x0255FEU: return run_exp250_block_BB_38_0x0255FE(bus, out_pc);
    case 0x025604U: return run_exp250_block_BB_39_0x025604(bus, out_pc);
    case 0x02560CU: return run_exp250_block_BB_40_0x02560C(bus, out_pc);
    case 0x025612U: return run_exp250_block_BB_41_0x025612(bus, out_pc);
    case 0x025616U: return run_exp250_block_BB_42_0x025616(bus, out_pc);
    case 0x02561CU: return run_exp250_block_BB_43_0x02561C(bus, out_pc);
    case 0x025622U: return run_exp250_block_BB_44_0x025622(bus, out_pc);
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
