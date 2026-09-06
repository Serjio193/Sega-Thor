# M11.18 Native Controlled Screen

## Scope and status

This is a bounded controlled screen for proving the native execution path,
not a reconstructed original Beyond Oasis room. The current status is
`NATIVE_VERTICAL_SLICE_PLAYABLE`: the supported Win32 path is implemented,
unit-tested and manually validated with the canonical ROM. The non-Windows
build still has no GUI backend; that portability limitation is recorded as the
single next recommendation and is not a Win32 vertical-slice runtime blocker.

## Native path

```text
NativeWindow input polling
    -> core::RuntimeLoop fixed logical step
    -> game::screen::ControlledScreen
    -> player::try_move
    -> ByteGridView footprint aggregation and terrain gate
    -> software framebuffer rasterization
    -> Win32 DIB presentation
```

`RuntimeLoop` receives one explicit `InputSnapshot` per logical frame.
Presentation is scheduled at a 60 Hz logical tick, but rendering does not
update game state. Arrow keys map to Up/Down/Left/Right. Polling returns an
empty controller state when the window is not foreground, preventing stuck
input after focus loss.

The framebuffer is a deterministic 320x224 array of packed `0x00RRGGBB`
pixels. Windows presents it through a scaled 32-bit DIB. The non-Windows
adapter intentionally reports unavailable rather than adding a GUI dependency
to the core project.

## Test fixture

`ControlledScreen` builds a synthetic 32x28 byte grid with 8-pixel cells:

- terrain code `0x02` is the free area;
- terrain code `0x05` is the visible blocking wall;
- a joined wall corner and an isolated block are included;
- the player starts at `(48, 48)` with a non-zero footprint radius of 6 pixels.

The fixture geometry is not ROM-derived and must not be described as the
original room layout. Corner behavior is a bounded fixture assumption: the
existing footprint aggregation evaluates the complete candidate square, so a
diagonal candidate touching either wall is blocked. Fixed-point candidates
below zero are rejected explicitly; the upper boundary is rejected by the
bounded grid view.

## Reused and verified behavior

- player movement uses the existing 16.16 constants: right `0x36000`,
  diagonal X `0x2A000`, and diagonal Y `0x25800`;
- all 16 direction nibbles, release, diagonals, obstacle, corner, boundary
  and footprint cases are covered by the native vertical-slice test;
- a fixed 600-frame replay compares x/y, intent, accumulated delta, direction,
  movement state, terrain state, footprint bits and movement outcome after
  every frame;
- replay states are identical when presentation occurs every 1, 7 or 31
  logical frames and across repeated runs;
- the final software framebuffer has a bounded canonical SHA-256 oracle:
  `3e1c211e1560ea42243e27f05be0704537c51ec3901995d89f228b0e616aa0da`;
- existing decompression, tile decoding and palette/VDP tests remain separate
  and are not weakened by this slice.

## ROM boundary

The executable requires the external canonical USA retail ROM and fails before
opening a window for any unsupported, modified or unknown identity. The
controlled fixture itself is synthetic. No commercial ROM bytes, extracted
tiles, palette data or room assets are compiled into the repository, and no
Ghidra, BizHawk, vasm, RE script or 68000 emulator is needed at runtime.

## M11.18.1 validation evidence

Baseline checkout: `19d364403757e1f19afbff5607235692523e893d`.

The Debug native executable was launched with the canonical USA retail ROM.
The Win32 window opened with the expected title and remained responsive during
the complete input/focus/collision sequence. Repeated presses visibly moved the
player in all four cardinal directions and in all four diagonal combinations.
Each discrete press released cleanly; after a movement sequence the player
stopped rather than continuing on a stale direction. The foreground was
switched to Explorer and back to the game; input was cleared while unfocused,
and no stuck movement was observed after focus returned. A fresh rightward
movement sequence visibly reached the fixture wall and remained blocked while
the window continued repainting/responding.

The UI automation key API provides discrete press/release events rather than a
separate held-key primitive. Accordingly, the release and focus checks record
the observable press/release and foreground-transition behavior supported by
the native validation harness; the product path itself uses foreground-safe
polling and is covered by the deterministic tests.

Canonical ROM acceptance and unsupported/beta ROM rejection were both checked.
The 600-frame replay and framebuffer SHA-256 oracle remained unchanged:
`3e1c211e1560ea42243e27f05be0704537c51ec3901995d89f228b0e616aa0da`.

## Unknowns and next work

This slice does not prove an original room format, camera behavior, sprite
engine semantics, combat, lifecycle flags or ROM-derived rendering. The next
recommendation is D: platform portability. That follow-up should add and verify
a native non-Windows window path or explicitly narrow the supported runtime
platform before broader gameplay features are added.
