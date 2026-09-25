#include "core/rom_identity.hpp"
#include "tools/re_assemble.hpp"
#include "tools/re_canonical_rom_bytes.hpp"
#include "tools/re_cfg_closure.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
std::uint32_t number(const char* text) {
    std::size_t used = 0;
    const auto value = std::stoull(text, &used, 0);
    if (text[used] || value > 0xFFFFFFFFULL) throw std::invalid_argument("invalid ROM offset");
    return static_cast<std::uint32_t>(value);
}

std::vector<std::uint32_t> read_seeds(const char* path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot read exact seed file");
    std::vector<std::uint32_t> seeds;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') continue;
        seeds.push_back(number(line.substr(0, line.find('\t')).c_str()));
    }
    return seeds;
}

std::vector<std::uint32_t> read_targets(const char* path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot read known-code range file");
    std::vector<std::uint32_t> targets;
    std::string line;
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') continue;
        targets.push_back(number(line.c_str()));
    }
    std::sort(targets.begin(), targets.end());
    targets.erase(std::unique(targets.begin(), targets.end()), targets.end());
    return targets;
}

void write(const std::filesystem::path& path, const std::string& text) {
    std::ofstream output(path, std::ios::binary);
    if (!output || !(output << text)) throw std::runtime_error("cannot write report artifact");
}

std::string blockers_json(const std::vector<std::string>& blockers) {
    std::string out = "[";
    for (std::size_t i = 0; i < blockers.size(); ++i) {
        if (i) out += ',';
        out += "\"" + blockers[i] + "\"";
    }
    return out + "]";
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 7) {
        std::cerr << "usage: oasis_re_m14_4_cfg <rom> <start> <end> <seeds.tsv> "
                     "<known-code.tsv> <output-dir>\n";
        return 2;
    }
    try {
        const auto rom = oasis::tools::CanonicalRomBytes::load(argv[1]);
        const auto start = number(argv[2]);
        const auto end = number(argv[3]);
        const auto seeds = read_seeds(argv[4]);
        const auto known = read_targets(argv[5]);
        const auto output = std::filesystem::path(argv[6]);
        std::filesystem::create_directories(output);
        const auto cfg = oasis::tools::decode_closed_cfg(rom.read(0, rom.size()),
            start, end, seeds, known);
        std::set<std::uint32_t> seed_set(seeds.begin(), seeds.end());
        std::vector<std::pair<std::uint32_t, std::uint32_t>> extents;
        for (const auto& instruction : cfg.instructions) {
            const auto instruction_end = instruction.address +
                static_cast<std::uint32_t>(instruction.bytes.size());
            if (extents.empty() || extents.back().second != instruction.address)
                extents.emplace_back(instruction.address, instruction_end);
            else extents.back().second = instruction_end;
        }
        for (std::size_t i = 0; i < extents.size(); ++i) {
            oasis::tools::DecodedSlice slice{};
            slice.entry = extents[i].first;
            slice.range_end = extents[i].second;
            for (const auto& instruction : cfg.instructions) {
                if (instruction.address >= slice.entry &&
                    instruction.address + instruction.bytes.size() <= slice.range_end)
                    slice.instructions.push_back(instruction);
            }
            const auto stem = std::string("extent-") + std::to_string(i);
            write(output / (stem + ".asm"), oasis::tools::slice_asm(slice));
            write(output / (stem + ".json"), oasis::tools::exact_slice_json(slice));
        }
        std::ostringstream report;
        report << "{\"schema\":\"oasis.m14.4.cfg-closure.v1\",\"rom_sha256\":\""
               << rom.sha256() << "\",\"rom_size\":" << rom.size()
               << ",\"analysis_window\":[" << start << ',' << end
               << "],\"seed_count\":" << seed_set.size()
               << ",\"instruction_count\":" << cfg.instructions.size()
               << ",\"decoded_bytes\":" << cfg.decoded_bytes
               << ",\"connected_components\":" << cfg.connected_components
               << ",\"closed_cfg\":" << (cfg.closed ? "true" : "false")
               << ",\"blockers\":" << blockers_json(cfg.blockers)
               << ",\"extents\":[";
        for (std::size_t i = 0; i < extents.size(); ++i) {
            if (i) report << ',';
            report << "{\"start\":" << extents[i].first << ",\"end\":" << extents[i].second
                   << ",\"size\":" << extents[i].second - extents[i].first
                   << ",\"asm\":\"extent-" << i << ".asm\",\"slice\":\"extent-" << i
                   << ".json\"}";
        }
        report << "]}\n";
        write(output / "cfg.json", report.str());
        std::cout << report.str();
        return cfg.closed ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 2;
    }
}
