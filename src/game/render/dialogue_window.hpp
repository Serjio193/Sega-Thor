#pragma once

#include "game/event/event_vm.hpp"
#include "genesis/vdp_state.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace oasis::game::render {

// Authentic dialogue window layout parameters (320x224 Genesis resolution)
// Window is 28 columns (224 pixels) wide, 6 rows (48 pixels) tall, centered at bottom.
struct DialogueWindowConfig {
    int start_tile_x{2};
    int start_tile_y{20};
    int width_tiles{36};
    int height_tiles{6};
    std::uint16_t border_pattern_base{0x0400U};
    std::uint16_t font_pattern_base{0x0420U};
    std::uint8_t palette_line{0U};
};

class DialogueWindow {
public:
    DialogueWindow() noexcept;

    void reset() noexcept;

    // Opens the dialogue window with the specified sequence
    void open(event::DialogueSequence sequence) noexcept;

    // Closes the dialogue window
    void close() noexcept;

    [[nodiscard]] bool is_open() const noexcept { return is_open_; }
    [[nodiscard]] const event::DialogueSequence& sequence() const noexcept { return sequence_; }
    [[nodiscard]] std::uint16_t message_id() const noexcept { return sequence_.message_id; }
    [[nodiscard]] std::size_t current_page_index() const noexcept { return current_page_index_; }

    // Advances to next page, or closes if on last page. Returns true if still open.
    bool advance_page() noexcept;

    // Updates internal frame timer (e.g. for typewriter / cursor blink)
    void step_frame() noexcept;

    // Renders the dialogue window and text tiles into VDP Plane A / VRAM
    void render_to_vdp(genesis::VdpState& vdp) const noexcept;

    // Renders text overlay directly to RGB framebuffer (320x224 RGB24)
    void render_to_rgb(std::span<std::uint8_t> rgb_buffer) const noexcept;

private:
    DialogueWindowConfig config_{};
    bool is_open_{false};
    event::DialogueSequence sequence_{};
    std::size_t current_page_index_{0};
    std::uint32_t frame_counter_{0};
};

} // namespace oasis::game::render
