#pragma once

#include "game/spirits/dytto_controller.hpp"
#include "genesis/vdp_state.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace oasis::game::spirits {

// ROM 0x014522 (DMA loader) & 0x014668..0x014798 (spawn) & 0x00DF54 (update dispatch).
struct SpiritResourceSpec {
    std::uint8_t spirit_index{0};       // 0=Dytto, 1=Efreet, 2=Shade, 3=Bow ($FF0D7A bit)
    std::uint16_t type_id{0};           // 0x0018, 0x001A, 0x001C, 0x001E
    std::uint8_t active_flag_bit{0};    // Bit in $FF0D7A
    std::uint32_t spawn_routine_pc{0};  // 0x014668..0x014798
    std::uint32_t update_handler_pc{0}; // 0x011B4E..0x01387C
    std::uint32_t dma_branch_pc{0};     // 0x01454C..0x0145F0
    std::uint32_t nemesis_rom_addr{0};  // Compressed sprite stream in ROM
    std::uint16_t vram_tile{0};         // Target VDP tile index
    std::uint16_t vram_byte_addr{0};    // Target VDP VRAM byte address (vram_tile * 32)
    std::uint16_t max_sp{100U};                 // $88/$8A(A6)
    std::uint16_t sp_drain_rate{10U};           // $8C(A6)
    std::uint16_t sp_drain_interval_frames{45U}; // $8E(A6) frames per 1 SP: 45/35/40/30
    std::uint32_t despawn_handler_pc{0U};       // 0x012314, 0x012BA8, 0x0136CC, 0x013F22
    std::uint32_t state_table_pc{0U};           // 0x011BAC, 0x0125AE, 0x013008, 0x0138E6
};

inline constexpr std::array<SpiritResourceSpec, 4> kAllSpiritSpecs = {{
    {0U, 0x0018U, 0U, 0x014668U, 0x011B4EU, 0x01454CU, 0x00141580U, 0x04D0U, 0x9A00U, 100U, 10U, 45U, 0x012314U, 0x011BACU}, // Dytto
    {1U, 0x001AU, 1U, 0x0146D6U, 0x01254CU, 0x014582U, 0x001425AEU, 0x0450U, 0x8A00U, 100U, 10U, 35U, 0x012BA8U, 0x0125AEU}, // Efreet
    {2U, 0x001CU, 2U, 0x014742U, 0x012F1EU, 0x0145B8U, 0x00143262U, 0x0280U, 0x5000U, 100U, 10U, 40U, 0x0136CCU, 0x013008U}, // Shade
    {3U, 0x001EU, 3U, 0x014798U, 0x01387CU, 0x0145F0U, 0x00143A42U, 0x0360U, 0x6C00U, 100U, 10U, 30U, 0x013F22U, 0x0138E6U}, // Bow
}};

[[nodiscard]] std::optional<SpiritResourceSpec> get_spirit_spec(std::uint8_t spirit_index) noexcept;
[[nodiscard]] std::optional<SpiritResourceSpec> get_spirit_spec_by_type(std::uint16_t type_id) noexcept;

// Decompresses spirit `spirit_index` (0..3) Nemesis stream via 0x00003820 and uploads
// to `vdp.vram` at `spec.vram_byte_addr`. Returns bytes written or 0 on error.
std::size_t upload_spirit_tiles_to_vram(
    std::span<const std::uint8_t> rom,
    std::uint8_t spirit_index,
    genesis::VdpState& vdp) noexcept;

// Unified controller managing any of the 4 summoned spirits (0=Dytto, 1=Efreet, 2=Shade, 3=Bow).
// Delegates Dytto (index 0) to DyttoSpiritController and tracks spawn/SP/VRAM state for all 4.
class ActiveSpiritController {
public:
    void reset() noexcept;

    bool summon_spirit_by_index(
        std::uint8_t spirit_index,
        std::int32_t spawn_x,
        std::int32_t spawn_y,
        std::int32_t spawn_z = 0,
        std::span<const std::uint8_t> rom = {},
        genesis::VdpState* vdp = nullptr) noexcept;

    // Models $8E(A6) live SP drain countdown (45/35/40/30 frames per 1 SP from $8E($FF19E8)).
    // Returns true if `player_sp` reached 0 and the active spirit despawned cleanly.
    bool step_sp_drain(std::uint16_t& player_sp) noexcept;

    [[nodiscard]] bool is_active() const noexcept { return active_; }
    [[nodiscard]] std::uint16_t sp_drain_timer() const noexcept { return sp_drain_timer_; }
    [[nodiscard]] bool despawned() const noexcept { return despawned_; }
    [[nodiscard]] std::uint32_t last_despawn_handler_pc() const noexcept { return last_despawn_handler_pc_; }
    [[nodiscard]] std::uint8_t spirit_index() const noexcept { return spirit_index_; }
    [[nodiscard]] std::uint16_t type_id() const noexcept { return type_id_; }
    [[nodiscard]] std::uint16_t vram_tile() const noexcept { return vram_tile_; }
    [[nodiscard]] std::uint16_t max_sp() const noexcept { return max_sp_; }
    [[nodiscard]] std::uint16_t current_sp() const noexcept { return current_sp_; }
    [[nodiscard]] std::uint16_t sp_drain_rate() const noexcept { return sp_drain_rate_; }
    [[nodiscard]] std::int32_t world_x() const noexcept { return world_x_; }
    [[nodiscard]] std::int32_t world_y() const noexcept { return world_y_; }
    [[nodiscard]] std::int32_t world_z() const noexcept { return world_z_; }
    [[nodiscard]] std::size_t vram_uploaded_bytes() const noexcept { return vram_uploaded_bytes_; }
    [[nodiscard]] DyttoSpiritController& dytto() noexcept { return dytto_; }
    [[nodiscard]] const DyttoSpiritController& dytto() const noexcept { return dytto_; }

private:
    bool active_{false};
    std::uint8_t spirit_index_{0U};
    std::uint16_t type_id_{0U};
    std::uint16_t vram_tile_{0U};
    std::uint16_t max_sp_{100U};
    std::uint16_t current_sp_{100U};
    std::uint16_t sp_drain_rate_{10U};
    std::int32_t world_x_{0};
    std::int32_t world_y_{0};
    std::int32_t world_z_{0};
    std::size_t vram_uploaded_bytes_{0U};
    std::uint16_t sp_drain_timer_{0U};
    bool despawned_{false};
    std::uint32_t last_despawn_handler_pc_{0U};
    DyttoSpiritController dytto_{};
};

} // namespace oasis::game::spirits
