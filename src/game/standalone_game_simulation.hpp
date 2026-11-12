#pragma once

#include "core/runtime.hpp"
#include "game/camera/camera_controller.hpp"
#include "game/combat/attack_controller.hpp"
#include "game/combat/sound_command_queue.hpp"
#include "game/entity/native_entity.hpp"
#include "game/entity/town_entity_scheduler.hpp"
#include "game/interaction/interaction_system.hpp"
#include "game/player/player.hpp"
#include "game/render/hud_overlay.hpp"
#include "game/render/rom_sprite_decoder.hpp"
#include "game/render/sat_pipeline_contract.hpp"
#include "game/render/vdp_dma_queue.hpp"
#include "game/standalone_game_loop.hpp"
#include "game/world/byte_grid.hpp"
#include "game/world/tilemap_streamer.hpp"
#include "game/world/town_scene_loader.hpp"
#include "genesis/vdp_state.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace oasis::game {

// Authentic player state restored from Beyond Oasis checkpoint.
struct CheckpointInitialState {
    std::int32_t x_fixed{0x01051000};       // 261.0625 fixed point ($FF19F0)
    std::int32_t y_fixed{0x006B0000};       // 107.0000 fixed point ($FF19F4)
    std::uint16_t movement_state{4};        // Turn recovery state ($FF19EC)
    std::uint16_t direction_code{0};        // Facing Up ($FF19EA)
    std::uint16_t turn_timer{10};           // 10 frame recovery counter ($FF19EE)
    std::int16_t hp{208};                   // Current HP ($FF1A0E)
    std::int16_t max_hp{218};               // Max HP ($FF1A10)
    int camera_x{96};                       // Viewport origin X ($FF1716)
    int camera_y{3};                        // Viewport origin Y ($FF171A)
};

// Authentic Town checkpoint state restored for continuous Layer C replay.
struct TownCheckpointInitialState {
    std::int32_t x_fixed{73 * 0x10000};
    std::int32_t y_fixed{200 * 0x10000};
    std::uint16_t movement_state{4};
    std::uint16_t direction_code{0};
    std::int16_t hp{208};
    std::int16_t max_hp{218};
    int camera_x{0};
    int camera_y{121};
    std::uint16_t tick_counter{2392};
    std::uint32_t frame_counter{775};
    std::uint16_t prng_seed{0x22BCU};
    std::uint16_t turn_timer{0};
    std::int32_t intent_x_fixed{0};
    std::int32_t intent_y_fixed{0};
    std::vector<NativeEntity> entities{};
    bool attract_banner_enabled{false};
    int initial_walk_tick{0};
};

// Standalone C++ game simulation vertical slice.
// Implements IGameSimulation without linking GPGX, Musashi or libretro.
// Connects player state machine, authentic 30Hz movement cadence, camera tracking,
// VDP plane scrolling, secondary scene entities, authentic entity interaction,
// and publishes hardware SAT entries through SatPipelineContract to VdpState.
class StandaloneGameSimulation : public IGameSimulation {
public:
    StandaloneGameSimulation();
    ~StandaloneGameSimulation() override = default;

    // Initializes simulation from baseline VDP state and optional coordinates.
    bool initialize(const genesis::VdpState& baseline_vdp,
                    std::int32_t initial_x = 261,
                    std::int32_t initial_y = 107);

    // Initializes simulation with full authentic checkpoint state.
    bool initialize_from_checkpoint(const genesis::VdpState& baseline_vdp,
                                    const CheckpointInitialState& cp_state);

    // Initializes simulation with authentic Town checkpoint state for Layer C replay.
    bool initialize_town_checkpoint(std::span<const std::uint8_t> rom,
                                    const TownCheckpointInitialState& town_cp);

    // Initializes simulation with authentic Town checkpoint state and initial VDP state.
    bool initialize_town_checkpoint(std::span<const std::uint8_t> rom,
                                    const TownCheckpointInitialState& town_cp,
                                    const genesis::VdpState& initial_vdp) {
        baseline_vdp_ = initial_vdp;
        current_vdp_ = initial_vdp;
        return initialize_town_checkpoint(rom, town_cp);
    }

    [[nodiscard]] const genesis::VdpState& current_vdp() const noexcept { return current_vdp_; }
    [[nodiscard]] const render::VdpDmaQueue& dma_queue() const noexcept { return dma_queue_; }

    // Optional: streams authentic 64x28 background tilemap from ROM into baseline VRAM.
    bool load_rom_background(std::span<const std::uint8_t> rom);

    // Load full authentic 64x64 screen map for dynamic tilemap streaming.
    bool load_screen_map(std::span<const std::uint16_t> map_data);
    bool load_screen_map_file(const char* path);

    // Load authentic 8-pose walk pattern data for dynamic pattern upload.
    bool load_walk_patterns_file(const char* path);

    // Access dynamic tilemap streamer.
    [[nodiscard]] const world::TilemapStreamer& streamer() const noexcept {
        return streamer_;
    }
    [[nodiscard]] world::TilemapStreamer& streamer() noexcept {
        return streamer_;
    }

    // Advances the simulation by exactly one deterministic frame.
    void step_frame(const FrameInput& input, genesis::VdpState& out_vdp) override;

    // Read-only access to current simulation state.
    [[nodiscard]] const SimulationState& state() const noexcept override {
        return sim_state_;
    }

    // Access internal player state.
    [[nodiscard]] const player::PlayerState& player() const noexcept {
        return player_;
    }

    // Access non-player entities in scene.
    [[nodiscard]] const std::vector<NativeEntity>& entities() const noexcept {
        return entities_;
    }

    // Access SAT pipeline contract.
    [[nodiscard]] const render::SatPipelineContract& sat_pipeline() const noexcept {
        return sat_pipeline_;
    }

    // Access HUD overlay generator.
    [[nodiscard]] const render::HudOverlay& hud() const noexcept {
        return hud_;
    }

    // Access native camera controller.
    [[nodiscard]] const camera::CameraController& camera() const noexcept {
        return camera_;
    }
    [[nodiscard]] camera::CameraController& camera() noexcept {
        return camera_;
    }

    // Reset player, entities and camera back to initialization state.
    void reset();

    // Directly set player world coordinates.
    void set_player_position(std::int32_t x, std::int32_t y) noexcept;

    // Viewport camera origin.
    [[nodiscard]] int camera_x() const noexcept { return camera_.camera_x(); }
    [[nodiscard]] int camera_y() const noexcept { return camera_.camera_y(); }

    // Toggle camera tracking player vs fixed camera.
    void set_camera_tracking(bool track) noexcept {
        camera_tracking_ = track;
        camera_.set_tracking_enabled(track);
    }
    [[nodiscard]] bool camera_tracking() const noexcept { return camera_tracking_; }

    // Walk animation cycle tick (0..15).
    [[nodiscard]] int walk_tick() const noexcept { return walk_tick_; }

    // Whether authentic oracle cadence mode is enabled.
    void set_oracle_mode(bool enable) noexcept { oracle_mode_ = enable; }
    [[nodiscard]] bool oracle_mode() const noexcept { return oracle_mode_; }

    void set_interaction_trigger_on_proximity(bool enable) noexcept {
        interaction_system_.set_trigger_on_proximity(enable);
    }
    void set_entity_position(std::size_t index, std::int32_t x, std::int32_t y) noexcept {
        if (index < entities_.size()) {
            entities_[index].world_x = x;
            entities_[index].world_y = y;
        }
    }

    // Replay external player trajectory (for differential oracle validation).
    void set_replay_player_trajectory(bool enable) noexcept { replay_player_trajectory_ = enable; }
    [[nodiscard]] bool replay_player_trajectory() const noexcept { return replay_player_trajectory_; }

    // Authentic entity interaction query.
    [[nodiscard]] bool chest_opened() const noexcept {
        return !entities_.empty() && entities_[0].state == 0x6AU;
    }
    [[nodiscard]] std::uint16_t sound_command() const noexcept {
        if (attack_controller_.sound_command() != 0U) return attack_controller_.sound_command();
        return interaction_system_.sound_command();
    }
    [[nodiscard]] std::uint16_t event_flag() const noexcept {
        return interaction_system_.event_flag();
    }
    [[nodiscard]] std::uint16_t interaction_timer() const noexcept {
        return interaction_system_.interaction_timer();
    }
    [[nodiscard]] std::uint16_t attack_timer() const noexcept {
        return attack_controller_.attack_timer();
    }
    [[nodiscard]] bool is_attacking() const noexcept {
        return attack_controller_.is_attacking() || player_.movement_state == 0x30U;
    }

    [[nodiscard]] const NativeInteractionSystem& interaction_system() const noexcept {
        return interaction_system_;
    }
    [[nodiscard]] NativeInteractionSystem& interaction_system() noexcept {
        return interaction_system_;
    }

    [[nodiscard]] const NativeAttackController& attack_controller() const noexcept {
        return attack_controller_;
    }
    [[nodiscard]] NativeAttackController& attack_controller() noexcept {
        return attack_controller_;
    }

    void set_allow_experimental_combat(bool allow) noexcept {
        attack_controller_.set_allow_experimental_combat(allow);
    }

    // Load authentic Town Screen(1,3) resources/spawns from ROM for continuous Beach->Town play.
    bool prepare_town_from_rom(std::span<const std::uint8_t> rom);

    // Cold-boot standalone gameplay directly from ROM with zero GPGX savestates or snapshots.
    bool initialize_native_from_rom(std::span<const std::uint8_t> rom,
                                    std::uint16_t start_map_id = world::kBeachMapId);

    // Transition immediately into Town Screen(1,3) (MAP_ID=0x005A), Town North (0x005B), or Beach (0x003D).
    void transition_to_town(bool settled_camera = false);
    void transition_to_town_north();
    void transition_to_beach();

    [[nodiscard]] bool is_native_rom_session() const noexcept { return native_rom_session_; }
    [[nodiscard]] std::uint32_t transition_count() const noexcept { return transition_count_; }
    [[nodiscard]] std::uint16_t map_id() const noexcept { return map_id_; }
    [[nodiscard]] const combat::SoundCommandQueue& sound_queue() const noexcept { return sound_queue_; }
    [[nodiscard]] const world::LoadedTownScene& town_scene() const noexcept { return town_scene_; }
    [[nodiscard]] std::uint16_t prng_seed() const noexcept { return town_ctx_.prng_seed; }
    void set_prng_seed(std::uint16_t seed) noexcept { town_ctx_.prng_seed = seed; }
    [[nodiscard]] std::uint16_t tick_counter() const noexcept { return tick_counter_; }
    void set_tick_counter(std::uint16_t tick) noexcept { tick_counter_ = tick; }
    [[nodiscard]] std::span<const render::TownEffect> town_effects() const noexcept { return town_effects_; }
    [[nodiscard]] std::span<render::TownEffect> town_effects() noexcept { return town_effects_; }

    // State-driven player animation pose query.
    [[nodiscard]] std::size_t current_player_pose() const noexcept;

private:
    void update_sat_sprites(genesis::VdpState& vdp);

    genesis::VdpState baseline_vdp_{};
    genesis::VdpState current_vdp_{};
    player::PlayerState player_{};
    player::PlayerMovementConfig movement_config_{};
    player::VelocityAdjustContext velocity_context_{};
    CheckpointInitialState checkpoint_state_{};

    camera::CameraController camera_{};
    world::TilemapStreamer streamer_{};
    std::vector<NativeEntity> entities_{};
    render::SatPipelineContract sat_pipeline_{};
    render::HudOverlay hud_{};

    NativeInteractionSystem interaction_system_{};
    NativeAttackController attack_controller_{};
    combat::SoundCommandQueue sound_queue_{};

    world::LoadedTownScene town_scene_{};
    entity::TownSchedulerContext town_ctx_{};
    std::vector<std::uint8_t> rom_bytes_{};
    std::uint16_t tick_counter_{0};
    bool town_prepared_{false};
    std::uint16_t map_id_{world::kBeachMapId};

    std::vector<std::uint8_t> terrain_buffer_{};
    world::ByteGridView terrain_{};
    std::vector<std::uint8_t> walk_patterns_{};

    SimulationState sim_state_{};
    bool camera_tracking_{true};
    bool oracle_mode_{true};
    bool replay_player_trajectory_{false};
    std::uint16_t prev_render_player_x_{261};
    std::uint16_t prev_render_player_y_{107};
    std::int32_t landing_velocity_x_{0};
    std::uint16_t landing_hold_ticks_{0};
    int walk_tick_{0};
    bool initialized_{false};
    render::VdpDmaQueue dma_queue_{};
    bool pending_vdp_stage_{false};
    bool attract_banner_enabled_{false};
    std::uint8_t town_water_timer_{7U};
    std::uint8_t town_water_phase_{0U};
    std::array<render::TownEffect, 4> town_effects_{};
    bool native_rom_session_{false};
    std::uint32_t transition_count_{0};
};

} // namespace oasis::game
