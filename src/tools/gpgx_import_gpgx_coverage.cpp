#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_assemble.hpp"
#include "tools/re_slice_decoder.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace {

constexpr std::string_view kCanonicalSha =
    "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263";
constexpr std::size_t kBitmapBytes = 3145728U / 16U;
constexpr std::array<std::uint32_t, 6> kAnchors{
    0x3820, 0x62CC, 0x9BF2, 0xA8DA, 0xD3B2, 0x6121A};
constexpr std::array<std::string_view, 6> kClasses{
    "ASM_ROUNDTRIP_EXACT", "CODE_STATIC_SUPPORTED", "CODE_EXECUTED",
    "DATA_REGION_SUPPORTED", "DATA_STRUCTURE_SUPPORTED", "UNKNOWN"};
struct Range {
    std::uint32_t start{};
    std::uint32_t end{};
    std::string classification;
    bool data{};
};
struct Capture {
    std::string id;
    std::string bitmap_kind;
    std::string bitmap_sha256;
    std::size_t address_count{};
    std::uint64_t frames{};
    std::uint64_t new_pcs{};
};
struct Instruction {
    std::uint32_t address{};
    std::string raw;
    std::string decoded;
    std::string status;
    std::string classification;
    std::string data_range;
};
struct Region {
    std::uint32_t start{};
    std::uint32_t end{};
    std::size_t count{};
};
struct RangeCoverage {
    Range range;
    std::size_t observed{};
    std::size_t decoded{};
    std::optional<std::uint32_t> first;
    std::optional<std::uint32_t> last;
};
struct Store {
    std::set<std::uint32_t> addresses;
    std::map<std::uint32_t, std::set<std::string>> facts;
    std::vector<Capture> captures;
};
std::string read_file(const std::filesystem::path& path, std::ios::openmode mode = {}) {
    std::ifstream input(path, mode | std::ios::binary);
    if (!input) throw std::runtime_error("unable to read " + path.string());
    return {std::istreambuf_iterator<char>(input), {}};
}
void write_file(const std::filesystem::path& path, const std::string& text) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to write " + path.string());
    output << text;
}
std::string json_escape(std::string_view value) {
    std::string result;
    for (const char c : value) {
        if (c == '\\') result += "\\\\";
        else if (c == '"') result += "\\\"";
        else if (c == '\n') result += "\\n";
        else result += c;
    }
    return result;
}

std::string hex(std::uint32_t value) {
    std::ostringstream output;
    output << "0x" << std::uppercase << std::hex << std::setw(6)
           << std::setfill('0') << value;
    return output.str();
}

std::string field_string(const std::string& json, std::string_view key) {
    const std::regex pattern("\\\"" + std::string(key) + "\\\"\\s*:\\s*\\\"([^\\\"]*)\\\"");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) return {};
    return match[1].str();
}

std::uint64_t field_number(const std::string& json, std::string_view key) {
    const std::regex pattern("\\\"" + std::string(key) + "\\\"\\s*:\\s*([0-9]+)");
    std::smatch match;
    if (!std::regex_search(json, match, pattern)) return 0;
    return std::stoull(match[1].str());
}

std::uint32_t parse_number(std::string value) {
    std::size_t used = 0;
    const auto number = std::stoull(value, &used, 0);
    if (used != value.size() || number > 0xFFFFFFFFULL)
        throw std::runtime_error("invalid range number: " + value);
    return static_cast<std::uint32_t>(number);
}

std::vector<Range> load_ranges(const std::filesystem::path& path) {
    const auto text = read_file(path);
    const std::regex number("\\\"(start|end)\\\"\\s*:\\s*(?:\\\"(0x[0-9A-Fa-f]+)\\\"|([0-9]+))");
    const std::regex classification("\\\"(?:classification|trust_level)\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
    std::vector<Range> ranges;
    std::optional<std::uint32_t> start;
    std::optional<std::uint32_t> end;
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        std::smatch match;
        if (std::regex_search(line, match, number)) {
            const auto value = match[2].matched ? match[2].str() : match[3].str();
            if (match[1] == "start") start = parse_number(value);
            else end = parse_number(value);
        }
        if (std::regex_search(line, match, classification) && start && end) {
            const auto label = match[1].str();
            ranges.push_back({*start, *end, label, label.rfind("DATA_", 0) == 0});
            start.reset();
            end.reset();
        }
    }
    return ranges;
}

std::vector<std::uint8_t> read_bitmap(const std::filesystem::path& path) {
    const auto bytes = read_file(path);
    if (bytes.size() != kBitmapBytes)
        throw std::runtime_error("bitmap size mismatch; expected 196608 bytes");
    return {bytes.begin(), bytes.end()};
}

std::vector<std::uint32_t> addresses_from_bitmap(const std::vector<std::uint8_t>& bitmap) {
    std::vector<std::uint32_t> result;
    for (std::uint32_t pc = 0; pc < 3145728U; pc += 2U) {
        const auto index = pc >> 1U;
        if ((bitmap[index >> 3U] & (1U << (index & 7U))) != 0U) result.push_back(pc);
    }
    return result;
}

std::string bitmap_key(std::string_view kind) {
    if (kind == "session_new") return "session_new_bitmap_sha256";
    if (kind == "session_all") return "session_all_bitmap_sha256";
    if (kind == "global_known") return "global_known_bitmap_sha256";
    throw std::runtime_error("bitmap kind must be session_new, session_all or global_known");
}

const Range* containing(const std::vector<Range>& ranges, std::uint32_t address, bool data) {
    for (const auto& range : ranges)
        if (range.data == data && address >= range.start && address < range.end) return &range;
    return nullptr;
}

std::string classify(std::uint32_t address, const std::vector<Range>& ranges,
                     std::string& data_range) {
    if (const auto* data = containing(ranges, address, true)) {
        data_range = hex(data->start) + "-" + hex(data->end);
        return "RUNTIME_DATA_CONFLICT";
    }
    if (const auto* code = containing(ranges, address, false)) {
        for (const auto label : kClasses)
            if (code->classification == label) return code->classification;
    }
    return "RUNTIME_EXECUTED_UNKNOWN";
}

std::string raw_bytes(const std::vector<std::uint8_t>& rom, std::uint32_t address, std::size_t count) {
    std::ostringstream output;
    output << std::uppercase << std::hex << std::setfill('0');
    count = std::min(count, rom.size() - address);
    for (std::size_t i = 0; i < count; ++i) output << std::setw(2) << unsigned(rom[address + i]);
    return output.str();
}

Instruction decode(const std::vector<std::uint8_t>& rom, std::uint32_t address,
                   const std::vector<Range>& ranges) {
    Instruction result{.address = address};
    const auto slice = oasis::tools::decode_m68k_slice(
        rom, {.entry = address, .byte_budget = 16, .instruction_budget = 1});
    if (slice.instructions.empty()) {
        result.raw = raw_bytes(rom, address, 2);
        result.status = "DECODE_UNSUPPORTED";
    } else {
        const auto& item = slice.instructions.front();
        result.raw = raw_bytes(item.bytes, 0, item.bytes.size());
        if (item.supported && item.exact) {
            try {
                result.decoded = oasis::tools::exact_instruction_asm(item);
                result.status = "DECODED";
            } catch (const std::exception&) { result.status = "DECODE_UNSUPPORTED"; }
        } else result.status = "DECODE_UNSUPPORTED";
    }
    result.classification = classify(address, ranges, result.data_range);
    return result;
}

std::vector<Region> regions_for(const std::vector<std::uint32_t>& addresses) {
    std::vector<Region> result;
    for (const auto address : addresses) {
        if (result.empty() || address != result.back().end + 2U)
            result.push_back({address, address, 1});
        else { result.back().end = address; ++result.back().count; }
    }
    return result;
}

std::vector<RangeCoverage> range_coverage(const std::vector<Range>& ranges,
                                          const std::set<std::uint32_t>& observed,
                                          const std::vector<std::uint8_t>& rom) {
    std::vector<RangeCoverage> result;
    for (const auto& range : ranges) {
        if (range.classification == "UNKNOWN" || range.start >= range.end) continue;
        RangeCoverage item{.range = range};
        for (std::uint32_t pc = range.start; pc < range.end; ) {
            const auto slice = oasis::tools::decode_m68k_slice(
                rom, {.entry = pc, .byte_budget = 16, .instruction_budget = 1});
            std::size_t length = 2;
            if (!slice.instructions.empty()) {
                const auto& instruction = slice.instructions.front();
                length = std::max<std::size_t>(2, instruction.bytes.size());
                if (instruction.supported && instruction.exact) ++item.decoded;
            }
            if (observed.contains(pc)) {
                ++item.observed;
                if (!item.first) item.first = pc;
                item.last = pc;
            }
            pc += static_cast<std::uint32_t>(length);
        }
        result.push_back(std::move(item));
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        return left.range.start != right.range.start ? left.range.start < right.range.start
                                                       : left.range.end < right.range.end;
    });
    return result;
}

std::string capture_id(std::string_view kind, std::string_view bitmap_sha,
                       std::string_view rom_sha) {
    return std::string(kind) + ":" + std::string(bitmap_sha) + ":" + std::string(rom_sha);
}

Store load_store(const std::filesystem::path& path, std::string_view canonical_sha) {
    Store store;
    if (!std::filesystem::exists(path)) return store;
    const auto text = read_file(path);
    if (field_string(text, "canonical_rom_sha256") != canonical_sha)
        throw std::runtime_error("existing evidence has a different canonical ROM");
    const auto section_start = text.find("\"executed_addresses\"");
    const auto section_end = text.find(']', section_start);
    if (section_start != std::string::npos && section_end != std::string::npos) {
        const std::regex address("0x[0-9A-Fa-f]+");
        for (auto it = std::sregex_iterator(text.begin() + section_start, text.begin() + section_end, address);
             it != std::sregex_iterator(); ++it)
            store.addresses.insert(parse_number(it->str()));
    }
    const std::regex capture("\\{\\\"capture_id\\\":\\\"([^\\\"]+)\\\",\\\"bitmap_kind\\\":\\\"([^\\\"]*)\\\",\\\"bitmap_sha256\\\":\\\"([^\\\"]*)\\\",\\\"address_count\\\":([0-9]+),\\\"frames\\\":([0-9]+),\\\"new_pcs\\\":([0-9]+)\\}");
    const std::regex fact("\\\"address\\\"\\s*:\\s*\\\"(0x[0-9A-Fa-f]+)\\\"[^}]*\\\"capture_id\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
    for (auto it = std::sregex_iterator(text.begin(), text.end(), fact);
         it != std::sregex_iterator(); ++it)
        store.facts[parse_number((*it)[1].str())].insert((*it)[2].str());
    for (auto it = std::sregex_iterator(text.begin(), text.end(), capture);
         it != std::sregex_iterator(); ++it)
        if (std::none_of(store.captures.begin(), store.captures.end(), [&](const auto& item) { return item.id == (*it)[1].str(); }))
            store.captures.push_back({(*it)[1].str(), (*it)[2].str(), (*it)[3].str(),
                                      static_cast<std::size_t>(std::stoull((*it)[4].str())),
                                      std::stoull((*it)[5].str()), std::stoull((*it)[6].str())});
    return store;
}

void merge_capture(Store& store, Capture capture, const std::vector<std::uint32_t>& addresses) {
    store.addresses.insert(addresses.begin(), addresses.end());
    for (const auto address : addresses) store.facts[address].insert(capture.id);
    if (std::none_of(store.captures.begin(), store.captures.end(),
                     [&](const auto& item) { return item.id == capture.id; }))
        store.captures.push_back(std::move(capture));
    std::sort(store.captures.begin(), store.captures.end(),
              [](const auto& left, const auto& right) { return left.id < right.id; });
}

std::string emit_json(const Store& store, const std::vector<Instruction>& instructions,
                      const std::vector<RangeCoverage>& coverage, const std::vector<Region>& unknown,
                      const std::string& canonical_sha) {
    std::map<std::string, std::size_t> counts;
    std::size_t decoded = 0, unsupported = 0, conflicts = 0, unknown_decoded = 0, unknown_unsupported = 0;
    for (const auto& item : instructions) {
        ++counts[item.classification];
        if (item.status == "DECODED") ++decoded; else ++unsupported;
        if (item.classification == "RUNTIME_DATA_CONFLICT") ++conflicts;
        if (item.classification == "RUNTIME_EXECUTED_UNKNOWN")
            (item.status == "DECODED" ? ++unknown_decoded : ++unknown_unsupported);
    }
    std::ostringstream out;
    out << "{\n  \"schema\": \"oasis.gpgx.runtime.execution.evidence.v1\",\n"
        << "  \"canonical_rom_sha256\": \"" << canonical_sha << "\",\n"
        << "  \"source\": \"GPGX_MANUAL_REALTIME\",\n"
        << "  \"evidence_type\": \"CODE_EXECUTED_AT_ADDRESS\",\n"
        << "  \"imported_captures\": [";
    for (std::size_t i = 0; i < store.captures.size(); ++i) {
        const auto& capture = store.captures[i];
        out << (i ? "," : "") << "\n    {\"capture_id\":\"" << json_escape(capture.id)
            << "\",\"bitmap_kind\":\"" << capture.bitmap_kind
            << "\",\"bitmap_sha256\":\"" << capture.bitmap_sha256
            << "\",\"address_count\":" << capture.address_count
            << ",\"frames\":" << capture.frames << ",\"new_pcs\":" << capture.new_pcs << "}";
    }
    out << "\n  ],\n  \"executed_addresses\": [";
    std::size_t index = 0;
    for (const auto address : store.addresses) out << (index++ ? "," : "") << "\"" << hex(address) << "\"";
    out << "],\n  \"execution_facts\": [";
    index = 0;
    for (const auto& [address, capture_ids] : store.facts) for (const auto& id : capture_ids) {
        const auto item = std::find_if(instructions.begin(), instructions.end(),
                                       [&](const auto& value) { return value.address == address; });
        out << (index++ ? "," : "") << "\n    {\"address\":\"" << hex(address)
            << "\",\"source\":\"GPGX_MANUAL_REALTIME\",\"evidence_type\":\"CODE_EXECUTED_AT_ADDRESS\",\"capture_id\":\""
            << json_escape(id) << "\",\"decoder_status\":\""
            << (item == instructions.end() ? "UNKNOWN" : item->status) << "\"}";
    }
    out << "\n  ],\n  \"classification_summary\": {";
    for (std::size_t i = 0; i < kClasses.size(); ++i)
        out << (i ? "," : "") << "\n    \"" << kClasses[i] << "\": " << counts[std::string(kClasses[i])];
    out << "\n  },\n  \"runtime_executed_unknown\": {\"count\": "
        << counts["RUNTIME_EXECUTED_UNKNOWN"] << ", \"decoded\": "
        << unknown_decoded << ", \"unsupported\": " << unknown_unsupported
        << ", \"regions\": " << unknown.size() << "},\n"
        << "  \"runtime_data_conflicts\": " << conflicts << ",\n"
        << "  \"range_level_changes\": [],\n  \"range_execution_coverage\": [";
    for (std::size_t i = 0; i < coverage.size(); ++i) {
        const auto& item = coverage[i];
        const double ratio = item.decoded == 0 ? 0.0 : static_cast<double>(item.observed) / item.decoded;
        out << (i ? "," : "") << "\n    {\"range_start\":\"" << hex(item.range.start)
            << "\",\"range_end\":\"" << hex(item.range.end)
            << "\",\"classification_before\":\"" << item.range.classification
            << "\",\"observed_instruction_starts\":" << item.observed
            << ",\"total_decoded_instruction_starts\":" << item.decoded
            << ",\"execution_coverage_ratio\":" << std::fixed << std::setprecision(6) << ratio;
        if (item.first) out << ",\"first_observed_pc\":\"" << hex(*item.first) << "\",\"last_observed_pc\":\"" << hex(*item.last) << "\"";
        else out << ",\"first_observed_pc\":null,\"last_observed_pc\":null";
        out << "}";
    }
    out << "\n  ],\n  \"address_level_facts_added\": "
        << store.addresses.size() << ",\n  \"decoded_instruction_starts\": " << decoded
        << ",\n  \"decode_unsupported\": " << unsupported << "\n}\n";
    return out.str();
}

std::string emit_report(const Store& store, const std::vector<Instruction>& instructions,
                        const std::vector<Range>& ranges, const std::vector<RangeCoverage>& coverage,
                        const std::vector<Region>& unknown,
                        const std::string& canonical_sha) {
    std::map<std::string, std::size_t> counts;
    std::size_t decoded = 0, unsupported = 0, conflicts = 0;
    std::set<std::uint32_t> observed;
    for (const auto& item : instructions) {
        ++counts[item.classification]; observed.insert(item.address);
        if (item.status == "DECODED") ++decoded; else ++unsupported;
        if (item.classification == "RUNTIME_DATA_CONFLICT") ++conflicts;
    }
    std::ostringstream out;
    out << "# GPGX Runtime Execution Trust\n\nCanonical ROM: PASS (`" << canonical_sha << "`)\n\n"
        << "Coverage source: `GPGX_MANUAL_REALTIME`\n\n"
        << "Global observed PCs: **" << store.addresses.size() << "**\n\n"
        << "## Classification summary\n\n| Classification | Count |\n|---|---:|\n";
    for (const auto label : kClasses) out << "| " << label << " | " << counts[std::string(label)] << " |\n";
    out << "| RUNTIME_EXECUTED_UNKNOWN | " << counts["RUNTIME_EXECUTED_UNKNOWN"] << " |\n"
        << "| RUNTIME_DATA_CONFLICT | " << conflicts << " |\n\n"
        << "Decoded instruction starts: " << decoded << "\n\n"
        << "Decoder unsupported: " << unsupported << "\n\n"
        << "## Anchors\n\n| Address | Observed | Classification |\n|---|---|---|\n";
    for (const auto address : kAnchors) {
        std::string data_range;
        out << "| " << hex(address) << " | " << (observed.contains(address) ? "yes" : "no")
            << " | " << classify(address, ranges, data_range) << " |\n";
    }
    out << "\n## Top runtime-executed unknown regions\n\n| Start | End | PCs |\n|---|---|---:|\n";
    std::vector<Region> top = unknown;
    std::sort(top.begin(), top.end(), [](const auto& left, const auto& right) {
        return left.count != right.count ? left.count > right.count : left.start < right.start;
    });
    for (std::size_t i = 0; i < std::min<std::size_t>(20, top.size()); ++i)
        out << "| " << hex(top[i].start) << " | " << hex(top[i].end) << " | " << top[i].count << " |\n";
    out << "\n## Existing range execution coverage\n\n| Start | End | Before | Observed | Decoded | Ratio | First | Last |\n|---|---|---|---:|---:|---:|---|---|\n";
    for (const auto& item : coverage) {
        const double ratio = item.decoded == 0 ? 0.0 : static_cast<double>(item.observed) / item.decoded;
        out << "| " << hex(item.range.start) << " | " << hex(item.range.end) << " | " << item.range.classification
            << " | " << item.observed << " | " << item.decoded << " | " << std::fixed << std::setprecision(3) << ratio << " | "
            << (item.first ? hex(*item.first) : "-") << " | " << (item.last ? hex(*item.last) : "-") << " |\n";
    }
    out << "\n## Trust changes\n\n- Address-level facts added: " << store.addresses.size()
        << " (`CODE_EXECUTED_AT_ADDRESS`)\n- Range-level changes: none\n"
        << "- Runtime/data conflicts: " << conflicts << "\n\n"
        << "Observed execution is an address-level fact; it does not prove full function execution, branch coverage or semantics.\n";
    return out.str();
}

bool self_test() {
    Store store;
    store.addresses.insert({0x100, 0x102});
    merge_capture(store, {"same", "session_new", "hash", 2, 1, 1}, {0x100, 0x102});
    merge_capture(store, {"same", "session_new", "hash", 2, 1, 1}, {0x100, 0x102});
    if (store.addresses.size() != 2 || store.captures.size() != 1) return false;
    const std::vector<Range> code{{0x100, 0x110, "CODE_STATIC_SUPPORTED", false}};
    const std::vector<Range> data{{0x200, 0x210, "DATA_STRUCTURE_SUPPORTED", true}};
    std::string conflict;
    if (classify(0x205, data, conflict) != "RUNTIME_DATA_CONFLICT") return false;
    if (classify(0x100, code, conflict) != "CODE_STATIC_SUPPORTED") return false;
    if (classify(0x300, {}, conflict) != "RUNTIME_EXECUTED_UNKNOWN") return false;
    if ((0x101U & 1U) == 0U || (0x100U & 1U) != 0U) return false;
    if (field_string("{\"rom_file_sha256\":\"unavailable\"}", "rom_file_sha256") != "unavailable") return false;
    const auto first = emit_json(store, {}, {}, {}, std::string(kCanonicalSha));
    return first == emit_json(store, {}, {}, {}, std::string(kCanonicalSha));
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--self-test")
        return self_test() ? 0 : 1;
    if (argc != 9) {
        std::cerr << "usage: oasis_re_import_gpgx_coverage <coverage_meta.json> <bitmap.bin> "
                     "<canonical_rom> <bitmap_kind> <code_manifest.json> <data.json> "
                     "<evidence.json> <trust_report.md>\n";
        return 2;
    }
    try {
        const auto metadata_path = std::filesystem::path(argv[1]);
        const auto bitmap_path = std::filesystem::path(argv[2]);
        const auto rom = oasis::Rom::load(argv[3]);
        const auto identity = oasis::identify_rom(rom.bytes());
        if (rom.size() != 3145728U || identity.status != oasis::RomSupportStatus::Supported ||
            identity.fingerprint.sha256 != kCanonicalSha)
            throw std::runtime_error("canonical ROM identity check failed");
        const auto metadata = read_file(metadata_path);
        if (field_string(metadata, "capture_mode") != "manual_realtime")
            throw std::runtime_error("coverage source is not manual_realtime");
        if (field_number(metadata, "bitmap_size") != kBitmapBytes)
            throw std::runtime_error("metadata bitmap_size mismatch");
        const auto file_sha = field_string(metadata, "rom_file_sha256");
        if (!file_sha.empty() && file_sha != "unavailable" && file_sha != kCanonicalSha)
            throw std::runtime_error("coverage ROM provenance mismatch");
        const auto bitmap = read_bitmap(bitmap_path);
        const auto actual_bitmap_sha = oasis::calculate_sha256(bitmap);
        const auto expected_key = bitmap_key(argv[4]);
        if (actual_bitmap_sha != field_string(metadata, expected_key))
            throw std::runtime_error("coverage bitmap hash mismatch");
        const auto addresses = addresses_from_bitmap(bitmap);
        for (const auto address : addresses)
            if ((address & 1U) != 0U || address >= rom.size()) throw std::runtime_error("invalid observed PC");
        auto ranges = load_ranges(argv[5]);
        const auto data_ranges = load_ranges(argv[6]);
        ranges.insert(ranges.end(), data_ranges.begin(), data_ranges.end());
        const auto bitmap_sha = actual_bitmap_sha;
        auto evidence_path = std::filesystem::path(argv[7]);
        auto report_path = std::filesystem::path(argv[8]);
        Store store = load_store(evidence_path, kCanonicalSha);
        merge_capture(store, {capture_id(argv[4], bitmap_sha, std::string(kCanonicalSha)), argv[4], bitmap_sha,
                              addresses.size(), field_number(metadata, "frames"), field_number(metadata, "new_pcs_session")}, addresses);
        std::vector<std::uint32_t> union_addresses(store.addresses.begin(), store.addresses.end());
        std::vector<Instruction> instructions;
        instructions.reserve(union_addresses.size());
        for (const auto address : union_addresses) instructions.push_back(decode(rom.bytes(), address, ranges));
        std::vector<Region> unknown;
        std::vector<std::uint32_t> unknown_addresses;
        for (const auto& item : instructions) if (item.classification == "RUNTIME_EXECUTED_UNKNOWN") unknown_addresses.push_back(item.address);
        unknown = regions_for(unknown_addresses);
        const auto coverage = range_coverage(ranges, store.addresses, rom.bytes());
        write_file(evidence_path, emit_json(store, instructions, coverage, unknown, std::string(kCanonicalSha)));
        write_file(report_path, emit_report(store, instructions, ranges, coverage, unknown, std::string(kCanonicalSha)));
        std::cout << "imported " << addresses.size() << " observed PCs; global evidence now "
                  << store.addresses.size() << " PCs\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
