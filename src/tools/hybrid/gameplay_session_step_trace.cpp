#include "tools/hybrid/gameplay_session_step_trace.hpp"

namespace oasis::hybrid::session {

void write_step_instruction(std::ostream& out, unsigned frame, unsigned pc, unsigned opcode,
                            unsigned previous_pc, const CpuSnapshot& cpu,
                            const SchedulerSnapshot& scheduler, bool native_step) {
    out << "{\"kind\":\"STEP_INSTRUCTION\",\"frame\":" << frame
        << ",\"pc\":" << pc << ",\"opcode\":" << opcode
        << ",\"previous_pc\":" << previous_pc
        << ",\"native_step\":" << (native_step ? "true" : "false")
        << ",\"m68k_cycles\":" << scheduler.m68k_cycles
        << ",\"refresh_cycles\":" << scheduler.refresh_cycles
        << ",\"cpu\":" << serialize_cpu_snapshot(cpu) << "}\n";
}

void observe_step_instruction_trace(std::ostream& out, unsigned frame, unsigned pc, unsigned opcode,
                                    unsigned previous_pc, RegisterRead read_register,
                                    SchedulerRead read_scheduler, bool native_step) {
    const bool in_step_region = (pc >= 0x9D4EU && pc < 0x9DB4U) ||
        (pc >= 0x9DB4U && pc < 0x9DD6U);
    if (in_step_region)
        write_step_instruction(out, frame, pc, opcode, previous_pc, read_cpu_snapshot(read_register),
                               read_scheduler_snapshot(read_scheduler), native_step);
}

} // namespace oasis::hybrid::session
