#pragma once

#include "game/entity/native_entity.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace oasis::game::enemy {

// ROM 0x01783E: 16-byte directional tile AABB table for Entity 0x0048 Charger.
struct DirectionalTileAabb {
    std::int8_t dx1{0};
    std::int8_t dy1{0};
    std::int8_t dx2{0};
    std::int8_t dy2{0};
};

// ROM 0x01778C..0x0178E8: Entity 0x0048 Charger 4-substate ($A6) AI controller.
class Charger0048Ai {
public:
    static constexpr std::uint32_t kUpdateHandlerRom = 0x0001778CU;
    static constexpr std::uint32_t kAabbTableRom = 0x0001783EU;
    static constexpr std::uint32_t kTriggerSfx = 0x00800399U;

    static constexpr std::array<DirectionalTileAabb, 4> kDirectionalAabbs = {{
        {-1, -24,  1, -1}, // Facing 0 (Up)
        { 1,  -1, 24,  1}, // Facing 1 (Right)
        {-1,   1,  1, 24}, // Facing 2 (Down)
        {-24, -1, -1,  1}, // Facing 3 (Left)
    }};

    void reset() noexcept { *this = Charger0048Ai{}; }

    // Tests whether (player_x, player_y) falls inside the facing-selected 0x01783E box.
    [[nodiscard]] static bool check_player_in_aabb(
        std::int32_t ent_x,
        std::int32_t ent_y,
        std::uint16_t facing,
        std::int32_t player_x,
        std::int32_t player_y,
        int cell_px = 8) noexcept;

    // Steps one frame of the 4-substate $A6 state machine (0x01778C).
    void step(NativeEntity& ent,
              std::int32_t player_x,
              std::int32_t player_y,
              int cell_px = 8) noexcept;

    [[nodiscard]] std::uint8_t substate() const noexcept { return substate_; }
    [[nodiscard]] std::int32_t home_x() const noexcept { return home_x_; }
    [[nodiscard]] std::int32_t home_y() const noexcept { return home_y_; }
    [[nodiscard]] std::uint8_t action() const noexcept { return action_; }
    [[nodiscard]] std::uint16_t charge_timer() const noexcept { return charge_timer_; }
    [[nodiscard]] std::int32_t vx_fp() const noexcept { return vx_fp_; }
    [[nodiscard]] std::int32_t vy_fp() const noexcept { return vy_fp_; }
    [[nodiscard]] std::uint32_t last_sound_cmd() const noexcept { return last_sound_cmd_; }

private:
    std::uint8_t substate_{0U}; // $A6(A6)
    std::int32_t home_x_{0};    // $A8(A6)
    std::int32_t home_y_{0};    // $AA(A6)
    std::uint8_t action_{0U};
    std::uint16_t charge_timer_{0U};    // $9E(A6)
    std::uint16_t charge_duration_{16U};
    std::int32_t vx_fp_{0};
    std::int32_t vy_fp_{0};
    std::uint32_t last_sound_cmd_{0U};
};

// ROM 0x015AE2..0x015B9C: Entity 0x0030 Gate Boss (Room 0x0105 Waterfall Gate Arena).
class GateBoss0030Ai {
public:
    static constexpr std::uint32_t kUpdateHandlerRom = 0x00015AE2U;
    static constexpr std::uint16_t kRoomId = 0x0105U;
    static constexpr std::size_t kChildCount = 5U;
    static constexpr std::uint16_t kChildStrideBytes = 0x00BCU; // 188 bytes
    static constexpr std::uint16_t kVramBase = 0x4200U;
    static constexpr std::uint8_t kActionA = 0x3AU;
    static constexpr std::uint8_t kActionB = 0x3CU;

    void reset() noexcept { substate_ = 0U; }

    // Steps one frame of the 4-substate $66 state machine (0x015AE2).
    void step(NativeEntity& boss, std::vector<NativeEntity>& children) noexcept;

    [[nodiscard]] std::uint8_t substate() const noexcept { return substate_; }

private:
    std::uint8_t substate_{0U}; // $66(A6)
};

} // namespace oasis::game::enemy
