#include "tools/re_ant.hpp"

#include <iomanip>
#include <sstream>

namespace oasis::tools {
namespace {

std::string quote(std::string_view value) {
    std::string result = "\"";
    for (const auto character : value) {
        if (character == '\\' || character == '"') result += '\\';
        if (character == '\n') result += 'n'; else if (character == '\r') result += 'r';
        else if (character == '\t') result += 't'; else result += character;
    }
    return result + '"';
}

std::string hex32(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << value;
    return out.str();
}

void bytes(std::ostringstream& out, const std::vector<std::uint8_t>& value) {
    out << '[';
    for (std::size_t index = 0; index < value.size(); ++index) { if (index) out << ','; out << static_cast<unsigned>(value[index]); }
    out << ']';
}

void optional_hex(std::ostringstream& out, const std::optional<std::uint32_t>& value) {
    if (value) out << quote(hex32(*value)); else out << "null";
}

void registers(std::ostringstream& out, const EmulatorRegisterSnapshot& value) {
    out << "{\"d\":[";
    for (std::size_t index = 0; index < value.d.size(); ++index) { if (index) out << ','; optional_hex(out, value.d[index]); }
    out << "],\"a\":[";
    for (std::size_t index = 0; index < value.a.size(); ++index) { if (index) out << ','; optional_hex(out, value.a[index]); }
    out << "],\"sr\":";
    if (value.sr) out << quote(hex32(*value.sr)); else out << "null";
    out << '}';
}

} // namespace

std::string ant_job_to_json(const AntJob& value) {
    std::ostringstream out;
    out << "{\"schema\":" << quote(value.schema) << ",\"job_id\":" << quote(value.job_id)
        << ",\"frontier_id\":" << quote(value.frontier_id) << ",\"rom_sha256\":" << quote(value.rom_sha256)
        << ",\"rom_size\":" << value.rom_size << ",\"backend\":" << quote(value.backend)
        << ",\"backend_version\":" << quote(value.backend_version) << ",\"scenario_id\":" << quote(value.scenario_id)
        << ",\"start_state\":" << quote(value.start_state) << ",\"input_events\":" << quote(value.input_events)
        << ",\"checkpoint_reference\":" << quote(value.checkpoint_reference)
        << ",\"source_entry\":" << quote(hex32(value.source_entry)) << ",\"source_pc\":" << quote(hex32(value.source_pc))
        << ",\"blocker_type\":" << quote(value.blocker_type) << ",\"instruction_bytes\":";
    bytes(out, value.instruction_bytes);
    out << ",\"instruction\":" << quote(value.instruction) << ",\"known_static_context\":" << quote(value.known_static_context)
        << ",\"expected_observation_type\":" << quote(value.expected_observation_type) << ",\"max_steps\":" << value.max_steps
        << ",\"max_frames\":" << value.max_frames << ",\"allowed_input_policy\":" << quote(value.allowed_input_policy)
        << ",\"provenance_requirement\":" << quote(value.provenance_requirement) << '}';
    return out.str();
}

std::string ant_result_to_json(const AntResult& value) {
    std::ostringstream out;
    out << "{\"schema\":" << quote(value.schema) << ",\"job_id\":" << quote(value.job_id)
        << ",\"frontier_id\":" << quote(value.frontier_id) << ",\"status\":" << quote(ant_status_name(value.status))
        << ",\"backend\":" << quote(value.backend) << ",\"backend_version\":" << quote(value.backend_version)
        << ",\"rom_sha256\":" << quote(value.rom_sha256) << ",\"reachability_class\":" << quote(ant_reachability_name(value.reachability_class))
        << ",\"source_entry\":" << quote(hex32(value.source_entry)) << ",\"source_pc\":" << quote(hex32(value.source_pc));
    if (value.observed) {
        const auto& observed = *value.observed;
        out << ",\"observed_actual_pc\":" << quote(hex32(observed.actual_pc)) << ",\"observed_instruction\":" << quote(observed.instruction)
            << ",\"instruction_bytes\":"; bytes(out, observed.instruction_bytes);
        out << ",\"observed_next_pc\":"; optional_hex(out, observed.next_pc);
        out << ",\"observed_indirect_target\":"; optional_hex(out, observed.indirect_target);
        out << ",\"observed_frame\":" << observed.frame << ",\"observed_sequence\":" << observed.sequence
            << ",\"target_inside_rom\":" << (observed.target_inside_rom ? "true" : "false") << ",\"observed_registers\":";
        registers(out, observed.registers);
    }
    out << ",\"stop_reason\":" << quote(value.stop_reason) << ",\"reproducible\":" << (value.reproducible ? "true" : "false")
        << ",\"result_hash\":" << quote(value.result_hash) << ",\"startup_load_ms\":" << value.startup_load_ms
        << ",\"checkpoint_restore_ms\":" << value.checkpoint_restore_ms << ",\"execution_ms\":" << value.execution_ms
        << ",\"total_wall_clock_ms\":" << value.total_wall_clock_ms << ",\"frames_executed\":" << value.frames_executed
        << ",\"instructions_until_observation\":" << value.instructions_until_observation << '}';
    return out.str();
}

std::string ant_result_to_text(const AntResult& value) {
    std::ostringstream out;
    out << value.schema << '\n' << "job_id=" << value.job_id << " frontier_id=" << value.frontier_id
        << " status=" << ant_status_name(value.status) << " backend=" << value.backend << " version=" << value.backend_version << '\n'
        << "source_entry=" << hex32(value.source_entry) << " source_pc=" << hex32(value.source_pc)
        << " reachability=" << ant_reachability_name(value.reachability_class) << " reproducible=" << (value.reproducible ? "yes" : "no") << '\n';
    if (value.observed) out << "observed_pc=" << hex32(value.observed->actual_pc) << " next_pc="
        << (value.observed->next_pc ? hex32(*value.observed->next_pc) : "UNKNOWN") << " target="
        << (value.observed->indirect_target ? hex32(*value.observed->indirect_target) : "UNKNOWN") << " frame="
        << value.observed->frame << " sequence=" << value.observed->sequence << " target_inside_rom="
        << (value.observed->target_inside_rom ? "yes" : "no") << '\n';
    out << "stop_reason=" << value.stop_reason << " result_hash=" << value.result_hash << '\n'
        << "timing startup_load_ms=" << value.startup_load_ms << " checkpoint_restore_ms=" << value.checkpoint_restore_ms
        << " execution_ms=" << value.execution_ms << " total_wall_clock_ms=" << value.total_wall_clock_ms
        << " frames=" << value.frames_executed << " instructions=" << value.instructions_until_observation << '\n';
    return out.str();
}

} // namespace oasis::tools
