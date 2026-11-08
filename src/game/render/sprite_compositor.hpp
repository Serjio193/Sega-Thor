#pragma once

#include "game/render/sat_pipeline_contract.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::genesis {
struct VdpState;
}

namespace oasis::game::render {

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

// Abstract C++20 domain interface for authentic Mega Drive sprite composition.
// Implements Sprite Mode 0 (ROM PC 0x00AD76) and Sprite Mode 1 (ROM PC 0x00A9A0).
class ISpriteCompositor {
public:
    virtual ~ISpriteCompositor() = default;

    virtual void reset_frame() noexcept = 0;

    // Sprite Mode 0 (0x00AD76): Standard world-space entity composition with camera offset.
    virtual bool compose_mode0(int world_x, int world_y,
                               std::span<const SpritePiece> pieces,
                               bool flip_h, bool flip_v) noexcept = 0;

    // Sprite Mode 1 (0x00A9A0): Attached / weapon / child sprite composition.
    virtual bool compose_mode1(int world_x, int world_y,
                               std::span<const SpritePiece> pieces,
                               bool flip_h, bool flip_v,
                               bool absolute_coords) noexcept = 0;

    virtual bool emit_piece(const SpritePiece& piece, int screen_x, int screen_y) noexcept = 0;
    virtual bool emit_shadow(int screen_x, int screen_y, std::uint16_t tile_index, std::uint8_t palette) noexcept = 0;
    virtual SatFrameSummary finalize_frame() noexcept = 0;

    [[nodiscard]] virtual std::size_t active_count() const noexcept = 0;
    [[nodiscard]] virtual std::span<const MegaDriveSatEntry> records() const noexcept = 0;
};

// Concrete C++20 Sprite Compositor implementing ISpriteCompositor.
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

    void commit_to_vdp(genesis::VdpState& vdp) const noexcept;
    [[nodiscard]] std::vector<MegaDriveSatEntry> active_chain() const;

    [[nodiscard]] std::size_t active_count() const noexcept override { return active_count_; }
    [[nodiscard]] std::span<const MegaDriveSatEntry> records() const noexcept override;

    void set_camera(int cam_x, int cam_y) noexcept { cam_x_ = cam_x; cam_y_ = cam_y; }
    [[nodiscard]] int cam_x() const noexcept { return cam_x_; }
    [[nodiscard]] int cam_y() const noexcept { return cam_y_; }

private:
    std::array<MegaDriveSatEntry, kMaxSprites> records_{};
    std::size_t active_count_{};
    int cam_x_{};
    int cam_y_{};
    bool dirty_{};
};

} // namespace oasis::game::render
