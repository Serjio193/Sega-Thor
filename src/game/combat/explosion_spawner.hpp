#pragma once

#include <cstdint>
#include <span>

namespace oasis::game::combat {

inline constexpr std::uint32_t kSpawnExplosionNormalAddr = 0x00E90EU;
inline constexpr std::uint32_t kSpawnExplosionHiAddr = 0x00E93AU;
inline constexpr std::uint32_t kInitExplosionAndSfxAddr = 0x00E96CU;
inline constexpr std::uint32_t kSyncShadePhaseFlagsAddr = 0x00E9DEU;

inline constexpr std::uint16_t kExplosionEntityType = 0x0034U;
inline constexpr std::uint16_t kExplosionSpriteBaseId = 0x055EU;
inline constexpr std::uint16_t kExplosionSpriteHiMask = 0x8000U;
inline constexpr std::uint16_t kExplosionSpriteHiId = 0x855EU;
inline constexpr std::uint16_t kExplosionSfxCooldownFrames = 7U;
inline constexpr std::uint32_t kExplosionSfx1 = 0x0090021BU;
inline constexpr std::uint32_t kExplosionSfx2 = 0x00600382U;
inline constexpr std::uint16_t kShadeSpiritId = 0x001EU;
inline constexpr std::size_t kEntityStructMinSize = 0xBCU; // 188 bytes

struct ExplosionSpawnRequest {
    std::int16_t x{0};
    std::int16_t y{0};
    std::int16_t z{0};
    std::uint16_t param_9e{0U};
    bool hi_variant{false};
};

struct ExplosionEntityState {
    std::uint16_t entity_type{kExplosionEntityType};
    std::int32_t pos_x_16_16{0};
    std::int32_t pos_y_16_16{0};
    std::int32_t pos_z_16_16{0};
    std::uint16_t timer_16{0U};
    std::uint16_t sprite_id{kExplosionSpriteBaseId};
    std::uint8_t flags_36{0U};
    std::uint8_t flags_38{0U};
    std::int32_t base_x_16_16{0};
    std::int32_t base_y_16_16{0};
    std::uint16_t param_9e{0U};
};

struct ExplosionSpawnResult {
    bool spawned{false};
    ExplosionEntityState entity{};
    bool sfx_played{false};
    std::uint32_t sfx_1{0U};
    std::uint32_t sfx_2{0U};
};

/// Evaluates 0x00E9AA..0x00E9DC: signed 16-bit negative delta (bmi) or delta >= 7
/// resets the cooldown timer to current_frame and triggers explosion SFX.
[[nodiscard]] bool check_and_update_explosion_sfx_cooldown(
    std::uint16_t current_frame,
    std::uint16_t& last_sfx_frame) noexcept;

/// Evaluates 0x00E9DE..0x00EA02: sets bit 1 of flags_36 and bit 2 of flags_38
/// when active spirit is Shade (0x1E), clears them otherwise.
void sync_shade_phase_flags(
    std::uint16_t active_spirit_id,
    std::uint8_t& flags_36,
    std::uint8_t& flags_38) noexcept;

/// Dispatches complete explosion entity creation and SFX cooldown logic.
[[nodiscard]] ExplosionSpawnResult spawn_explosion(
    const ExplosionSpawnRequest& request,
    std::uint16_t current_frame,
    std::uint16_t& last_sfx_frame,
    bool slot_available = true,
    std::uint16_t active_spirit_id = 0U) noexcept;

/// Helper for normal explosion variant (0x00E90E..0x00E938).
[[nodiscard]] ExplosionSpawnResult spawn_explosion_normal(
    std::int16_t x,
    std::int16_t y,
    std::int16_t z,
    std::uint16_t param_9e,
    std::uint16_t current_frame,
    std::uint16_t& last_sfx_frame,
    bool slot_available = true,
    std::uint16_t active_spirit_id = 0U) noexcept;

/// Helper for hi explosion variant (0x00E93A..0x00E96A).
[[nodiscard]] ExplosionSpawnResult spawn_explosion_hi(
    std::int16_t x,
    std::int16_t y,
    std::int16_t z,
    std::uint16_t param_9e,
    std::uint16_t current_frame,
    std::uint16_t& last_sfx_frame,
    bool slot_available = true,
    std::uint16_t active_spirit_id = 0U) noexcept;

/// Directly writes initialized explosion entity fields into a 0xBC-byte entity slot.
bool write_explosion_to_entity_slot(
    std::span<std::uint8_t> slot_bytes,
    const ExplosionSpawnRequest& request,
    std::uint16_t active_spirit_id = 0U) noexcept;

} // namespace oasis::game::combat
