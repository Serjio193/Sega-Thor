# Standalone Handoff: M15.279 — Event VM Opcode `0x12` Conditional Branch (`0x00FAB2`..`0x00FC6B`)

## Summary
Event VM Opcode `0x12` (`0x00FAB2`) is the script conditional branch instruction. It dispatches on byte `+2(a0)` through a 12-entry signed self-relative jump table at `0x00FAC2`, and numeric sub-actions share a 6-entry comparator jump table at `0x00FB64`.

## Instruction Layout (8 bytes when false, variable jump when true)
- `+0` (`u8`): `0x12` (opcode)
- `+1` (`u8`): unused/padding
- `+2` (`u8`): sub-action (`0x00..0x0B`)
- `+3` (`u8`): comparator selector (`0, 2, 4, 6, 8, 10`) or boolean polarity (`0` = branch if true/set, nonzero = branch if false/clear)
- `+4` (`u16` BE): `imm16` (comparison threshold, bit index, or entity ID)
- `+6` (`s16` BE): relative byte offset added to `a0` at `0x00FBA0` when the condition passes (`0x00FBA6` advances `a0 += 8` when it fails).

## Sub-Actions (`0x00FAC2`, `0x00..0x0B`)
| Sub | Target | Check |
|---:|---|---|
| `0x00` | `0x00FADA` | Active non-spirit entity count via `0x0101EC` (`$FF1CD8`, 17 slots, stride `0xBC`, `word[+0] > 0 && !(byte[+0x3A] & 1)`) compared with `imm16` |
| `0x01` | `0x00FBAC` | Bit `imm16` in global event flag bitset `$FF0DBC` (`+3 == 0`: branch if set; else if clear) |
| `0x02` | `0x00FBDC` | Bit `imm16` in chest/pickup flag bitset `$FF0E1C` |
| `0x03` | `0x00FBFC` | Bit `imm16` in secondary flag bitset `$FF0E2C` |
| `0x04` | `0x00FC1C` | Entity ID `imm16` present in `$FF1CD8..` via `0x010232` (`+3 == 0`: branch if found; else if absent) |
| `0x05` | `0x00FAE8` | Player HP% (`$FF1A72 * 100 / $FF1A70` via `0x010254`) compared with `imm16` |
| `0x06` | `0x00FAF6` | Player SP% (`$FF1A76 * 100 / $FF1A74` via `0x010268`) compared with `imm16` |
| `0x07` | `0x00FC42` | Active spirit ID `$FF1AA4 == imm16` (`+3 == 0`: branch if equal; else if not equal) |
| `0x08` | `0x00FB04` | Player X `$FF19F0` compared with `imm16` |
| `0x09` | `0x00FB14` | Player Y `$FF19F4` compared with `imm16` |
| `0x0A` | `0x00FB24` | If `$FF1AA4 == 0x1A`, compare spirit X `$FF1AAC` with `imm16`; else false |
| `0x0B` | `0x00FB40` | If `$FF1AA4 == 0x1A`, compare spirit Y `$FF1AB0` with `imm16`; else false |

## Comparators (`0x00FB64`)
`0`: `==` (`0x00FB70`), `2`: `!=` (`0x00FB78`), `4`: `>` (`0x00FB80`), `6`: `>=` (`0x00FB88`), `8`: `<=` (`0x00FB90`), `10`: `<` (`0x00FB98`).

## C++20 Standalone Target
- Mirror in `src/game/events/event_vm_op12.*` with deterministic unit tests in `tests/` covering all 12 sub-actions and all 6 comparators.
- Evidence artifact: `data/thor-evidence/rom_event_vm_op12_m15_279.json`.
