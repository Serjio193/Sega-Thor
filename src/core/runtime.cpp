#include "core/runtime.hpp"

namespace oasis::core {

void RuntimeLoop::step(const InputSnapshot& input) {
    client_.update(FrameContext{frame_index_, input});
    ++frame_index_;
}

} // namespace oasis::core
