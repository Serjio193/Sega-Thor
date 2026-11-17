#pragma once

#include <cmath>
#include <cstdint>
#include <optional>

namespace oasis::game::combat {

inline constexpr std::uint32_t kDeathExplosionBurstAddr = 0x00EA04U;
inline constexpr std::uint32_t kCarriedLatchFollowAddr = 0x00EA76U;
inline constexpr std::uint32_t kProjectileWallAndHitProbeAddr = 0x00EAC0U;

inline constexpr std::uint16_t kBurstJitterMask = 0x1FU;
inline constexpr std::int16_t kBurstJitterBias = -16;
inline constexpr std::uint32_t kBurstSfx1 = 0x0090021BU;
inline constexpr std::uint32_t kBurstSfx2 = 0x00600382U;
inline constexpr std::uint32_t kProjBounceSfx = 0x00700378U;
inline constexpr std::int16_t kProjProbeHalfExtentX = 6;
inline constexpr std::int16_t kProjProbeHalfExtentY = 6;
inline constexpr std::int16_t kProjProbeHalfExtentZ = 2;
inline constexpr std::uint8_t kDefaultHitstunFrames = 10U;

struct DeathBurstState {
    std::uint16_t burst_timer_9e{0U};
    std::uint8_t flags_37{0U};
    std::int16_t base_x_4e{0};
    std::int16_t base_y_52{0};
    std::int16_t current_x{0};
    std::int16_t current_y{0};
    bool active{true};
};

struct DeathBurstResult {
    DeathBurstState next_state{};
    bool destroyed{false};
    bool sfx_triggered{false};
    std::uint32_t sfx_1{0U};
    std::uint32_t sfx_2{0U};
};

struct LatchFollowState {
    std::uint8_t latch_timer_b8{0U};
    std::int16_t local_x_ae{0};
    std::int16_t local_y_b0{0};
    std::int16_t local_z_b2{0};
    std::int16_t pos_x{0};
    std::int16_t pos_y{0};
    std::int16_t pos_z{0};
};

enum class LatchFollowAction : std::uint8_t {
    Following,
    TransitionToProbe,
    DespawnExpired,
    DetachedOrNoParent
};

struct LatchFollowResult {
    LatchFollowState next_state{};
    LatchFollowAction action{LatchFollowAction::Following};
};

struct ProjectileProbeInput {
    std::uint16_t impact_timer_86{0U};
    std::uint8_t terrain_flags_40{0U};
    std::uint8_t terrain_flags_41{0U};
    std::uint16_t anim_timer_16{0U};
    std::uint32_t payload_aa{0U};
    std::uint16_t damage_attr_a8{0U};
};

struct ProbeTargetEntity {
    std::uint8_t trait_flags_3a{0U};
    std::uint8_t render_flags_38{0U};
    std::uint8_t hitstun_flag_82{0U};
    std::uint32_t damage_mailbox_7e{0U};
    std::uint16_t damage_attr_86{0U};
    std::uint8_t hitstun_timer_83{0U};
    std::uint8_t status_flag_9d{0U};
};

enum class ProjectileProbeOutcome : std::uint8_t {
    AlreadyImpacted,
    WallBounce,
    TerrainDespawn,
    ShieldBounce,
    IntangiblePass,
    InvulnerableSpark,
    HitApplied,
    NoTarget
};

struct ProjectileProbeResult {
    ProjectileProbeOutcome outcome{ProjectileProbeOutcome::NoTarget};
    bool sfx_triggered{false};
    std::uint32_t sfx{0U};
    bool target_modified{false};
    ProbeTargetEntity modified_target{};
};

/// Models 0x00EA04..0x00EA74: decrements burst timer, applies (prng & 0x1F) - 16 jitter,
/// triggers dual SFX 0x0090021B + 0x00600382, and clears entity slot at 0.
[[nodiscard]] DeathBurstResult step_death_explosion_burst(
    DeathBurstState state,
    std::uint16_t prng_x,
    std::uint16_t prng_y) noexcept;

/// Models 0x00EA76..0x00EABE: manages latch timer at +0xB8 and binds local XYZ
/// (+0xAE, +0xB0, +0xB2) to parent entity coordinates.
[[nodiscard]] LatchFollowResult step_carried_latch_follow(
    LatchFollowState state,
    bool parent_valid,
    std::int16_t parent_x,
    std::int16_t parent_y,
    std::int16_t parent_z) noexcept;

/// Checks if target is within the [-6..+6, -6..+6, -2..+2] probe bounding box.
[[nodiscard]] bool is_in_projectile_probe_box(
    std::int16_t proj_x, std::int16_t proj_y, std::int16_t proj_z,
    std::int16_t target_x, std::int16_t target_y, std::int16_t target_z) noexcept;

/// Models 0x00EAC0..0x00EB4C: probes terrain wall collision and target hitbox,
/// handling shield bounce, intangible/invulnerability bypass, and payload transfer.
[[nodiscard]] ProjectileProbeResult evaluate_projectile_probe(
    const ProjectileProbeInput& input,
    std::optional<ProbeTargetEntity> target) noexcept;

} // namespace oasis::game::combat
