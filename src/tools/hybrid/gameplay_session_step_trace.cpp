#include "tools/hybrid/gameplay_session_step_trace.hpp"

#include <cstdint>

namespace oasis::hybrid::session {
namespace {
std::uint64_t e268_native_entries{};
bool e268_sample_active{};
}

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
    if (pc == 0xE268U)
        out << "{\"kind\":\"E268_CALL\",\"frame\":" << frame
            << ",\"source_pc\":" << previous_pc << ",\"native\":"
            << (native_step ? "true" : "false") << "}\n";
    if (pc == 0xE268U && native_step) {
        ++e268_native_entries;
        e268_sample_active = e268_native_entries <= 8U || e268_native_entries % 5000U == 0U;
        out << "{\"kind\":\"NATIVE_HELPER_ENTRY\",\"helper_id\":\"0x00E268\",\"frame\":"
            << frame << ",\"caller_pc\":" << previous_pc << ",\"sr\":"
            << read_register(17U) << ",\"d0\":" << read_register(0U)
            << ",\"a7\":" << read_register(15U) << "}\n";
    }
    if (e268_sample_active && pc >= 0xE268U && pc <= 0xE2A0U) {
        write_step_instruction(out, frame, pc, opcode, previous_pc,
            read_cpu_snapshot(read_register), read_scheduler_snapshot(read_scheduler), native_step);
        if (pc == 0xE2A0U && !native_step) e268_sample_active = false;
    }
    const bool in_step_region = (pc >= 0x9D4EU && pc < 0x9DB4U) ||
        (pc >= 0x9DB4U && pc < 0x9DD6U) || (pc >= 0x94A2U && pc <= 0x94D0U) ||
        (pc >= 0x9C40U && pc <= 0x9C7AU) || (pc >= 0x8EF8U && pc <= 0x8F10U);
    if (in_step_region)
        write_step_instruction(out, frame, pc, opcode, previous_pc, read_cpu_snapshot(read_register),
                               read_scheduler_snapshot(read_scheduler), native_step);
}

bool is_native_step_helper_pc(unsigned pc) noexcept {
    return pc == 0x9D70U || pc == 0x9D4EU || pc == 0x9DB4U || pc == 0x9D92U ||
        pc == 0x94A2U || pc == 0x9C40U || pc == 0x8EF8U || pc == 0xE268U;
}

} // namespace oasis::hybrid::session
