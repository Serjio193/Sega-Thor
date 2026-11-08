#pragma once

#include "core/runtime.hpp"
#include "genesis/vdp_rasterizer.hpp"
#include "genesis/vdp_state.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace oasis::game {

// Decoupled input representation for a single logical simulation frame.
struct FrameInput {
    std::uint32_t frame_index{0};
    core::ControllerState controller_1{};
    core::ControllerState controller_2{};
};

// Decoupled game world and memory state representation.
struct SimulationState {
    std::vector<std::uint8_t> work_ram;
    std::uint32_t frame_counter{0};
    std::uint16_t player_x{0};
    std::uint16_t player_y{0};
    std::uint8_t current_screen_id{0};
    bool paused{false};
};

// Simulation engine interface: consumes input and updates internal game state & VDP state.
// Decoupled from any host windowing, audio, or physical rendering hardware.
class IGameSimulation {
public:
    virtual ~IGameSimulation() = default;

    // Advances the simulation by exactly one deterministic frame.
    // Produces updated VDP state ready for rasterization.
    virtual void step_frame(const FrameInput& input, genesis::VdpState& out_vdp) = 0;

    // Read-only access to current simulation state.
    [[nodiscard]] virtual const SimulationState& state() const noexcept = 0;
};

// Frame sequencer: an implementation for playing and navigating authentic captured VDP frame sequences.
// Used for deterministic regression testing, oracle validation, and standalone playback.
class FrameSequenceSource {
public:
    FrameSequenceSource() = default;
    explicit FrameSequenceSource(std::vector<genesis::VdpState> frames)
        : frames_(std::move(frames)) {}

    [[nodiscard]] std::size_t frame_count() const noexcept { return frames_.size(); }
    [[nodiscard]] std::size_t current_index() const noexcept { return current_idx_; }
    [[nodiscard]] bool is_empty() const noexcept { return frames_.empty(); }

    void seek(std::size_t index) noexcept {
        if (!frames_.empty()) current_idx_ = index % frames_.size();
    }

    void advance() noexcept {
        if (!frames_.empty()) current_idx_ = (current_idx_ + 1) % frames_.size();
    }

    void rewind() noexcept {
        if (!frames_.empty()) current_idx_ = (current_idx_ + frames_.size() - 1) % frames_.size();
    }

    void reset() noexcept { current_idx_ = 0; }

    [[nodiscard]] const genesis::VdpState* current_vdp() const noexcept {
        if (frames_.empty()) return nullptr;
        return &frames_[current_idx_];
    }

    void add_frame(genesis::VdpState state) {
        frames_.push_back(std::move(state));
    }

private:
    std::vector<genesis::VdpState> frames_;
    std::size_t current_idx_{0};
};

// Pipeline context: connects input -> simulation/sequence -> VDP rasterizer -> framebuffer presentation.
struct StandaloneFrameContext {
    FrameInput input;
    genesis::VdpState vdp;
    std::vector<std::uint32_t> rgba32_framebuffer;
    genesis::RasterMetrics metrics;
};

} // namespace oasis::game
