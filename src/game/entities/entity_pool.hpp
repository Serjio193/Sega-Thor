#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace oasis::game::entities {

struct EntityPoolSpec {
    std::uint32_t base_address{};
    std::uint16_t record_count{};
    std::uint16_t record_stride{};
    std::uint32_t dispatcher_address{};
    std::uint32_t movement_entry_address{};
};

inline constexpr EntityPoolSpec kEntityPoolAtFf2954{
    0x00FF2954, 4, 0x5A, 0x00008EAA, 0x00008F12};
inline constexpr EntityPoolSpec kEntityPoolAtFf19e8{
    0x00FF19E8, 21, 0xBC, 0x00008ECC, 0x00008F22};
inline constexpr EntityPoolSpec kEntityPoolAtFf2d8c{
    0x00FF2D8C, 6, 0x5A, 0x00008EEE, 0x00008F12};

class EntityPoolView {
public:
    EntityPoolView(std::span<const std::uint8_t> storage,
                   EntityPoolSpec spec) noexcept
        : storage_(storage), spec_(spec) {}

    [[nodiscard]] const EntityPoolSpec& spec() const noexcept { return spec_; }

    [[nodiscard]] std::optional<std::span<const std::uint8_t>> record(
        std::size_t index) const noexcept;

    [[nodiscard]] bool active(std::size_t index) const noexcept;

private:
    std::span<const std::uint8_t> storage_;
    EntityPoolSpec spec_{};
};

} // namespace oasis::game::entities
