#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace oasis::game {

struct LoadedResource {
    std::uint16_t resource_id{};
    std::uint32_t table_entry{};
    std::uint32_t rom_input_start{};
    std::uint32_t rom_input_end{};
    std::size_t compressed_size{};
    std::string compressed_sha256;
    std::string decoded_sha256;
    std::vector<std::uint8_t> bytes;
    std::string backend_status;
    std::string migration_status;
    std::string override_mode;
    std::string override_rejection_reason;
    std::string external_path;
    std::size_t rom_payload_bytes_passed_to_decoder{};
    bool external_file_verified{};
    bool external_file_active{};
};

inline constexpr std::uint8_t kVerifiedResourceId = 3;
inline constexpr std::uint16_t kGfx3820ResourceCount = 107;
inline constexpr std::uint32_t kResourceTableAddress = 0x05CE96U;
inline constexpr std::uint32_t kResourceTableEntry = 0x05CEA2U; // ID 3
inline constexpr std::uint32_t kResourceInputAddress = 0x001AE1A8U; // ID 3
inline constexpr std::size_t kResourceCompressedSize = 0x702U; // ID 3
inline constexpr std::size_t kResourceOutputSize = 0x1000U;
inline constexpr std::size_t kResourceVramAddress = 0x4000U;
inline constexpr char kGfx3820SourceCensusSha256[] =
    "c3c7b3efa6fe09e6cd3b55aa4119a1dafba99b49b7a5b180aed8fb627bdb70bb";
inline constexpr char kGfx3820ManifestSha256[] =
    "caddd97ba04ec65088a390a920ad3240837d7acfe0ce98f5b5791f05b37b69a5";
inline constexpr char kCanonicalRomSha256[] =
    "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263";
inline constexpr char kResourceOutputSha256[] =
    "36bbea13cb564ab194dd438b1fe75076525525068b57f2f02a4c0142630dc277";

void validate_resource_id3_output(std::size_t compressed_size,
                                  std::span<const std::uint8_t> output);

[[nodiscard]] LoadedResource load_graphics_resource(
    std::span<const std::uint8_t> rom,
    std::uint16_t resource_id,
    const std::filesystem::path& override_directory = "assets/gfx3820");

[[nodiscard]] LoadedResource load_verified_resource(
    std::span<const std::uint8_t> rom,
    std::uint8_t resource_id,
    const std::filesystem::path& override_directory = "assets/gfx3820");

} // namespace oasis::game
