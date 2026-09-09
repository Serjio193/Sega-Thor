#include "tools/hybrid/caller_attribution.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace oasis::hybrid {
namespace {
constexpr unsigned kRamFlag = 0x0604BC;
constexpr unsigned kCaller604F6 = 0x0604F6;
constexpr unsigned kCaller60BCC = 0x060BCC;

std::string hex32(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << value;
    return out.str();
}

unsigned byte(const CallerAttributionApi& api, unsigned address) {
    const auto value = api.peek(address & 0xFFFFFFU);
    if (value < 0 || value > 0xFF) throw std::runtime_error("caller attribution peek failed");
    return static_cast<unsigned>(value);
}

unsigned word(const CallerAttributionApi& api, unsigned address) {
    return (byte(api, address) << 8U) | byte(api, address + 1U);
}

std::uint32_t stack_long(const CallerAttributionApi& api, unsigned address) {
    return (byte(api, address) << 24U) | (byte(api, address + 1U) << 16U) |
           (byte(api, address + 2U) << 8U) | byte(api, address + 3U);
}

bool direct_bsr_w(const CallerAttributionApi& api, unsigned site, unsigned target,
                  std::uint32_t return_pc) {
    if ((word(api, site) & 0xFF00U) != 0x6100U || return_pc != site + 4U) return false;
    const auto displacement = static_cast<std::int16_t>(word(api, site + 2U));
    return static_cast<std::uint32_t>(static_cast<std::int32_t>(site + 2U) + displacement) == target;
}

void json_string(std::ostringstream& out, const std::string& value) {
    out << '"';
    for (const auto character : value) {
        if (character == '\\' || character == '"') out << '\\';
        if (character == '\n') out << 'n';
        else if (character == '\r') out << 'r';
        else if (character == '\t') out << 't';
        else out << character;
    }
    out << '"';
}

void registers_json(std::ostringstream& out, const std::array<unsigned, 18>& registers) {
    out << '[';
    for (unsigned index = 0; index < registers.size(); ++index) {
        if (index) out << ',';
        json_string(out, hex32(registers[index]));
    }
    out << ']';
}

} // namespace

const char* caller_classification_name(CallerClassification value) noexcept {
    switch (value) {
    case CallerClassification::Caller604F6: return "0x604F6";
    case CallerClassification::Caller60BCC: return "0x60BCC";
    case CallerClassification::OtherProvenCaller: return "OTHER_PROVEN_CALLER";
    case CallerClassification::UnknownCaller: return "UNKNOWN_CALLER";
    }
    return "UNKNOWN_CALLER";
}

CallerAttributionObserver::CallerAttributionObserver(std::filesystem::path output,
                                                     CallerAttributionApi api)
    : output_(std::move(output)), api_(api) {
    if (!api_.reg || !api_.peek || !api_.cycles || !api_.refresh_cycles)
        throw std::invalid_argument("caller attribution requires complete API");
}

void CallerAttributionObserver::entry(std::uint32_t target,
                                      std::uint32_t previous_execute_pc, unsigned frame) {
    if (finished_) throw std::logic_error("caller attribution already finished");
    if (target != kRamFlag) return;
    CallerAttributionRecord record;
    record.invocation_ordinal = static_cast<unsigned>(records_.size() + 1U);
    record.call_site_pc = previous_execute_pc & 0xFFFFFFU;
    record.stack_a7 = api_.reg(15) & 0xFFFFFFU;
    record.return_pc = stack_long(api_, record.stack_a7);
    record.entry_frame = frame;
    record.entry_cycles = api_.cycles();
    record.entry_refresh_cycles = api_.refresh_cycles();
    for (unsigned index = 0; index < record.registers.size(); ++index)
        record.registers[index] = api_.reg(index);

    if (record.call_site_pc == kCaller604F6 &&
        direct_bsr_w(api_, record.call_site_pc, target, record.return_pc)) {
        record.classification = CallerClassification::Caller604F6;
        // 0x0604EC..0x0604EF are zero data; 0x0604F0 is the bounded code entry.
        record.caller_pc = 0x0604F0;
        record.entry_source = "NATURAL_HOOK_PREVIOUS_EXECUTE_AND_STACK_RETURN";
    } else if (record.call_site_pc == kCaller60BCC &&
               direct_bsr_w(api_, record.call_site_pc, target, record.return_pc)) {
        record.classification = CallerClassification::Caller60BCC;
        record.caller_pc = 0x060BC4;
        record.entry_source = "NATURAL_HOOK_PREVIOUS_EXECUTE_AND_STACK_RETURN";
    } else if (direct_bsr_w(api_, record.call_site_pc, target, record.return_pc)) {
        record.classification = CallerClassification::OtherProvenCaller;
        record.entry_source = "NATURAL_HOOK_PREVIOUS_EXECUTE_AND_STACK_RETURN";
    } else {
        record.entry_source = "NATURAL_TARGET_ENTRY_WITHOUT_PROVEN_DIRECT_BSR_PAIR";
        ++unknown_count_;
    }
    records_.push_back(std::move(record));
}

void CallerAttributionObserver::finish() {
    if (finished_ || output_.empty()) return;
    finished_ = true;
    std::ofstream output(output_);
    output.exceptions(std::ios::failbit | std::ios::badbit);
    output << caller_attribution_to_json(records_, unknown_count_);
}

std::string caller_attribution_to_json(const std::vector<CallerAttributionRecord>& records,
                                       unsigned unknown_count) {
    std::ostringstream out;
    out << "{\"schema\":\"oasis.hybrid.caller-attribution.v1\",\"target\":\"0x0604BC\",\"unknown_count\":"
        << unknown_count << ",\"invocations\":[";
    for (std::size_t index = 0; index < records.size(); ++index) {
        if (index) out << ',';
        const auto& record = records[index];
        out << "{\"invocation_ordinal\":" << record.invocation_ordinal
            << ",\"classification\":";
        json_string(out, caller_classification_name(record.classification));
        out << ",\"caller_pc\":";
        json_string(out, record.caller_pc ? hex32(record.caller_pc) : "UNKNOWN");
        out << ",\"call_site_pc\":";
        json_string(out, hex32(record.call_site_pc));
        out << ",\"return_pc\":";
        json_string(out, hex32(record.return_pc));
        out << ",\"stack_a7\":";
        json_string(out, hex32(record.stack_a7));
        out << ",\"entry_frame\":" << record.entry_frame
            << ",\"entry_cycles\":" << record.entry_cycles
            << ",\"entry_refresh_cycles\":" << record.entry_refresh_cycles
            << ",\"registers\":";
        registers_json(out, record.registers);
        out << ",\"entry_source\":";
        json_string(out, record.entry_source);
        out << '}';
    }
    out << "]}\n";
    return out.str();
}

} // namespace oasis::hybrid
