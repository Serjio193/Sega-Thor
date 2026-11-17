# STANDALONE HANDOFF: M15.428 — ROM Dialogue Script Virtual Machine & Message Window Presentation Controller

## 1. Routine Overview
- **Addresses:** `0x00C2EC..0x00C566` (634 bytes)
- **String Table:** ROM `0x051514` (98 strings: `0x00..0x61`, 16-bit relative signed offsets)
- **Primary Entrypoints:**
  - `0x00C2EC`: `display_dialogue_message` (Universal story script caller entrypoint & debug test validator)
  - `0x00C326`: `dialogue_box_exec_window` (Sets up window graphics, entity lifecycle, and starts script VM)
  - `0x00C45C`: `dialogue_close_window` (Deactivates entity, debounces joypad, and restores registers)
  - `0x00C536`: `dialogue_wait_button` (Interactive button wait loop: checks A/B/C on `$FF165F`)
- **Key Callers:** Called from >40 story event scripts across the entire ROM (e.g. `0x017B12`, `0x018C88`, `0x02336E`, `0x02444A`, `0x0251CC`, `0x027560`, `0x0333B4`).

## 2. String Table Structure (`0x051514`)
- Located at ROM address `0x051514`.
- Contains 98 relative signed 16-bit offsets (`0x00..0x61`).
- Target address calculation:
  ```cpp
  int16_t offset = read_be16(rom.data() + 0x051514 + string_id * 2);
  uint32_t string_addr = 0x051514 + string_id * 2 + offset;
  ```
- **Historical Note:** Immediately following string 0x61 offset table (at `0x051514 + 0x62 * 2 = 0x0515D8`), developer placeholder text exists: `b'? ? ? ? ? ? ? ? ?\x01Oh!!!Kari Serihu!\x00'` ("Kari Serifu" = Japanese for "Temporary / Placeholder Dialogue"). This explains why the debug loop at `0x00C31E` explicitly bounds testing to `d0 < 0x62`.

## 3. Bytecode Instruction Set
The dialogue interpreter processes bytes from the string stream:
- `byte >= 0x20` (' '..0xFF): Printable glyph character. Sent to font renderer / VRAM streamer.
- `byte < 0x20`: Control opcodes:
  - `0x00`: **OP_TERMINATE** — Sets dismiss flag `$FF1887`, waits for button press (`0x00C536`), then closes dialogue window (`0x00C45C`).
  - `0x01`: **OP_NEWLINE** — Advances line index (`0x00C566`), waits 1 frame.
  - `0x02`: **OP_PROMPT_NEWLINE** — Waits for button press, then advances line.
  - `0x03`: **OP_DELAY_FRAMES** — Reads next byte `N`, delays `N` frames.
  - `0x05`: **OP_PROMPT_WAIT** — Waits for button press (stays on current line).
  - `0x06`: **OP_CLEAR_PAGE** — Scrolls 4 lines, resets row offsets (`12, 24, 36, 48`) and cursor X to 0.
  - `0x07`: **OP_PROMPT_CLEAR_PAGE** — Waits for button press, then clears page.

## 4. Interactive Controller Button Logic (`0x00C536`)
- Activates prompt wait icon: sets `$FF1886 = 1`.
- Frame wait loop checks `$FF165F` joypad state:
  - Bit 6: Button A
  - Bit 4: Button B
  - Bit 5: Button C
- When any of bits 4, 5, 6 is pressed (`joypad & 0x70`), exits loop and clears prompt wait icon (`$FF1886 = 0`).

## 5. Cleanroom C++20 Interface
```cpp
namespace thor::dialogue {

enum class Opcode : uint8_t {
    Terminate = 0x00,
    Newline = 0x01,
    PromptNewline = 0x02,
    DelayFrames = 0x03,
    PromptWait = 0x05,
    ClearPage = 0x06,
    PromptClearPage = 0x07,
};

struct Token {
    bool is_opcode{false};
    char text_char{0};
    Opcode op{Opcode::Terminate};
    uint8_t param{0};
};

struct VMState {
    uint16_t dialogue_id{0};
    std::array<int16_t, 4> line_row_offsets{12, 24, 36, 48};
    int16_t cursor_x{0};
    int16_t line_idx{0};
    bool prompt_icon{false};
    bool dismissal_wait{false};
    bool window_open{true};
    bool entity_active{true};
};

uint32_t resolve_dialogue_address(std::span<const uint8_t> rom, uint16_t id);
std::vector<Token> decode_dialogue_stream(std::span<const uint8_t> stream);
void step_dialogue_vm(VMState& state, const Token& token, uint8_t joypad_pressed);

} // namespace thor::dialogue
```
