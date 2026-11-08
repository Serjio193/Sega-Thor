#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::entities {

struct SpritePiece {
    std::int16_t x_offset{};
    std::int16_t y_offset{};
    std::uint8_t width_cells{1};
    std::uint8_t height_cells{1};
    std::uint16_t tile_index{};
    std::uint8_t palette_line{};
    bool hflip{false};
    bool vflip{false};
};

struct EntityHitbox {
    std::int32_t left{};
    std::int32_t top{};
    std::int32_t right{};
    std::int32_t bottom{};
    std::int32_t z_min{0};
    std::int32_t z_max{0};

    [[nodiscard]] constexpr bool overlaps(const EntityHitbox& other) const noexcept {
        return left <= other.right && right >= other.left &&
               top <= other.bottom && bottom >= other.top &&
               z_min <= other.z_max && z_max >= other.z_min;
    }
};

enum class EntityInteractionState : std::uint8_t {
    idle,
    ready,
    triggered,
    opened
};

[[nodiscard]] inline EntityHitbox player_hitbox(
    std::int32_t px, std::int32_t py, std::int32_t pz = 0) noexcept {
    constexpr std::int32_t kRadius = 10;
    return EntityHitbox{
        .left = px - kRadius,
        .top = (py + pz) - kRadius,
        .right = px + kRadius,
        .bottom = (py + pz) + kRadius,
        .z_min = pz - 8,
        .z_max = pz + 8,
    };
}

struct EntityDescriptor {
    std::uint16_t type_id{};
    std::uint8_t dim_x{};
    std::uint8_t dim_y{};
    std::uint32_t gfx_descriptor_pointer{};
    std::uint32_t anim_descriptor_pointer{};
    std::uint32_t update_callback_pointer{};
};

struct AnimationState {
    std::uint32_t script_address{0};
    std::uint32_t cursor{0};
    std::uint32_t current_frame_pointer{0};
    std::uint16_t current_frame_index{0};
    std::uint16_t ticks_remaining{0};
    std::uint16_t loop_count{0};
    bool loop_pending{false};
    bool halted{false};
};

struct ActorRuntime {
    std::uint8_t state{0};            // 0: init, 1: wait/idle, 2: walk/charge, 3: return/turn
    std::uint8_t substate{0};         // $66 in ROM
    std::uint16_t timer{0};           // $2 in ROM (decremented in state 2)
    std::uint8_t direction{2};        // 0: Up, 1: Right, 2: Down, 3: Left ($16 in ROM)
    std::int32_t anchor_x{0};         // $a8 in ROM
    std::int32_t anchor_y{0};         // $aa in ROM
    std::int32_t velocity_x_fixed{0}; // $4e in ROM (16.16)
    std::int32_t velocity_y_fixed{0}; // $52 in ROM (16.16)
    std::uint16_t anim_selector{0};   // $2a in ROM
    std::uint8_t wait_duration{30};   // $9e in ROM (duration of walk/charge)
    std::int32_t speed_fixed{0x18000}; // $a0 in ROM (1.5 px/frame fixed)
    std::int16_t hp{20};              // $8a in ROM (Type 0x0048 has 20 HP)
    std::int16_t max_hp{20};          // $88 in ROM
    std::int16_t defense{0};          // $92 in ROM
    std::uint8_t invuln_timer{0};     // $82 in ROM
    std::uint8_t knockback_timer{0};  // $83 in ROM
    bool is_actor{false};
    bool halted{false};

    [[nodiscard]] constexpr bool is_hurt() const noexcept { return state == 4; }
    [[nodiscard]] constexpr bool is_dead() const noexcept { return state == 2 || hp <= 0; }
};

// Reads entry for type_id from canonical ROM table at 0x03F2FA
[[nodiscard]] EntityDescriptor load_entity_descriptor_from_rom(
    std::span<const std::uint8_t> rom,
    std::uint16_t type_id);

// Decodes sprite pieces from ROM frame descriptor (format handled by 0xB730 / 0x167DAC)
[[nodiscard]] std::vector<SpritePiece> decode_sprite_frame_from_rom(
    std::span<const std::uint8_t> rom,
    std::uint32_t frame_address);

} // namespace oasis::game::entities

namespace oasis::game::world {
class ByteGridView;
}

namespace oasis::game::entities {

class ScreenEntity {
public:
    static constexpr std::uint32_t kEntityTypeTableAddress = 0x03F2FAU;

    ScreenEntity() noexcept = default;

    static ScreenEntity spawn_from_rom(
        std::span<const std::uint8_t> rom,
        std::uint16_t type_id,
        std::int32_t world_x,
        std::int32_t world_y,
        std::int32_t world_z = 0,
        std::uint8_t initial_direction = 2);

    static ScreenEntity spawn_drop_from_rom(
        std::span<const std::uint8_t> rom,
        std::uint16_t item_id,
        std::int32_t world_x,
        std::int32_t world_y,
        std::int32_t world_z = 0,
        std::uint8_t count = 1);

    void update(std::span<const std::uint8_t> rom = {},
                const world::ByteGridView* terrain = nullptr,
                std::int32_t player_x = 0,
                std::int32_t player_y = 0) noexcept;
    void trigger_interaction() noexcept;

    [[nodiscard]] std::uint16_t type_id() const noexcept { return type_id_; }
    [[nodiscard]] std::int32_t world_x() const noexcept { return world_x_; }
    [[nodiscard]] std::int32_t world_y() const noexcept { return world_y_; }
    [[nodiscard]] std::int32_t world_z() const noexcept { return world_z_; }
    [[nodiscard]] bool active() const noexcept { return active_; }
    [[nodiscard]] const EntityDescriptor& descriptor() const noexcept { return descriptor_; }
    [[nodiscard]] const AnimationState& anim_state() const noexcept { return anim_state_; }
    [[nodiscard]] const ActorRuntime& actor() const noexcept { return actor_; }
    [[nodiscard]] ActorRuntime& actor() noexcept { return actor_; }
    [[nodiscard]] bool is_actor() const noexcept { return actor_.is_actor; }
    [[nodiscard]] bool is_item() const noexcept { return is_item_; }
    [[nodiscard]] std::uint16_t item_id() const noexcept { return item_id_; }
    [[nodiscard]] std::uint8_t item_count() const noexcept { return item_count_; }
    [[nodiscard]] bool drop_pending() const noexcept { return drop_pending_; }
    [[nodiscard]] std::uint16_t drop_item_id() const noexcept { return drop_item_id_; }
    [[nodiscard]] std::uint8_t drop_count() const noexcept { return drop_count_; }
    void clear_drop_pending() noexcept { drop_pending_ = false; }
    void collect() noexcept { active_ = false; }
    [[nodiscard]] bool is_flame() const noexcept { return type_id_ == 0x004C; }
    [[nodiscard]] std::uint8_t flame_substate() const noexcept { return flame_substate_; }
    [[nodiscard]] std::uint16_t flame_timer() const noexcept { return flame_timer_; }
    [[nodiscard]] bool flame_visible() const noexcept { return flame_visible_; }
    [[nodiscard]] std::uint16_t persistent_id() const noexcept { return persistent_id_; }
    void set_persistent_id(std::uint16_t id) noexcept { persistent_id_ = id; }
    void extinguish() noexcept { flame_substate_ = 0xFF; }

    [[nodiscard]] std::span<const SpritePiece> pieces() const noexcept { return pieces_; }
    [[nodiscard]] std::uint16_t tile_base() const noexcept { return tile_base_; }
    [[nodiscard]] EntityInteractionState interaction_state() const noexcept {
        return interaction_state_;
    }

    [[nodiscard]] EntityHitbox hitbox() const noexcept {
        const std::int32_t hw = descriptor_.dim_x > 0 ? (descriptor_.dim_x / 2) : 8;
        const std::int32_t hh = descriptor_.dim_y > 0 ? (descriptor_.dim_y / 2) : 8;
        return EntityHitbox{
            .left = world_x_ - hw,
            .top = (world_y_ + world_z_) - hh,
            .right = world_x_ + hw,
            .bottom = (world_y_ + world_z_) + hh,
            .z_min = world_z_ - 8,
            .z_max = world_z_ + 8,
        };
    }

    [[nodiscard]] std::optional<EntityHitbox> attack_hitbox() const noexcept;

    void set_tile_base(std::uint16_t base) noexcept { tile_base_ = base; }
    void set_active(bool act) noexcept { active_ = act; }
    void set_interaction_state(EntityInteractionState st) noexcept {
        interaction_state_ = st;
    }
    void set_anim_script(std::uint32_t script_address) noexcept {
        anim_state_.script_address = script_address;
        anim_state_.cursor = 0U;
        anim_state_.ticks_remaining = 1U;
        anim_state_.halted = false;
    }

    void update_actor_state_machine(
        const world::ByteGridView* terrain,
        std::int32_t px,
        std::int32_t py) noexcept;
    void apply_actor_anim_frame() noexcept;
    bool apply_damage(std::int16_t damage, std::uint8_t hit_direction) noexcept;

private:
    std::uint16_t type_id_{0};
    std::int32_t world_x_{0};
    std::int32_t world_y_{0};
    std::int32_t world_z_{0};
    bool active_{false};
    bool is_item_{false};
    std::uint16_t item_id_{0};
    std::uint8_t item_count_{1};
    bool drop_pending_{false};
    std::uint16_t drop_item_id_{0x001E};
    std::uint8_t drop_count_{1};
    std::uint8_t flame_substate_{0};
    std::uint16_t flame_timer_{0};
    std::uint16_t persistent_id_{0};
    bool flame_visible_{true};
    std::uint16_t tile_base_{0};
    EntityInteractionState interaction_state_{EntityInteractionState::idle};
    EntityDescriptor descriptor_{};
    AnimationState anim_state_{};
    ActorRuntime actor_{};
    std::vector<SpritePiece> pieces_{};
    std::span<const std::uint8_t> rom_{};
};

} // namespace oasis::game::entities
