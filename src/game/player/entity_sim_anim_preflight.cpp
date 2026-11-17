#include "game/player/entity_sim_anim_preflight.hpp"

namespace oasis::game::player {

namespace {

inline void write_u16_be(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t val) noexcept {
    if (offset + 2U <= buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

inline void write_i32_be(std::span<std::uint8_t> buf, std::size_t offset, std::int32_t val) noexcept {
    if (offset + 4U <= buf.size()) {
        auto u = static_cast<std::uint32_t>(val);
        buf[offset] = static_cast<std::uint8_t>((u >> 24U) & 0xFFU);
        buf[offset + 1U] = static_cast<std::uint8_t>((u >> 16U) & 0xFFU);
        buf[offset + 2U] = static_cast<std::uint8_t>((u >> 8U) & 0xFFU);
        buf[offset + 3U] = static_cast<std::uint8_t>(u & 0xFFU);
    }
}

inline std::uint16_t read_u16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 2U <= buf.size()) {
        return static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(buf[offset]) << 8U) |
            static_cast<std::uint16_t>(buf[offset + 1U]));
    }
    return 0U;
}

inline std::int32_t read_i32_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 4U <= buf.size()) {
        std::uint32_t u = (static_cast<std::uint32_t>(buf[offset]) << 24U) |
                          (static_cast<std::uint32_t>(buf[offset + 1U]) << 16U) |
                          (static_cast<std::uint32_t>(buf[offset + 2U]) << 8U) |
                          static_cast<std::uint32_t>(buf[offset + 3U]);
        return static_cast<std::int32_t>(u);
    }
    return 0;
}

} // namespace

FreezeSlowmoResult step_entity_freeze_slowmo(
    std::span<std::uint8_t> slot,
    bool global_freeze,
    bool global_slowmo) noexcept {
    FreezeSlowmoResult result{};
    if (slot.size() < 0x7AU) {
        return result;
    }

    std::uint16_t type = read_u16_be(slot, EntitySimConfig::kTypeOffset);
    std::uint16_t timer_9c = 0U;
    if (slot.size() > EntitySimConfig::kSubtimer9dOffset) {
        timer_9c = read_u16_be(slot, EntitySimConfig::kSubtimer9cOffset);
    }

    // 0x008F22..0x008F48: Subtimer 9D (freeze timer) & 9C (subtimer)
    if (slot.size() > EntitySimConfig::kSubtimer9dOffset) {
        std::uint8_t sub_9d = slot[EntitySimConfig::kSubtimer9dOffset];
        if (sub_9d > 0U) {
            slot[EntitySimConfig::kSubtimer9dOffset] = static_cast<std::uint8_t>(sub_9d - 1U);
            write_i32_be(slot, EntitySimConfig::kVelXOffset, 0);
            write_i32_be(slot, EntitySimConfig::kVelYOffset, 0);
            result.frozen = true;
            result.skip_anim = true;
            return result;
        }

        std::uint8_t sub_9c = slot[EntitySimConfig::kSubtimer9cOffset];
        if (sub_9c > 0U) {
            slot[EntitySimConfig::kSubtimer9cOffset] = static_cast<std::uint8_t>(sub_9c - 1U);
        }
    }

    // 0x008F4C..0x008F68: Global freeze check (for entity types > 2)
    if (global_freeze && type > 2U) {
        write_i32_be(slot, EntitySimConfig::kVelXOffset, 0);
        write_i32_be(slot, EntitySimConfig::kVelYOffset, 0);
        result.frozen = true;
        result.skip_anim = true;
        return result;
    }

    // 0x008F6C: Bit 0 of +0x37 skips animation stepper
    if (slot.size() > EntitySimConfig::kFlags37Offset) {
        result.skip_anim = (slot[EntitySimConfig::kFlags37Offset] & 0x01U) != 0U;
    }

    std::int32_t vx = read_i32_be(slot, EntitySimConfig::kVelXOffset);
    std::int32_t vy = read_i32_be(slot, EntitySimConfig::kVelYOffset);

    // 0x008F76..0x008FA0: Slow-motion check
    bool slowmo_active = (timer_9c != 0U) || (type > 2U && global_slowmo);
    if (slowmo_active) {
        vx >>= 1;
        vy >>= 1;
        write_i32_be(slot, EntitySimConfig::kVelXOffset, vx);
        write_i32_be(slot, EntitySimConfig::kVelYOffset, vy);
    }

    result.vel_x = vx;
    result.vel_y = vy;
    return result;
}

void step_entity_animation(
    std::span<std::uint8_t> slot,
    std::span<const std::uint8_t> anim_data) noexcept {
    if (slot.size() < 0x38U || anim_data.empty()) {
        return;
    }

    std::uint8_t flags_37 = slot[EntitySimConfig::kFlags37Offset];
    auto keyframe_timer = static_cast<std::int16_t>(read_u16_be(slot, EntitySimConfig::kKeyframeTimerOffset));
    std::uint16_t cur_state = read_u16_be(slot, EntitySimConfig::kStateOffset);

    // 0x008FB0: bclr.b #1, d2
    flags_37 &= static_cast<std::uint8_t>(~(1U << EntitySimConfig::kFlagLoopEndTriggerBit));

    // 0x008FB4: State transition check
    std::uint16_t prev_state = read_u16_be(slot, EntitySimConfig::kPrevStateOffset);
    if (cur_state != prev_state) {
        write_u16_be(slot, EntitySimConfig::kPrevStateOffset, cur_state);
        write_u16_be(slot, EntitySimConfig::kFrameOffsetOffset, static_cast<std::uint16_t>(0xFFFCU)); // -4
        keyframe_timer = 0;
        flags_37 &= static_cast<std::uint8_t>(~(1U << EntitySimConfig::kFlagLoopMarkBit));
    }

    // 0x008FD6: subq.w #1, d3
    keyframe_timer = static_cast<std::int16_t>(keyframe_timer - 1);

    if (keyframe_timer < 0) {
        // 0x008FF0: Advance to next frame
        auto frame_off = static_cast<std::int16_t>(read_u16_be(slot, EntitySimConfig::kFrameOffsetOffset));
        frame_off = static_cast<std::int16_t>(frame_off + 4);
        flags_37 &= static_cast<std::uint8_t>(~(1U << 3U)); // bclr.b #3, d2

        // If loop mark bit (bit 2) was set, loop back to frame 0
        if ((flags_37 & (1U << EntitySimConfig::kFlagLoopMarkBit)) != 0U) {
            flags_37 &= static_cast<std::uint8_t>(~(1U << EntitySimConfig::kFlagLoopMarkBit));
            frame_off = 0;
        }

        auto off = static_cast<std::size_t>(frame_off);
        if (off + 4U <= anim_data.size()) {
            std::uint16_t sprite_id = read_u16_be(anim_data, off);
            std::uint16_t raw_dur = read_u16_be(anim_data, off + 2U);

            write_u16_be(slot, EntitySimConfig::kSpriteAssetOffset, sprite_id);
            write_u16_be(slot, EntitySimConfig::kFrameOffsetOffset, static_cast<std::uint16_t>(frame_off));

            // Bit 15 of duration marks loop end point
            if ((raw_dur & 0x8000U) != 0U) {
                flags_37 |= static_cast<std::uint8_t>(1U << EntitySimConfig::kFlagLoopMarkBit);
            }

            std::uint16_t dur = static_cast<std::uint16_t>((raw_dur & 0x7FFFU) >> 1U);
            if (dur > 0U) {
                keyframe_timer = static_cast<std::int16_t>(dur - 1U);
            } else {
                keyframe_timer = 0;
            }

            if (keyframe_timer == 0 && (flags_37 & (1U << EntitySimConfig::kFlagLoopMarkBit)) != 0U) {
                flags_37 |= static_cast<std::uint8_t>(1U << EntitySimConfig::kFlagLoopEndTriggerBit);
            }
        }
    } else if (keyframe_timer == 0) {
        // 0x008FE0..0x008FE8: Timer reached 0 on loop frame
        if ((flags_37 & (1U << EntitySimConfig::kFlagLoopMarkBit)) != 0U) {
            flags_37 |= static_cast<std::uint8_t>(1U << EntitySimConfig::kFlagLoopEndTriggerBit);
        }
    }

    write_u16_be(slot, EntitySimConfig::kKeyframeTimerOffset, static_cast<std::uint16_t>(keyframe_timer));
    slot[EntitySimConfig::kFlags37Offset] = flags_37;
}

std::vector<std::uint8_t> pack_anim_sequence(
    std::span<const AnimKeyframe> keyframes) {
    std::vector<std::uint8_t> buf(keyframes.size() * 4U, 0U);
    for (std::size_t i = 0U; i < keyframes.size(); ++i) {
        write_u16_be(buf, i * 4U, keyframes[i].sprite_id);
        write_u16_be(buf, i * 4U + 2U, keyframes[i].raw_duration);
    }
    return buf;
}

TerrainPreflightResult preflight_terrain_collision(
    std::span<std::uint8_t> slot,
    bool layer_select_bit7) noexcept {
    TerrainPreflightResult result{};
    if (slot.size() < 0x76U) {
        return result;
    }

    // 0x009040..0x009046: Clear collision result flags
    slot[EntitySimConfig::kCollision40Offset] = 0U;
    slot[EntitySimConfig::kCollision41Offset] = 0U;

    // 0x00904A: Bit 4 of flags_37 is intangible check
    if ((slot[EntitySimConfig::kFlags37Offset] & (1U << EntitySimConfig::kFlagIntangibleBit)) != 0U) {
        result.action = TerrainPreflightAction::BypassIntangible;
        return result;
    }

    // 0x009052..0x009062: Layer tilemap selection
    result.layer_tilemap_ram = layer_select_bit7
        ? EntitySimConfig::kLayer2TilemapRam
        : EntitySimConfig::kLayer1TilemapRam;

    // 0x009068: Bit 1 of +0x38 is flying check
    if ((slot[EntitySimConfig::kFlags38Offset] & (1U << EntitySimConfig::kFlagFlyingBit)) != 0U) {
        result.action = TerrainPreflightAction::Flying;
        return result;
    }

    // 0x009078..0x009084: Velocity X evaluation
    std::int32_t vx = read_i32_be(slot, EntitySimConfig::kVelXOffset);
    if (vx == 0) {
        result.action = TerrainPreflightAction::SweepY;
    } else if (vx < 0) {
        result.action = TerrainPreflightAction::SweepNegX;
    } else {
        slot[EntitySimConfig::kCollision40Offset] = EntitySimConfig::kMoveCodePosX;
        result.action = TerrainPreflightAction::SweepPosX;
        result.move_code = EntitySimConfig::kMoveCodePosX;
    }

    return result;
}

} // namespace oasis::game::player
