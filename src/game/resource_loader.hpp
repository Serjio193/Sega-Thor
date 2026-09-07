#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game {

struct LoadedResource {
    std::uint8_t resource_id{};
    std::uint32_t table_entry{};
    std::uint32_t rom_input_start{};
    std::size_t compressed_size{};
    std::vector<std::uint8_t> bytes;
};

inline constexpr std::uint8_t kVerifiedResourceId = 3;
inline constexpr std::uint32_t kResourceTableAddress = 0x05CE96U;
inline constexpr std::uint32_t kResourceTableEntry = 0x05CEA2U;
inline constexpr std::uint32_t kResourceInputAddress = 0x001AE1A8U;
inline constexpr std::size_t kResourceCompressedSize = 0x702U;
inline constexpr std::size_t kResourceOutputSize = 0x1000U;
inline constexpr std::size_t kResourceVramAddress = 0x4000U;
inline constexpr char kCanonicalRomSha256[] =
    "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263";
inline constexpr char kResourceOutputSha256[] =
    "36bbea13cb564ab194dd438b1fe75076525525068b57f2f02a4c0142630dc277";

void validate_resource_id3_output(std::size_t compressed_size,
                                  std::span<const std::uint8_t> output);

[[nodiscard]] LoadedResource load_verified_resource(
    std::span<const std::uint8_t> rom, std::uint8_t resource_id);

} // namespace oasis::game
