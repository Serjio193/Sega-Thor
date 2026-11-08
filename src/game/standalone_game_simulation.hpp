#pragma once

#include "core/runtime.hpp"
#include "game/player/player.hpp"
#include "game/render/sat_pipeline_contract.hpp"
#include "game/standalone_game_loop.hpp"
#include "game/world/byte_grid.hpp"
#include "genesis/vdp_state.hpp"

#include <array>
#include <cstdint>
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
    int camera_x{125};                      // Viewport origin X
    int camera_y{46};                       // Viewport origin Y
};

// Authentic non-player entity in scene.
struct NativeEntity {
    std::uint16_t type_id{0x0060};          // Entity type in ROM
    std::int32_t world_x{256};              // World X coordinate
    std::int32_t world_y{88};               // World Y coordinate
    std::uint8_t state{0};                  // Entity state
    std::uint8_t anim_frame{0};             // Current animation frame
    std::uint16_t tile_base{0x04A8};        // SAT base tile index
    bool active{true};                      // Whether entity is active
};

// Standalone C++ game simulation vertical slice.
// Implements IGameSimulation without linking GPGX, Musashi or libretro.
// Connects player state machine, authentic 30Hz movement cadence, secondary entities,
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

    // Reset player, entities and camera back to initialization state.
    void reset();

    // Directly set player world coordinates.
    void set_player_position(std::int32_t x, std::int32_t y) noexcept;

    // Viewport camera origin.
    [[nodiscard]] int camera_x() const noexcept { return camera_x_; }
    [[nodiscard]] int camera_y() const noexcept { return camera_y_; }

    // Toggle camera tracking player vs fixed camera.
    void set_camera_tracking(bool track) noexcept { camera_tracking_ = track; }
    [[nodiscard]] bool camera_tracking() const noexcept { return camera_tracking_; }

    // Walk animation cycle tick (0..15).
    [[nodiscard]] int walk_tick() const noexcept { return walk_tick_; }

    // Whether authentic oracle cadence mode is enabled.
    void set_oracle_mode(bool enable) noexcept { oracle_mode_ = enable; }
    [[nodiscard]] bool oracle_mode() const noexcept { return oracle_mode_; }

private:
    void update_sat_sprites(genesis::VdpState& vdp);

    genesis::VdpState baseline_vdp_{};
    player::PlayerState player_{};
    player::PlayerMovementConfig movement_config_{};
    player::VelocityAdjustContext velocity_context_{};
    CheckpointInitialState checkpoint_state_{};

    std::vector<NativeEntity> entities_{};
    render::SatPipelineContract sat_pipeline_{};

    std::vector<std::uint8_t> terrain_buffer_{};
    world::ByteGridView terrain_{};

    SimulationState sim_state_{};
    int camera_x_{125};
    int camera_y_{46};
    bool camera_tracking_{false};
    bool oracle_mode_{true};
    int walk_tick_{0};
    bool initialized_{false};
};

} // namespace oasis::game
