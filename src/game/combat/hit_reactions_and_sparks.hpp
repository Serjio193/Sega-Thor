#pragma once

#include <array>
#include <cstdint>

namespace oasis::game::combat {

// ROM 0x00BF26: 4 Hit-Reaction Profiles.
enum class HitReactionKind : std::uint8_t {
    LightGroundOrJuggle = 0, // 0x00BF2E: (0x300, 0x2C0, 0) grounded; airborne -> bit 7 + MediumKnockback
    VerticalLauncher = 2,    // 0x00BF98: (0, 0, 0x70000)
    MediumKnockback = 4,     // 0x00BF58: (0x400, 0x3C0, 0x28000)
    HeavyKnockback = 6,      // 0x00BF78: (0x700, 0x680, 0x30000)
};

struct HitReactionPhysicsState {
    std::int32_t vx{0};
    std::int32_t vy{0};
    std::int32_t vz{0};
    std::uint8_t flags_86{0U}; // bit 7 (0x80) set when LightGroundOrJuggle hits airborne target
    bool airborne{false};      // $10 != 0
};

// ROM 0x00BF26..0x00BFAE: Applies directional (vx, vy, vz) for all 4 profiles.
// `attacker_facing`: 0=up, 1=right, 2=down, 3=left.
void apply_hit_reaction_physics(
    HitReactionPhysicsState& target,
    HitReactionKind kind,
    std::uint8_t attacker_facing) noexcept;

struct HitSparkSlot {
    bool active{false};
    std::uint8_t type{0U};  // 2 on spawn (0x00C012)
    std::uint8_t timer{0U}; // 5 frames on spawn
    std::int32_t x{0};
    std::int32_t y{0};
};

// ROM 0x00C012: 4-slot Hit-Spark Pool at $FF19C8 (stride 8, type = 2, timer = 5 frames).
class HitSparkPool4 {
public:
    static constexpr std::size_t kSlotCount = 4U;
    static constexpr std::uint8_t kDefaultSparkType = 2U;
    static constexpr std::uint8_t kDefaultSparkTimer = 5U;

    void reset() noexcept;
    bool spawn(std::int32_t x, std::int32_t y) noexcept;
    void tick_sparks() noexcept;

    [[nodiscard]] const std::array<HitSparkSlot, kSlotCount>& slots() const noexcept { return slots_; }
    [[nodiscard]] std::size_t active_count() const noexcept;

private:
    std::array<HitSparkSlot, kSlotCount> slots_{};
};

struct ShieldParryResult {
    std::uint32_t sfx_cmd{0x00900202U};
    bool ali_recoil_triggered{false}; // true when weapon_ff198c is 6 or 10 (0x008530)
    bool spark_spawned{false};
};

// ROM 0x00BFAE: Emits SFX 0x00900202, flags Ali recoil (0x008530) when weapon $FF198C
// is 6 or 10, and allocates a spark in `pool`.
[[nodiscard]] ShieldParryResult apply_shield_parry(
    std::uint16_t weapon_ff198c,
    std::int32_t spark_x,
    std::int32_t spark_y,
    HitSparkPool4& pool) noexcept;

} // namespace oasis::game::combat
