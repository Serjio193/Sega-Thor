#pragma once

#include "game/render/sat_pipeline_contract.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::render {

// Standalone C++20 SAT Record definition matching Mega Drive VDP hardware layout.
struct SATRecord {
    std::uint16_t y_pos{};
    std::uint8_t size{};
    std::uint8_t link{};
    std::uint16_t attr{};
    std::uint16_t x_pos{};

    [[nodiscard]] constexpr int screen_x() const noexcept { return static_cast<int>(x_pos & 0x01FFU) - 128; }
    [[nodiscard]] constexpr int screen_y() const noexcept { return static_cast<int>(y_pos & 0x03FFU) - 128; }
    [[nodiscard]] constexpr std::uint8_t width_tiles() const noexcept { return static_cast<std::uint8_t>(((size >> 2U) & 0x03U) + 1U); }
    [[nodiscard]] constexpr std::uint8_t height_tiles() const noexcept { return static_cast<std::uint8_t>((size & 0x03U) + 1U); }
    [[nodiscard]] constexpr std::uint16_t tile_index() const noexcept { return attr & 0x07FFU; }
    [[nodiscard]] constexpr std::uint8_t palette() const noexcept { return static_cast<std::uint8_t>((attr >> 13U) & 0x03U); }
    [[nodiscard]] constexpr bool priority() const noexcept { return (attr & 0x8000U) != 0U; }
    [[nodiscard]] constexpr bool v_flip() const noexcept { return (attr & 0x1000U) != 0U; }
    [[nodiscard]] constexpr bool h_flip() const noexcept { return (attr & 0x0800U) != 0U; }
    [[nodiscard]] constexpr std::uint8_t link_target() const noexcept { return link; }
};
static_assert(sizeof(SATRecord) == 8, "SATRecord must be exactly 8 bytes");

// High-level sprite piece descriptor before composition and coordinate mapping.
struct SpritePiece {
    std::int16_t offset_x{};
    std::int16_t offset_y{};
    std::uint8_t width_tiles{1};
    std::uint8_t height_tiles{1};
    std::uint16_t tile_index{};
    std::uint8_t palette{};
    bool priority{true};
    bool h_flip{false};
    bool v_flip{false};
};

// Abstract C++20 domain interface for sprite composition (Mode 0 + Mode 1).
class ISpriteCompositor {
public:
    virtual ~ISpriteCompositor() = default;

    virtual void reset_frame() noexcept = 0;

    // Sprite Engine Mode 0: standard world-space entity composition with camera subtraction.
    virtual bool compose_mode0(int world_x, int world_y,
                               std::span<const SpritePiece> pieces,
                               bool flip_h, bool flip_v) noexcept = 0;

    // Sprite Engine Mode 1: weapon/shadow/attached sprite composition with absolute/relative support.
    virtual bool compose_mode1(int world_x, int world_y,
                               std::span<const SpritePiece> pieces,
                               bool flip_h, bool flip_v,
                               bool absolute_coords) noexcept = 0;

    virtual bool emit_piece(const SpritePiece& piece, int screen_x, int screen_y) noexcept = 0;
    virtual bool emit_shadow(int screen_x, int screen_y, std::uint16_t tile_index, std::uint8_t palette) noexcept = 0;
    virtual SatFrameSummary finalize_frame() noexcept = 0;

    [[nodiscard]] virtual std::size_t active_count() const noexcept = 0;
    [[nodiscard]] virtual std::span<const SATRecord> records() const noexcept = 0;
};

// Reusable concrete C++20 Sprite Compositor implementing ISpriteCompositor.
class SpriteCompositor final : public ISpriteCompositor {
public:
    static constexpr std::size_t kMaxSprites = 80;
    static constexpr int kCoordOffset = 128;
    static constexpr int kScreenWidth = 320;
    static constexpr int kScreenHeight = 224;

    SpriteCompositor() noexcept;

    void reset_frame() noexcept override;
    bool compose_mode0(int world_x, int world_y,
                       std::span<const SpritePiece> pieces,
                       bool flip_h, bool flip_v) noexcept override;
    bool compose_mode1(int world_x, int world_y,
                       std::span<const SpritePiece> pieces,
                       bool flip_h, bool flip_v,
                       bool absolute_coords) noexcept override;
    bool emit_piece(const SpritePiece& piece, int screen_x, int screen_y) noexcept override;
    bool emit_shadow(int screen_x, int screen_y, std::uint16_t tile_index, std::uint8_t palette) noexcept override;
    SatFrameSummary finalize_frame() noexcept override;

    [[nodiscard]] std::size_t active_count() const noexcept override { return active_count_; }
    [[nodiscard]] std::span<const SATRecord> records() const noexcept override;

    void set_camera(int cam_x, int cam_y) noexcept { cam_x_ = cam_x; cam_y_ = cam_y; }

private:
    std::array<SATRecord, kMaxSprites> records_{};
    std::size_t active_count_{};
    int cam_x_{};
    int cam_y_{};
    bool dirty_{};
};

} // namespace oasis::game::render
