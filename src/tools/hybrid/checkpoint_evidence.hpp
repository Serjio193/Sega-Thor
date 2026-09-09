#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>

namespace oasis::hybrid {

[[nodiscard]] std::string checkpoint_identity_hash(std::span<const std::uint8_t> state);

class CheckpointEvidence {
public:
    explicit CheckpointEvidence(const std::filesystem::path& directory);
    ~CheckpointEvidence();

    CheckpointEvidence(const CheckpointEvidence&) = delete;
    CheckpointEvidence& operator=(const CheckpointEvidence&) = delete;

    void record(unsigned frame, std::span<const std::uint8_t> state,
                std::uint64_t cpu_cycles, std::string_view raw_hash,
                std::string_view identity_hash);
    void close();

private:
    std::size_t record_count_{};
    std::size_t record_bytes_{};
    std::string state_hashes_;
    class Files;
    Files* files_{};
};

} // namespace oasis::hybrid
