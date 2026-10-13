#pragma once

#include "tools/hybrid/gameplay_session_cpu.hpp"
#include "tools/hybrid/replay_equivalence_trace.hpp"

#include <ostream>

namespace oasis::hybrid::session {

void write_step_instruction(std::ostream& out, unsigned frame, unsigned pc, unsigned opcode,
                            unsigned previous_pc, const CpuSnapshot& cpu,
                            const SchedulerSnapshot& scheduler, bool native_step);
void observe_step_instruction_trace(std::ostream& out, unsigned frame, unsigned pc, unsigned opcode,
                                    unsigned previous_pc, RegisterRead read_register,
                                    SchedulerRead read_scheduler, bool native_step);

} // namespace oasis::hybrid::session
