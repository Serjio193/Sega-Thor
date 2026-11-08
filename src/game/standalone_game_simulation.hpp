#pragma once

#include "core/runtime.hpp"
#include "game/player/player.hpp"
#include "game/standalone_game_loop.hpp"
#include "game/world/byte_grid.hpp"
#include "genesis/vdp_state.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace oasis::game {

// Standalone C++ game simulation vertical slice.
// Implements IGameSimulation without linking GPGX, Musashi or libretro.
// Consumes frame input, advances reverse-engineered player state machine and movement math,
// and publishes updated VDP SAT/VRAM state for standalone rasterization.
class StandaloneGameSimulation : public IGameSimulation {
public:
    StandaloneGameSimulation();
    ~StandaloneGameSimulation() override = default;

    // Initializes baseline VDP state and player initial position.
    bool initialize(const genesis::VdpState& baseline_vdp,
                    std::int32_t initial_x = 261,
                    std::int32_t initial_y = 107);

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

    // Access player movement configuration.
    [[nodiscard]] const player::PlayerMovementConfig& config() const noexcept {
        return movement_config_;
    }

    // Reset player position and state back to initialization parameters.
    void reset();

    // Directly set player world coordinates.
    void set_player_position(std::int32_t x, std::int32_t y) noexcept;

    // Camera origin offset in world coordinates.
    [[nodiscard]] int camera_x() const noexcept { return camera_x_; }
    [[nodiscard]] int camera_y() const noexcept { return camera_y_; }

    // Toggle camera tracking player vs fixed camera.
    void set_camera_tracking(bool track) noexcept { camera_tracking_ = track; }
    [[nodiscard]] bool camera_tracking() const noexcept { return camera_tracking_; }

private:
    void update_sat_sprites(genesis::VdpState& vdp) const;

    genesis::VdpState baseline_vdp_{};
    player::PlayerState player_{};
    player::PlayerMovementConfig movement_config_{};
    player::VelocityAdjustContext velocity_context_{};

    std::vector<std::uint8_t> terrain_buffer_{};
    world::ByteGridView terrain_{};

    SimulationState sim_state_{};
    std::int32_t initial_x_{261};
    std::int32_t initial_y_{107};
    int camera_x_{125};
    int camera_y_{46};
    bool camera_tracking_{false};
    int walk_tick_{0};
    bool initialized_{false};
};

} // namespace oasis::game
