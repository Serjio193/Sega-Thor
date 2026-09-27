#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace oasis::thor::runtime_map {

// Bits are observed facts about use of ROM bytes. Zero means unproven.
enum Property : std::uint16_t {
    M68K_EXECUTED_ENCODING = 1U << 0U,
    Z80_EXECUTED_ENCODING = 1U << 1U,
    M68K_DATA_READ = 1U << 2U,
    Z80_DATA_READ = 1U << 3U,
    VDP_VRAM_SOURCE = 1U << 4U,
    VDP_CRAM_SOURCE = 1U << 5U,
    VDP_VSRAM_SOURCE = 1U << 6U,
    AUDIO_PAYLOAD_PROVEN = 1U << 7U,
    COMPRESSED_GRAPHICS_SOURCE = 1U << 8U,
};
inline constexpr std::uint16_t ALL_PROPERTIES = 0x01ffU;
inline constexpr std::string_view SCHEMA_ID = "thor.rom-properties.v1";
inline constexpr std::string_view CONTRACT_SHA256 =
    "c88eb4dcc273b55681bc1d0fc04e483b4d200d292b054488f7c87348b2c07842";
inline constexpr std::size_t DIRTY_PAGE_BYTES = 4096;

struct Identity {
    std::string rom_sha256;
    std::uint64_t rom_size{};
    std::string schema_id;
    std::string contract_sha256;
    std::string core_build_id;
    std::string run_id;
    std::uint64_t generation{};
    std::uint64_t capabilities{};
    std::string validation_state;
};

struct Range {
    std::uint64_t start{};
    std::uint64_t end{}; // exclusive
    std::uint16_t properties{};
};

class PropertyMap {
public:
    explicit PropertyMap(std::size_t rom_size);
    [[nodiscard]] std::size_t size() const noexcept { return properties_.size(); }
    [[nodiscard]] std::uint16_t at(std::size_t offset) const;
    [[nodiscard]] const std::vector<std::uint16_t>& bytes() const noexcept { return properties_; }
    [[nodiscard]] const std::vector<std::uint8_t>& dirty_pages() const noexcept { return dirty_; }
    void clear_dirty() noexcept;

    // Caller must establish the property's proof contract before this low-level operation.
    void apply_validated(std::uint64_t start, std::uint64_t length, std::uint16_t property);
    void merge_validated(const PropertyMap& other);
    [[nodiscard]] std::vector<Range> ranges() const;
    [[nodiscard]] static PropertyMap from_ranges(std::size_t rom_size,
                                                  const std::vector<Range>& ranges);

private:
    std::vector<std::uint16_t> properties_;
    std::vector<std::uint8_t> dirty_;
};

struct Contribution {
    Identity identity;
    std::vector<std::uint16_t> properties;
};

// Compatibility is deliberately exact. Cross-build compatibility needs a new contract.
void validate_identity(const Identity& identity);
void require_compatible(const Identity& a, const Identity& b);
[[nodiscard]] PropertyMap rebuild_union(const std::vector<Contribution>& runs,
                                        const Identity& target);
void save_contribution(const std::filesystem::path& path, const Contribution& contribution);
[[nodiscard]] Contribution load_contribution(const std::filesystem::path& path);

[[nodiscard]] std::vector<std::uint8_t> encode_checkpoint(const Identity& identity,
                                                          const PropertyMap& map);
[[nodiscard]] PropertyMap decode_checkpoint(const std::vector<std::uint8_t>& file,
                                            Identity& identity);
void save_checkpoint(const std::filesystem::path& path, const Identity& identity,
                     const PropertyMap& map);
[[nodiscard]] PropertyMap load_checkpoint(const std::filesystem::path& path,
                                          Identity& identity);

} // namespace oasis::thor::runtime_map
