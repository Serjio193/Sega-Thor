#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace oasis::game::event {

// Dialogue table base in authentic Beyond Oasis ROM ($00C3FE)
constexpr std::uint32_t kDialogueTableBaseRom = 0x051514U;

// Execution state of the minimal Event VM
enum class EventVmState : std::uint8_t {
    Idle = 0,
    Running,
    WaitingTimer,
    WaitingInput,
    WaitingDialogue,
    Terminated,
    Error
};

// Represents a decoded page of text from ROM dialogue
struct DialoguePage {
    std::vector<std::string> lines{};
};

// Represents a full dialogue sequence containing one or more pages
struct DialogueSequence {
    std::uint16_t message_id{0};
    std::uint32_t rom_address{0};
    std::vector<DialoguePage> pages{};
};

// Resolves authentic dialogue text address from message ID via table at 0x051514.
// Addressing: entry = 0x051514 + msg_id * 2; text_addr = entry + *(int16_t*)entry.
[[nodiscard]] std::uint32_t resolve_dialogue_address(
    std::span<const std::uint8_t> rom,
    std::uint16_t message_id) noexcept;

// Decodes dialogue text from original ROM starting at `rom_address`.
// Delimiters:
//   0x00 = end of dialogue
//   0x01 = newline
//   0x02 = clear page & newline
//   0x03 = frame delay operand (1 byte)
//   0x05 = wait button press
//   0x06 = scroll 4 lines
//   0x07 = page pause (wait for user confirmation)
//   '#'  = boundary tag / speaker tag (skipped in display prose)
[[nodiscard]] DialogueSequence decode_dialogue_from_rom(
    std::span<const std::uint8_t> rom,
    std::uint32_t rom_address,
    std::uint16_t message_id = 0);

// Decodes dialogue sequence using authentic message ID lookup through 0x051514.
[[nodiscard]] DialogueSequence decode_dialogue_by_id(
    std::span<const std::uint8_t> rom,
    std::uint16_t message_id);

// Represents an entity spawn record decoded from opcode 0x15 / 0x16.
struct SpawnPropRecord {
    std::uint8_t flag_bank{0};
    std::uint16_t type_id{0};
    std::uint16_t flag_index{0};
    std::uint8_t field6{0};
    std::uint8_t field7{0};
    std::uint16_t message_id{0};
    std::int16_t world_x{0};
    std::int16_t world_y{0};
    std::int16_t world_z{0};
    std::uint16_t vram_tile_base{0};
    bool condition_passed{false};
};

// Minimal reusable ROM Event VM executing bytecode instructions.
// Models original routines at 0x00F788, 0x00F7D0, 0x00FCCE, 0x00FCA6, 0x00FEDE, 0x00FF1A.
class EventVm {
public:
    static constexpr std::size_t kGlobalFlagBytes = 64; // $FF0DBC area
    static constexpr std::size_t kAreaFlagBytes   = 16; // $FF0E1C area
    static constexpr std::size_t kRoomFlagBytes   = 16; // $FF0E2C area

    EventVm() noexcept;

    void reset() noexcept;

    // Starts execution of an event script located at `script_address` in ROM.
    bool start_script(std::span<const std::uint8_t> rom, std::uint32_t script_address) noexcept;

    // Advances the script execution by one step/frame.
    // `advance_dialogue_input` is true if player pressed an action confirmation button.
    void step_frame(bool advance_dialogue_input = false) noexcept;

    // Direct flag management ($FF0DBC global, $FF0E1C area, $FF0E2C room, $FF0D7A active spirit)
    [[nodiscard]] bool get_event_flag(std::uint16_t flag_index, std::uint8_t bank = 1) const noexcept;
    void set_event_flag(std::uint16_t flag_index, bool value = true, std::uint8_t bank = 1) noexcept;
    void modify_event_flag(std::uint8_t mode, std::uint16_t flag_index, std::uint8_t bank = 1) noexcept;

    [[nodiscard]] std::uint8_t active_spirit_flags() const noexcept { return active_spirit_flags_; }
    [[nodiscard]] bool has_active_spirit(std::uint8_t spirit_bit) const noexcept {
        return (spirit_bit < 8U) && ((active_spirit_flags_ & static_cast<std::uint8_t>(1U << spirit_bit)) != 0U);
    }
    void set_active_spirit_bit(std::uint8_t spirit_bit, bool value = true) noexcept {
        if (spirit_bit >= 8U) return;
        const auto mask = static_cast<std::uint8_t>(1U << spirit_bit);
        if (value) {
            active_spirit_flags_ |= mask;
        } else {
            active_spirit_flags_ &= static_cast<std::uint8_t>(~mask);
        }
    }
    void set_active_spirit_flags(std::uint8_t flags) noexcept { active_spirit_flags_ = flags; }

    [[nodiscard]] std::span<const std::uint8_t> global_flags_bytes() const noexcept { return global_flags_; }
    [[nodiscard]] std::span<std::uint8_t> global_flags_bytes_mut() noexcept { return global_flags_; }
    [[nodiscard]] std::span<const std::uint8_t> area_flags_bytes() const noexcept { return area_flags_; }
    [[nodiscard]] std::span<std::uint8_t> area_flags_bytes_mut() noexcept { return area_flags_; }
    [[nodiscard]] std::span<const std::uint8_t> room_flags_bytes() const noexcept { return room_flags_; }
    [[nodiscard]] std::span<std::uint8_t> room_flags_bytes_mut() noexcept { return room_flags_; }

    [[nodiscard]] EventVmState state() const noexcept { return state_; }
    [[nodiscard]] bool is_active() const noexcept {
        return state_ != EventVmState::Idle && state_ != EventVmState::Terminated && state_ != EventVmState::Error;
    }
    [[nodiscard]] bool is_waiting_dialogue() const noexcept {
        return state_ == EventVmState::WaitingDialogue;
    }

    [[nodiscard]] std::uint32_t pc() const noexcept { return pc_; }
    [[nodiscard]] std::uint16_t wait_timer() const noexcept { return wait_timer_; }
    [[nodiscard]] std::uint8_t last_opcode() const noexcept { return last_opcode_; }
    [[nodiscard]] std::uint32_t executed_instructions_count() const noexcept { return executed_count_; }

    // Last spawned entity prop from opcode 0x15 / 0x16
    [[nodiscard]] const std::vector<SpawnPropRecord>& spawned_props() const noexcept { return spawned_props_; }

    // Active dialogue access
    [[nodiscard]] const DialogueSequence& active_dialogue() const noexcept { return active_dialogue_; }
    [[nodiscard]] std::size_t active_page_index() const noexcept { return active_page_index_; }
    [[nodiscard]] bool has_more_pages() const noexcept {
        return (active_page_index_ + 1U) < active_dialogue_.pages.size();
    }

    // Triggers a dialogue sequence directly (e.g. from NPC interaction)
    void trigger_dialogue(DialogueSequence dialogue) noexcept;

private:
    void execute_next_instruction() noexcept;

    std::span<const std::uint8_t> rom_{};
    std::uint32_t pc_{0};
    std::uint16_t wait_timer_{0};
    EventVmState state_{EventVmState::Idle};
    std::uint8_t last_opcode_{0};
    std::uint32_t executed_count_{0};

    // Flag banks: Global ($FF0DBC), Area ($FF0E1C), Room ($FF0E2C), Active Spirit ($FF0D7A)
    std::array<std::uint8_t, kGlobalFlagBytes> global_flags_{};
    std::array<std::uint8_t, kAreaFlagBytes> area_flags_{};
    std::array<std::uint8_t, kRoomFlagBytes> room_flags_{};
    std::uint8_t active_spirit_flags_{0};

    // Spawn records recorded during script execution
    std::vector<SpawnPropRecord> spawned_props_{};

    // Current dialogue progression state
    DialogueSequence active_dialogue_{};
    std::size_t active_page_index_{0};
};

} // namespace oasis::game::event
