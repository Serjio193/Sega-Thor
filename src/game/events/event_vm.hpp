#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace oasis::game::events {

// ROM 0x00F7D0: 31-opcode Event VM ISA (0x00..0x1E).
enum class EventVmOpcode : std::uint8_t {
    EndScriptYield = 0x00,
    SpawnEnemyIfCap = 0x01,
    SetSpawnTable1 = 0x02,
    SetSpawnTable2 = 0x03,
    WaitUntilCondition = 0x04,
    JumpRel16 = 0x05,
    LoopCounterDecBranch = 0x06,
    LoopCounterSet = 0x07,
    WaitFrames = 0x08,
    SpawnEffect09 = 0x09,
    CallEntityHandler0A = 0x0A,
    SetMaxActiveEnemies = 0x0B,
    LoadPalette0C = 0x0C,
    LoadPalette0D = 0x0D,
    SpawnEntity0E = 0x0E,
    SetClearGlobalFlag = 0x0F,
    SetClearChestFlag = 0x10,
    SetClearSecondaryFlag = 0x11,
    BranchIfCondition = 0x12,
    CallSubroutine24Bit = 0x13,
    ReturnFromSubroutine = 0x14,
    BranchIfFlag15 = 0x15,
    SpawnItemOrTrigger16 = 0x16,
    SpawnEntityCapMinus1 = 0x17,
    SpawnSpecial18 = 0x18,
    SpawnSpecial19 = 0x19,
    TriggerVdpOrTile1A = 0x1A,
    PlaySoundCmd = 0x1B,
    SetEventVar1C = 0x1C,
    SetCameraTarget1D = 0x1D,
    CameraPanOrFade1E = 0x1E,
};

struct EventVmEntitySlot {
    std::uint16_t type_id{0U};
    std::uint8_t flags_3a{0U}; // bit 0 == 1 -> spirit / exempt from non-spirit count (0x0101EC)
};

// ROM 0x00F788 & 0x00FAB2: Event VM RAM state.
struct EventVmState {
    std::uint32_t pc{0U};                  // $FF17B2
    std::uint8_t wait_frames{0U};          // $FF17B6
    std::uint8_t max_active_enemies{0U};   // $FF17B7
    std::uint8_t vm_flags{0U};             // $FF17B8 (bit 1 pauses VM execution)
    std::uint8_t pan_state{0U};            // $FF17BD
    std::array<std::uint16_t, 8> loop_counters{}; // $FF1826
    std::uint32_t return_pc{0U};           // $FF1836
    std::uint16_t event_var{0U};           // $FF17C4

    std::array<std::uint8_t, 32> global_flags{};    // $FF0DBC
    std::array<std::uint8_t, 16> chest_flags{};     // $FF0E1C
    std::array<std::uint8_t, 16> secondary_flags{}; // $FF0E2C

    std::uint16_t active_spirit_id{0U};    // $FF1AA4
    std::uint16_t player_hp{100U};         // $FF1A72
    std::uint16_t player_max_hp{100U};     // $FF1A70
    std::uint16_t player_sp{100U};         // $FF1A76
    std::uint16_t player_max_sp{100U};     // $FF1A74
    std::uint16_t player_x{0U};            // $FF19F0
    std::uint16_t player_y{0U};            // $FF19F4
    std::uint16_t spirit_x{0U};            // $FF1AAC
    std::uint16_t spirit_y{0U};            // $FF1AB0

    std::array<EventVmEntitySlot, 17> entities{}; // $FF1CD8 (17 slots, stride 0xBC)
    std::vector<std::uint32_t> sound_commands{};
    std::vector<std::uint16_t> loaded_palettes{};
    std::vector<std::uint16_t> spawn_table_1{};
    std::vector<std::pair<std::uint16_t, std::uint16_t>> spawn_table_2{};
    std::uint8_t player_lock_flag{0U};     // $FF1982
    bool halted{false};
};

// ROM 0x00F7D0: Returns the 68k handler address for opcode 0x00..0x1E (or 0 if >= 0x1F).
[[nodiscard]] std::uint32_t lookup_event_vm_opcode_target(std::uint8_t opcode) noexcept;

// ROM 0x00FB64: Evaluates comparator selector (0, 2, 4, 6, 8, 10 -> ==, !=, >, >=, <=, <).
[[nodiscard]] bool evaluate_op12_comparator(
    std::uint8_t selector,
    std::uint16_t lhs,
    std::uint16_t rhs) noexcept;

// ROM 0x00FAB2..0x00FC6B: Evaluates Opcode 0x12 conditional branch (12 sub-actions 0x00..0x0B).
[[nodiscard]] bool evaluate_op12_condition(
    const EventVmState& state,
    std::span<const std::uint8_t> rom,
    std::uint32_t instr_pc) noexcept;

// ROM 0x00F788..0x00F80D: 31-opcode Event VM interpreter.
class EventVmInterpreter {
public:
    void reset(std::uint32_t start_pc = 0U) noexcept;

    // Executes one instruction at state_.pc. Returns true if VM should continue this frame,
    // false if the instruction yielded, halted, or hit out-of-bounds.
    bool execute_opcode(std::span<const std::uint8_t> rom);

    // Models 0x00F788: checks vm_flags bit 1, wait_frames countdown, pan_state, and runs loop.
    void step_frame(std::span<const std::uint8_t> rom, std::uint32_t max_ops_per_frame = 256U);

    [[nodiscard]] EventVmState& state() noexcept { return state_; }
    [[nodiscard]] const EventVmState& state() const noexcept { return state_; }

private:
    EventVmState state_{};
};

} // namespace oasis::game::events
