#pragma once

#include "tools/hybrid/basic_block.hpp"

#include <span>

namespace oasis::hybrid::generated {

// Stable declarations retained for the existing provenance tests; new callers
// should use blocks() so the generated registry remains data-driven.
BlockExit execute_0x002D66(BasicBlockApi&, unsigned);
BlockExit execute_0x0604BC(BasicBlockApi&, unsigned);
BlockExit execute_0x061032(BasicBlockApi&, unsigned);
BlockExit execute_0x03A85E(BasicBlockApi&, unsigned);
BlockExit execute_0x03A8BA(BasicBlockApi&, unsigned);
BlockExit execute_0x03A88C(BasicBlockApi&, unsigned);
BlockExit execute_0x0003A0(BasicBlockApi&, unsigned);
BlockExit execute_0x03A8AC(BasicBlockApi&, unsigned);
BlockExit execute_0x060312(BasicBlockApi&, unsigned);
BlockExit execute_0x002230(BasicBlockApi&, unsigned);
BlockExit execute_0x061360(BasicBlockApi&, unsigned);
BlockExit execute_0x003A0E(BasicBlockApi&, unsigned);
BlockExit execute_0x0038A0(BasicBlockApi&, unsigned);
BlockExit execute_0x0003F2(BasicBlockApi&, unsigned);
BlockExit execute_0x003818(BasicBlockApi&, unsigned);
BlockExit execute_0x0030BE(BasicBlockApi&, unsigned);
BlockExit execute_0x03A758(BasicBlockApi&, unsigned);
BlockExit execute_0x00D994(BasicBlockApi&, unsigned);
BlockExit execute_0x003186(BasicBlockApi&, unsigned);
BlockExit execute_0x003250(BasicBlockApi&, unsigned);
BlockExit execute_0x061938(BasicBlockApi&, unsigned);
BlockExit execute_0x002C18(BasicBlockApi&, unsigned);
BlockExit execute_0x0032EE(BasicBlockApi&, unsigned);
BlockExit execute_0x03A9AC(BasicBlockApi&, unsigned);
BlockExit execute_0x03A9B4(BasicBlockApi&, unsigned);
BlockExit execute_0x03A9CA(BasicBlockApi&, unsigned);
BlockExit execute_0x000380(BasicBlockApi&, unsigned);
BlockExit execute_0x03A864(BasicBlockApi&, unsigned);

unsigned instruction_count_from_0x002D66(unsigned);
unsigned instruction_count_from_0x0604BC(unsigned);
unsigned instruction_count_from_0x061032(unsigned);
unsigned instruction_count_from_0x03A85E(unsigned);
unsigned instruction_count_from_0x03A8BA(unsigned);
unsigned instruction_count_from_0x03A88C(unsigned);
unsigned instruction_count_from_0x0003A0(unsigned);
unsigned instruction_count_from_0x03A8AC(unsigned);
unsigned instruction_count_from_0x060312(unsigned);
unsigned instruction_count_from_0x002230(unsigned);
unsigned instruction_count_from_0x061360(unsigned);
unsigned instruction_count_from_0x003A0E(unsigned);
unsigned instruction_count_from_0x0038A0(unsigned);
unsigned instruction_count_from_0x0003F2(unsigned);
unsigned instruction_count_from_0x003818(unsigned);
unsigned instruction_count_from_0x0030BE(unsigned);
unsigned instruction_count_from_0x03A758(unsigned);
unsigned instruction_count_from_0x00D994(unsigned);
unsigned instruction_count_from_0x003186(unsigned);
unsigned instruction_count_from_0x003250(unsigned);
unsigned instruction_count_from_0x061938(unsigned);
unsigned instruction_count_from_0x002C18(unsigned);
unsigned instruction_count_from_0x0032EE(unsigned);
unsigned instruction_count_from_0x03A9AC(unsigned);
unsigned instruction_count_from_0x03A9B4(unsigned);
unsigned instruction_count_from_0x03A9CA(unsigned);
unsigned instruction_count_from_0x000380(unsigned);
unsigned instruction_count_from_0x03A864(unsigned);

[[nodiscard]] std::span<const GeneratedBlockSpec> blocks();

} // namespace oasis::hybrid::generated
