#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oasis::game {

struct ScreenResourcePath {
    std::uint16_t root_id{};
    std::uint32_t root_address{};
    std::uint32_t root_source{};
    std::uint16_t root_index{};
    std::string descriptor_id;
    std::uint32_t descriptor_address{};
};

struct LoadedScreenResource {
    std::string stream_id;
    std::uint32_t rom_start{};
    std::uint32_t rom_end{};
    std::size_t compressed_size{};
    std::string compressed_sha256;
    std::size_t decoded_size{};
    std::string decoded_sha256;
    std::vector<std::uint8_t> bytes;
    std::vector<ScreenResourcePath> paths;
    std::string backend_status;
    std::string override_rejection_reason;
    std::string external_path;
    std::size_t rom_payload_bytes_passed_to_decoder{};
    bool external_file_active{};
};

inline constexpr std::uint16_t kGfxScreenRootCount = 21;
inline constexpr std::uint16_t kGfxScreenPathUseCount = 167;
inline constexpr std::uint16_t kGfxScreenDescriptorCount = 163;
inline constexpr std::uint16_t kGfxScreenStreamCount = 159;
inline constexpr char kGfxScreenRootsManifestSha256[] =
    "0320f9ae4b90f24dfd37ccb61af5f8d965ad94ddc78d79fbea209515dd3bdda9";
inline constexpr char kGfxScreenRootsSourceCensusSha256[] =
    "91f0c0c73edc39fd8840be4398a66e0eb23e18ff4bc72c6fe047a932eee4d13b";
inline constexpr char kGfxScreenRootsRomSha256[] =
    "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263";

[[nodiscard]] LoadedScreenResource load_screen_graphics_resource(
    std::span<const std::uint8_t> rom,
    std::string_view stream_id,
    const std::filesystem::path& manifest =
        "assets/gfx3820_screen_roots/manifest.tsv",
    const std::filesystem::path& override_directory =
        "assets/gfx3820_screen_roots/streams");

} // namespace oasis::game
