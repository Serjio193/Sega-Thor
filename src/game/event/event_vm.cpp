#include "game/event/event_vm.hpp"

#include <algorithm>
#include <cstdint>

namespace oasis::game::event {

std::uint32_t resolve_dialogue_address(
    std::span<const std::uint8_t> rom,
    std::uint16_t message_id) noexcept {
    const std::size_t ptr_addr = static_cast<std::size_t>(kDialogueTableBaseRom + message_id * 2U);
    if (ptr_addr + 2U > rom.size()) {
        return 0U;
    }
    const auto offset = static_cast<std::int16_t>(
        (static_cast<std::uint16_t>(rom[ptr_addr]) << 8U) | rom[ptr_addr + 1U]);
    const auto target = static_cast<std::int32_t>(ptr_addr) + static_cast<std::int32_t>(offset);
    if (target < 0 || static_cast<std::size_t>(target) >= rom.size()) {
        return 0U;
    }
    return static_cast<std::uint32_t>(target);
}

DialogueSequence decode_dialogue_from_rom(
    std::span<const std::uint8_t> rom,
    std::uint32_t rom_address,
    std::uint16_t message_id) {
    DialogueSequence seq{};
    seq.message_id = message_id;
    seq.rom_address = rom_address;

    if (rom.empty() || rom_address >= rom.size()) {
        return seq;
    }

    DialoguePage curr_page{};
    std::string curr_line{};
    std::size_t pos = static_cast<std::size_t>(rom_address);

    while (pos < rom.size()) {
        const std::uint8_t b = rom[pos++];
        if (b == 0x00U) { // End of dialogue
            if (!curr_line.empty()) {
                curr_page.lines.push_back(std::move(curr_line));
                curr_line.clear();
            }
            if (!curr_page.lines.empty()) {
                seq.pages.push_back(std::move(curr_page));
            }
            break;
        }
        if (b == 0x07U || b == 0x02U) { // Page break / clear page (controller confirmation wait)
            if (!curr_line.empty()) {
                curr_page.lines.push_back(std::move(curr_line));
                curr_line.clear();
            }
            if (!curr_page.lines.empty()) {
                seq.pages.push_back(std::move(curr_page));
                curr_page = DialoguePage{};
            }
            continue;
        }
        if (b == 0x01U || b == 0x06U) { // Line break / scroll
            curr_page.lines.push_back(std::move(curr_line));
            curr_line.clear();
            continue;
        }
        if (b == 0x03U) { // Frame delay operand (skip 1 byte)
            if (pos < rom.size()) {
                ++pos;
            }
            continue;
        }
        if (b == 0x05U) { // Wait button press inline
            continue;
        }
        if (b == static_cast<std::uint8_t>('#')) {
            // Authentic speaker or frame delimiter tag, not displayed in text prose
            continue;
        }
        if (b >= 0x20U && b <= 0x7EU) {
            curr_line.push_back(static_cast<char>(b));
        }
    }

    return seq;
}

DialogueSequence decode_dialogue_by_id(
    std::span<const std::uint8_t> rom,
    std::uint16_t message_id) {
    const std::uint32_t text_addr = resolve_dialogue_address(rom, message_id);
    if (text_addr == 0U) {
        return DialogueSequence{.message_id = message_id, .rom_address = 0U, .pages = {}};
    }
    return decode_dialogue_from_rom(rom, text_addr, message_id);
}

EventVm::EventVm() noexcept {
    reset();
}

void EventVm::reset() noexcept {
    rom_ = {};
    pc_ = 0;
    wait_timer_ = 0;
    state_ = EventVmState::Idle;
    last_opcode_ = 0;
    executed_count_ = 0;
    global_flags_.fill(0U);
    area_flags_.fill(0U);
    room_flags_.fill(0U);
    spawned_props_.clear();
    active_dialogue_ = {};
    active_page_index_ = 0;
}

bool EventVm::start_script(std::span<const std::uint8_t> rom, std::uint32_t script_address) noexcept {
    if (rom.empty() || script_address >= rom.size()) {
        state_ = EventVmState::Error;
        return false;
    }
    rom_ = rom;
    pc_ = script_address;
    wait_timer_ = 0;
    state_ = EventVmState::Running;
    last_opcode_ = 0;
    executed_count_ = 0;
    spawned_props_.clear();
    return true;
}

void EventVm::trigger_dialogue(DialogueSequence dialogue) noexcept {
    active_dialogue_ = std::move(dialogue);
    active_page_index_ = 0;
    if (!active_dialogue_.pages.empty()) {
        state_ = EventVmState::WaitingDialogue;
    } else {
        state_ = EventVmState::Idle;
    }
}

bool EventVm::get_event_flag(std::uint16_t flag_index, std::uint8_t bank) const noexcept {
    const std::span<const std::uint8_t> flags = (bank == 2U) ? std::span<const std::uint8_t>(area_flags_)
                                              : (bank == 3U) ? std::span<const std::uint8_t>(room_flags_)
                                                             : std::span<const std::uint8_t>(global_flags_);
    const std::size_t byte_idx = static_cast<std::size_t>(flag_index >> 3U);
    if (byte_idx >= flags.size()) return false;
    const std::uint8_t bit_idx = static_cast<std::uint8_t>(flag_index & 0x07U);
    return (flags[byte_idx] & static_cast<std::uint8_t>(1U << bit_idx)) != 0U;
}

void EventVm::set_event_flag(std::uint16_t flag_index, bool value, std::uint8_t bank) noexcept {
    std::span<std::uint8_t> flags = (bank == 2U) ? std::span<std::uint8_t>(area_flags_)
                                  : (bank == 3U) ? std::span<std::uint8_t>(room_flags_)
                                                 : std::span<std::uint8_t>(global_flags_);
    const std::size_t byte_idx = static_cast<std::size_t>(flag_index >> 3U);
    if (byte_idx >= flags.size()) return;
    const std::uint8_t bit_idx = static_cast<std::uint8_t>(flag_index & 0x07U);
    if (value) {
        flags[byte_idx] |= static_cast<std::uint8_t>(1U << bit_idx);
    } else {
        flags[byte_idx] &= static_cast<std::uint8_t>(~(1U << bit_idx));
    }
}

void EventVm::modify_event_flag(std::uint8_t mode, std::uint16_t flag_index, std::uint8_t bank) noexcept {
    std::span<std::uint8_t> flags = (bank == 2U) ? std::span<std::uint8_t>(area_flags_)
                                  : (bank == 3U) ? std::span<std::uint8_t>(room_flags_)
                                                 : std::span<std::uint8_t>(global_flags_);
    const std::size_t byte_idx = static_cast<std::size_t>(flag_index >> 3U);
    if (byte_idx >= flags.size()) return;
    const std::uint8_t bit_mask = static_cast<std::uint8_t>(1U << (flag_index & 0x07U));
    if (mode == 0U) {
        flags[byte_idx] &= static_cast<std::uint8_t>(~bit_mask);
    } else if (mode == 1U) {
        flags[byte_idx] |= bit_mask;
    } else {
        flags[byte_idx] ^= bit_mask;
    }
}

void EventVm::step_frame(bool advance_dialogue_input) noexcept {
    if (state_ == EventVmState::WaitingTimer) {
        if (wait_timer_ > 0U) {
            --wait_timer_;
        }
        if (wait_timer_ == 0U) {
            state_ = EventVmState::Running;
        } else {
            return;
        }
    }

    if (state_ == EventVmState::WaitingDialogue) {
        if (advance_dialogue_input) {
            if (has_more_pages()) {
                ++active_page_index_;
            } else {
                state_ = EventVmState::Idle;
                active_dialogue_ = {};
                active_page_index_ = 0;
            }
        }
        return;
    }

    if (state_ != EventVmState::Running) {
        return;
    }

    while (state_ == EventVmState::Running) {
        execute_next_instruction();
        if (state_ == EventVmState::WaitingTimer) {
            break;
        }
    }
}

void EventVm::execute_next_instruction() noexcept {
    if (pc_ >= rom_.size()) {
        state_ = EventVmState::Error;
        return;
    }

    const std::uint8_t op = rom_[pc_++];
    last_opcode_ = op;
    ++executed_count_;

    switch (op) {
    case 0x00U: // Halt / Yield (0x00F91C / 0x00FCCE)
        state_ = EventVmState::Terminated;
        break;

    case 0x07U: { // Set loop counter / wait timer (0x00FC94)
        if (pc_ + 3U <= rom_.size()) {
            pc_ += 3U;
        } else {
            state_ = EventVmState::Error;
        }
        break;
    }

    case 0x08U: { // Wait frames (0x00FCA6)
        if (pc_ < rom_.size()) {
            wait_timer_ = rom_[pc_++];
            state_ = (wait_timer_ > 0U) ? EventVmState::WaitingTimer : EventVmState::Running;
        } else {
            state_ = EventVmState::Error;
        }
        break;
    }

    case 0x0AU: { // Preload Nemesis DMA (0x00FCB8: u8:flag, u32:rom_src, u16:vram_dst)
        if (pc_ + 7U <= rom_.size()) {
            pc_ += 7U; // Preload enqueued / consumed
        } else {
            state_ = EventVmState::Error;
        }
        break;
    }

    case 0x0FU: { // Global flag operation (0x010140: mode:u8, flag_idx:u16)
        if (pc_ + 3U <= rom_.size()) {
            const std::uint8_t mode = rom_[pc_++];
            const std::uint16_t flag_idx = static_cast<std::uint16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            modify_event_flag(mode, flag_idx, 1U); // Bank 1: Global flags ($FF0DBC)
        } else {
            state_ = EventVmState::Error;
        }
        break;
    }

    case 0x10U: { // Area flag operation (0x010170: mode:u8, flag_idx:u16)
        if (pc_ + 3U <= rom_.size()) {
            const std::uint8_t mode = rom_[pc_++];
            const std::uint16_t flag_idx = static_cast<std::uint16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            modify_event_flag(mode, flag_idx, 2U); // Bank 2: Area flags ($FF0E1C)
        } else {
            state_ = EventVmState::Error;
        }
        break;
    }

    case 0x11U: { // Room flag operation (0x0101A0: mode:u8, flag_idx:u16)
        if (pc_ + 3U <= rom_.size()) {
            const std::uint8_t mode = rom_[pc_++];
            const std::uint16_t flag_idx = static_cast<std::uint16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            modify_event_flag(mode, flag_idx, 3U); // Bank 3: Room flags ($FF0E2C)
        } else {
            state_ = EventVmState::Error;
        }
        break;
    }

    case 0x14U: { // Subroutine return (0x0101E4) or Branch conditional
        // In jump table 0x00F7D0: 0x14 is return subroutine (0x0101E4)
        break;
    }

    case 0x15U: { // Spawn Gated Prop (0x00FEDE, 17 bytes operands)
        if (pc_ + 17U <= rom_.size()) {
            SpawnPropRecord rec{};
            rec.flag_bank = rom_[pc_++];
            rec.type_id = static_cast<std::uint16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            rec.flag_index = static_cast<std::uint16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            rec.field6 = rom_[pc_++];
            rec.field7 = rom_[pc_++];
            rec.message_id = static_cast<std::uint16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            rec.world_x = static_cast<std::int16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            rec.world_y = static_cast<std::int16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            rec.world_z = static_cast<std::int16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            rec.vram_tile_base = static_cast<std::uint16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;

            // In 0x00FEDE: if flag is clear, spawn entity!
            const bool flag_set = get_event_flag(rec.flag_index, rec.flag_bank);
            rec.condition_passed = !flag_set;
            spawned_props_.push_back(rec);
        } else {
            state_ = EventVmState::Error;
        }
        break;
    }

    case 0x16U: { // Spawn Static Prop (0x00FF1A, 17 bytes operands, unconditional)
        if (pc_ + 17U <= rom_.size()) {
            SpawnPropRecord rec{};
            rec.flag_bank = rom_[pc_++];
            rec.type_id = static_cast<std::uint16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            rec.flag_index = static_cast<std::uint16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            rec.field6 = rom_[pc_++];
            rec.field7 = rom_[pc_++];
            rec.message_id = static_cast<std::uint16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            rec.world_x = static_cast<std::int16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            rec.world_y = static_cast<std::int16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            rec.world_z = static_cast<std::int16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            rec.vram_tile_base = static_cast<std::uint16_t>((static_cast<std::uint16_t>(rom_[pc_]) << 8U) | rom_[pc_ + 1U]);
            pc_ += 2U;
            rec.condition_passed = true;
            spawned_props_.push_back(rec);
        } else {
            state_ = EventVmState::Error;
        }
        break;
    }

    default:
        state_ = EventVmState::Error;
        break;
    }
}

} // namespace oasis::game::event
