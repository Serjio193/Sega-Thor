#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace oasis {

enum class RomSupportStatus {
    Supported,
    KnownUnsupported,
    Modified,
    Unknown,
};

struct MegaDriveHeader {
    std::string console_name;
    std::string domestic_title;
    std::string international_title;
    std::string product_code;
    std::string region;
    std::uint16_t stored_checksum{};
    std::uint32_t rom_start{};
    std::uint32_t rom_end{};
    bool header_signature_valid{};
};

struct RomFingerprint {
    std::size_t size{};
    std::uint32_t crc32{};
    std::string sha1;
    std::string sha256;
    std::uint16_t calculated_sega_checksum{};
    bool sega_checksum_valid{};
};

struct RomIdentity {
    std::string id;
    std::string display_name;
    RomSupportStatus status{RomSupportStatus::Unknown};
    MegaDriveHeader header;
    RomFingerprint fingerprint;
};

[[nodiscard]] MegaDriveHeader parse_mega_drive_header(std::span<const std::uint8_t> rom);
[[nodiscard]] std::uint16_t calculate_sega_checksum(std::span<const std::uint8_t> rom);
[[nodiscard]] std::uint32_t calculate_crc32(std::span<const std::uint8_t> data);
[[nodiscard]] std::string calculate_sha1(std::span<const std::uint8_t> data);
[[nodiscard]] std::string calculate_sha256(std::span<const std::uint8_t> data);
[[nodiscard]] RomIdentity identify_rom(std::span<const std::uint8_t> rom);
[[nodiscard]] std::string_view to_string(RomSupportStatus status) noexcept;

} // namespace oasis
