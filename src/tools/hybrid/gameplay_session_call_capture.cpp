#include "tools/hybrid/gameplay_session_call_capture.hpp"

#include "tools/hybrid/grid_query_native_model.hpp"

#include <sstream>
#include <stdexcept>

namespace oasis::hybrid::session {
namespace {
constexpr unsigned kSourcePc = 0xE376U;
constexpr unsigned kTargetPc = 0x9C40U;
}

std::string serialize_call_capture(const CallCapture& call, Peek peek_memory,
                                   const std::vector<std::uint8_t>& before_ram) {
    const auto after_ram = read_work_ram(peek_memory);
    if (before_ram.size() != after_ram.size())
        throw std::runtime_error("call capture RAM snapshot size changed");
    std::ostringstream out;
    out << "{\"source_pc\":" << kSourcePc << ",\"source_opcode\":" << call.source_opcode
        << ",\"source_state\":" << serialize_cpu_snapshot(call.source)
        << ",\"target_pc\":" << kTargetPc << ",\"target_opcode\":" << call.entry_opcode
        << ",\"entry\":" << serialize_cpu_snapshot(call.entry)
        << ",\"stack_return\":" << call.stack_return
        << ",\"rts_pre\":" << serialize_cpu_snapshot(call.rts)
        << ",\"post_return\":" << serialize_cpu_snapshot(call.returned)
        << ",\"continuation\":" << serialize_cpu_snapshot(call.continuation) << ',';
    grid_query::write_json_fields(out, call.native_grid, call.native_d5_match);
    out << ",\"pc_path\":[";
    for (std::size_t i = 0; i < call.pcs.size(); ++i)
        out << (i ? "," : "") << call.pcs[i];
    out << "],\"events\":[";
    for (std::size_t i = 0; i < call.events.size(); ++i) {
        const auto& event = call.events[i];
        out << (i ? "," : "") << "{\"type\":" << event.type << ",\"pc\":" << event.pc
            << ",\"address\":" << event.address << ",\"width\":" << event.width
            << ",\"value\":" << event.value << '}';
    }
    out << "],\"ram_diff\":[";
    bool first = true;
    for (std::size_t i = 0; i < after_ram.size(); ++i) {
        if (after_ram[i] == before_ram[i]) continue;
        out << (first ? "" : ",") << '[' << (0xFF0000U + i) << ','
            << static_cast<unsigned>(before_ram[i]) << ','
            << static_cast<unsigned>(after_ram[i]) << ']';
        first = false;
    }
    out << "]}";
    return out.str();
}

} // namespace oasis::hybrid::session
