#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace oasis::game {
class StandaloneGameSimulation;
}

namespace oasis::game::save {

// ROM 0x001182..0x001360 & 0x001754: Genesis odd-byte SRAM & 596-byte persistent WRAM layout.
struct SramConfig {
    static constexpr std::uint32_t kWramBase = 0x00FF0BFEU;
    static constexpr std::uint32_t kWramEndInclusive = 0x00FF0E51U;
    static constexpr std::size_t kPayloadBytes = 596U; // 0x254
    static constexpr std::size_t kMagicBytesLen = 6U;
    static constexpr std::size_t kSlotCount = 4U;

    // 6-byte magic signature at ROM 0x001754
    static constexpr std::array<std::uint8_t, kMagicBytesLen> kMagic = {
        0x1AU, 0xBCU, 0x00U, 0x02U, 0x2AU, 0x4EU};

    // Genesis odd-byte SRAM ($200000..$203FFF, stride +2 per logical byte, 0x4B8 SRAM bytes/region)
    static constexpr std::uint32_t kSramBaseAddr = 0x00200000U;
    static constexpr std::size_t kSramSizeBytes = 0x4000U;
    static constexpr std::array<std::uint32_t, kSlotCount> kPrimarySlotAddrs = {
        0x00200009U, 0x002004C1U, 0x00200979U, 0x00200E31U};
    static constexpr std::array<std::uint32_t, kSlotCount> kBackupSlotAddrs = {
        0x002012E9U, 0x002017A1U, 0x00201C59U, 0x00202111U};
    static constexpr std::uint32_t kLastSlotSramAddr = 0x002025C9U;

    // Offsets within 596-byte block ($FF0BFE = 0)
    static constexpr std::size_t kOffActiveSpiritBits = 0x017CU; // $FF0D7A
    static constexpr std::size_t kOffPowerWord        = 0x0180U; // $FF0D7E
    static constexpr std::size_t kOffEquipStatusWord  = 0x0182U; // $FF0D80 (0x01FF init)
    static constexpr std::size_t kOffWeaponSlots      = 0x0184U; // $FF0D82 (7 x u16)
    static constexpr std::size_t kOffItemSlots        = 0x0192U; // $FF0D90 (16 x 2B)
    static constexpr std::size_t kOffSpiritLevels     = 0x01B8U; // $FF0DB6 (4B)
    static constexpr std::size_t kOffSpiritSlotFlags  = 0x01BCU; // $FF0DBA (1B)
    static constexpr std::size_t kOffGlobalFlags      = 0x01BEU; // $FF0DBC (32B)
    static constexpr std::size_t kOffAreaFlags        = 0x021EU; // $FF0E1C (16B)
    static constexpr std::size_t kOffRoomFlags        = 0x022EU; // $FF0E2C (16B)
    static constexpr std::size_t kOffRoomId           = 0x0242U; // $FF0E40 (u16 0xMMRR)
};

// 596-byte (0x254) persistent WRAM block ($FF0BFE..$FF0E51) cleared on New Game (0x001300).
struct PersistentWramBlock {
    std::array<std::uint8_t, SramConfig::kPayloadBytes> raw{};

    void clear_new_game() noexcept;
    void init_default_inventory() noexcept; // ROM 0x001450 weapon/equip defaults

    [[nodiscard]] std::uint8_t active_spirit_bits() const noexcept;
    void set_active_spirit_bits(std::uint8_t v) noexcept;

    [[nodiscard]] std::uint16_t power_word() const noexcept;
    void set_power_word(std::uint16_t v) noexcept;

    [[nodiscard]] std::uint16_t equip_status_word() const noexcept;
    void set_equip_status_word(std::uint16_t v) noexcept;

    [[nodiscard]] std::uint16_t weapon_slot(std::size_t idx) const noexcept;
    void set_weapon_slot(std::size_t idx, std::uint16_t v) noexcept;

    [[nodiscard]] std::uint8_t item_id(std::size_t idx) const noexcept;
    [[nodiscard]] std::uint8_t item_count(std::size_t idx) const noexcept;
    void set_item_slot(std::size_t idx, std::uint8_t id, std::uint8_t count) noexcept;

    [[nodiscard]] std::uint8_t spirit_level(std::size_t idx) const noexcept;
    void set_spirit_level(std::size_t idx, std::uint8_t lvl) noexcept;

    [[nodiscard]] std::uint8_t spirit_slot_flags() const noexcept;
    void set_spirit_slot_flags(std::uint8_t v) noexcept;

    [[nodiscard]] std::uint16_t current_room_id() const noexcept;
    void set_current_room_id(std::uint16_t room_id) noexcept;
};

// 16-bit unsigned byte-sum checksum of the 596-byte payload (ROM 0x00129A).
[[nodiscard]] std::uint16_t compute_sram_checksum(
    std::span<const std::uint8_t, SramConfig::kPayloadBytes> payload) noexcept;

// Models Genesis odd-byte SRAM ($200000..$203FFF) with 4 primary + 4 backup slots,
// 6-byte magic validation (0x00129A), slot scanning (0x001182), and backup-to-primary repair (0x0012E8).
class SramSaveBank {
public:
    SramSaveBank() noexcept { sram_.fill(0xFFU); }

    void clear_all() noexcept { sram_.fill(0xFFU); }

    bool save_slot(std::uint8_t slot, const PersistentWramBlock& block) noexcept;
    [[nodiscard]] bool validate_region(std::uint32_t sram_addr, PersistentWramBlock* out = nullptr) const noexcept;
    [[nodiscard]] std::uint8_t scan_slots() const noexcept; // Returns $FF10AA 4-bit mask
    bool load_slot(std::uint8_t slot, PersistentWramBlock& out, bool* repaired_from_backup = nullptr) noexcept;

    [[nodiscard]] std::uint16_t last_selected_slot() const noexcept;
    [[nodiscard]] std::span<std::uint8_t> raw_sram_mut() noexcept { return sram_; }
    [[nodiscard]] std::span<const std::uint8_t> raw_sram() const noexcept { return sram_; }

private:
    void write_region(std::uint32_t sram_addr, const PersistentWramBlock& block) noexcept;
    void copy_region(std::uint32_t src_addr, std::uint32_t dst_addr) noexcept;

    std::array<std::uint8_t, SramConfig::kSramSizeBytes> sram_{};
};

// Captures StandaloneGameSimulation persistent flags, room ID, and Dytto unlock state into WRAM block.
void capture_simulation_to_wram(const StandaloneGameSimulation& sim, PersistentWramBlock& out) noexcept;

// Restores WRAM block flags, active spirit bits, and Dytto state into StandaloneGameSimulation.
void restore_wram_to_simulation(const PersistentWramBlock& in, StandaloneGameSimulation& sim) noexcept;

} // namespace oasis::game::save
