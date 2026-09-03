#pragma once

#include <cstdint>

namespace oasis::core {

enum class Button : std::uint16_t {
    Up    = 1U << 0U,
    Down  = 1U << 1U,
    Left  = 1U << 2U,
    Right = 1U << 3U,
    A     = 1U << 4U,
    B     = 1U << 5U,
    C     = 1U << 6U,
    Start = 1U << 7U,
    X     = 1U << 8U,
    Y     = 1U << 9U,
    Z     = 1U << 10U,
    Mode  = 1U << 11U,
};

struct ControllerState {
    std::uint16_t buttons{};

    [[nodiscard]] constexpr bool pressed(Button button) const noexcept {
        return (buttons & static_cast<std::uint16_t>(button)) != 0;
    }

    constexpr void set(Button button, bool down = true) noexcept {
        const auto mask = static_cast<std::uint16_t>(button);
        if (down) buttons = static_cast<std::uint16_t>(buttons | mask);
        else buttons = static_cast<std::uint16_t>(buttons & ~mask);
    }
};

struct InputSnapshot {
    ControllerState port1{};
    ControllerState port2{};
};

struct FrameContext {
    std::uint64_t frame_index{};
    InputSnapshot input{};
};

class FrameClient {
public:
    virtual ~FrameClient() = default;
    virtual void update(const FrameContext& frame) = 0;
};

class RuntimeLoop {
public:
    explicit RuntimeLoop(FrameClient& client) noexcept : client_(client) {}

    void step(const InputSnapshot& input);

    [[nodiscard]] std::uint64_t frame_index() const noexcept { return frame_index_; }

private:
    FrameClient& client_;
    std::uint64_t frame_index_{};
};

} // namespace oasis::core
