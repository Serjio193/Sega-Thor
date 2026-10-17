#pragma once

#include "core/runtime.hpp"
#include "game/genesis_graphics.hpp"
#include "game/player/player.hpp"
#include "game/player/secondary_weapon.hpp"
#include "game/render/framebuffer.hpp"
#include "game/screen_instance.hpp"
#include "game/spirits/spirit_slots.hpp"
#include "game/world/byte_grid.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

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

    static constexpr std::uint16_t kRomMapColumns = 64;
    static constexpr std::uint16_t kRomMapRows = 28;

    explicit ControlledScreen(std::span<const std::uint8_t> rom = {});
    ~ControlledScreen() override = default;

    ControlledScreen(const ControlledScreen&) = delete;
    ControlledScreen& operator=(const ControlledScreen&) = delete;
    ControlledScreen(ControlledScreen&&) noexcept = default;
    ControlledScreen& operator=(ControlledScreen&&) noexcept = default;

    void update(const core::FrameContext& frame) override;

    void render(std::span<std::uint32_t> destination) const noexcept;

    bool trigger_screen_transition(world::ScreenId target_id, bool explicit_debug = true);

    [[nodiscard]] bool is_rom_backed() const noexcept { return is_rom_backed_; }
    [[nodiscard]] const ScreenState& state() const noexcept { return state_; }
    [[nodiscard]] std::span<const std::uint8_t> fixture_cells() const noexcept {
        return fixture_cells_;
    }
    [[nodiscard]] const player::PlayerMovementConfig& movement_config() const noexcept {
        return movement_config_;
    }

    void toggle_debug_overlay() noexcept { show_debug_overlay_ = !show_debug_overlay_; }
    [[nodiscard]] bool debug_overlay_visible() const noexcept { return show_debug_overlay_; }

    [[nodiscard]] std::int32_t camera_x() const noexcept { return camera_x_; }
    [[nodiscard]] std::int32_t camera_y() const noexcept { return camera_y_; }

    [[nodiscard]] std::uint16_t current_screen_setup_id() const noexcept;
    [[nodiscard]] std::uint16_t screen_setup_id() const noexcept {
        return current_screen_setup_id();
    }
    [[nodiscard]] std::uint16_t previous_screen_setup_id() const noexcept {
        return previous_screen_setup_id_;
    }
    [[nodiscard]] std::uint32_t transition_count() const noexcept {
        return transition_count_;
    }
    [[nodiscard]] std::uint32_t descriptor_address() const noexcept;
    [[nodiscard]] std::uint32_t primary_resource_address() const noexcept;
    [[nodiscard]] std::array<std::uint8_t, 4> gfx_resource_ids() const noexcept;

    [[nodiscard]] const ScreenInstance* current_screen() const noexcept {
        return current_screen_.get();
    }
    [[nodiscard]] const ScreenInstance* previous_screen() const noexcept {
        return previous_screen_.get();
    }
    [[nodiscard]] const world::ByteGridView& terrain() const noexcept {
        return terrain_;
    }

    [[nodiscard]] std::size_t entity_count() const noexcept {
        return current_screen_ ? current_screen_->entities.size() : 0U;
    }
    [[nodiscard]] std::span<const entities::ScreenEntity> entities() const noexcept {
        if (current_screen_) return current_screen_->entities;
        return {};
    }
    [[nodiscard]] bool overlap_detected() const noexcept { return overlap_detected_; }
    [[nodiscard]] std::uint16_t interaction_target_type() const noexcept {
        return interaction_target_type_;
    }
    [[nodiscard]] const std::string& interaction_state_str() const noexcept {
        return interaction_state_str_;
    }
    [[nodiscard]] entities::EntityHitbox player_hitbox() const noexcept {
        const auto px = static_cast<std::int32_t>(state_.player.x_fixed / 0x10000);
        const auto py = static_cast<std::int32_t>(state_.player.y_fixed / 0x10000);
        return entities::player_hitbox(px, py);
    }

    [[nodiscard]] const spirits::ActiveSpirit& spirit() const noexcept { return spirit_; }
    [[nodiscard]] spirits::ActiveSpirit& spirit() noexcept { return spirit_; }
    [[nodiscard]] const spirits::SpiritSlots& spirit_slots() const noexcept { return spirit_slots_; }
    [[nodiscard]] spirits::SpiritSlots& spirit_slots() noexcept { return spirit_slots_; }

    [[nodiscard]] bool is_persistent_flag_set(std::uint16_t flag_id) const noexcept {
        const auto byte_idx = static_cast<std::size_t>(flag_id >> 3U);
        if (byte_idx >= persistent_flags_.size()) return false;
        return (persistent_flags_[byte_idx] & (1U << (flag_id & 7U))) != 0;
    }
    void set_persistent_flag(std::uint16_t flag_id) noexcept {
        const auto byte_idx = static_cast<std::size_t>(flag_id >> 3U);
        if (byte_idx < persistent_flags_.size()) {
            persistent_flags_[byte_idx] |= static_cast<std::uint8_t>(1U << (flag_id & 7U));
        }
    }
    [[nodiscard]] std::span<const std::uint8_t> persistent_flags() const noexcept {
        return persistent_flags_;
    }
    [[nodiscard]] const player::WeaponInventory& weapon_inventory() const noexcept { return weapon_inventory_; }
    [[nodiscard]] player::WeaponInventory& weapon_inventory() noexcept { return weapon_inventory_; }
    [[nodiscard]] const player::ArrowProjectile& arrow() const noexcept { return arrow_; }
    [[nodiscard]] player::ArrowProjectile& arrow() noexcept { return arrow_; }

private:
    void build_fixture() noexcept;
    void init_rom_backed(std::span<const std::uint8_t> rom);
    void render_debug_overlay(std::span<std::uint32_t> destination) const noexcept;

    bool is_rom_backed_{false};
    bool show_debug_overlay_{true};
    bool mode_button_held_{false};
    bool button_x_held_{false};
    bool button_a_held_{false};
    bool button_y_held_{false};
    bool button_c_held_{false};
    bool overlap_detected_{false};
    std::uint16_t interaction_target_type_{0};
    std::string interaction_state_str_{"IDLE"};

    std::span<const std::uint8_t> rom_{};
    std::uint16_t previous_screen_setup_id_{0};
    std::uint32_t transition_count_{0};

    std::unique_ptr<ScreenInstance> current_screen_{};
    std::unique_ptr<ScreenInstance> previous_screen_{};

    std::array<std::uint8_t, kGridWidth * kGridHeight> fixture_cells_{};
    world::ByteGridView terrain_;
    player::PlayerMovementConfig movement_config_{};
    ScreenState state_{};
    spirits::ActiveSpirit spirit_{};
    spirits::SpiritSlots spirit_slots_{0x01};
    std::array<std::uint8_t, 32> persistent_flags_{};
    player::WeaponInventory weapon_inventory_{player::default_weapon_inventory()};
    player::ArrowProjectile arrow_{};

    mutable std::int32_t camera_x_{0};
    mutable std::int32_t camera_y_{0};
};

} // namespace oasis::game::screen
