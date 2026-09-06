#include "game/render/framebuffer.hpp"

namespace oasis::game::render {

SoftwareFramebuffer::SoftwareFramebuffer()
    : pixels_(static_cast<std::size_t>(kWidth) * kHeight, 0U) {}

} // namespace oasis::game::render
