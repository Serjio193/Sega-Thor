#include "game/save/sram_save_system.hpp"

#include "game/standalone_game_simulation.hpp"

#include <algorithm>
#include <cstring>

namespace oasis::game::save {

namespace {

[[nodiscard]] std::uint16_t read_be16(std::span<const std::uint8_t> buf, std::size_t off) noexcept {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(buf[off]) << 8U) | buf[off + 1U]);
}

void write_be16(std::span<std::uint8_t> buf, std::size_t off, std::uint16_t val) noexcept {
    buf[off] = static_cast<std::uint8_t>((val >> 8U) & 0xFFU);
    buf[off + 1U] = static_cast<std::uint8_t>(val & 0xFFU);
}

} // namespace

void PersistentWramBlock::clear_new_game() noexcept {
    raw.fill(0U);
}

void PersistentWramBlock::init_default_inventory() noexcept {
    // ROM 0x001450: equip status 0x01FF, 7 weapon slots, 4 spirit levels 0x0F
    set_equip_status_word(0x01FFU);
    static constexpr std::array<std::uint16_t, 7> kInitWeapons = {
        0x05FEU, 0x07FEU, 0x08FEU, 0x0CFEU, 0x09FEU, 0x0AFEU, 0x0BFEU};
    for (std::size_t i = 0; i < kInitWeapons.size(); ++i) {
        set_weapon_slot(i, kInitWeapons[i]);
    }
    for (std::size_t i = 0; i < 4U; ++i) {
        set_spirit_level(i, 0x0FU);
    }
}

std::uint8_t PersistentWramBlock::active_spirit_bits() const noexcept {
    return raw[SramConfig::kOffActiveSpiritBits];
}
void PersistentWramBlock::set_active_spirit_bits(std::uint8_t v) noexcept {
    raw[SramConfig::kOffActiveSpiritBits] = v;
}

std::uint16_t PersistentWramBlock::power_word() const noexcept {
    return read_be16(raw, SramConfig::kOffPowerWord);
}
void PersistentWramBlock::set_power_word(std::uint16_t v) noexcept {
    write_be16(raw, SramConfig::kOffPowerWord, v);
}

std::uint16_t PersistentWramBlock::equip_status_word() const noexcept {
    return read_be16(raw, SramConfig::kOffEquipStatusWord);
}
void PersistentWramBlock::set_equip_status_word(std::uint16_t v) noexcept {
    write_be16(raw, SramConfig::kOffEquipStatusWord, v);
}

std::uint16_t PersistentWramBlock::weapon_slot(std::size_t idx) const noexcept {
    return (idx < 7U) ? read_be16(raw, SramConfig::kOffWeaponSlots + idx * 2U) : 0U;
}
void PersistentWramBlock::set_weapon_slot(std::size_t idx, std::uint16_t v) noexcept {
    if (idx < 7U) write_be16(raw, SramConfig::kOffWeaponSlots + idx * 2U, v);
}

std::uint8_t PersistentWramBlock::item_id(std::size_t idx) const noexcept {
    return (idx < 16U) ? raw[SramConfig::kOffItemSlots + idx * 2U] : 0U;
}
std::uint8_t PersistentWramBlock::item_count(std::size_t idx) const noexcept {
    return (idx < 16U) ? raw[SramConfig::kOffItemSlots + idx * 2U + 1U] : 0U;
}
void PersistentWramBlock::set_item_slot(std::size_t idx, std::uint8_t id, std::uint8_t count) noexcept {
    if (idx < 16U) {
        raw[SramConfig::kOffItemSlots + idx * 2U] = id;
        raw[SramConfig::kOffItemSlots + idx * 2U + 1U] = count;
    }
}

std::uint8_t PersistentWramBlock::spirit_level(std::size_t idx) const noexcept {
    return (idx < 4U) ? raw[SramConfig::kOffSpiritLevels + idx] : 0U;
}
void PersistentWramBlock::set_spirit_level(std::size_t idx, std::uint8_t lvl) noexcept {
    if (idx < 4U) raw[SramConfig::kOffSpiritLevels + idx] = lvl;
}

std::uint8_t PersistentWramBlock::spirit_slot_flags() const noexcept {
    return raw[SramConfig::kOffSpiritSlotFlags];
}
void PersistentWramBlock::set_spirit_slot_flags(std::uint8_t v) noexcept {
    raw[SramConfig::kOffSpiritSlotFlags] = v;
}

std::uint16_t PersistentWramBlock::current_room_id() const noexcept {
    return read_be16(raw, SramConfig::kOffRoomId);
}
void PersistentWramBlock::set_current_room_id(std::uint16_t room_id) noexcept {
    write_be16(raw, SramConfig::kOffRoomId, room_id);
}

std::uint16_t compute_sram_checksum(
    std::span<const std::uint8_t, SramConfig::kPayloadBytes> payload) noexcept {
    std::uint32_t sum = 0U;
    for (const std::uint8_t b : payload) {
        sum += b;
    }
    return static_cast<std::uint16_t>(sum & 0xFFFFU);
}

void SramSaveBank::write_region(std::uint32_t sram_addr, const PersistentWramBlock& block) noexcept {
    const std::size_t base = static_cast<std::size_t>(sram_addr - SramConfig::kSramBaseAddr);
    std::size_t pos = base;

    for (const std::uint8_t m : SramConfig::kMagic) {
        if (pos < sram_.size()) sram_[pos] = m;
        pos += 2U;
    }
    for (const std::uint8_t b : block.raw) {
        if (pos < sram_.size()) sram_[pos] = b;
        pos += 2U;
    }
    const std::uint16_t csum = compute_sram_checksum(block.raw);
    if (pos + 2U < sram_.size()) {
        sram_[pos] = static_cast<std::uint8_t>((csum >> 8U) & 0xFFU);
        sram_[pos + 2U] = static_cast<std::uint8_t>(csum & 0xFFU);
    }
}

void SramSaveBank::copy_region(std::uint32_t src_addr, std::uint32_t dst_addr) noexcept {
    // ROM 0x0012E8: 6 magic + 596 payload + 2 checksum = 604 logical odd bytes (0x4B8 SRAM bytes)
    const std::size_t src = static_cast<std::size_t>(src_addr - SramConfig::kSramBaseAddr);
    const std::size_t dst = static_cast<std::size_t>(dst_addr - SramConfig::kSramBaseAddr);
    constexpr std::size_t kLogicalBytes = SramConfig::kMagicBytesLen + SramConfig::kPayloadBytes + 2U;
    for (std::size_t i = 0; i < kLogicalBytes; ++i) {
        if (src + i * 2U < sram_.size() && dst + i * 2U < sram_.size()) {
            sram_[dst + i * 2U] = sram_[src + i * 2U];
        }
    }
}

bool SramSaveBank::save_slot(std::uint8_t slot, const PersistentWramBlock& block) noexcept {
    if (slot >= SramConfig::kSlotCount) return false;
    write_region(SramConfig::kPrimarySlotAddrs[slot], block);
    write_region(SramConfig::kBackupSlotAddrs[slot], block);
    const std::size_t last_off = static_cast<std::size_t>(SramConfig::kLastSlotSramAddr - SramConfig::kSramBaseAddr);
    if (last_off < sram_.size()) sram_[last_off] = slot;
    return true;
}

bool SramSaveBank::validate_region(std::uint32_t sram_addr, PersistentWramBlock* out) const noexcept {
    const std::size_t base = static_cast<std::size_t>(sram_addr - SramConfig::kSramBaseAddr);
    constexpr std::size_t kLogicalBytes = SramConfig::kMagicBytesLen + SramConfig::kPayloadBytes + 2U;
    if (base + (kLogicalBytes - 1U) * 2U >= sram_.size()) return false;

    std::size_t pos = base;
    for (const std::uint8_t m : SramConfig::kMagic) {
        if (sram_[pos] != m) return false;
        pos += 2U;
    }

    PersistentWramBlock tmp{};
    for (std::size_t i = 0; i < SramConfig::kPayloadBytes; ++i) {
        tmp.raw[i] = sram_[pos];
        pos += 2U;
    }
    const std::uint16_t stored_csum = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(sram_[pos]) << 8U) | sram_[pos + 2U]);
    if (stored_csum != compute_sram_checksum(tmp.raw)) return false;

    if (out != nullptr) *out = tmp;
    return true;
}

std::uint8_t SramSaveBank::scan_slots() const noexcept {
    // ROM 0x001182: builds $FF10AA bitmask across slots 0..3 (valid if primary or backup passes)
    std::uint8_t mask = 0U;
    for (std::uint8_t s = 0; s < SramConfig::kSlotCount; ++s) {
        if (validate_region(SramConfig::kPrimarySlotAddrs[s]) ||
            validate_region(SramConfig::kBackupSlotAddrs[s])) {
            mask |= static_cast<std::uint8_t>(1U << s);
        }
    }
    return mask;
}

bool SramSaveBank::load_slot(std::uint8_t slot, PersistentWramBlock& out, bool* repaired_from_backup) noexcept {
    if (repaired_from_backup != nullptr) *repaired_from_backup = false;
    if (slot >= SramConfig::kSlotCount) return false;

    if (validate_region(SramConfig::kPrimarySlotAddrs[slot], &out)) {
        return true;
    }
    if (validate_region(SramConfig::kBackupSlotAddrs[slot], &out)) {
        // ROM 0x0012E8: primary corrupt, backup valid -> repair primary from backup
        copy_region(SramConfig::kBackupSlotAddrs[slot], SramConfig::kPrimarySlotAddrs[slot]);
        if (repaired_from_backup != nullptr) *repaired_from_backup = true;
        return true;
    }
    return false;
}

std::uint16_t SramSaveBank::last_selected_slot() const noexcept {
    const std::size_t off = static_cast<std::size_t>(SramConfig::kLastSlotSramAddr - SramConfig::kSramBaseAddr);
    if (off >= sram_.size()) return 0U;
    const std::uint16_t v = sram_[off];
    return (v <= 3U) ? v : 0U;
}

void capture_simulation_to_wram(const StandaloneGameSimulation& sim, PersistentWramBlock& out) noexcept {
    const auto& vm = sim.event_vm();
    out.set_active_spirit_bits(vm.active_spirit_flags());
    const std::uint16_t room_id = static_cast<std::uint16_t>(
        (sim.screen_id().group << 8U) | sim.screen_id().index);
    out.set_current_room_id(room_id);

    const auto gf = vm.global_flags_bytes();
    const auto af = vm.area_flags_bytes();
    const auto rf = vm.room_flags_bytes();
    std::memcpy(out.raw.data() + SramConfig::kOffGlobalFlags, gf.data(), std::min<std::size_t>(32U, gf.size()));
    std::memcpy(out.raw.data() + SramConfig::kOffAreaFlags, af.data(), std::min<std::size_t>(16U, af.size()));
    std::memcpy(out.raw.data() + SramConfig::kOffRoomFlags, rf.data(), std::min<std::size_t>(16U, rf.size()));
}

void restore_wram_to_simulation(const PersistentWramBlock& in, StandaloneGameSimulation& sim) noexcept {
    auto& vm = sim.event_vm();
    vm.set_active_spirit_flags(in.active_spirit_bits());

    auto gf = vm.global_flags_bytes_mut();
    auto af = vm.area_flags_bytes_mut();
    auto rf = vm.room_flags_bytes_mut();
    std::memcpy(gf.data(), in.raw.data() + SramConfig::kOffGlobalFlags, std::min<std::size_t>(32U, gf.size()));
    std::memcpy(af.data(), in.raw.data() + SramConfig::kOffAreaFlags, std::min<std::size_t>(16U, af.size()));
    std::memcpy(rf.data(), in.raw.data() + SramConfig::kOffRoomFlags, std::min<std::size_t>(16U, rf.size()));

    if (vm.has_active_spirit(0U) && !sim.dytto_controller().is_active()) {
        const auto px = static_cast<std::int32_t>(sim.player().x_fixed / 0x10000);
        const auto py = static_cast<std::int32_t>(sim.player().y_fixed / 0x10000);
        sim.spawn_dytto_awakening(px, py, 0);
    }
}

} // namespace oasis::game::save
