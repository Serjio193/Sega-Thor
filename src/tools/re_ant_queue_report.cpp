#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_atlas.hpp"
#include "tools/re_candidate_map.hpp"
#include "tools/re_ant_queue.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {

std::string read_file(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("unable to open queue input: " + std::string(path));
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write_file(const char* path, const std::string& value) {
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("unable to open queue output: " + std::string(path));
    output << value;
}

int make_queue(int argc, char** argv) {
    if (argc != 7) throw std::invalid_argument("usage: oasis_re_ant_queue make <usa_rom> <explore.json> <reach.json> <queue.json> <queue.txt>");
    const auto rom = oasis::Rom::load(argv[2]);
    const auto identity = oasis::identify_rom(rom.bytes());
    if (identity.status != oasis::RomSupportStatus::Supported) throw std::runtime_error("queue requires supported USA ROM");
    const auto queue = oasis::tools::make_ant_queue(read_file(argv[3]), read_file(argv[4]),
                                                     identity.fingerprint.sha256, rom.bytes().size(), "2.11.1");
    write_file(argv[5], oasis::tools::ant_queue_to_json(queue));
    write_file(argv[6], oasis::tools::ant_queue_to_text(queue));
    std::cout << "created " << queue.queue_id << " jobs=" << queue.jobs.size() << '\n';
    return 0;
}

int claim_queue(int argc, char** argv) {
    if (argc != 5) throw std::invalid_argument("usage: oasis_re_ant_queue claim <queue.json> <queue-out.json> <job.json>");
    auto queue = oasis::tools::parse_ant_queue(read_file(argv[2]));
    oasis::tools::recover_stale_ant_claims(queue);
    const auto index = oasis::tools::claim_next_ant_job(queue);
    if (!index) return 3;
    write_file(argv[3], oasis::tools::ant_queue_to_json(queue));
    write_file(argv[4], oasis::tools::ant_job_to_json(queue.jobs[*index].job));
    std::cout << queue.jobs[*index].job.job_id << '\n';
    return 0;
}

int finalize_queue(int argc, char** argv) {
    if (argc != 6) throw std::invalid_argument("usage: oasis_re_ant_queue finalize <queue.json> <result.json> <queue-out.json> <usa_rom>");
    auto queue = oasis::tools::parse_ant_queue(read_file(argv[2]));
    const auto result = oasis::tools::parse_ant_result(read_file(argv[3]));
    const auto index = std::find_if(queue.jobs.begin(), queue.jobs.end(), [&](const auto& item) {
        return item.lifecycle == oasis::tools::AntQueueLifecycle::claimed && item.job.job_id == result.job_id;
    });
    if (index == queue.jobs.end()) throw std::runtime_error("result does not match a CLAIMED queue job");
    const auto rom = oasis::Rom::load(argv[5]);
    const auto identity = oasis::identify_rom(rom.bytes());
    const auto merge = oasis::tools::merge_ant_result(index->job, result, rom.bytes().size(), identity.fingerprint.sha256);
    oasis::tools::finalize_ant_job(queue, static_cast<std::size_t>(index - queue.jobs.begin()), result,
                                   merge.accepted, merge.accepted ? "accepted_dynamic_evidence" : merge.reason);
    write_file(argv[4], oasis::tools::ant_queue_to_json(queue));
    std::cout << result.job_id << " state=" << oasis::tools::ant_queue_lifecycle_name(index->lifecycle)
              << " accepted=" << (merge.accepted ? "yes" : "no") << '\n';
    return 0;
}

int merge_queue(int argc, char** argv) {
    if (argc != 13 && argc != 14) throw std::invalid_argument("usage: oasis_re_ant_queue merge <usa_rom> <ghidra.json> <before.json> <queue.json> <result0.json> <result1.json> <result2.json> <result3.json> <result4.json> <after.json> <after.txt> [beta_rom]");
    const auto rom = oasis::Rom::load(argv[2]);
    const auto identity = oasis::identify_rom(rom.bytes());
    if (identity.status != oasis::RomSupportStatus::Supported) throw std::runtime_error("merge requires supported USA ROM");
    const auto queue = oasis::tools::parse_ant_queue(read_file(argv[5]));
    oasis::tools::ExploreOptions options;
    options.rom_wide = true;
    std::size_t observations = 0, accepted = 0, rejected = 0;
    for (int argument = 6; argument <= 10; ++argument) {
        const auto result = oasis::tools::parse_ant_result(read_file(argv[argument]));
        const auto job = std::find_if(queue.jobs.begin(), queue.jobs.end(), [&](const auto& item) { return item.job.job_id == result.job_id; });
        if (job == queue.jobs.end()) throw std::runtime_error("result does not belong to queue");
        ++observations;
        const auto merge = oasis::tools::merge_ant_result(job->job, result, rom.bytes().size(), identity.fingerprint.sha256);
        if (merge.accepted) { options.dynamic_edges.push_back(merge.edge); ++accepted; }
        else ++rejected;
    }
    const auto ghidra = oasis::tools::parse_ghidra_map(read_file(argv[3]));
    const auto atlas = argc == 14 ? oasis::tools::build_rom_atlas(rom.bytes(), oasis::Rom::load(argv[13]).bytes()) : oasis::tools::build_rom_atlas(rom.bytes());
    const auto candidates = oasis::tools::build_candidate_map(ghidra, atlas);
    const auto after = oasis::tools::explore_m68k(rom.bytes(), candidates, atlas, options);
    write_file(argv[11], oasis::tools::explore_to_json(after));
    write_file(argv[12], oasis::tools::explore_to_text(after));
    std::cout << "observations=" << observations << " accepted=" << accepted << " rejected=" << rejected
              << " dynamic_edges=" << after.metrics.dynamic_indirect_edges
              << " after_frontiers=" << after.metrics.frontier_count << '\n';
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: oasis_re_ant_queue <make|claim|finalize> ...\n"; return 2; }
    try {
        const std::string mode = argv[1];
        if (mode == "make") return make_queue(argc, argv);
        if (mode == "claim") return claim_queue(argc, argv);
        if (mode == "finalize") return finalize_queue(argc, argv);
        if (mode == "merge") return merge_queue(argc, argv);
        throw std::invalid_argument("unknown queue mode");
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n'; return 1;
    }
}
