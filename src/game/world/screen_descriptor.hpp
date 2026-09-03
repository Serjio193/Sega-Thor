#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::world {

struct ScreenId {
    std::uint8_t group{};
    std::uint8_t index{};
};

struct ScreenDescriptorRaw {
    static constexpr std::size_t kSize = 26;

    std::uint32_t rom_address{};
    std::array<std::uint8_t, kSize> bytes{};
    std::uint32_t init_function{};

    [[nodiscard]] std::uint32_t primary_stream_pointer() const noexcept;
    [[nodiscard]] std::array<std::uint8_t, 4> resource_ids() const noexcept;
    [[nodiscard]] std::array<std::int8_t, 4> signed_parameters() const noexcept;
    [[nodiscard]] std::array<std::uint16_t, 5> trailing_words() const noexcept;
};

[[nodiscard]] ScreenDescriptorRaw load_screen_descriptor(
    std::span<const std::uint8_t> rom,
    ScreenId id);

} // namespace oasis::game::world
