#include "game/enemy/enemy_ai_0030_0048.hpp"

#include <algorithm>
#include <cstdlib>

namespace oasis::game::enemy {

bool Charger0048Ai::check_player_in_aabb(
    std::int32_t ent_x,
    std::int32_t ent_y,
    std::uint16_t facing,
    std::int32_t player_x,
    std::int32_t player_y,
    int cell_px) noexcept {
    const auto& box = kDirectionalAabbs[facing & 3U];
    const std::int32_t min_x = ent_x + static_cast<std::int32_t>(box.dx1) * cell_px;
    const std::int32_t max_x = ent_x + static_cast<std::int32_t>(box.dx2) * cell_px;
    const std::int32_t min_y = ent_y + static_cast<std::int32_t>(box.dy1) * cell_px;
    const std::int32_t max_y = ent_y + static_cast<std::int32_t>(box.dy2) * cell_px;
    return player_x >= min_x && player_x <= max_x && player_y >= min_y && player_y <= max_y;
}

void Charger0048Ai::step(
    NativeEntity& ent,
    std::int32_t player_x,
    std::int32_t player_y,
    int cell_px) noexcept {
    if (!ent.active) {
        return;
    }

    if (substate_ == 0U) {
        // 0x0177A6: record home (x, y) in $A8/$AA, action = 0 -> substate 1
        home_x_ = ent.world_x;
        home_y_ = ent.world_y;
        ent.x_fp = ent.world_x * 0x10000;
        ent.y_fp = ent.world_y * 0x10000;
        action_ = 0U;
        substate_ = 1U;
        return;
    }

    if (substate_ == 1U) {
        // 0x0177C6: check 0x01783E directional AABB -> charge substate 2
        if (check_player_in_aabb(ent.world_x, ent.world_y, ent.facing, player_x, player_y, cell_px)) {
            static constexpr std::int32_t kDirX[4] = {0, 1, 0, -1};
            static constexpr std::int32_t kDirY[4] = {-1, 0, 1, 0};
            const std::uint16_t dir = ent.facing & 3U;
            vx_fp_ = kDirX[dir] * 0x00010000;
            vy_fp_ = kDirY[dir] * 0x00010000;
            action_ = 2U;
            charge_timer_ = charge_duration_;
            last_sound_cmd_ = kTriggerSfx;
            substate_ = 2U;
        }
        return;
    }

    if (substate_ == 2U) {
        // 0x01784E: accelerate first half of $9E, decelerate second half, flip facing -> substate 3
        static constexpr std::int32_t kDirX[4] = {0, 1, 0, -1};
        static constexpr std::int32_t kDirY[4] = {-1, 0, 1, 0};
        const std::uint16_t dir = ent.facing & 3U;
        const std::int32_t accel = (charge_timer_ > (charge_duration_ / 2U)) ? 0x00008000 : -0x00008000;
        vx_fp_ += kDirX[dir] * accel;
        vy_fp_ += kDirY[dir] * accel;
        ent.x_fp += vx_fp_;
        ent.y_fp += vy_fp_;
        ent.world_x = ent.x_fp >> 16;
        ent.world_y = ent.y_fp >> 16;

        if (charge_timer_ > 0U) {
            --charge_timer_;
        }
        if (charge_timer_ == 0U) {
            vx_fp_ = 0;
            vy_fp_ = 0;
            ent.facing ^= 2U; // bchg #1, $17(a6)
            substate_ = 3U;
        }
        return;
    }

    // Substate 3 (0x0178B2): return to home (x, y), flip facing -> substate 1
    const std::int32_t dx = home_x_ - ent.world_x;
    const std::int32_t dy = home_y_ - ent.world_y;
    if (dx == 0 && dy == 0) {
        ent.facing ^= 2U;
        action_ = 0U;
        substate_ = 1U;
        return;
    }
    ent.world_x += std::clamp<std::int32_t>(dx, -2, 2);
    ent.world_y += std::clamp<std::int32_t>(dy, -2, 2);
    ent.x_fp = ent.world_x * 0x10000;
    ent.y_fp = ent.world_y * 0x10000;
}

void GateBoss0030Ai::step(NativeEntity& boss, std::vector<NativeEntity>& children) noexcept {
    if (!boss.active) {
        return;
    }

    if (substate_ == 0U) {
        // 0x015AE2: spawn 5 linked child slots (+0xBC stride, action 0x3A/0x3C, VRAM 0x4200)
        children.resize(kChildCount);
        for (std::size_t i = 0; i < kChildCount; ++i) {
            NativeEntity& c = children[i];
            c = NativeEntity{};
            c.active = true;
            c.type_id = 0x0030U;
            c.slot_index = static_cast<std::uint16_t>(i + 1U);
            c.world_x = boss.world_x + static_cast<std::int32_t>((i - 2U) * 16);
            c.world_y = boss.world_y;
            c.tile_base = kVramBase;
            c.anim_frame = ((i % 2U) == 0U) ? kActionA : kActionB;
            c.linked_slot_9e = (i + 1U < kChildCount) ? static_cast<std::int16_t>(i + 1U) : -1;
        }
        substate_ = 1U;
        return;
    }

    if (boss.hp <= 0) {
        boss.active = false;
        for (auto& c : children) {
            c.active = false;
        }
        substate_ = 3U;
        return;
    }

    if (substate_ == 1U) {
        substate_ = 2U;
    }
}

} // namespace oasis::game::enemy
