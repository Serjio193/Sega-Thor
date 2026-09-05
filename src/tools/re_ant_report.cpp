#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_atlas.hpp"
#include "tools/re_candidate_map.hpp"
#include "tools/re_ant.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {

std::string read_file(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("unable to open ant input: " + std::string(path));
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write_file(const char* path, const std::string& value) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to open ant output: " + std::string(path));
    output << value;
    if (!output) throw std::runtime_error("unable to write ant output: " + std::string(path));
}

void require_usa(const oasis::Rom& rom) {
    const auto identity = oasis::identify_rom(rom.bytes());
    if (identity.status != oasis::RomSupportStatus::Supported)
        throw std::runtime_error("ant requires the supported USA ROM");
}

int make_job(int argc, char** argv) {
    if (argc != 5) throw std::invalid_argument("usage: oasis_re_ant make-job <usa_rom> <explore.json> <job.json>");
    const auto rom = oasis::Rom::load(argv[2]);
    require_usa(rom);
    const auto frontier = oasis::tools::select_ant_frontier(read_file(argv[3]), 0x045AU);
    if (!frontier) throw std::runtime_error("selected natural frontier 0x045A is absent or not INDIRECT_FLOW");
    const auto identity = oasis::identify_rom(rom.bytes());
    const auto job = oasis::tools::make_ant_job(*frontier, identity.fingerprint.sha256, rom.bytes().size(), "2.11.1");
    write_file(argv[4], oasis::tools::ant_job_to_json(job));
    std::cout << "created " << job.job_id << " for " << job.frontier_id << '\n';
    return 0;
}

int merge_job(int argc, char** argv) {
    if (argc != 9 && argc != 10) throw std::invalid_argument("usage: oasis_re_ant merge <usa_rom> <ghidra.json> <before.json> <job.json> <result.json> <after.json> <after.txt> [beta_rom]");
    const auto rom = oasis::Rom::load(argv[2]);
    require_usa(rom);
    const auto job = oasis::tools::parse_ant_job(read_file(argv[5]));
    const auto result = oasis::tools::parse_ant_result(read_file(argv[6]));
    const auto frontier = oasis::tools::select_ant_frontier(read_file(argv[4]), job.source_pc);
    if (!frontier || frontier->id != job.frontier_id) throw std::runtime_error("job does not reference the latest selected frontier");
    const auto identity = oasis::identify_rom(rom.bytes());
    const auto merge = oasis::tools::merge_ant_result(job, result, rom.bytes().size(), identity.fingerprint.sha256);
    if (!merge.accepted) throw std::runtime_error("ant merge rejected: " + merge.reason);
    const auto ghidra = oasis::tools::parse_ghidra_map(read_file(argv[3]));
    const auto atlas = argc == 10
        ? oasis::tools::build_rom_atlas(rom.bytes(), oasis::Rom::load(argv[9]).bytes())
        : oasis::tools::build_rom_atlas(rom.bytes());
    const auto candidates = oasis::tools::build_candidate_map(ghidra, atlas);
    oasis::tools::ExploreOptions options;
    options.rom_wide = true;
    options.dynamic_edges.push_back(merge.edge);
    const auto after = oasis::tools::explore_m68k(rom.bytes(), candidates, atlas, options);
    write_file(argv[7], oasis::tools::explore_to_json(after));
    write_file(argv[8], oasis::tools::explore_to_text(after));
    std::cout << "merged " << merge.edge.source_pc << " -> " << merge.edge.target
              << " evidence=" << merge.edge.evidence_class << " after_frontiers="
              << after.metrics.frontier_count << " dynamic_edges=" << after.metrics.dynamic_indirect_edges << '\n';
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: oasis_re_ant <make-job|merge> ...\n"; return 2; }
    try {
        if (std::string(argv[1]) == "make-job") return make_job(argc, argv);
        if (std::string(argv[1]) == "merge") return merge_job(argc, argv);
        throw std::invalid_argument("unknown ant mode");
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
