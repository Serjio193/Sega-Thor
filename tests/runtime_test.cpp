#include "core/runtime.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <vector>

namespace {

struct RecordingClient final : oasis::core::FrameClient {
    std::vector<std::uint64_t> trace;

    void update(const oasis::core::FrameContext& frame) override {
        std::uint64_t value = frame.frame_index;
        value = value * 1315423911ULL + frame.input.port1.buttons;
        value = value * 1315423911ULL + frame.input.port2.buttons;
        trace.push_back(value);
    }
};

std::vector<std::uint64_t> run_sequence(
    const std::array<oasis::core::InputSnapshot, 5>& sequence) {
    RecordingClient client;
    oasis::core::RuntimeLoop runtime(client);
    for (const auto& input : sequence) runtime.step(input);
    assert(runtime.frame_index() == sequence.size());
    return client.trace;
}

} // namespace

int main() {
    using oasis::core::Button;
    using oasis::core::ControllerState;
    using oasis::core::InputSnapshot;

    ControllerState pad;
    pad.set(Button::A);
    pad.set(Button::Right);
    assert(pad.pressed(Button::A));
    assert(pad.pressed(Button::Right));
    assert(!pad.pressed(Button::B));
    pad.set(Button::A, false);
    assert(!pad.pressed(Button::A));

    std::array<InputSnapshot, 5> sequence{};
    sequence[0].port1.set(Button::Right);
    sequence[1].port1.set(Button::Right);
    sequence[1].port1.set(Button::B);
    sequence[2].port1.set(Button::B);
    sequence[3].port1.set(Button::Start);
    sequence[4].port2.set(Button::A);

    const auto first = run_sequence(sequence);
    const auto second = run_sequence(sequence);
    assert(first == second);
    assert(first.size() == sequence.size());

    auto changed = sequence;
    changed[2].port1.set(Button::C);
    const auto third = run_sequence(changed);
    assert(first != third);

    return 0;
}
