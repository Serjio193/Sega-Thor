#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/gpgx_import_io.hpp"
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
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace {

constexpr std::string_view kCanonicalSha =
    "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263";
constexpr std::string_view kRuntimeBufferSha =
    "9ab80b5bbf33d9067015ad705537997fc3198228d731f457889a1c17b39aa19d";
constexpr std::size_t kBitmapBytes = 3145728U / 16U;
constexpr std::array<std::uint32_t, 6> kAnchors{
    0x3820, 0x62CC, 0x9BF2, 0xA8DA, 0xD3B2, 0x6121A};
constexpr std::array<std::string_view, 6> kClasses{
    "ASM_ROUNDTRIP_EXACT", "CODE_STATIC_SUPPORTED", "CODE_EXECUTED",
    "DATA_REGION_SUPPORTED", "DATA_STRUCTURE_SUPPORTED", "UNKNOWN"};
using oasis::tools::gpgx::Capture;
using oasis::tools::gpgx::Range;
using oasis::tools::gpgx::Store;
using oasis::tools::gpgx::load_ranges;
using oasis::tools::gpgx::load_store;
using oasis::tools::gpgx::allowed_gpgx_build;
using oasis::tools::gpgx::field_number;
using oasis::tools::gpgx::field_string;
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

void merge_capture(Store& store, Capture capture, const std::vector<std::uint32_t>& addresses) {
    if (!capture.build_id.empty() && !store.build_ids.empty() &&
        !store.build_ids.contains(capture.build_id))
        throw std::runtime_error("cannot merge evidence from an incompatible GPGX build");
    const auto build_id = capture.build_id;
    store.addresses.insert(addresses.begin(), addresses.end());
    for (const auto address : addresses) store.facts[address].insert(capture.id);
    if (std::none_of(store.captures.begin(), store.captures.end(),
                     [&](const auto& item) { return item.id == capture.id; }))
        store.captures.push_back(std::move(capture));
    if (!build_id.empty()) store.build_ids.insert(build_id);
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
            << ",\"frames\":" << capture.frames << ",\"new_pcs\":" << capture.new_pcs;
        if (!capture.build_id.empty()) out << ",\"gpgx_build_id\":\"" << capture.build_id << "\"";
        out << "}";
    }
    out << "\n  ],\n  \"provenance_status\": \""
        << (store.legacy_provenance ? "LEGACY_WEAK" : "STRONG")
        << "\",\n  \"executed_addresses\": [";
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
    if (classify(0x100, code, conflict) != "CODE_STATIC_SUPPORTED" ||
        classify(0x10F, code, conflict) != "CODE_STATIC_SUPPORTED" ||
        classify(0x110, code, conflict) != "RUNTIME_EXECUTED_UNKNOWN") return false;
    const auto first = emit_json(store, {}, {}, {}, std::string(kCanonicalSha));
    if (first.find("execution_facts") == std::string::npos ||
        first.find("0x000100") == std::string::npos) return false;
    if (first != emit_json(store, {}, {}, {}, std::string(kCanonicalSha))) return false;
    Store incompatible;
    incompatible.build_ids.insert("old-build");
    bool incompatible_rejected = false;
    try { merge_capture(incompatible, {"new", "session_new", "hash", 0, 0, 0, "new-build"}, {}); }
    catch (const std::exception&) { incompatible_rejected = true; }
    if (!incompatible_rejected || allowed_gpgx_build("unknown-build") ||
        !allowed_gpgx_build("7e2fe295e905e6046b043155b249c7fd289701ab")) return false;
    const auto temporary = std::filesystem::temp_directory_path() / "oasis_gpgx_json_self_test.json";
    const auto equivalent = std::array<std::string, 3>{
        R"({"ranges":[{"start":"0x100","end":"0x110","classification":"CODE_STATIC_SUPPORTED"}]})",
        R"({ "ranges" : [ { "classification":"CODE_STATIC_SUPPORTED", "end":272, "start":256 } ] })",
        "{\n  \"ranges\": [\n    {\"end\":\"0x110\",\"start\":\"0x100\",\"classification\":\"CODE_STATIC_SUPPORTED\"}\n  ]\n}"};
    std::vector<Range> parsed;
    for (const auto& text : equivalent) {
        write_file(temporary, text);
        const auto current = load_ranges(temporary);
        if (current.size() != 1 || current[0].start != 0x100 || current[0].end != 0x110 ||
            current[0].classification != "CODE_STATIC_SUPPORTED") {
            std::filesystem::remove(temporary);
            return false;
        }
        if (parsed.empty()) parsed = current;
        else if (parsed[0].start != current[0].start || parsed[0].end != current[0].end ||
                 parsed[0].classification != current[0].classification) {
            std::filesystem::remove(temporary);
            return false;
        }
    }
    write_file(temporary, R"({"ranges":[{"start":256}]})");
    bool malformed_rejected = false;
    try { (void)load_ranges(temporary); }
    catch (const std::exception&) { malformed_rejected = true; }
    std::filesystem::remove(temporary);
    return malformed_rejected;
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
        const auto schema = field_string(metadata, "schema");
        const auto build_id = field_string(metadata, "gpgx_build_id");
        const auto session_id = field_string(metadata, "capture_id");
        const auto runtime_sha = field_string(metadata, "rom_runtime_buffer_sha256");
        const auto transform = field_string(metadata, "runtime_buffer_transform");
        const bool has_new_provenance = !schema.empty() || !build_id.empty() || !session_id.empty();
        if (has_new_provenance) {
            if (schema != "gpgx.coverage.capture.v2" || session_id.empty() ||
                !allowed_gpgx_build(build_id))
                throw std::runtime_error("coverage metadata has incomplete or incompatible provenance");
            if (runtime_sha != kRuntimeBufferSha ||
                transform != "canonical_rom_word_byteswapped_16")
                throw std::runtime_error("runtime ROM buffer provenance mismatch");
            if (file_sha != kCanonicalSha)
                throw std::runtime_error("canonical ROM file SHA-256 is required for strong provenance");
        } else if (!runtime_sha.empty() && runtime_sha != "unavailable" &&
                   runtime_sha != kRuntimeBufferSha) {
            throw std::runtime_error("legacy runtime ROM buffer provenance mismatch");
        }
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
                              addresses.size(), field_number(metadata, "frames"), field_number(metadata, "new_pcs_session"),
                              build_id}, addresses);
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
