#include "explosion_spawner.hpp"

namespace oasis::game::combat {

namespace {

void write_be16(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t val) noexcept {
    buf[offset] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
    buf[offset + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
}

void write_be32(std::span<std::uint8_t> buf, std::size_t offset, std::int32_t val) noexcept {
    const auto uval = static_cast<std::uint32_t>(val);
    buf[offset] = static_cast<std::uint8_t>((uval >> 24U) & 0xFFU);
    buf[offset + 1U] = static_cast<std::uint8_t>((uval >> 16U) & 0xFFU);
    buf[offset + 2U] = static_cast<std::uint8_t>((uval >> 8U) & 0xFFU);
    buf[offset + 3U] = static_cast<std::uint8_t>(uval & 0xFFU);
}

} // namespace

bool check_and_update_explosion_sfx_cooldown(
    std::uint16_t current_frame,
    std::uint16_t& last_sfx_frame) noexcept {
    const auto delta_u16 = static_cast<std::uint16_t>(current_frame - last_sfx_frame);
    const auto delta_s16 = static_cast<std::int16_t>(delta_u16);

    // ROM 0x00E9B6..0x00E9BC: bmi -> play, cmpi.w #7, d0; bcs -> skip
    if (delta_s16 < 0 || delta_u16 >= kExplosionSfxCooldownFrames) {
        last_sfx_frame = current_frame;
        return true;
    }
    return false;
}

void sync_shade_phase_flags(
    std::uint16_t active_spirit_id,
    std::uint8_t& flags_36,
    std::uint8_t& flags_38) noexcept {
    // ROM 0x00E9DE..0x00EA02
    if (active_spirit_id == kShadeSpiritId) {
        flags_36 |= 0x02U;
        flags_38 |= 0x04U;
    } else {
        flags_36 &= static_cast<std::uint8_t>(~0x02U);
        flags_38 &= static_cast<std::uint8_t>(~0x04U);
    }
}

ExplosionSpawnResult spawn_explosion(
    const ExplosionSpawnRequest& request,
    std::uint16_t current_frame,
    std::uint16_t& last_sfx_frame,
    bool slot_available,
    std::uint16_t active_spirit_id) noexcept {
    ExplosionSpawnResult result{};
    if (!slot_available) {
        return result; // 0x00E916 / 0x00E942: bcs carry set -> abort
    }

    result.spawned = true;
    result.entity.entity_type = kExplosionEntityType;
    result.entity.pos_x_16_16 = static_cast<std::int32_t>(request.x) << 16;
    result.entity.pos_y_16_16 = static_cast<std::int32_t>(request.y) << 16;
    result.entity.pos_z_16_16 = static_cast<std::int32_t>(request.z) << 16;
    result.entity.timer_16 = 0U;
    result.entity.sprite_id = request.hi_variant
        ? static_cast<std::uint16_t>(kExplosionSpriteBaseId | kExplosionSpriteHiMask)
        : kExplosionSpriteBaseId;
    result.entity.base_x_16_16 = result.entity.pos_x_16_16;
    result.entity.base_y_16_16 = result.entity.pos_y_16_16;
    result.entity.param_9e = request.param_9e;

    sync_shade_phase_flags(
        active_spirit_id,
        result.entity.flags_36,
        result.entity.flags_38);

    if (check_and_update_explosion_sfx_cooldown(current_frame, last_sfx_frame)) {
        result.sfx_played = true;
        result.sfx_1 = kExplosionSfx1;
        result.sfx_2 = kExplosionSfx2;
    }

    return result;
}

ExplosionSpawnResult spawn_explosion_normal(
    std::int16_t x,
    std::int16_t y,
    std::int16_t z,
    std::uint16_t param_9e,
    std::uint16_t current_frame,
    std::uint16_t& last_sfx_frame,
    bool slot_available,
    std::uint16_t active_spirit_id) noexcept {
    return spawn_explosion(
        ExplosionSpawnRequest{x, y, z, param_9e, false},
        current_frame,
        last_sfx_frame,
        slot_available,
        active_spirit_id);
}

ExplosionSpawnResult spawn_explosion_hi(
    std::int16_t x,
    std::int16_t y,
    std::int16_t z,
    std::uint16_t param_9e,
    std::uint16_t current_frame,
    std::uint16_t& last_sfx_frame,
    bool slot_available,
    std::uint16_t active_spirit_id) noexcept {
    return spawn_explosion(
        ExplosionSpawnRequest{x, y, z, param_9e, true},
        current_frame,
        last_sfx_frame,
        slot_available,
        active_spirit_id);
}

bool write_explosion_to_entity_slot(
    std::span<std::uint8_t> slot_bytes,
    const ExplosionSpawnRequest& request,
    std::uint16_t active_spirit_id) noexcept {
    if (slot_bytes.size() < kEntityStructMinSize) {
        return false;
    }

    const auto pos_x = static_cast<std::int32_t>(request.x) << 16;
    const auto pos_y = static_cast<std::int32_t>(request.y) << 16;
    const auto pos_z = static_cast<std::int32_t>(request.z) << 16;
    const auto sprite_id = request.hi_variant
        ? static_cast<std::uint16_t>(kExplosionSpriteBaseId | kExplosionSpriteHiMask)
        : kExplosionSpriteBaseId;

    write_be16(slot_bytes, 0x00U, kExplosionEntityType);
    write_be32(slot_bytes, 0x08U, pos_x);
    write_be32(slot_bytes, 0x0CU, pos_y);
    write_be32(slot_bytes, 0x10U, pos_z);
    write_be16(slot_bytes, 0x16U, 0U);
    write_be16(slot_bytes, 0x18U, sprite_id);

    std::uint8_t f36 = slot_bytes[0x36U];
    std::uint8_t f38 = slot_bytes[0x38U];
    sync_shade_phase_flags(active_spirit_id, f36, f38);
    slot_bytes[0x36U] = f36;
    slot_bytes[0x38U] = f38;

    write_be32(slot_bytes, 0x4EU, pos_x);
    write_be32(slot_bytes, 0x52U, pos_y);
    write_be16(slot_bytes, 0x9EU, request.param_9e);

    return true;
}

} // namespace oasis::game::combat
