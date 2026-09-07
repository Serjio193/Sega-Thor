#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_assemble.hpp"
#include "tools/re_slice_decoder.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct Range {
    std::uint32_t start{};
    std::uint32_t end{};
    std::string classification;
    bool data{};
};

struct InstructionReport {
    std::uint32_t address{};
    std::string raw_bytes;
    std::string decoded;
    std::string status;
    std::string classification;
    std::size_t length{};
};

std::string hex_u32(std::uint32_t value) {
    std::ostringstream output;
    output << "0x" << std::uppercase << std::hex << std::setw(6)
           << std::setfill('0') << value;
    return output.str();
}

std::string json_escape(std::string_view value) {
    std::string result;
    for (const char character : value) {
        if (character == '\\') result += "\\\\";
        else if (character == '"') result += "\\\"";
        else if (character == '\n') result += "\\n";
        else result += character;
    }
    return result;
}

std::string bytes_hex(const std::vector<std::uint8_t>& bytes) {
    std::ostringstream output;
    output << std::uppercase << std::hex << std::setfill('0');
    for (const auto byte : bytes) output << std::setw(2) << unsigned(byte);
    return output.str();
}

std::string bytes_hex(const std::vector<std::uint8_t>& rom,
                      std::uint32_t address, std::size_t count) {
    const auto begin = rom.begin() + address;
    const auto end = begin + std::min(count, rom.size() - address);
    return bytes_hex(std::vector<std::uint8_t>(begin, end));
}

std::uint32_t parse_number(std::string value) {
    std::size_t consumed = 0;
    const auto result = std::stoull(value, &consumed, 0);
    if (consumed != value.size() || result > 0xFFFFFFFFULL) {
        throw std::runtime_error("invalid range number: " + value);
    }
    return static_cast<std::uint32_t>(result);
}

std::vector<Range> load_ranges(const char* path) {
    if (path == nullptr) return {};
    std::ifstream input(path);
    if (!input) throw std::runtime_error("unable to open classification file: " + std::string(path));
    const std::regex number("\\\"(start|end)\\\"\\s*:\\s*(?:\\\"(0x[0-9A-Fa-f]+)\\\"|([0-9]+))");
    const std::regex classification("\\\"(?:classification|trust_level)\\\"\\s*:\\s*\\\"([^\\\"]+)\\\"");
    std::vector<Range> result;
    std::optional<std::uint32_t> start;
    std::optional<std::uint32_t> end;
    std::string line;
    while (std::getline(input, line)) {
        std::smatch match;
        if (std::regex_search(line, match, number)) {
            const auto value = match[2].matched ? match[2].str() : match[3].str();
            if (match[1] == "start") start = parse_number(value);
            else end = parse_number(value);
        }
        if (std::regex_search(line, match, classification) && start && end) {
            const auto label = match[1].str();
            result.push_back({*start, *end, label,
                              label.rfind("DATA_", 0) == 0});
            start.reset();
            end.reset();
        }
    }
    return result;
}

std::string classify(std::uint32_t address, const std::vector<Range>& ranges) {
    const Range* data = nullptr;
    const Range* code = nullptr;
    for (const auto& range : ranges) {
        if (address < range.start || address >= range.end) continue;
        if (range.data) data = &range;
        else if (range.classification != "UNKNOWN") code = &range;
    }
    if (data && code) return "TRUSTED_DATA_CONFLICT";
    if (data) return data->classification;
    if (code) return code->classification;
    return "UNKNOWN";
}

std::vector<std::uint32_t> read_bitmap(const char* path,
                                       std::size_t expected_size,
                                       std::size_t rom_size) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("unable to open session bitmap");
    std::vector<std::uint8_t> bitmap((std::istreambuf_iterator<char>(input)), {});
    if (bitmap.size() != expected_size) {
        throw std::runtime_error("session bitmap has wrong size; expected " +
                                 std::to_string(expected_size));
    }
    std::vector<std::uint32_t> addresses;
    for (std::uint32_t pc = 0; pc < rom_size; pc += 2U) {
        const auto index = pc >> 1U;
        if ((bitmap[index >> 3U] & (1U << (index & 7U))) != 0U)
            addresses.push_back(pc);
    }
    return addresses;
}

InstructionReport decode_one(const std::vector<std::uint8_t>& rom,
                             std::uint32_t address,
                             const std::vector<Range>& ranges) {
    InstructionReport report{};
    report.address = address;
    const auto slice = oasis::tools::decode_m68k_slice(
        rom, {.entry = address, .byte_budget = 16, .instruction_budget = 1});
    if (slice.instructions.empty()) {
        report.status = "DECODE_UNSUPPORTED";
        report.raw_bytes = bytes_hex(rom, address, 2);
    } else {
        const auto& instruction = slice.instructions.front();
        report.raw_bytes = bytes_hex(instruction.bytes);
        report.length = instruction.bytes.size();
        if (instruction.supported && instruction.exact) {
            try {
                report.decoded = oasis::tools::exact_instruction_asm(instruction);
                report.status = "DECODED";
            } catch (const std::exception&) {
                report.status = "DECODE_UNSUPPORTED";
            }
        } else {
            report.status = "DECODE_UNSUPPORTED";
        }
    }
    report.classification = classify(address, ranges);
    return report;
}

void write_json(const char* path, const std::vector<InstructionReport>& reports,
                const std::vector<std::pair<std::uint32_t, std::uint32_t>>& regions) {
    std::ofstream output(path);
    if (!output) throw std::runtime_error("unable to write JSON report");
    std::map<std::string, std::size_t> counts;
    for (const auto& report : reports) ++counts[report.classification];
    output << "{\n  \"instruction_count\": " << reports.size() << ",\n";
    output << "  \"classification_counts\": {";
    bool first = true;
    for (const auto& [name, count] : counts) {
        if (!first) output << ",";
        output << "\n    \"" << json_escape(name) << "\": " << count;
        first = false;
    }
    output << "\n  },\n  \"instructions\": [";
    for (std::size_t i = 0; i < reports.size(); ++i) {
        const auto& report = reports[i];
        output << (i ? "," : "") << "\n    {\"address\": \"" << hex_u32(report.address)
               << "\", \"raw_bytes\": \"" << report.raw_bytes
               << "\", \"decoded_instruction\": \"" << json_escape(report.decoded)
               << "\", \"instruction_length\": " << report.length
               << ", \"status\": \"" << report.status
               << "\", \"classification\": \"" << report.classification << "\"}";
    }
    output << "\n  ],\n  \"executed_regions\": [";
    for (std::size_t i = 0; i < regions.size(); ++i)
        output << (i ? "," : "") << "\n    {\"start\": \"" << hex_u32(regions[i].first)
               << "\", \"end\": \"" << hex_u32(regions[i].second)
               << "\", \"instruction_count\": "
               << ((regions[i].second - regions[i].first) / 2U + 1U) << "}";
    output << "\n  ]\n}\n";
}

void write_text(const char* path, const std::vector<InstructionReport>& reports,
                const std::vector<std::pair<std::uint32_t, std::uint32_t>>& regions) {
    std::ofstream output(path);
    if (!output) throw std::runtime_error("unable to write text report");
    output << "NEW EXECUTED CODE REPORT\n\n";
    output << "Instructions: " << reports.size() << "\n";
    output << "Regions (neutral executed_region, not functions): " << regions.size() << "\n\n";
    output << "INSTRUCTIONS\n";
    for (const auto& report : reports)
        output << hex_u32(report.address) << " " << report.raw_bytes << " "
               << report.status << " " << report.classification << " "
               << (report.decoded.empty() ? "-" : report.decoded) << "\n";
    output << "\nEXECUTED_REGIONS\n";
    for (const auto& region : regions)
        output << hex_u32(region.first) << "-" << hex_u32(region.second)
               << " count=" << ((region.second - region.first) / 2U + 1U) << "\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 5 || argc > 7) {
        std::cerr << "usage: oasis_gpgx_coverage_report <canonical_rom> "
                     "<session_new_pc_bitmap.bin> <new_executed_code.txt> "
                     "<new_executed_code.json> [code_manifest.json] [data.json]\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported)
            throw std::runtime_error("report requires the supported canonical USA ROM");
        const auto expected_size = rom.size() / 16U;
        const auto addresses = read_bitmap(argv[2], expected_size, rom.size());
        std::vector<Range> ranges = load_ranges(argc >= 6 ? argv[5] : nullptr);
        const auto data_ranges = load_ranges(argc == 7 ? argv[6] : nullptr);
        ranges.insert(ranges.end(), data_ranges.begin(), data_ranges.end());
        std::vector<InstructionReport> reports;
        reports.reserve(addresses.size());
        for (const auto address : addresses) reports.push_back(decode_one(rom.bytes(), address, ranges));
        std::vector<std::pair<std::uint32_t, std::uint32_t>> regions;
        for (const auto address : addresses) {
            if (regions.empty() || address != regions.back().second + 2U)
                regions.emplace_back(address, address);
            else regions.back().second = address;
        }
        write_text(argv[3], reports, regions);
        write_json(argv[4], reports, regions);
        std::cout << "reported " << reports.size() << " NEW executed PCs in "
                  << regions.size() << " neutral regions\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
