#include "tools/hybrid/checkpoint_evidence.hpp"
#include "tools/hybrid/gpgx_checkpoint_layout.hpp"

#include "core/rom_identity.hpp"

#include <algorithm>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace oasis::hybrid {
namespace {

std::string quote(std::string_view value) {
    std::string result{"\""};
    result.append(value);
    result.push_back('"');
    return result;
}

void canonicalize_host_representation(std::vector<std::uint8_t>& state) {
    const auto& layout = gpgx_checkpoint_layout();
    if (state.size() != layout.state_size) {
        throw std::runtime_error("unsupported GPGX checkpoint state size");
    }
    if (std::string_view(reinterpret_cast<const char*>(state.data()), 16) != layout.state_version) {
        throw std::runtime_error("unsupported GPGX checkpoint state version");
    }
    for (const auto& span : layout.representation_spans) {
        std::fill_n(state.begin() + static_cast<std::ptrdiff_t>(span.offset),
                    static_cast<std::ptrdiff_t>(span.size), std::uint8_t{0});
    }
}

} // namespace

std::string checkpoint_identity_hash(std::span<const std::uint8_t> state) {
    std::vector<std::uint8_t> canonical(state.begin(), state.end());
    canonicalize_host_representation(canonical);
    return oasis::calculate_sha256(canonical);
}

class CheckpointEvidence::Files {
public:
    explicit Files(const std::filesystem::path& directory)
        : records(directory / "checkpoint_records.bin", std::ios::binary | std::ios::trunc),
          manifest(directory / "checkpoint_manifest.jsonl", std::ios::out | std::ios::trunc) {
        records.exceptions(std::ios::failbit | std::ios::badbit);
        manifest.exceptions(std::ios::failbit | std::ios::badbit);
        manifest << "{\"schema\":\"oasis.checkpoint-evidence.v2\","
                    "\"record_order\":\"ordinal_ascending\","
                    "\"raw_records\":\"concatenated_full_serialize_buffers\","
                    "\"aggregate_input\":\"concatenated_lowercase_canonical_state_sha256_values\"}\n";
    }

    std::ofstream records;
    std::ofstream manifest;
};

CheckpointEvidence::CheckpointEvidence(const std::filesystem::path& directory)
    : files_(new Files(directory)) {}

CheckpointEvidence::~CheckpointEvidence() {
    try {
        close();
    } catch (...) {
    }
    delete files_;
}

void CheckpointEvidence::record(unsigned frame, std::span<const std::uint8_t> state,
                                 std::uint64_t cpu_cycles, std::string_view raw_hash,
                                 std::string_view identity_hash) {
    if (files_ == nullptr) throw std::logic_error("checkpoint evidence is closed");
    if (oasis::calculate_sha256(state) != raw_hash || checkpoint_identity_hash(state) != identity_hash)
        throw std::runtime_error("checkpoint evidence hash mismatch");

    const auto offset = record_bytes_;
    files_->records.write(reinterpret_cast<const char*>(state.data()),
                          static_cast<std::streamsize>(state.size()));
    files_->manifest << "{\"ordinal\":" << record_count_ << ",\"frame\":" << frame
                     << ",\"offset\":" << offset << ",\"size\":" << state.size()
                     << ",\"raw_state_sha256\":" << quote(raw_hash)
                     << ",\"state_sha256\":" << quote(identity_hash)
                     << ",\"cpu_cycles\":" << cpu_cycles << "}\n";
    state_hashes_ += identity_hash;
    ++record_count_;
    record_bytes_ += state.size();
}

void CheckpointEvidence::close() {
    if (files_ == nullptr) return;
    const auto aggregate = oasis::calculate_sha256({
        reinterpret_cast<const std::uint8_t*>(state_hashes_.data()), state_hashes_.size()});
    files_->manifest << "{\"final\":true,\"aggregate_sha256\":" << quote(aggregate)
                     << ",\"record_count\":" << record_count_
                     << ",\"record_bytes\":" << record_bytes_ << "}\n";
    files_->records.close();
    files_->manifest.close();
    delete files_;
    files_ = nullptr;
}

} // namespace oasis::hybrid
