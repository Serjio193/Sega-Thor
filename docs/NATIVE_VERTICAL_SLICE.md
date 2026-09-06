# M11.18 Native Controlled Screen

## Scope and status

This is a bounded controlled screen for proving the native execution path,
not a reconstructed original Beyond Oasis room. The current status is
`NATIVE_VERTICAL_SLICE_PARTIAL`: the interactive Windows path is implemented
and unit-tested, while the non-Windows build has no GUI backend and end-to-end
window/input observation remains a manual platform check.

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

## Unknowns and next work

This slice does not prove an original room format, camera behavior, sprite
engine semantics, combat, lifecycle flags or ROM-derived rendering. The next
recommendation is D: fix the remaining vertical-slice blocker. The platform
follow-up should add and verify a native non-Windows window path or explicitly
narrow the supported runtime platform before broader gameplay features are
added.
