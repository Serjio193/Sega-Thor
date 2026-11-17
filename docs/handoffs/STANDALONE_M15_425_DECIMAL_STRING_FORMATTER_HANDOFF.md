# STANDALONE HANDOFF: ROM Fixed-Width & Space-Padded Decimal String Formatters (0x00C0BE..0x00C17A)

## 1. Routine Overview
- **ROM Range:** `0x00C0BE..0x00C17A` (188 bytes).
- **Function:** HUD, Menu & Save-File Decimal String Formatter. Converts 16-bit unsigned integers into 5-byte ASCII strings at buffer `$FF192A`. Provides two entrypoints: `format_dec_fixed5` (`0x00C0BE`, fixed 5-digit zero-padded `"00123"`) and `format_dec_spaces` (`0x00C114`, leading-space padded `"  123"`).
- **Evidence JSON:** `data/thor-evidence/rom_decimal_string_formatter_m15_425.json`.

## 2. Ingress Entry Points
1. `0x00C0BE` (`format_dec_fixed5`):
   - Input: `d0` (16-bit unsigned integer).
   - Writes 5 ASCII characters `'0'..'9'` to `$FF192A..$FF192E`.
   - Returns with `a0` pointing to start of buffer `$FF192A`.
2. `0x00C114` (`format_dec_spaces`):
   - Input: `d0` (16-bit unsigned integer).
   - Writes leading spaces `0x20` for zero values, followed by remaining decimal digits (units digit is always printed).
   - Returns with `a0` pointing to start of buffer `$FF192A`.

## 3. Algorithm & Logic
1. **Division Sequence:**
   - Divides successively by 10000, 1000, 100, 10 (`divu.w`).
   - Remainder swapped into low word for each stage.
   - Units digit extracted from remainder of divide by 10.
2. **Leading Space Suppression (`0x00C114`):**
   - Tests quotient at each stage (`tst.b d0`).
   - If zero: outputs ASCII space `#$20` (`' '`), continues suppression.
   - If non-zero: jumps into digit conversion path of `0x00C0BE`, formatting all remaining stages as digits `'0'..'9'`.
   - Units stage is unconditional: always formats as digit `d0.b + '0'`.
3. **Pointer Rewind:**
   - Bytes written: 4 with post-increment `(a0)+`, 1 without post-increment `(a0)`.
   - Executes `lea -$4(a0), a0` to rewind pointer back to the string start.

## 4. Recommended C++20 Interface
```cpp
namespace thor::hud {

class DecimalFormatter {
public:
    static constexpr size_t kBufferLen = 5;

    // Fixed 5-digit zero-padded, e.g. "00042"
    static std::array<char, kBufferLen> format_fixed5(uint16_t val);

    // Leading-space padded, e.g. "   42", "    0"
    static std::array<char, kBufferLen> format_spaces(uint16_t val);
};

} // namespace thor::hud
```
