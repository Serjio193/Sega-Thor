# Standalone Handoff: M15.280 — Event VM Main 31-Opcode ISA (`0x00F788`..`0x00F80D`)

## Summary
The Beyond Oasis Event VM executes script bytecode from `$FF17B2` (`a0`) each frame via `0x00F788` -> `0x00F7B4`, dispatching opcode byte `(a0)` (`0x00..0x1E`) through the 31-entry signed self-relative jump table at `0x00F7D0`.

## VM State RAM Map
- `$FF17B2` (`u32`): Current script program counter (`a0`)
- `$FF17B6` (`u8`): Frame wait counter (decremented each tick at `0x00F796`)
- `$FF17B7` (`u8`): Max active non-spirit entity cap for spawns
- `$FF17B8` (`u8`): VM pause/control flags (bit 1 pauses VM execution)
- `$FF17BD` (`u8`): Camera pan/cutscene active flag (`0x00F7AA` branches to `0x00F894` when nonzero)
- `$FF1826` (`u16[]`): Loop counters indexed by opcode `0x06`/`0x07`
- `$FF1836` (`u32`): Single-level subroutine return PC (`0x13` call / `0x14` return)

## Key Opcodes (`0x00F7D0`, `0x00..0x1E`)
- Flow control: `0x00` yield/end (`0x00F91C`), `0x04` wait_until_condition (`0x00F96A`), `0x05` jump_rel16 (`0x00FAAC`), `0x06`/`0x07` loop counter (`0x00FC6C`/`0x00FC94`), `0x08` wait_frames (`0x00FCA6`), `0x12` branch_if_condition (`0x00FAB2`, M15.279), `0x13`/`0x14` call/return (`0x0101D0`/`0x0101E4`), `0x15` branch_if_flag (`0x00FEDE`).
- Flags & vars: `0x0F` global flag `$FF0DBC` (`0x010140`), `0x10` chest flag `$FF0E1C` (`0x010170`), `0x11` secondary flag `$FF0E2C` (`0x0101A0`), `0x1C` event var `$FF17C4` (`0x0100EE`).
- Spawning & world: `0x01`, `0x02`, `0x03`, `0x09`, `0x0A`, `0x0B`, `0x0E`, `0x16`, `0x17`, `0x18`, `0x19`.
- Audio/Visual/Camera: `0x0C`/`0x0D` load palette (`0x010086`/`0x0100AA`), `0x1A` trigger VDP/tile (`0x0100D6` -> `0x00E78E`), `0x1B` play sound cmd (`0x0100E4` -> `0x060004`), `0x1D` camera target (`0x0100F8`), `0x1E` camera pan/fade (`0x00F854`).

## C++20 Standalone Target
- Implement `EventVm` interpreter in `src/game/events/` dispatching all 31 opcodes (`0x00..0x1E`) alongside M15.279's Opcode `0x12` evaluator.
- Evidence artifact: `data/thor-evidence/rom_event_vm_isa_m15_280.json`.
