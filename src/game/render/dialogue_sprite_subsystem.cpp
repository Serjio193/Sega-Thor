#include "game/render/dialogue_sprite_subsystem.hpp"

namespace oasis::game::render {

namespace {

[[nodiscard]] inline std::int16_t read_i16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 1 >= buf.size()) {
        return 0;
    }
    return static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(buf[offset]) << 8) |
        static_cast<std::uint16_t>(buf[offset + 1]));
}

[[nodiscard]] inline std::uint16_t read_u16_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 1 >= buf.size()) {
        return 0U;
    }
    return static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(buf[offset]) << 8) |
        static_cast<std::uint16_t>(buf[offset + 1]));
}

[[nodiscard]] inline std::uint32_t read_u32_be(std::span<const std::uint8_t> buf, std::size_t offset) noexcept {
    if (offset + 3 >= buf.size()) {
        return 0U;
    }
    return (static_cast<std::uint32_t>(buf[offset]) << 24) |
           (static_cast<std::uint32_t>(buf[offset + 1]) << 16) |
           (static_cast<std::uint32_t>(buf[offset + 2]) << 8) |
           static_cast<std::uint32_t>(buf[offset + 3]);
}

inline void write_u16_be(std::span<std::uint8_t> buf, std::size_t offset, std::uint16_t val) noexcept {
    if (offset + 1 < buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
        buf[offset + 1] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

inline void write_u32_be(std::span<std::uint8_t> buf, std::size_t offset, std::uint32_t val) noexcept {
    if (offset + 3 < buf.size()) {
        buf[offset] = static_cast<std::uint8_t>((val >> 24) & 0xFFU);
        buf[offset + 1] = static_cast<std::uint8_t>((val >> 16) & 0xFFU);
        buf[offset + 2] = static_cast<std::uint8_t>((val >> 8) & 0xFFU);
        buf[offset + 3] = static_cast<std::uint8_t>(val & 0xFFU);
    }
}

// Authentic ROM templates
constexpr std::array<VdpSpriteTemplate, 9> kPrimaryTemplate = {{
    {136U, 0x0901U, 0x8781U, 104},
    {152U, 0x0302U, 0x8787U, 112},
    {184U, 0x0203U, 0x878BU, 112},
    {208U, 0x0904U, 0x878EU, 104},
    {224U, 0x0305U, 0x8794U, 112},
    {256U, 0x0206U, 0x8798U, 112},
    {136U, 0x0907U, 0x879BU, 104},
    {152U, 0x0308U, 0x87A1U, 112},
    {184U, 0x0209U, 0x87A5U, 112},
}};

constexpr std::array<VdpSpriteTemplate, 9> kSecondaryTemplate = {{
    {136U, 0x0001U, 0x8780U, 104},
    {152U, 0x0002U, 0x8780U, 112},
    {184U, 0x0003U, 0x8780U, 112},
    {208U, 0x0004U, 0x8780U, 104},
    {224U, 0x0005U, 0x8780U, 112},
    {256U, 0x0006U, 0x8780U, 112},
    {136U, 0x0007U, 0x8780U, 104},
    {152U, 0x0008U, 0x8780U, 112},
    {184U, 0x0009U, 0x8780U, 112},
}};

} // namespace

const std::array<VdpSpriteTemplate, 9>& get_primary_dialogue_template() noexcept {
    return kPrimaryTemplate;
}

const std::array<VdpSpriteTemplate, 9>& get_secondary_dialogue_template() noexcept {
    return kSecondaryTemplate;
}

DialogueSpriteCompositionResult compose_dialogue_sprites(
    const DialogueSpriteInput& input) noexcept {
    DialogueSpriteCompositionResult result{};

    const auto& tmpl = (input.style == 0U) ? kPrimaryTemplate : kSecondaryTemplate;

    std::uint16_t cur_link = input.start_link_count;
    std::uint16_t cur_offset = input.start_sat_offset;

    // 1. Emit 6 dialogue border sprites (0x00A36A..0x00A37E)
    for (std::size_t i = 0U; i < DialogueSpriteSubsystemConfig::kBorderSpriteCount; ++i) {
        cur_link = static_cast<std::uint16_t>((cur_link + 1U) & 0xFFU);
        DialogueSpriteOutput sp{};
        sp.y = tmpl[i].y;
        sp.size = static_cast<std::uint8_t>((tmpl[i].size_link >> 8) & 0xFFU);
        sp.link = static_cast<std::uint8_t>(cur_link);
        sp.attr = tmpl[i].attr;
        sp.x = static_cast<std::int16_t>(tmpl[i].x + input.offset_x);
        result.sprites.push_back(sp);
        cur_offset = static_cast<std::uint16_t>(cur_offset + 8U);
    }

    // 2. Emit 3 speaker portrait sprites if active (0x00A380..0x00A3A6)
    if (input.portrait_active) {
        const std::int16_t portrait_base_x = static_cast<std::int16_t>(
            DialogueSpriteSubsystemConfig::kPortraitBaseX - input.offset_x);
        for (std::size_t i = 6U; i < 9U; ++i) {
            cur_link = static_cast<std::uint16_t>((cur_link + 1U) & 0xFFU);
            DialogueSpriteOutput sp{};
            sp.y = tmpl[i].y;
            sp.size = static_cast<std::uint8_t>((tmpl[i].size_link >> 8) & 0xFFU);
            sp.link = static_cast<std::uint8_t>(cur_link);
            sp.attr = tmpl[i].attr;
            sp.x = static_cast<std::int16_t>(tmpl[i].x + portrait_base_x);
            result.sprites.push_back(sp);
            cur_offset = static_cast<std::uint16_t>(cur_offset + 8U);
        }
    }

    // 3. Emit 1 flashing cursor sprite if active (0x00A3A8..0x00A3DA)
    if (input.cursor_active) {
        if (cur_link < DialogueSpriteSubsystemConfig::kCursorMaxSpriteLimit) {
            cur_link = static_cast<std::uint16_t>((cur_link + 1U) & 0xFFU);
            DialogueSpriteOutput sp{};
            sp.y = DialogueSpriteSubsystemConfig::kCursorFixedY;
            sp.size = 0x0AU; // 0x0A00 >> 8
            sp.link = static_cast<std::uint8_t>(cur_link);
            sp.attr = DialogueSpriteSubsystemConfig::kCursorTileAttr;
            sp.x = static_cast<std::int16_t>(input.offset_x + DialogueSpriteSubsystemConfig::kCursorXOffset);
            result.sprites.push_back(sp);
            cur_offset = static_cast<std::uint16_t>(cur_offset + 8U);
        }

        // 4. Cursor frame animation & DMA queue (0x00A3DC..0x00A422)
        result.cursor_dma = calculate_cursor_dma_command(input.cursor_new_frame, input.cursor_prev_frame);
    }

    result.new_sat_byte_offset = cur_offset;
    result.new_sprite_link_count = cur_link;
    return result;
}

DialogueSpriteCompositionResult compose_dialogue_sprites_raw(
    std::span<std::uint8_t> ram_buffer,
    std::uint32_t ram_base_addr) {
    auto ram_offset = [ram_base_addr, buf_size = ram_buffer.size()](std::uint32_t addr) -> std::size_t {
        if (addr < ram_base_addr || (addr - ram_base_addr) >= buf_size) {
            return buf_size;
        }
        return static_cast<std::size_t>(addr - ram_base_addr);
    };

    DialogueSpriteInput input{};
    const std::size_t style_pos = ram_offset(DialogueSpriteSubsystemConfig::kDialogueStyleRam);
    if (style_pos < ram_buffer.size()) {
        input.style = ram_buffer[style_pos];
    }

    const std::size_t x_pos = ram_offset(DialogueSpriteSubsystemConfig::kDialogueOffsetXRam);
    input.offset_x = read_i16_be(ram_buffer, x_pos);

    const std::size_t port_pos = ram_offset(DialogueSpriteSubsystemConfig::kPortraitActiveRam);
    input.portrait_active = (port_pos < ram_buffer.size()) && (ram_buffer[port_pos] != 0U);

    const std::size_t cur_act_pos = ram_offset(DialogueSpriteSubsystemConfig::kCursorActiveRam);
    input.cursor_active = (read_u16_be(ram_buffer, cur_act_pos) != 0U);

    const std::size_t f_new_pos = ram_offset(DialogueSpriteSubsystemConfig::kCursorNewFrameRam);
    if (f_new_pos < ram_buffer.size()) {
        input.cursor_new_frame = ram_buffer[f_new_pos];
    }

    const std::size_t f_prev_pos = ram_offset(DialogueSpriteSubsystemConfig::kCursorPrevFrameRam);
    if (f_prev_pos < ram_buffer.size()) {
        input.cursor_prev_frame = ram_buffer[f_prev_pos];
    }

    const std::size_t sat_off_pos = ram_offset(DialogueSpriteSubsystemConfig::kSatByteOffsetRam);
    input.start_sat_offset = read_u16_be(ram_buffer, sat_off_pos);

    const std::size_t sat_lnk_pos = ram_offset(DialogueSpriteSubsystemConfig::kSatLinkCountRam);
    input.start_link_count = read_u16_be(ram_buffer, sat_lnk_pos);

    DialogueSpriteCompositionResult res = compose_dialogue_sprites(input);

    // Write emitted sprites to VDP SAT buffer
    const std::size_t sat_buf_pos = ram_offset(DialogueSpriteSubsystemConfig::kVdpSatBufferRam + input.start_sat_offset);
    if (sat_buf_pos < ram_buffer.size()) {
        std::size_t out_off = sat_buf_pos;
        for (const auto& sp : res.sprites) {
            if (out_off + 7 < ram_buffer.size()) {
                write_u16_be(ram_buffer, out_off + 0, sp.y);
                ram_buffer[out_off + 2] = sp.size;
                ram_buffer[out_off + 3] = sp.link;
                write_u16_be(ram_buffer, out_off + 4, sp.attr);
                write_u16_be(ram_buffer, out_off + 6, static_cast<std::uint16_t>(sp.x));
                out_off += 8;
            }
        }
    }

    // Process cursor DMA if triggered
    if (res.cursor_dma.triggered) {
        if (f_prev_pos < ram_buffer.size()) {
            ram_buffer[f_prev_pos] = res.cursor_dma.updated_frame;
        }

        const std::size_t q_ptr_pos = ram_offset(DialogueSpriteSubsystemConfig::kDmaQueuePtrRam);
        std::uint32_t q_ptr = read_u32_be(ram_buffer, q_ptr_pos);
        const std::size_t q_off = ram_offset(q_ptr);
        if (q_off + 7 < ram_buffer.size()) {
            write_u32_be(ram_buffer, q_off + 0, res.cursor_dma.rom_source_addr);
            write_u16_be(ram_buffer, q_off + 4, res.cursor_dma.vram_dest_addr);
            write_u16_be(ram_buffer, q_off + 6, res.cursor_dma.word_length);
            write_u32_be(ram_buffer, q_ptr_pos, q_ptr + 8U);
        }
    }

    // Update SAT offset and link count in RAM
    write_u16_be(ram_buffer, sat_off_pos, res.new_sat_byte_offset);
    write_u16_be(ram_buffer, sat_lnk_pos, res.new_sprite_link_count);

    return res;
}

SpecialEntityAction evaluate_special_entity_preset(
    std::int16_t entity_type,
    std::uint32_t script_ptr) noexcept {
    if (entity_type == -2) {
        if (script_ptr != 0U) {
            return SpecialEntityAction::ExecuteScriptPointer;
        }
    }
    if (entity_type < 0) {
        return SpecialEntityAction::InvokeDemoSpriteHook;
    }
    return SpecialEntityAction::None;
}

} // namespace oasis::game::render
