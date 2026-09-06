#pragma once

#include "core/runtime.hpp"
#include "game/player/player.hpp"
#include "game/render/framebuffer.hpp"

#include <array>
#include <cstdint>
#include <span>

namespace oasis::game::screen {

struct ScreenState {
    std::uint64_t frame_index{};
    player::PlayerState player{};
    player::MovementResult movement{};

    friend bool operator==(const ScreenState&, const ScreenState&) noexcept;
};

class ControlledScreen final : public core::FrameClient {
public:
    static constexpr std::uint16_t kGridWidth = 32;
    static constexpr std::uint16_t kGridHeight = 28;

    ControlledScreen();

    void update(const core::FrameContext& frame) override;

    void render(std::span<std::uint32_t> destination) const noexcept;

    [[nodiscard]] const ScreenState& state() const noexcept { return state_; }
    [[nodiscard]] std::span<const std::uint8_t> fixture_cells() const noexcept {
        return fixture_cells_;
    }
    [[nodiscard]] const player::PlayerMovementConfig& movement_config() const noexcept {
        return movement_config_;
    }

private:
    void build_fixture() noexcept;

    std::array<std::uint8_t, kGridWidth * kGridHeight> fixture_cells_{};
    world::ByteGridView terrain_;
    player::PlayerMovementConfig movement_config_{};
    ScreenState state_{};
};

} // namespace oasis::game::screen
