# M7 World / Map Reverse Engineering

This note records confirmed M7 evidence separately from hypotheses. Do not rename unknown fields until their semantics are demonstrated by callers or ROM-backed tests.

## Screen dispatcher — CONFIRMED

The original dispatcher at `0xC8F0` treats the 16-bit screen ID as two independent bytes.

- high byte: index into the 21-entry longword group table at `0xC92C`;
- low byte: index into a table of signed 16-bit relative offsets inside that group;
- the relative offset resolves to a 26-byte screen descriptor;
- screen initialization code starts directly at `descriptor + 26`.

Confirmed reference-ROM mappings:

| Screen ID | Descriptor |
|---:|---:|
| `0x0009` | `0x02CF82` |
| `0x000C` | `0x02D3E8` |
| `0x0704` | `0x032144` |
| `0x0705` | `0x03285C` |

Native implementation: `src/game/world/screen_descriptor.*`.

## Map/grid structures — CONFIRMED structural behavior

A map-state structure selected at `0xFF1716` or `0xFF173E` is consumed by routines around `0xD7C0..0xDD14`.

Observed fields:

- `+22`: pointer to a word-addressed 2D data buffer;
- `+32` and `+34`: bounds/dimensions used for wrapping and row calculations;
- `+36`: shift used in row-address calculations;
- `+8` and `+10`: wrapped indices derived from structure coordinates divided by eight.

The effective address pattern is equivalent to a word grid: base plus a shifted row term plus `2 * column`. Routines `0xDB84..` copy rectangular word regions. Routines `0xDBCA..` copy rectangular byte regions through auxiliary structures at `0xFF1766` / `0xFF1770`.

These paths also feed VDP updates, so they establish world/tile-grid behavior but do not by themselves prove collision semantics.

## `0x10382` — active entity ID lookup, NOT collision

The collision probe rejected an earlier hypothesis.

`0x10382` receives an ID in `D0`, walks the entity array starting at `0xFF1CD8`, and checks 17 slots with stride 188 bytes.

For each slot:

1. word `+0` must be positive/active;
2. word `+24` is masked with `0x07FF`;
3. the masked value is compared with incoming `D0`.

Result contract:

- matching active entity: carry clear;
- no matching entity: carry set.

Callers at `0x1027C`, `0x102BE`, `0x10300`, and nearby iterate words from `0xFF17C6`, mask them with `0x07FF`, and use `0x10382` to determine whether a corresponding active entity exists. Therefore `0xFF17C6` is not evidence for a collision map in this path.

Direct absolute callers elsewhere were also found at `0x26DE4`, `0x26DF2`, `0x26E00`, and `0x26E0E`.

## `0x10594` — target steering / velocity generation, NOT collision

A second collision candidate was rejected after caller and routine analysis.

Fourteen direct callers pass world-like coordinates in `D0/D1`, current direction in `D2`, and movement magnitudes in `D3/D4`. Common callers load `D0/D1` from `0xFF19F0/0xFF19F4`.

The routine:

1. subtracts entity position fields `FP+8` / `FP+12` from the target coordinates to form signed X/Y deltas;
2. uses helper `0x10660` and the ROM table at `0x5D906` to derive a 256-step direction angle;
3. turns the existing `D2` direction toward that target direction by at most eight angle units per call;
4. maps the resulting angle to a coarse orientation stored in `FP+22`;
5. uses the signed lookup table at `0x5D706` as sine/cosine-style data;
6. scales those components by `D3/D4` and writes the resulting fixed-point velocity components to `FP+78` and `FP+82`.

Examples:

- caller `0x227BC` uses magnitudes `0x00020000` and `0x00018000`;
- caller `0x270BE` uses `0x00030000` and `0x00024000`.

This routine belongs with later entity/player/enemy movement work, not the M7 collision API.

## Current collision target

Search now focuses on readers of the auxiliary byte-grid structures at `0xFF1766` / `0xFF1770`. Rendering code proves these structures participate in row/column addressing, but the next required evidence is a routine that reads individual grid values from world coordinates and returns a gameplay property such as blocking, surface, height, or traversal type.
