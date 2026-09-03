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

## Auxiliary byte grids `0xFF1766` / `0xFF1770` — CONFIRMED

The loader around `0xD560` initializes two parallel byte-grid descriptors. Confirmed fields are:

- `+0`: pointer to byte-grid backing data;
- `+4`: row stride in bytes;
- `+6`: second dimension loaded from the resource header;
- `+8`: row-address shift corresponding to the power-of-two row stride.

The active grid is selected by bit 7 of `0xFF16F0`.

### `0x9C40` — single-cell world query

`0x9C40` is a direct single-cell byte-grid reader. It takes entity coordinates from `FP+8` and `FP+12`, arithmetic-shifts both by three (8-pixel cells), and computes:

```text
cell_x = world_x >> 3
cell_y = world_y >> 3
index  = cell_x + (cell_y << row_shift)
value  = grid[index]
```

The raw byte is returned in `D5`. This establishes the native world-to-byte-grid addressing rule.

Native implementation: `src/game/world/byte_grid.*`. The safe C++ view rejects negative or out-of-span coordinates instead of reproducing unchecked 68000 memory access.

### `0x9AD6` — terrain/property decoding

`0x9AD6` samples the same byte grid from entity world coordinates and masks the result with `0x0F`. The resulting low-nibble code is passed through ROM table `0x96F8`, and the mapped value affects subsequent movement/height logic. Neighboring grid cells are also compared and table-derived values are interpolated.

Therefore the low nibble is a confirmed gameplay terrain/property code. Individual bits are not yet named as `blocked`, `water`, `slope`, or similar until their meanings are proven.

### `0x9BF2` / `0x9D00` — footprint terrain aggregation

`0x9BF2` samples a rectangular region of the byte grid around supplied coordinates and accumulates both OR and AND results across the covered cells. Wrapper `0x9BC2` supplies entity coordinates and a footprint/radius-like field. `0x9D00` stores the aggregate results into entity state and invokes `0x9AD6` for the single-position terrain calculation.

This confirms that the byte grid participates directly in entity movement/terrain handling rather than only rendering.

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

This routine belongs with later entity/player/enemy movement work, not the M7 collision API.

## M7 next proof

The world-grid storage and raw terrain-code sampling contract are now translated. The remaining M7 proof target is the exact interpretation of terrain-code lookup results in `0x9AD6` / `0x9D00`: which combinations prevent movement, alter height, or represent traversable slopes/surfaces. Do not collapse this into a boolean collision flag before that behavior is demonstrated.
