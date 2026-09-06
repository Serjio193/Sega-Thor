#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::render {

class SoftwareFramebuffer {
public:
    static constexpr std::uint32_t kWidth = 320;
    static constexpr std::uint32_t kHeight = 224;

    SoftwareFramebuffer();

    [[nodiscard]] std::span<std::uint32_t> pixels() noexcept { return pixels_; }
    [[nodiscard]] std::span<const std::uint32_t> pixels() const noexcept {
        return pixels_;
    }

private:
    std::vector<std::uint32_t> pixels_;
};

} // namespace oasis::game::render
