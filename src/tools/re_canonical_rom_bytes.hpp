#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace oasis::tools {

class CanonicalRomBytes {
public:
    static constexpr std::size_t kSize = 3'145'728;
    static constexpr const char* kSha256 =
        "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263";

    [[nodiscard]] static CanonicalRomBytes load(const std::filesystem::path& path);
    [[nodiscard]] std::span<const std::uint8_t> read(std::size_t offset,
                                                     std::size_t length) const;
    [[nodiscard]] const std::string& sha256() const noexcept { return sha256_; }
    [[nodiscard]] std::size_t size() const noexcept { return bytes_.size(); }

private:
    CanonicalRomBytes(std::vector<std::uint8_t> bytes, std::string sha256)
        : bytes_(std::move(bytes)), sha256_(std::move(sha256)) {}
    std::vector<std::uint8_t> bytes_;
    std::string sha256_;
};

} // namespace oasis::tools
