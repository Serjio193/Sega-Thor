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

// These offsets are intentionally raw. Their meanings are only promoted when
// a caller proves them; the common movement entry reads the listed bytes.
inline constexpr std::size_t kEntityFieldType = 0x00;
inline constexpr std::size_t kEntityFieldWord10 = 0x10;
inline constexpr std::size_t kEntityFieldWord14 = 0x14;
inline constexpr std::size_t kEntityFieldPositionX = 0x08;
inline constexpr std::size_t kEntityFieldPositionY = 0x0C;
inline constexpr std::size_t kEntityFieldWord42 = 0x42;
inline constexpr std::size_t kEntityFieldWord44 = 0x44;
inline constexpr std::size_t kEntityFieldWord4A = 0x4A;
inline constexpr std::size_t kEntityFieldPointer22 = 0x22;
inline constexpr std::size_t kEntityFieldPointer26 = 0x26;
inline constexpr std::size_t kEntityFieldWord2A = 0x2A;
inline constexpr std::size_t kEntityFieldWord2C = 0x2C;
inline constexpr std::size_t kEntityFieldWord2E = 0x2E;
inline constexpr std::size_t kEntityFieldWord30 = 0x30;
inline constexpr std::size_t kEntityFieldWord32 = 0x32;
inline constexpr std::size_t kEntityFieldFlags37 = 0x37;
inline constexpr std::size_t kEntityFieldFlags38 = 0x38;
inline constexpr std::size_t kEntityFieldDeltaX = 0x72;
inline constexpr std::size_t kEntityFieldDeltaY = 0x76;
inline constexpr std::size_t kEntityFieldCounter9C = 0x9C;
inline constexpr std::size_t kEntityFieldCounter9D = 0x9D;

class EntityRecordView {
public:
    explicit EntityRecordView(std::span<const std::uint8_t> bytes) noexcept
        : bytes_(bytes) {}

    [[nodiscard]] std::optional<std::uint16_t> read_u16(
        std::size_t offset) const noexcept;
    [[nodiscard]] std::optional<std::uint32_t> read_u32(
        std::size_t offset) const noexcept;
    [[nodiscard]] std::optional<std::uint32_t> pointer22() const noexcept {
        return read_u32(kEntityFieldPointer22);
    }

private:
    std::span<const std::uint8_t> bytes_;
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

    [[nodiscard]] std::optional<EntityRecordView> record_view(
        std::size_t index) const noexcept;

    [[nodiscard]] bool active(std::size_t index) const noexcept;

private:
    std::span<const std::uint8_t> storage_;
    EntityPoolSpec spec_{};
};

} // namespace oasis::game::entities
