#pragma once

#include "game/entity/native_entity.hpp"
#include "game/spirits/dytto_abilities.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::spirits {

// ROM constants for Dytto Spirit (0x014668 spawn, 0x01454C DMA, 0x014F60 State 0x1A, 0x0150A0 Follow).
struct DyttoEntityConfig {
    static constexpr std::uint16_t kEntityType = 0x0018U;
    static constexpr std::uint16_t kCollisionFlags = 0x0002U;
    static constexpr std::uint16_t kInitialVramPattern = 0x04F8U;
    static constexpr std::uint16_t kVramTileBase = 0x04D0U;
    static constexpr std::uint32_t kVramByteOffset = 0x9A00U;
    static constexpr std::uint32_t kRomGfxSource = 0x00141580U;
    static constexpr std::uint32_t kDecompBufferRam = 0x00FF2FA8U;
    static constexpr std::uint32_t kDmaQueueRam = 0x00FF1892U;
    static constexpr std::uint16_t kMaxSp = 100U;
    static constexpr std::uint16_t kSpDrainRate = 10U;
    static constexpr std::uint16_t kDefaultState = 0x0002U;
    static constexpr std::uint16_t kDefaultAction = 0x0004U;
    static constexpr std::uint16_t kAppearState = 0x001AU;
    static constexpr std::uint16_t kAppearAction = 0x0022U;
    static constexpr std::uint16_t kFollowState = 0x0004U;
    static constexpr std::int16_t kElevationDeltaPx = 16;
    static constexpr std::uint8_t kAngularTurnStep = 8U;
    static constexpr std::uint8_t kVelocityDampingShift = 7U; // ASR.L #7

    // State 0x1A (0x014F60) timeline milestones
    static constexpr std::uint16_t kTimelineBurstSfxFrame = 180U;         // 0x00B4
    static constexpr std::uint16_t kTimelinePalettePulseFrame = 240U;     // 0x00F0
    static constexpr std::uint16_t kTimelineDialogueCompleteFrame = 300U; // 0x012C
    static constexpr std::uint16_t kTimelineCameraUnlockFrame = 360U;     // 0x0168
    static constexpr std::uint16_t kTimelineCompleteFrame = 420U;         // 0x01A4
};

struct DyttoAwakeningEvents {
    bool burst_sfx_triggered{false};
    bool palette_pulse_triggered{false};
    bool dialogue_complete_handoff{false};
    bool camera_unlocked{false};
    bool cutscene_completed{false};
};

// Native controller for Dytto (0x0018) spawn, State 0x1A awakening timeline, and State 0x04 follow steering.
class DyttoSpiritController {
public:
    enum class State : std::uint16_t {
        Idle = 0x0000U,
        IdleDispatch = 0x0000U,
        Default = 0x0002U,
        NoOp = 0x0002U,
        FollowPlayer = 0x0004U,
        MagicAttack = 0x0006U,
        MagicAttack1 = 0x0006U,
        MagicAttack2 = 0x0008U,
        MagicRecovery = 0x000AU,
        KnockbackRecovery = 0x000EU,
        SpecialSpread = 0x0010U,
        ReturnToAli = 0x0016U,
        AliContactBuff = 0x0018U,
        AppearCutscene = 0x001AU,
    };

    void reset() noexcept;

    // Models ROM 0x014668 (and optional 0x018CDE altar override to State 0x1A / Action 0x22).
    void init_spawn(std::int32_t spawn_x,
                    std::int32_t spawn_y,
                    std::int32_t spawn_z = 0,
                    std::uint16_t facing = 2U,
                    bool altar_awakening = true) noexcept;

    // Advances one frame in the 14-state machine (0x014908 / 0x014914) and steps active projectiles.
    void update_frame(std::int32_t player_x,
                      std::int32_t player_y,
                      std::span<NativeEntity> entities = {},
                      std::uint16_t prng_word = 0xFFFFU,
                      bool cutscene_locked = false) noexcept;

    // ROM 0x014932 (State 0x00): autonomous PRNG command dispatch (0x2000 / 0x6000 / 0x9000).
    void dispatch_autonomous_command(std::uint16_t prng_word,
                                     bool cutscene_locked,
                                     std::int32_t player_x,
                                     std::int32_t player_y) noexcept;

    // ROM 0x014EE6 (State 0x0E): enters knockback recovery with initial fixed-point velocity.
    void trigger_knockback(std::int32_t vx_fp, std::int32_t vy_fp, std::int32_t vz_fp) noexcept;

    void set_state(State s) noexcept { state_ = s; }

    // Models ROM 0x00E6F0 binary atan2 returning 0..255 angle.
    [[nodiscard]] static std::uint8_t binary_atan2(std::int32_t dx, std::int32_t dy) noexcept;

    // Steps current 0..255 angle toward target angle by at most `step` units (ROM 0x0150A0: MOVEQ #8, D1).
    [[nodiscard]] static std::uint8_t step_angle_toward(
        std::uint8_t current,
        std::uint8_t target,
        std::uint8_t step = DyttoEntityConfig::kAngularTurnStep) noexcept;

    // Arithmetic right shift by 7 (ASR.L #7, D6).
    [[nodiscard]] static std::int32_t damp_velocity_asr7(std::int32_t vel_fp) noexcept;

    [[nodiscard]] bool is_active() const noexcept { return active_; }
    [[nodiscard]] std::uint16_t type_id() const noexcept { return type_id_; }
    [[nodiscard]] std::uint16_t collision_flags() const noexcept { return collision_flags_; }
    [[nodiscard]] std::uint16_t vram_pattern() const noexcept { return vram_pattern_; }
    [[nodiscard]] std::uint16_t vram_tile_base() const noexcept { return vram_tile_base_; }
    [[nodiscard]] std::uint32_t rom_gfx_source() const noexcept { return rom_gfx_source_; }
    [[nodiscard]] State state() const noexcept { return state_; }
    [[nodiscard]] std::uint16_t action() const noexcept { return action_; }
    [[nodiscard]] std::uint16_t appear_timer() const noexcept { return appear_timer_; }
    [[nodiscard]] const DyttoAwakeningEvents& awakening_events() const noexcept { return events_; }

    [[nodiscard]] std::uint16_t max_sp() const noexcept { return max_sp_; }
    [[nodiscard]] std::uint16_t current_sp() const noexcept { return current_sp_; }
    [[nodiscard]] std::uint16_t sp_drain_rate() const noexcept { return sp_drain_rate_; }
    [[nodiscard]] std::uint16_t sp_drain_timer() const noexcept { return sp_drain_timer_; }

    [[nodiscard]] std::int32_t world_x() const noexcept { return world_x_; }
    [[nodiscard]] std::int32_t world_y() const noexcept { return world_y_; }
    [[nodiscard]] std::int32_t world_z() const noexcept { return world_z_; }
    [[nodiscard]] std::int32_t x_fp() const noexcept { return x_fp_; }
    [[nodiscard]] std::int32_t y_fp() const noexcept { return y_fp_; }
    [[nodiscard]] std::uint16_t facing() const noexcept { return facing_; }
    [[nodiscard]] std::uint8_t current_angle() const noexcept { return current_angle_; }
    [[nodiscard]] std::int32_t velocity_fp() const noexcept { return velocity_fp_; }
    [[nodiscard]] std::int16_t target_offset_x() const noexcept { return target_offset_x_; }
    [[nodiscard]] std::int16_t target_offset_y() const noexcept { return target_offset_y_; }
    [[nodiscard]] bool dma_requested() const noexcept { return dma_requested_; }
    void clear_dma_requested() noexcept { dma_requested_ = false; }
    [[nodiscard]] const std::vector<DyttoProjectile>& projectiles() const noexcept { return projectiles_; }
    [[nodiscard]] std::uint32_t last_sound_cmd() const noexcept { return last_sound_cmd_; }
    [[nodiscard]] bool ali_buff_triggered() const noexcept { return ali_buff_triggered_; }
    [[nodiscard]] std::uint32_t last_callback_addr() const noexcept { return last_callback_addr_; }
    [[nodiscard]] std::uint16_t wander_timer() const noexcept { return wander_timer_; }
    [[nodiscard]] std::int32_t kb_vx_fp() const noexcept { return kb_vx_fp_; }
    [[nodiscard]] std::int32_t kb_vy_fp() const noexcept { return kb_vy_fp_; }
    [[nodiscard]] std::int32_t kb_vz_fp() const noexcept { return kb_vz_fp_; }

    void set_target_offset(std::int16_t ox, std::int16_t oy) noexcept {
        target_offset_x_ = ox;
        target_offset_y_ = oy;
    }

private:
    bool active_{false};
    std::uint16_t type_id_{DyttoEntityConfig::kEntityType};
    std::uint16_t collision_flags_{DyttoEntityConfig::kCollisionFlags};
    std::uint16_t vram_pattern_{DyttoEntityConfig::kInitialVramPattern};
    std::uint16_t vram_tile_base_{DyttoEntityConfig::kVramTileBase};
    std::uint32_t rom_gfx_source_{DyttoEntityConfig::kRomGfxSource};
    State state_{State::Idle};
    std::uint16_t action_{DyttoEntityConfig::kDefaultAction};
    std::uint16_t appear_timer_{0};
    DyttoAwakeningEvents events_{};

    std::uint16_t max_sp_{DyttoEntityConfig::kMaxSp};
    std::uint16_t current_sp_{DyttoEntityConfig::kMaxSp};
    std::uint16_t sp_drain_rate_{DyttoEntityConfig::kSpDrainRate};
    std::uint16_t sp_drain_timer_{DyttoEntityConfig::kSpDrainRate};

    std::int32_t world_x_{0};
    std::int32_t world_y_{0};
    std::int32_t world_z_{0};
    std::int32_t x_fp_{0};
    std::int32_t y_fp_{0};
    std::uint16_t facing_{2U};
    std::uint8_t current_angle_{0U};
    std::int32_t velocity_fp_{0};
    std::int16_t target_offset_x_{0};
    std::int16_t target_offset_y_{-16};
    bool dma_requested_{false};

    // M16.54 ability & state machine fields (0x014914..0x015214)
    std::vector<DyttoProjectile> projectiles_{};
    std::uint32_t last_sound_cmd_{0U};
    std::uint16_t wander_timer_{0U};
    std::uint16_t magic_frame_counter_{0U};
    bool ali_buff_triggered_{false};
    std::uint32_t last_callback_addr_{0U};
    std::int32_t kb_vx_fp_{0};
    std::int32_t kb_vy_fp_{0};
    std::int32_t kb_vz_fp_{0};
};

} // namespace oasis::game::spirits
