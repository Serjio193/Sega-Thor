#pragma once

#include "core/runtime.hpp"
#include "game/camera/camera_controller.hpp"
#include "game/player/player.hpp"
#include "game/render/hud_overlay.hpp"
#include "game/render/sat_pipeline_contract.hpp"
#include "game/standalone_game_loop.hpp"
#include "game/world/byte_grid.hpp"
#include "game/world/tilemap_streamer.hpp"
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

// Authentic non-player entity in scene.
struct NativeEntity {
    std::uint16_t type_id{0x0060};          // Entity type in ROM (0x0060 = beach chest)
    std::int32_t world_x{256};              // World X coordinate
    std::int32_t world_y{88};               // World Y coordinate
    std::uint8_t state{0};                  // Entity state (0 = closed, 0x6A = opened)
    std::uint8_t anim_frame{0};             // Current animation frame
    std::uint16_t tile_base{0x04A8};        // SAT base tile index
    bool active{true};                      // Whether entity is active
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

    // Replay external player trajectory (for differential oracle validation).
    void set_replay_player_trajectory(bool enable) noexcept { replay_player_trajectory_ = enable; }
    [[nodiscard]] bool replay_player_trajectory() const noexcept { return replay_player_trajectory_; }

    // Authentic entity interaction query.
    [[nodiscard]] bool chest_opened() const noexcept {
        return !entities_.empty() && entities_[0].state == 0x6AU;
    }

    // State-driven player animation pose query.
    [[nodiscard]] std::size_t current_player_pose() const noexcept;

private:
    void update_sat_sprites(genesis::VdpState& vdp);
    void update_entity_interactions(const FrameInput& input);

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

    std::vector<std::uint8_t> terrain_buffer_{};
    world::ByteGridView terrain_{};
    std::vector<std::uint8_t> walk_patterns_{};

    SimulationState sim_state_{};
    bool camera_tracking_{true};
    bool oracle_mode_{true};
    bool replay_player_trajectory_{false};
    std::uint16_t prev_render_player_x_{261};
    std::uint16_t prev_render_player_y_{107};
    int walk_tick_{0};
    bool initialized_{false};
};

} // namespace oasis::game
