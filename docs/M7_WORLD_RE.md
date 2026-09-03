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

`0x9C40` takes entity coordinates from `FP+8` and `FP+12`, arithmetic-shifts both by three and reads one byte:

```text
cell_x = world_x >> 3
cell_y = world_y >> 3
index  = cell_x + (cell_y << row_shift)
value  = grid[index]
```

The raw byte is returned in `D5`. Native implementation: `src/game/world/byte_grid.*`. The safe C++ view rejects negative or out-of-span coordinates instead of reproducing unchecked 68000 memory access.

### `0x9BF2` — footprint OR/AND aggregation

`0x9BF2` samples every byte-grid cell intersected by a square centered at `(D1,D2)` with radius `D3`:

```text
x0 = (center_x - radius) >> 3
x1 = (center_x + radius) >> 3
y0 = (center_y - radius) >> 3
y1 = (center_y + radius) >> 3
```

For the inclusive rectangle it computes:

```text
D4.low = OR  of all covered grid bytes, starting from 0x00
D5.low = AND of all covered grid bytes, starting from 0xFF
```

Wrapper `0x9BC2` supplies entity `FP+8`, `FP+12` and footprint/radius field `FP+70`. Native `ByteGridView::aggregate_world_square()` reproduces the confirmed query with bounds checking and synthetic OR/AND tests.

### Terrain code and state tables

The low nibble of the grid byte is a confirmed gameplay terrain/property code.

ROM table `0x96E8` maps terrain code to the state used by movement gates:

```text
code:   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
state: -1  0  2  1  4 -1  3 -1  6  7 -1 -1  5 -1 -1 -1
```

ROM table `0x96F8` maps terrain code to a height/behavior class:

```text
code:      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
behavior: -1  0  2  1  4  4  3  3  6  6  6  6  5  5  5  5
```

`0x9AD6` uses the `0x96F8` class. Even classes select a direct runtime value from `0xFF1706`; odd classes interpolate between neighboring terrain cells. The interpolation behavior is confirmed, but surface names such as ramp/water/ledge are not assigned yet.

`0x9D00` calls `0x9BF2`, stores the aggregate bytes in entity fields `FP+111` and `FP+110`, converts `(D5 & 0x0F)` through `0x96E8` into `FP+112`, then calls `0x9AD6` and stores its derived value in `FP+20`.

## `0x938E` — directional terrain movement gate — CONFIRMED

The normal X-axis movement loop calls `0x938E` immediately before committing a step and branches on Carry:

```text
BSR 0x938E
BCS blocked_path
```

Therefore:

- Carry clear = movement permitted;
- Carry set = movement blocked / collision response.

The routine samples the prospective footprint edge, converts the common low-nibble terrain code through `0x96E8`, compares the prospective terrain state with the entity's current `FP+112` state, and applies entity/grid flags.

For the boolean allow/block result, the confirmed decision is:

1. entity flag bit 0 at `FP+56` bypasses the gate;
2. negative current terrain state bypasses the gate;
3. negative prospective terrain state blocks movement;
4. terrain-state differences `>= +2` or `<= -2` block movement;
5. for a nonzero difference, entity flag bit 5 blocks the transition;
6. for equal states, prospective aggregate bit 7 combined with entity flag bit 6 blocks movement;
7. prospective aggregate bit 4 blocks movement;
8. otherwise movement is allowed.

Large downward transitions have additional side effects involving runtime height table `0xFF1706`, `0xFF196E` and entity status bit 5, but still return Carry set. Those side effects are intentionally not part of the pure boolean C++ gate.

Native implementation: `src/game/world/terrain_collision.*`. `TerrainGateResult` models only the original Carry result; player/entity response side effects remain for later milestones.

Sibling routines beginning near `0x94D2` and `0x95AA` apply the same state-transition logic to other directional edges and set the corresponding entity collision/status byte.

## Rejected collision candidates

### `0x10382` — active entity ID lookup, NOT collision

It receives an ID in `D0`, walks 17 entity slots from `0xFF1CD8` with stride 188 bytes and compares `(word +24 & 0x07FF)`. Matching active entity returns Carry clear; no match returns Carry set. `0xFF17C6` is an entity/object ID list in this path, not a collision map.

### `0x10594` — target steering / velocity generation, NOT collision

It computes target direction, turns toward it by at most eight angle units per call, maps the angle to orientation and uses lookup data to generate fixed-point velocity components. It belongs with later player/enemy movement work.

## Remaining M7 work

The collision representation and boolean movement-gate semantics are now evidenced and translated. Remaining acceptance work is integration/verification: run the new synthetic tests, keep the supported-ROM screen descriptor oracle green, update project state/worklog, and only then decide whether M7 can be marked DONE.
