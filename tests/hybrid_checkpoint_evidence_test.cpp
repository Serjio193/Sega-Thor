#include "tools/hybrid/checkpoint_evidence.hpp"

#include "core/rom_identity.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

std::string read_text(const std::filesystem::path& path) {
    std::ifstream input(path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::vector<std::uint8_t> read_bytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write_evidence(const std::filesystem::path& directory) {
    oasis::hybrid::CheckpointEvidence evidence(directory);
    std::vector<std::uint8_t> first(0xfd000);
    std::vector<std::uint8_t> second(0xfd000);
    first[659] = 0x01;
    second[659] = 0x02;
    evidence.record(60, first, 123, oasis::calculate_sha256(first),
                    oasis::hybrid::checkpoint_identity_hash(first));
    evidence.record(120, second, 456, oasis::calculate_sha256(second),
                    oasis::hybrid::checkpoint_identity_hash(second));
    evidence.close();
}

} // namespace

int main() {
    std::vector<std::uint8_t> pointer_a(0xfd000);
    std::vector<std::uint8_t> pointer_b = pointer_a;
    pointer_a[140654] = 0x12;
    pointer_b[140654] = 0x98;
    assert(oasis::hybrid::checkpoint_identity_hash(pointer_a) ==
           oasis::hybrid::checkpoint_identity_hash(pointer_b));
    pointer_b[140659] = 0x01;
    assert(oasis::hybrid::checkpoint_identity_hash(pointer_a) !=
           oasis::hybrid::checkpoint_identity_hash(pointer_b));

    const auto suffix = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto root = std::filesystem::temp_directory_path() /
                      ("oasis_checkpoint_evidence_test_" + std::to_string(suffix));
    const auto first = root / "first";
    const auto second = root / "second";
    std::filesystem::create_directories(first);
    std::filesystem::create_directories(second);
    write_evidence(first);
    write_evidence(second);

    assert(read_bytes(first / "checkpoint_records.bin") ==
           read_bytes(second / "checkpoint_records.bin"));
    assert(read_text(first / "checkpoint_manifest.jsonl") ==
           read_text(second / "checkpoint_manifest.jsonl"));
    assert(read_text(first / "checkpoint_manifest.jsonl").find(
               "\"aggregate_sha256\":") != std::string::npos);

    std::filesystem::remove(first / "checkpoint_records.bin");
    std::filesystem::remove(first / "checkpoint_manifest.jsonl");
    std::filesystem::remove(second / "checkpoint_records.bin");
    std::filesystem::remove(second / "checkpoint_manifest.jsonl");
    std::filesystem::remove(first);
    std::filesystem::remove(second);
    std::filesystem::remove(root);
}
