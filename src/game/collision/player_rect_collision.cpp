#include "game/collision/player_rect_collision.hpp"

namespace oasis::game::collision {

namespace {

[[nodiscard]] inline std::uint16_t read_be16(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 1U >= buf.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(buf[offset]) << 8U) |
        static_cast<std::uint16_t>(buf[offset + 1U]));
}

[[nodiscard]] inline std::int16_t read_be16_signed(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    return static_cast<std::int16_t>(read_be16(buf, offset));
}

inline void write_be16(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t value) noexcept {
    if (offset + 1U < buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
    }
}

inline void write_be16_signed(std::span<std::uint8_t> buf, std::size_t offset, std::int16_t value) noexcept {
    write_be16(buf, offset, static_cast<std::uint16_t>(value));
}

[[nodiscard]] inline std::size_t to_ram_offset(std::uint32_t addr) noexcept {
    if (addr >= 0x00FF0000U && addr <= 0x00FFFFFFU) {
        return static_cast<std::size_t>(addr - 0x00FF0000U);
    }
    return static_cast<std::size_t>(addr & 0xFFFFU);
}

} // namespace

PlayerCollisionState read_player_collision_raw(std::span<const std::uint8_t> ram) {
    const std::size_t off = to_ram_offset(PlayerRectCollisionConfig::kPlayerEntityRam);
    if (off + 0x48U > ram.size()) {
        return PlayerCollisionState{};
    }

    return PlayerCollisionState{
        .world_x = read_be16_signed(ram, off + PlayerRectCollisionConfig::kPlayerWorldXOffset),
        .world_y = read_be16_signed(ram, off + PlayerRectCollisionConfig::kPlayerWorldYOffset),
        .radius = read_be16_signed(ram, off + PlayerRectCollisionConfig::kPlayerCollisionRadOffset)
    };
}

void write_player_collision_raw(std::span<std::uint8_t> ram, const PlayerCollisionState& player) {
    const std::size_t off = to_ram_offset(PlayerRectCollisionConfig::kPlayerEntityRam);
    if (off + 0x48U > ram.size()) {
        return;
    }

    write_be16_signed(ram, off + PlayerRectCollisionConfig::kPlayerWorldXOffset, player.world_x);
    write_be16_signed(ram, off + PlayerRectCollisionConfig::kPlayerWorldYOffset, player.world_y);
    write_be16_signed(ram, off + PlayerRectCollisionConfig::kPlayerCollisionRadOffset, player.radius);
}

bool is_player_inside_rect_raw(std::span<const std::uint8_t> ram, const TileRect& rect) {
    const PlayerCollisionState player = read_player_collision_raw(ram);
    return is_player_inside_rect(rect, player);
}

bool does_player_overlap_rect_raw(std::span<const std::uint8_t> ram, const TileRect& rect) {
    const PlayerCollisionState player = read_player_collision_raw(ram);
    return does_player_overlap_rect(rect, player);
}

bool is_player_inside_packed_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t reg_d1,
    std::uint32_t reg_d2) {
    const PlayerCollisionState player = read_player_collision_raw(ram);
    return is_player_inside_packed(reg_d1, reg_d2, player);
}

bool does_player_overlap_packed_raw(
    std::span<const std::uint8_t> ram,
    std::uint32_t reg_d1,
    std::uint32_t reg_d2) {
    const PlayerCollisionState player = read_player_collision_raw(ram);
    return does_player_overlap_packed(reg_d1, reg_d2, player);
}

} // namespace oasis::game::collision
