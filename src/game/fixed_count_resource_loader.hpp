#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oasis::game {

struct FixedCountRecord {
    std::uint16_t word0{};
    std::uint16_t word1{};
    std::uint16_t word2{};
};

struct LoadedFixedCountStream {
    std::uint32_t row_id{};
    std::string stream_id;
    std::uint32_t rom_start{};
    std::uint32_t rom_end{};
    std::size_t byte_length{};
    std::uint16_t count_value{};
    std::size_t record_count{};
    std::string raw_sha256;
    std::vector<std::uint8_t> raw_bytes;
    std::vector<FixedCountRecord> records;
    std::string backend_status;
    std::string override_rejection_reason;
    std::filesystem::path external_path;
    std::size_t rom_payload_bytes_selected{};
    bool external_file_active{};
};

inline constexpr std::size_t kFixedCountActiveRowCount = 96;
inline constexpr std::size_t kFixedCountUniqueStreamCount = 78;
inline constexpr char kFixedCountRomSha256[] =
    "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263";

[[nodiscard]] LoadedFixedCountStream load_fixed_count_stream(
    std::span<const std::uint8_t> rom, std::uint32_t row_id,
    const std::filesystem::path& manifest =
        "assets/fixed_count_records/manifest.tsv",
    const std::filesystem::path& override_directory =
        "assets/fixed_count_records/streams");

} // namespace oasis::game
