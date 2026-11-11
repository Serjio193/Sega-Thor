#pragma once
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_common.hpp"
#include "tools/thor_ir/generated_m15_256/exp256_dispatch_router.hpp"
namespace oasis::recompiler::ir {
template <typename Bus> inline bool run_exp256_block_BB_15_0x010F7C(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x010F7C(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_16_0x010F7E(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x010F7E(bus, next_pc)) return false;
  if (!step_ir_0x010F82(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_17_0x0112CC(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0112CC(bus, next_pc)) return false;
  if (!step_ir_0x0112D0(bus, next_pc)) return false;
  if (!step_ir_0x0112D6(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_18_0x0112D8(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0112D8(bus, next_pc)) return false;
  if (!step_ir_0x0112DC(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_19_0x0112DE(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0112DE(bus, next_pc)) return false;
  if (!step_ir_0x0112E2(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_20_0x0112E6(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0112E6(bus, next_pc)) return false;
  if (!step_ir_0x0112E8(bus, next_pc)) return false;
  if (!step_ir_0x0112EC(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_21_0x0141CA(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0141CA(bus, next_pc)) return false;
  if (!step_ir_0x0141CE(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_22_0x0141D0(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0141D0(bus, next_pc)) return false;
  if (!step_ir_0x0141D4(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_23_0x0141D8(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0141D8(bus, next_pc)) return false;
  if (!step_ir_0x0141DC(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_24_0x0141E0(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0141E0(bus, next_pc)) return false;
  if (!step_ir_0x0141E4(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_25_0x0141E8(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0141E8(bus, next_pc)) return false;
  if (!step_ir_0x0141EC(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_26_0x0141F0(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0141F0(bus, next_pc)) return false;
  if (!step_ir_0x0141F2(bus, next_pc)) return false;
  if (!step_ir_0x0141F4(bus, next_pc)) return false;
  if (!step_ir_0x0141F6(bus, next_pc)) return false;
  if (!step_ir_0x0141F8(bus, next_pc)) return false;
  if (!step_ir_0x0141FA(bus, next_pc)) return false;
  if (!step_ir_0x0141FC(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_27_0x0141FE(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0141FE(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_28_0x0302F4(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0302F4(bus, next_pc)) return false;
  if (!step_ir_0x0302FA(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_block_BB_29_0x0302FC(Bus& bus, std::uint32_t& out_pc) noexcept {
  std::uint32_t next_pc = 0;
  if (!step_ir_0x0302FC(bus, next_pc)) return false;
  if (!step_ir_0x030300(bus, next_pc)) return false;
  out_pc = next_pc; return true;
}
template <typename Bus> inline bool run_exp256_mode_b_chunk_1(Bus& bus, std::uint32_t pc, std::uint32_t& out_pc) noexcept {
  switch (pc) {
    case 0x010F7CU: return run_exp256_block_BB_15_0x010F7C(bus, out_pc);
    case 0x010F7EU: return run_exp256_block_BB_16_0x010F7E(bus, out_pc);
    case 0x0112CCU: return run_exp256_block_BB_17_0x0112CC(bus, out_pc);
    case 0x0112D8U: return run_exp256_block_BB_18_0x0112D8(bus, out_pc);
    case 0x0112DEU: return run_exp256_block_BB_19_0x0112DE(bus, out_pc);
    case 0x0112E6U: return run_exp256_block_BB_20_0x0112E6(bus, out_pc);
    case 0x0141CAU: return run_exp256_block_BB_21_0x0141CA(bus, out_pc);
    case 0x0141D0U: return run_exp256_block_BB_22_0x0141D0(bus, out_pc);
    case 0x0141D8U: return run_exp256_block_BB_23_0x0141D8(bus, out_pc);
    case 0x0141E0U: return run_exp256_block_BB_24_0x0141E0(bus, out_pc);
    case 0x0141E8U: return run_exp256_block_BB_25_0x0141E8(bus, out_pc);
    case 0x0141F0U: return run_exp256_block_BB_26_0x0141F0(bus, out_pc);
    case 0x0141FEU: return run_exp256_block_BB_27_0x0141FE(bus, out_pc);
    case 0x0302F4U: return run_exp256_block_BB_28_0x0302F4(bus, out_pc);
    case 0x0302FCU: return run_exp256_block_BB_29_0x0302FC(bus, out_pc);
    default: return false;
  }
}
} // namespace oasis::recompiler::ir
