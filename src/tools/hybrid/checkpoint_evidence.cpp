#include "tools/hybrid/checkpoint_evidence.hpp"
#include "tools/hybrid/gpgx_checkpoint_layout.hpp"

#include "core/rom_identity.hpp"

#include <algorithm>
#include <cstdlib>
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

std::string hex_bytes(std::span<const std::uint8_t> bytes) {
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(bytes.size() * 2);
    for (const auto byte : bytes) {
        result.push_back(digits[byte >> 4]);
        result.push_back(digits[byte & 0x0f]);
    }
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
        : root(directory), records(directory / "checkpoint_records.bin", std::ios::binary | std::ios::trunc),
          manifest(directory / "checkpoint_manifest.jsonl", std::ios::out | std::ios::trunc) {
        records.exceptions(std::ios::failbit | std::ios::badbit);
        manifest.exceptions(std::ios::failbit | std::ios::badbit);
        manifest << "{\"schema\":\"oasis.checkpoint-evidence.v2\","
                    "\"record_order\":\"ordinal_ascending\","
                    "\"raw_records\":\"concatenated_full_serialize_buffers\","
                    "\"aggregate_input\":\"concatenated_lowercase_canonical_state_sha256_values\"}\n";
    }

    std::filesystem::path root;
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

void CheckpointEvidence::record(unsigned frame, std::span<const std::uint8_t> state) {
    const auto raw_hash = oasis::calculate_sha256(state);
    record(frame, state, 0, raw_hash, checkpoint_identity_hash(state));
}

void CheckpointEvidence::record_with_roundtrip_option(
    unsigned frame, std::span<const std::uint8_t> state,
    bool (*unserialize)(const void*, std::size_t), bool (*serialize)(void*, std::size_t)) {
    record(frame, state);
    const auto* selected = std::getenv("OASIS_CHECKPOINT_ROUNDTRIP_FRAME");
    if (selected == nullptr || std::stoul(selected) != frame) return;
    if (!unserialize || !serialize || !unserialize(state.data(), state.size()))
        throw std::runtime_error("checkpoint roundtrip restore failed");

    std::vector<std::uint8_t> restored(state.size());
    if (!serialize(restored.data(), restored.size()))
        throw std::runtime_error("checkpoint roundtrip serialization failed");
    std::ofstream report(files_->root / "checkpoint_roundtrip.json", std::ios::trunc);
    if (!report) throw std::runtime_error("checkpoint roundtrip report open failed");
    report << "{\"schema\":\"oasis.checkpoint-roundtrip.v1\",\"frame\":" << frame
           << ",\"raw_before_sha256\":\"" << oasis::calculate_sha256(state)
           << "\",\"raw_after_sha256\":\"" << oasis::calculate_sha256(restored)
           << "\",\"canonical_before_sha256\":\"" << checkpoint_identity_hash(state)
           << "\",\"canonical_after_sha256\":\"" << checkpoint_identity_hash(restored)
           << "\",\"byte_identical\":" << (std::equal(state.begin(), state.end(), restored.begin()) ? "true" : "false")
           << ",\"diff_ranges\":[";
    bool first = true;
    for (std::size_t offset = 0; offset < state.size();) {
        if (state[offset] == restored[offset]) { ++offset; continue; }
        const auto start = offset;
        while (offset < state.size() && state[offset] != restored[offset]) ++offset;
        if (!first) report << ',';
        first = false;
        report << "{\"start\":" << start << ",\"end_exclusive\":" << offset
               << ",\"before_hex\":\""
               << hex_bytes(state.subspan(start, offset - start)) << "\",\"after_hex\":\""
               << hex_bytes(std::span<const std::uint8_t>(restored).subspan(start, offset - start)) << "\"}";
    }
    report << "]}\n";
}

void maybe_record_checkpoint(const std::filesystem::path& directory, unsigned frame,
                             std::span<const std::uint8_t> state,
                             bool (*unserialize)(const void*, std::size_t),
                             bool (*serialize)(void*, std::size_t)) {
    if (std::getenv("OASIS_CAPTURE_RAW_CHECKPOINTS") == nullptr) return;
    static auto evidence = std::make_unique<CheckpointEvidence>(directory);
    evidence->record_with_roundtrip_option(frame, state, unserialize, serialize);
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
