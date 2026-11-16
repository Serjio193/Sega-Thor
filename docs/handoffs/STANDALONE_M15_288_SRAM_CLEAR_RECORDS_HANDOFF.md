# Standalone Handoff: M15.288 — SRAM Clear Records & Best Time (`0x001DDC`..`0x001F20`)

## Summary
Beyond Oasis persists Best 5 Players clear records and speedrun best-time data in odd-byte cartridge SRAM (stride `2`, controlled via `$A130F1`: `0x01` write, `0x02` lock, `0x03` read):
- **`$2025CD` (`"Rank00"` at `0x0004BA`):** 6-byte magic header + `130` (`0x82`) payload bytes + `u16_be` byte-sum checksum (`0x001DDC` load / `0x001E46` save). Zeroes payload on magic or checksum mismatch.
- **`$2026E1` (`"Race00"` at `0x0004C0`):** 6-byte magic header + `u32_be` best time (`0x001ECE` load).
- **`0x001E98`:** Clears first magic byte at both `$2025CD` and `$2026E1`.

## C++20 Standalone Target
- Extend SRAM persistence in `src/game/` with `"Rank00"` (`130` bytes + `u16_be` byte-sum) and `"Race00"` (`u32_be` best time) encode/decode/verify helpers.
- Evidence artifact: `data/thor-evidence/rom_sram_clear_records_m15_288.json`.
