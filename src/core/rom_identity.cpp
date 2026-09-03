#include "core/rom_identity.hpp"

#include <array>
#include <bit>
#include <iomanip>
#include <sstream>

namespace oasis {
namespace {

std::string read_text(std::span<const std::uint8_t> data, std::size_t offset, std::size_t length) {
    if (data.size() < offset + length) return {};
    std::string value(reinterpret_cast<const char*>(data.data() + offset), length);
    while (!value.empty() && (value.back() == ' ' || value.back() == '\0')) value.pop_back();
    return value;
}

std::uint16_t read_be16(std::span<const std::uint8_t> data, std::size_t offset) {
    if (data.size() < offset + 2) return 0;
    return static_cast<std::uint16_t>((data[offset] << 8U) | data[offset + 1]);
}

std::uint32_t read_be32(std::span<const std::uint8_t> data, std::size_t offset) {
    if (data.size() < offset + 4) return 0;
    return (static_cast<std::uint32_t>(data[offset]) << 24U) |
           (static_cast<std::uint32_t>(data[offset + 1]) << 16U) |
           (static_cast<std::uint32_t>(data[offset + 2]) << 8U) |
           static_cast<std::uint32_t>(data[offset + 3]);
}

std::string hex_words(std::span<const std::uint32_t> words) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (const auto word : words) out << std::setw(8) << word;
    return out.str();
}

void sha1_transform(std::array<std::uint32_t, 5>& state, const std::uint8_t* block) {
    std::array<std::uint32_t, 80> w{};
    for (std::size_t i = 0; i < 16; ++i) {
        w[i] = (static_cast<std::uint32_t>(block[i * 4]) << 24U) |
               (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16U) |
               (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8U) |
               static_cast<std::uint32_t>(block[i * 4 + 3]);
    }
    for (std::size_t i = 16; i < 80; ++i) {
        w[i] = std::rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    auto a = state[0]; auto b = state[1]; auto c = state[2]; auto d = state[3]; auto e = state[4];
    for (std::size_t i = 0; i < 80; ++i) {
        std::uint32_t f{};
        std::uint32_t k{};
        if (i < 20) { f = (b & c) | ((~b) & d); k = 0x5A827999U; }
        else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1U; }
        else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDCU; }
        else { f = b ^ c ^ d; k = 0xCA62C1D6U; }
        const auto temp = std::rotl(a, 5) + f + e + k + w[i];
        e = d; d = c; c = std::rotl(b, 30); b = a; a = temp;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
}

constexpr std::array<std::uint32_t, 64> kSha256Constants{
    0x428a2f98U,0x71374491U,0xb5c0fbcfU,0xe9b5dba5U,0x3956c25bU,0x59f111f1U,0x923f82a4U,0xab1c5ed5U,
    0xd807aa98U,0x12835b01U,0x243185beU,0x550c7dc3U,0x72be5d74U,0x80deb1feU,0x9bdc06a7U,0xc19bf174U,
    0xe49b69c1U,0xefbe4786U,0x0fc19dc6U,0x240ca1ccU,0x2de92c6fU,0x4a7484aaU,0x5cb0a9dcU,0x76f988daU,
    0x983e5152U,0xa831c66dU,0xb00327c8U,0xbf597fc7U,0xc6e00bf3U,0xd5a79147U,0x06ca6351U,0x14292967U,
    0x27b70a85U,0x2e1b2138U,0x4d2c6dfcU,0x53380d13U,0x650a7354U,0x766a0abbU,0x81c2c92eU,0x92722c85U,
    0xa2bfe8a1U,0xa81a664bU,0xc24b8b70U,0xc76c51a3U,0xd192e819U,0xd6990624U,0xf40e3585U,0x106aa070U,
    0x19a4c116U,0x1e376c08U,0x2748774cU,0x34b0bcb5U,0x391c0cb3U,0x4ed8aa4aU,0x5b9cca4fU,0x682e6ff3U,
    0x748f82eeU,0x78a5636fU,0x84c87814U,0x8cc70208U,0x90befffaU,0xa4506cebU,0xbef9a3f7U,0xc67178f2U};

void sha256_transform(std::array<std::uint32_t, 8>& state, const std::uint8_t* block) {
    std::array<std::uint32_t, 64> w{};
    for (std::size_t i = 0; i < 16; ++i) {
        w[i] = (static_cast<std::uint32_t>(block[i * 4]) << 24U) |
               (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16U) |
               (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8U) |
               static_cast<std::uint32_t>(block[i * 4 + 3]);
    }
    for (std::size_t i = 16; i < 64; ++i) {
        const auto s0 = std::rotr(w[i - 15], 7) ^ std::rotr(w[i - 15], 18) ^ (w[i - 15] >> 3U);
        const auto s1 = std::rotr(w[i - 2], 17) ^ std::rotr(w[i - 2], 19) ^ (w[i - 2] >> 10U);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    auto a = state[0]; auto b = state[1]; auto c = state[2]; auto d = state[3];
    auto e = state[4]; auto f = state[5]; auto g = state[6]; auto h = state[7];
    for (std::size_t i = 0; i < 64; ++i) {
        const auto s1 = std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
        const auto ch = (e & f) ^ ((~e) & g);
        const auto temp1 = h + s1 + ch + kSha256Constants[i] + w[i];
        const auto s0 = std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
        const auto maj = (a & b) ^ (a & c) ^ (b & c);
        const auto temp2 = s0 + maj;
        h = g; g = f; f = e; e = d + temp1; d = c; c = b; b = a; a = temp1 + temp2;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

template <std::size_t N, typename Transform>
std::string calculate_merkle_damgard_hash(
    std::span<const std::uint8_t> data,
    std::array<std::uint32_t, N> state,
    Transform transform) {
    std::size_t offset = 0;
    while (offset + 64 <= data.size()) {
        transform(state, data.data() + offset);
        offset += 64;
    }
    std::array<std::uint8_t, 128> tail{};
    const auto remaining = data.size() - offset;
    for (std::size_t i = 0; i < remaining; ++i) tail[i] = data[offset + i];
    tail[remaining] = 0x80;
    const std::size_t total_tail = remaining < 56 ? 64 : 128;
    const auto bit_length = static_cast<std::uint64_t>(data.size()) * 8U;
    for (int i = 0; i < 8; ++i) tail[total_tail - 1 - i] = static_cast<std::uint8_t>(bit_length >> (i * 8));
    transform(state, tail.data());
    if (total_tail == 128) transform(state, tail.data() + 64);
    return hex_words(state);
}

} // namespace

MegaDriveHeader parse_mega_drive_header(std::span<const std::uint8_t> rom) {
    MegaDriveHeader header;
    if (rom.size() < 0x200) return header;
    header.console_name = read_text(rom, 0x100, 16);
    header.domestic_title = read_text(rom, 0x120, 48);
    header.international_title = read_text(rom, 0x150, 48);
    header.product_code = read_text(rom, 0x180, 14);
    header.stored_checksum = read_be16(rom, 0x18E);
    header.rom_start = read_be32(rom, 0x1A0);
    header.rom_end = read_be32(rom, 0x1A4);
    header.region = read_text(rom, 0x1F0, 16);
    header.header_signature_valid = header.console_name.rfind("SEGA", 0) == 0;
    return header;
}

std::uint16_t calculate_sega_checksum(std::span<const std::uint8_t> rom) {
    std::uint32_t sum = 0;
    for (std::size_t i = 0x200; i + 1 < rom.size(); i += 2) {
        sum += static_cast<std::uint16_t>((rom[i] << 8U) | rom[i + 1]);
    }
    return static_cast<std::uint16_t>(sum & 0xFFFFU);
}

std::uint32_t calculate_crc32(std::span<const std::uint8_t> data) {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (const auto byte : data) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1U) ^ (0xEDB88320U & (0U - (crc & 1U)));
        }
    }
    return ~crc;
}

std::string calculate_sha1(std::span<const std::uint8_t> data) {
    return calculate_merkle_damgard_hash(
        data,
        std::array<std::uint32_t, 5>{0x67452301U,0xEFCDAB89U,0x98BADCFEU,0x10325476U,0xC3D2E1F0U},
        sha1_transform);
}

std::string calculate_sha256(std::span<const std::uint8_t> data) {
    return calculate_merkle_damgard_hash(
        data,
        std::array<std::uint32_t, 8>{0x6a09e667U,0xbb67ae85U,0x3c6ef372U,0xa54ff53aU,
                                     0x510e527fU,0x9b05688cU,0x1f83d9abU,0x5be0cd19U},
        sha256_transform);
}

RomIdentity identify_rom(std::span<const std::uint8_t> rom) {
    RomIdentity result;
    result.header = parse_mega_drive_header(rom);
    result.fingerprint.size = rom.size();
    result.fingerprint.crc32 = calculate_crc32(rom);
    result.fingerprint.sha1 = calculate_sha1(rom);
    result.fingerprint.sha256 = calculate_sha256(rom);
    result.fingerprint.calculated_sega_checksum = calculate_sega_checksum(rom);
    result.fingerprint.sega_checksum_valid =
        result.header.stored_checksum == result.fingerprint.calculated_sega_checksum;

    constexpr std::size_t kReferenceSize = 3U * 1024U * 1024U;
    constexpr std::uint32_t kUsaCrc32 = 0xC4728225U;
    constexpr std::string_view kUsaSha1 = "2944910c07c02eace98c17d78d07bef7859d386a";
    constexpr std::uint32_t kEuropeCrc32 = 0x1110B0DBU;

    if (rom.size() == kReferenceSize &&
        result.fingerprint.crc32 == kUsaCrc32 &&
        result.fingerprint.sha1 == kUsaSha1) {
        result.id = "beyond-oasis-usa-retail";
        result.display_name = "Beyond Oasis (USA retail)";
        result.status = RomSupportStatus::Supported;
    } else if (rom.size() == kReferenceSize && result.fingerprint.crc32 == kEuropeCrc32) {
        result.id = "story-of-thor-europe-retail";
        result.display_name = "The Story of Thor (Europe retail)";
        result.status = RomSupportStatus::KnownUnsupported;
    } else if (rom.size() == kReferenceSize &&
               result.header.header_signature_valid &&
               (result.header.international_title.find("BEYOND OASIS") != std::string::npos ||
                result.header.domestic_title.find("BEYOND OASIS") != std::string::npos)) {
        result.id = "probable-beyond-oasis-modified";
        result.display_name = "Probable modified Beyond Oasis ROM";
        result.status = RomSupportStatus::Modified;
    } else {
        result.id = "unknown";
        result.display_name = "Unknown ROM";
        result.status = RomSupportStatus::Unknown;
    }
    return result;
}

std::string_view to_string(RomSupportStatus status) noexcept {
    switch (status) {
        case RomSupportStatus::Supported: return "SUPPORTED";
        case RomSupportStatus::KnownUnsupported: return "KNOWN_UNSUPPORTED";
        case RomSupportStatus::Modified: return "MODIFIED";
        case RomSupportStatus::Unknown: return "UNKNOWN";
    }
    return "UNKNOWN";
}

} // namespace oasis
