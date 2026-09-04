#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_atlas.hpp"
#include "tools/re_emulator_trace.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {

std::string read_file(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("unable to open external trace: " + std::string(path));
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write_file(const char* path, const std::string& value) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to open trace report: " + std::string(path));
    output << value;
}

std::vector<std::uint32_t> atlas_code_addresses(const oasis::tools::AtlasReport& atlas) {
    std::vector<std::uint32_t> result;
    for (const auto& entry : atlas.entries) {
        if (entry.type != oasis::tools::AtlasEntryType::function && entry.type != oasis::tools::AtlasEntryType::bounded_code) continue;
        const auto end = entry.end ? entry.end : entry.bounded_evidence_end;
        if (!end || *end < entry.start || *end - entry.start > 0x10000U) continue;
        for (auto address = entry.start; address < *end; ++address) result.push_back(address);
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: oasis_re_emulator_trace <usa_rom> <external_trace.txt> <report.json> <report.txt>\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        const auto identity = oasis::identify_rom(rom.bytes());
        if (identity.status != oasis::RomSupportStatus::Supported)
            throw std::runtime_error("emulator trace import requires the supported USA reference ROM");
        auto capture = oasis::tools::parse_external_trace(read_file(argv[2]));
        const auto atlas = oasis::tools::build_rom_atlas(rom.bytes());
        const auto report = oasis::tools::normalize_emulator_trace(
            std::move(capture), atlas_code_addresses(atlas), oasis::tools::read_reset_vectors(rom.bytes()),
            identity.id, identity.fingerprint.sha256);
        write_file(argv[3], oasis::tools::emulator_trace_to_json(report));
        std::cout << oasis::tools::emulator_trace_to_text(report);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
