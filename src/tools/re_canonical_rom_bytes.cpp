#include "tools/re_canonical_rom_bytes.hpp"

#include "core/rom.hpp"
#include "core/rom_identity.hpp"

#include <stdexcept>

namespace oasis::tools {

CanonicalRomBytes CanonicalRomBytes::load(const std::filesystem::path& path) {
    auto rom = Rom::load(path);
    if (rom.size() != kSize) throw std::runtime_error("STOP_CANONICAL_ROM_SIZE_MISMATCH");
    const auto identity = identify_rom(rom.bytes());
    if (identity.fingerprint.sha256 != kSha256)
        throw std::runtime_error("STOP_CANONICAL_ROM_SHA256_MISMATCH");
    return CanonicalRomBytes(rom.bytes(), identity.fingerprint.sha256);
}

std::span<const std::uint8_t> CanonicalRomBytes::read(std::size_t offset,
                                                      std::size_t length) const {
    if (offset > bytes_.size() || length > bytes_.size() - offset)
        throw std::out_of_range("STOP_CANONICAL_ROM_READ_OUT_OF_BOUNDS");
    return std::span<const std::uint8_t>(bytes_).subspan(offset, length);
}

} // namespace oasis::tools
