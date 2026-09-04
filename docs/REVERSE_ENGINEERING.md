# Reverse-Engineering Ledger
This file records what is known about the original Beyond Oasis binary. Do not promote guesses to facts without evidence.
## M11.5 — bounded resolution, CFG audit, closure and MOVEA transfer
**Status:** VERIFIED bounded tooling; no semantics inferred. Prior resolution
examined 390 refs, resolved 294 and left 96; Atlas remains 577 raw unresolved.
`oasis.m68k.re-cfg-audit.v1` accounts for 80 nonreachable refs in 17 islands.
`oasis.m68k.re-reachable-closure.v1` accounts for exactly 16 reachable refs:
14 prior `call_clobber`, 2 stack-unknown `other`, newly resolved 0, after 16.
The former unsupported boundary is now the exact longword rule
`MOVEA.L (A7)+,An`: mode 3/A7 to mode 1/An, `An=memory[old A7]`, `A7 += 4`.
Narrow stack provenance accepts only immediate long push, known-address PEA,
proven `MOVE.L An,-(A7)` and this pop. USA `0x60BD0` is `20 5F`; both target
paths cross `BSR 0x60BCC`, so stack value/A7 input remain unknown. No callee,
ABI, return-address effect, semantic role, dynamic scenario, runtime,
whole-ROM analysis or M12 work was added.
Exact reachable addresses: `604EA`, `60BD8`, `60BFA`, `60C08`, `60C1E`, `60C34`,
`60C4A`, `60C60`, `60C76`, `60C8A`, `60C94`, `60CAA`, `60CC2`, `60D94`, `60DB0`, `60DC8`.
## Known hardware addresses
| Address | Meaning | Confidence | Evidence/status |
|---|---|---:|---|
| `0x00C00000` | Mega Drive VDP data port | CONFIRMED | Public ROM-hacking constants + standard Mega Drive mapping |
| `0x00C00004` | Mega Drive VDP control port | CONFIRMED | Public ROM-hacking constants + standard Mega Drive mapping |
| `0x00FF0000` | 68000 work RAM base | CONFIRMED | Public ROM-hacking constants + standard Mega Drive mapping |
## Reference ROM identity

### USA retail — canonical engineering reference
| Field | Value | Confidence |
|---|---|---|
| Title | `Beyond Oasis (USA)` | CONFIRMED |
| Size | `3145728` bytes | CONFIRMED |
| CRC32 | `C4728225` | CONFIRMED |
| SHA-1 | `2944910c07c02eace98c17d78d07bef7859d386a` | CONFIRMED |
| SHA-256 | `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263` | CONFIRMED from uploaded reference |
| Detector result | `SUPPORTED` | VERIFIED |

### Region policy
- USA addresses are the default notation in reverse-engineering documents.
- Region-specific offsets must not leak into native gameplay code.
- Europe/Japan may be compared to USA as secondary evidence.
- Final C++ game model is region-independent.

## `[0x00003820, 0x00003B3E)` — graphics decompression routine
**Status:** VERIFIED and translated to native C++.

**Semantic confidence:** CONFIRMED for decompression behavior and call contract.

### Boundaries and call contract
- Exact half-open range: `[0x3820, 0x3B3E)`.
- Verified USA ROM contains 52 direct absolute `JSR 0x3820` calls and 0 direct absolute JMPs.
- `A0`: compressed source pointer; returns advanced to immediately after consumed input — CONFIRMED.
- `A1`: destination pointer; returns advanced to immediately after output — CONFIRMED.
- `D0-D2/A2` are preserved; format B additionally preserves `D3/D6/D7`.
- No hardware access or nested subroutine call occurs inside the decompressor.

### Format dispatch
At `0x3824`, byte `source[2]` selects the format:
- nonzero: command-stream format A;
- zero: bitstream format B.

### Format A
Observed command families:
- literal byte runs;
- repeated-byte runs;
- sliding-window backreferences into already-produced output;
- chained `0b011xxxxx` extensions continuing the preceding backreference;
- block-length framing and byte terminator.

### Format B
Observed behavior:
- three-byte block header followed by LSB-first control bits;
- literal tokens;
- multiple backreference forms;
- distance `1` special repeated-byte run;
- distance `0` block terminator;
- 16-bit control-bit refill.

### Native mapping and oracle traces
Implementation: `src/game/graphics_decompress.*`.

| Format | Caller | ROM source | Source consumed | Output bytes | Output SHA-256 |
|---|---:|---:|---:|---:|---|
| A | `0x00C394` | `0x16943C` | 1217 | 3072 | `65e99e74020fedbdcb97c8249a5ccfe540aca5bb5d29bfb260352cd6f388c31a` |
| B | `0x03C276` | `0x1894EA` | 112 | 128 | `167d4e5409f6b075b3b6f2bc61dbb747e8d8c857e8699745184ddf48d83bcda9` |

Native C++ matches original 68000 execution on source consumed, output length and SHA-256 for both oracle cases.

## M7 — indexed compressed-resource table at `0x0005CE96`
**Status:** INVESTIGATING.

### Table structure
- Base used by original code: `0x0005CE96` — CONFIRMED.
- Entry width: 4-byte absolute ROM pointer — CONFIRMED by reader at `0xD3B2`.
- Entry 0: `0x000000` — CONFIRMED; semantic role remains UNKNOWN (likely sentinel/unused).
- Entries 1..107 form a dense run of valid compressed-resource pointers from `0x1AD000` through `0x1E6EDA` — CONFIRMED.
- First entries:
  - index 1 -> `0x1AD000`;
  - index 2 -> `0x1AD9D4`;
  - index 3 -> `0x1AE1A8`;
  - index 4 -> `0x1AE8AA`.
- Sample pointed streams have nonzero `source[2]` and therefore use verified decompressor format A — CONFIRMED for sampled entries.

### Reader at `0x0000D3B2`
**Behavior: CONFIRMED.**

Relevant original sequence:
```text
D3B2 save D0/A0/A1
D3B6 A1 = 0xFF2FA8
D3BC A0 = 0x05CE96
D3C2 D0 <<= 2
D3C4 A0 = *(A0 + D0)
D3C8 JSR 0x3820
...
D404 RTS
```

Therefore incoming `D0` is a 0-based resource-table index and selected entry is decompressed into work RAM `0xFF2FA8`.

### Direct calls to `0xD3B2`
Static absolute-call search found seven direct calls:
- `0x02CFAA`, `0x02CFB8`;
- `0x02D410`;
- `0x032174`, `0x032182`;
- `0x032884`, `0x032892`.

Immediate `D0` values observed immediately before these calls include:
- `3`, `4`;
- `0x57` (87);
- `0x23` (35), `0x24` (36).

This proves the table is selected by stable numeric IDs used in gameplay/scene code. What those IDs mean is still under investigation.

### Developer screen-name list
The canonical ROM contains a developer-facing screen-name list in the same general metadata region:
- `VILLAGE` at `0x05DB4D`;
- `ECAPITAL` at `0x05DB56`;
- `HARBOR` at `0x05DB61`;
- `01-00` at `0x05DC48`;
- `01-05 BOSS` at `0x05DC70`;
- `14-01 KING` at `0x05E110`.

**Current hypothesis:** the `0x5CE96` indexed compressed-resource table may correspond to screen/room resources because its ~107 meaningful entries and stable numeric selectors are compatible with the ROM's developer screen inventory. **Confidence: LIKELY, not CONFIRMED.** Exact index-to-name correlation is the next proof target.

### Structure-processing routine at `0x0000D406`
**Status:** INVESTIGATING; high relevance to world/screen setup.

This routine is called extensively from code in roughly `0x2Cxxx..0x3Axxx` and reads a structured record through `A1`:
- words at offsets `+16`, `+18`, `+20` copied to RAM `0xFF16F4..0xFF16F8`;
- word `+22` -> `D6`;
- word `+24` -> `D7` and RAM `0xFF16F0`;
- long at `+8` -> RAM `0xFF16FA`;
- bytes beginning at `+12` are transformed into derived RAM tables;
- additional state buffers begin at `0xFF1716` and `0xFF173E`.

Do not name the record fields until caller/data evidence establishes semantics.

### Related resource loading at `0xD4C8..`
The same `0x5CE96` table is also indexed by four bytes from RAM `0xFF16FA`; each nonzero index selects and decompresses a resource into separate 4096-byte-spaced destinations beginning at `0xFF3FA8`. This strongly indicates the table contains reusable scene-related graphic/data resources, but exact semantics remain UNCONFIRMED.

### M7 next proof
1. enumerate developer screen names in exact order and compare indices `3`, `4`, `35`, `36`, `87` against table selectors;
2. disassemble the seven `0xD3B2` callers far enough backward to identify their surrounding screen/event identity;
3. decompress representative table entries with native C++ and compare header/structure patterns;
4. only after correlation, introduce a portable room/screen resource loader.

## M8 — player input and movement slice
**Status:** IMPLEMENTED as a portable movement/state slice; full animation/entity callback semantics remain INVESTIGATING.

### Player entity selection
- `0x0013D6..0x00142E` initializes the main entity at work RAM `0xFF19E8` and writes entity type `2` before calling `0x8D06` — **CONFIRMED**.
- The main entity pool uses 21 records with stride `0xBC`; `0x008EB2..0x008ED0` iterates that pool — **CONFIRMED**.

### Controller normalization and direction mapping
- `0x00217C..0x002188` calls the controller reader at `0x2992`; normalized port state is stored beginning at `0xFF165C`, with the movement nibble in `0xFF165E` — **CONFIRMED**.
- `0x0085E2` masks `0xFF165E` with `0x0F`, dispatches through the table at `0x85FA`, and returns direction plus fixed-point deltas — **CONFIRMED**.
- Cardinal vectors are `(+0x36000,0)`, `(-0x36000,0)`, `(0,+0x30000)`, `(0,-0x30000)`; diagonal vectors are `(+/-0x2A000,+/-0x25800)` — **CONFIRMED** from `0x8624..0x86AE`.
- The native mapping preserves the original low-nibble behavior, including opposite-direction collapse and diagonal combinations; unsupported combinations resolve to no movement — **VERIFIED by synthetic tests**.

### Movement and collision contract
- The main game loop at `0x008B22` calls `0x00557A` (player update) before `0x008E90` (active-entity movement), and then `0x00A196` (sprite/entity scheduling) — **CONFIRMED**.
- `0x00557A` selects `0xFF19E8` and enters `0x005670`; the state dispatch at `0x0059BA` routes entity `+0x04` state `0` to `0x0061F6`, state `2` to `0x0062E4`, and state `4` to `0x006516` — **CONFIRMED**.
- `0x0061F6` reads the normalized direction, writes direction `+0x16`, intent deltas `+0x4E/+0x52`, accumulated deltas `+0x72/+0x76`, and transitions `+0x04` to state `2` — **CONFIRMED**.
- The state-2 branch at `0x0062E4` calls the same direction routine; its no-input path at `0x0062CC` clears `+0x4E/+0x52`, writes `+0x2A=0` and returns `+0x04` to `0`; shared movement owns cleanup of `+0x72/+0x76` — **CONFIRMED**.
- The state-4 branch at `0x006516` maps `+0x16` through `0x83D4`, gates the turn on `+0x17` and the normalized input, and accumulates the selected axis through `+0x72/+0x76` — **CONFIRMED**.
- When state-4 input is absent, `0x006618` compares `FF197E` with `6`; the short path reaches the state-2 stop block, while the timeout path writes `+0x04=0x000C` at `0x006624` — **CONFIRMED**.
- The shared movement routine consumes `+0x72/+0x76` before committing position; `+0x2A` and `+0x26` participate in the animation/state sequence, but their presentation semantics remain **INVESTIGATING**.
- `0x008F12..0x00938C` is the shared active-entity movement update; main-pool records enter at `0x8F22` — **CONFIRMED**.
- X/Y fixed-point deltas are accumulated in entity fields `+0x72/+0x76`; integer positions are committed to `+0x08/+0x0C` — **CONFIRMED**.
- `0x009BF2` aggregates the entity footprint, and `0x00938E` is called before an axis commit; carry set takes the blocked path — **CONFIRMED**.
- Native `update_movement_state` mirrors the confirmed state-2 stop and state-4 axis-selection/accumulation rules; `PlayerState::try_move` then consumes those deltas through `ByteGridView::aggregate_world_square` and `evaluate_terrain_gate`.
- Native `VelocityAdjustContext` mirrors the three confirmed `0x64C4` outcomes and is optional until the external flag lifecycle is reconstructed.
- The shared footprint OR result is written at entity `+0x6F`; state-4 calls `0x64C4` with the retained axis delta, where global flags and the low nibble of `+0x6F` select no scaling, half scaling, or a Y-only half scaling — **CONFIRMED**, exact global lifecycle **INVESTIGATING**.
- `FF1985` is written by several event/control paths including `0x56F2`, `0x57C6` and `0x7E50`, and is read by the player dispatcher and `0x64C4`; it is not a player-owned field — **CONFIRMED**, lifecycle **INVESTIGATING**.
- `FF1984` is cleared/set by the active-entity checks around `0x2D220` and `0x2F250`, including a main-entity test against `+0x6E` bit 5 and `+0x10`; it is an external context flag — **CONFIRMED**.
- Bit 4 of `FF16F1` is read by `0x64C4`; no direct bit-4 writer was found in the scanned ROM references, so its producer remains **UNKNOWN**.
- Rendering, animation scripts and the unknown entity callback at `+0x22` are intentionally outside this slice.

## M9 — common entity pool framework
**Status:** IMPLEMENTED; raw pool iteration and one representative callback-dispatch path are verified. Semantic entity behavior remains outside M9.

### Pool iteration evidence
- `0x008E90` iterates four records from `FF2954` with stride `0x5A`, stores dispatcher `0x8EAA` in `FF193C`, and branches active records to `0x8F12` — **CONFIRMED**.
- `0x008EB2` iterates 21 records from `FF19E8` with stride `0xBC`, stores dispatcher `0x8ECC` in `FF193C`, and branches active records to `0x8F22` — **CONFIRMED**.
- `0x008ED4` iterates six records from `FF2D8C` with stride `0x5A`, stores dispatcher `0x8EEE` in `FF193C`, and branches active records to `0x8F12` — **CONFIRMED**.
- Each loop reads record word `+0x00` and uses `bgt` as the active test; zero and negative values are skipped — **CONFIRMED**.
- The native `EntityPoolView` exposes bounds, raw record spans and this active predicate without assigning enemy/NPC semantics to records.

### M9 boundary
The first M9 slice does not translate AI, attacks, animation callbacks, spawn tables or entity field names beyond the raw offsets required by pool iteration. Those require separate caller/data evidence.

### M9 shared record fields and representative dispatch

- The common movement entry at `0x8F22` reads raw record offsets `+0x9C`,
  `+0x9D`, `+0x2A`, `+0x2C`, `+0x2E`, `+0x30`, `+0x32`, `+0x37`, `+0x38`,
  `+0x72` and `+0x76` — **CONFIRMED** by direct accesses in the shared
  entry. The initializer at `0x8D06` copies two ROM pointers into `+0x26`
  and `+0x22`; their presentation/behavior meanings remain unassigned.
- A representative non-player path is the `FF2954` processing block at
  `0xA6A4`: it scans four `0x5A`-byte records, gates on `+0x00 > 0` and
  bit 2 of `+0x3A`, then reaches `0xA7D4`, which performs the proven
  indirect `jmp (+0x22)`. This is recorded as a raw callback-dispatch
  behavior, not as enemy/NPC AI.
- `EntityRecordView` exposes bounded big-endian word/long reads and the raw
  `+0x22` pointer. It deliberately does not invoke ROM addresses or assign
  semantic names to the remaining fields.

## Existing translated compatibility behavior

## M10 — spirit slot and dispatch trace
**Status:** IMPLEMENTED as a narrow deterministic slice; spirit names, button
meanings, targeting, abilities and presentation semantics remain UNKNOWN.

### Slot storage and lifecycle evidence
- At `0x007BE8`, the event handler subtracts `0x16` from the event code and
  uses the result as a bit index for `0x00FF0DBA` — **CONFIRMED**. The four
  observed event values `0x16..0x19` therefore map to slot bits `0..3`.
- At `0x005202`, the status-display path reads `0x00FF0DBA`; the adjacent
  byte table at `0x00522E` is `12 13 14 15 16 17 18 19` — **CONFIRMED**.
  This establishes four contiguous slot/event entries, but does not prove
  their character names or ability semantics.

### Active dispatch evidence
- `0x0031B80` checks bits `3` and `1` of entity field `+0x41` before entering
  the observed path — **CONFIRMED**. The meanings of these raw input bits are
  intentionally unassigned.
- The path tests bit `1` of `0x00FF0DBA`, then tests bit `0` of
  `0x00FF0DC4` as a one-shot guard — **CONFIRMED**.
- When the slot gate is open, it calls `0x0000C2EC` with selector `0x13`,
  fixed values `D5=0`, `D6=0x4000`, `D7=0x4800`, and sets guard bit 0 —
  **CONFIRMED**.
- It then falls through to queue selector `0x15` through `0x0000CA24`, using
  the player record's `+0x08` position-derived value and `D4=0x18` —
  **CONFIRMED**. The selectors are retained as raw trace values; their
  resource/effect semantics are not promoted beyond what the callers prove.

### Native mapping and oracle
`src/game/spirits/spirit_slots.*` models event-to-slot bit updates, the raw
`0x31B80` gate/selector trace and the `0x7A10`/`0x846C` summon-entry seed
without invoking ROM addresses. Synthetic tests cover inactive input,
unavailable slot, open guard, repeated guarded dispatch and the accepted or
rejected summon-entry gate. `oasis_spirit_slots_reference` checks the USA
bytes at `0x7BE8`, `0x5202`, `0x522E`, `0x31B80`, `0x31BC4`, `0x7A10` and
`0x846C`.

### Target-selection evidence
- `0x17CA6` calls `0xB922` with relative bounds `[-10,-6,10,6,0,4]`, then
  checks the returned record's raw type word against `0x16` at `0x17CE4` —
  **CONFIRMED**.
- `0xB922` scans the 21-record `FF19E8` pool in order, skips an inactive
  record or a record whose pointer equals the owner pointer, applies the
  observed X/Y interval checks and Z containment check, and returns the first
  spatial match — **CONFIRMED**. In the observed path the owner is from
  `FF2D8C`, so equal numeric indices across the two pools are not excluded.
- The native `find_observed_target` preserves the first-match-then-type-check
  behavior. The raw fields used by the query are exposed without semantic
  names; interaction meaning remains UNKNOWN. The ROM oracle also checks the
  query setup at `0x17CA6` and the scan prologue at `0xB922`.
- `0x846C` separately initializes raw type `0x16` in record `FF1AA4`; because
  `0xB922` scans `FF19E8`, this is tracked as a possible related producer but
  not as proof of a summon/ability relationship — **UNKNOWN**.
- The callback path is cross-pool: initializer `0xFFDE` obtains an owner from
  the six-record `FF2D8C` pool via `0xD9F0`, stores callback `0x17A96`, and
  `0x17A96` enters `0x17CA6` when its state word is zero. The query helper
  `0xB922` receives that owner in `A6` but scans target records from
  `FF19E8` — **CONFIRMED**. This corrects the native boundary; it does not
  prove summon or ability names.
- A static scan of field `+0x00` writers found the only literal raw type
  `0x16` write at `0x847C`, targeting singleton record `FF1AA4`. The observed
  `FF19E8` construction sites at `0x11DF0`, `0x27B2A` and `0x2BD20` use other
  literal/table values or data-driven values; no producer of raw type `0x16`
  in `FF19E8` is proven — **UNKNOWN**.
- The additional loader scan confirms that `0xFFDE` consumes a data stream
  into the auxiliary `FF2D8C` pool, while the generic stream loaders at
  `0xFF1A` and `0xFCB8` allocate from `FF1CD8`. These are concrete pool
  boundaries, but they do not identify a producer for raw type `0x16` in
  `FF19E8` or prove that the `0x17A96`/`0x17CA6` callback is a summon —
  **CONFIRMED boundary; UNKNOWN semantics**.
- A separate summon-entry candidate is now byte-backed: `0x7A10` rejects
  caller flag bit 1 and calls `0x846C` only for caller state `+0x30 == 0x18`.
  `0x846C` writes raw type `0x16` to singleton `FF1AA4`, copies caller
  position `+0x08/+0x0C`, writes raw `0x13` to `+0x10`, copies `+0x17` to
  `+0x66` and `+0x14`, writes `0x4F8` to `+0x18/+0x5A`, and clears `+0xA6`
  and `+0xAA` — **CONFIRMED raw entry; summon identity UNKNOWN**. The
  table-derived velocity tail beginning at `0x84B2` remains outside the native
  slice until its complete data contract is recovered.

### Tile copy to work RAM
Initial C++ compatibility implementation exists, derived from the public `tilecopy_to_ram` macro. Revisit after data interfaces stabilize.

### Tile copy to VRAM
Initial C++ compatibility implementation exists, derived from the public `tilecopy_to_vram` macro. Current VDP model is intentionally narrow.

## M11 — raw event producer and router boundary
**Status:** IMPLEMENTED as a bounded raw-data slice; event names, progression,
dialogue and command semantics remain UNKNOWN.

### Producer evidence
- `0x0082AE` calls bounded helper `0xB9EC` with the active `FF19E8` pool and
  a search window built from raw bounds `[-6,+6]` — **CONFIRMED**.
- The first returned record is accepted only when raw type `+0x00` equals
  `0x0008`; the source type is then cleared — **CONFIRMED**.
- The producer composes `FF1976` from source `+0x32` shifted left by eight
  and source byte `+0x52`, then copies source `+0x04` to `FF1978` and source
  long `+0x4E` to `FF197A` — **CONFIRMED**.
- A static USA-ROM scan found no direct literal `BSR` or absolute `JSR` to
  `0x82AE`; its caller may be indirect or data-driven and remains UNKNOWN.
- The source type-8 meaning and resulting event-code meaning are not
  assigned. Internal selection semantics of `0xB9EC` remain outside this
  slice.

### Router evidence
- `0x007A28` tests caller field `+0x37` bit 1, reads byte `FF1976` and
  dispatches bounded raw ranges to `0x7B64`, `0x7BD4`, `0x7BF6`, `0x7BA4`,
  `0x7BE8` and `0x7B84`; zero and values above `0x3F` fall through to
  `0x7A6C` — **CONFIRMED**.
- When the tested bit is clear, control goes to raw address `0x7B28`, an
  immediate `RTS`; the adjacent routine begins at `0x7B2A` — **CONFIRMED**.
- The separate `0x7B2A` routine calls external raw address `0x60004` with constants
  `0x0006` and `0x0008`, masks the result to `0x01FF`, returns on the sentinel
  `0x01FF`, then clears mask `0xFFF9` at `FF17B8` and writes raw `FF0D7E` to
  current-record `+0x06` and `0xFFFF` to `+0x5C` — **CONFIRMED**. The helper
  and field meanings remain UNKNOWN.
- The external entry at `0x60004` contains `BRA.W +0x0424`, whose 68000
  word-displacement target is `0x6042A`; `0x6042C` is the following
  instruction. Its raw command
  dispatcher compares `D0` against values `1..8`, and the command `0x0006`
  branch at `0x60478` reaches `0x609C6` — **CONFIRMED**. That handler starts
  with `D0=0`, builds a raw flag mask from driver RAM bit 4 values and returns
  through the shared driver epilogue at `0x611D8` — **CONFIRMED**.
- The command `0x0008` branch reaches `0x60D10`, which performs raw 68000/Z80
  bus operations and copies `0x0606` bytes from `A01000` to `FF0022` —
  **CONFIRMED**. Driver protocol and audio meaning remain outside M11.
- The command `0x0006` handler at `0x609C6` starts `D0` at zero, sets it to
  `0x01FF` when bit 0 of `FF001A` is set, then ORs bits from bit 4 of eleven
  driver RAM locations before returning through `0x611D8` — **CONFIRMED**.
  The event-side meaning of this mask remains UNKNOWN.
- On the `0x01FF` path, `0x7B2A` calls command `0x0008`, performs the raw
  state writes, then branches to `0x62CC`; `0x62CC` clears current-record
  fields `+0x4E`, `+0x52`, `+0x2A` and `+0x04` — **CONFIRMED**. This closes
  the bounded raw side-effect contract without assigning driver semantics.
- The native `event_router` module exposes the producer transfer, raw
  handler-address mapping and the bounded adjacent `0x7B2A` trace. Synthetic tests and
  the USA-ROM oracle cover the type gate, field composition and dispatch
  boundaries.

### M11 boundary
No generic event-stream parser, dialogue decoder, progression model or
unproven command is introduced. Further work must establish the caller/data
contract around the selected type-8 source or a downstream handler first.

### M11 post-completion RE-acceleration checkpoint — bounded `0x60004` slice
**Status:** VERIFIED as developer-only evidence tooling; no native gameplay
behavior is inferred or added.

- The local USA-ROM tool reads through `Rom::load`, validates the canonical
  fingerprint, and decodes only reachable direct control flow in the explicit
  half-open range `[0x60004, 0x61204)` — **VERIFIED**.
- The bytes `60 00 04 24` at `0x60004` are `BRA.W +0x0424`; the 68000
  word-displacement target is `0x6042A`. `0x6042C` is the following opcode,
  correcting the earlier ledger wording — **CONFIRMED by oracle**.
- The deterministic report contains 801 reachable instructions, 109 basic
  blocks, 72 direct branches, 17 direct calls, 3 absolute ROM references and
  114 absolute RAM references. Immediate constants are attached to decoded
  instructions — **VERIFIED by local report**.
- Direct edges include `0x60004 -> 0x6042A`, `0x60478 -> 0x609C6` and
  `0x60488 -> 0x60D10`; the report also contains explicit separate arrays for
  unresolved indirect control flow and unsupported opcodes. Neither category
  occurs on the current reachable USA slice; synthetic tests cover both —
  **VERIFIED**.
- JSON schema `oasis.m68k.re-slice.v1` and the human report are generated from
  sorted deterministic data. Debug/Release outputs have the same SHA-256 —
  **VERIFIED**.
- Decoder coverage is intentionally bounded to opcode families exercised by
  this slice. It is not a generic 68000 decoder, emulator, whole-ROM
  recompiler or production runtime dependency. Any unsupported instruction or
  indirect target must remain explicit — **CONFIRMED design boundary**.

**Open questions:** driver command meanings, audio protocol, event/progression
semantics and the producer caller remain **UNKNOWN**.

### M11.5 second RE-acceleration slice — bounded multi-function report
**Status:** VERIFIED as developer-only tooling; no production C++ behavior or
semantic names were added.
- The local-USA CLI analyzes four existing evidence targets: exact documented
  `[0x3820,0x3B3E)` and `[0xD3B2,0xD406)`, plus bounded-only windows beginning
  at `0x8E90` (`0x120` bytes) and `0xA6A4` (`0x180` bytes). Boundary discovery
  marks a return boundary only when every bounded path is complete; no boundary
  is guessed for the two windows.
- The deterministic `oasis.m68k.re-program.v1` report contains 421 decoded
  instructions in 131 basic blocks, one direct call site and one analyzed
  caller→callee edge (`0xD3B2 -> 0x3820`). Call sites retain caller function,
  basic block and instruction address; indirect/unresolved flow is separate.
- It records 18 confirmed absolute references (including `0x5CE96` and
  `FF2FA8`), 114 unresolved register-based references, one unresolved indirect
  jump at `0xA7E2`, and two unsupported opcode locations. Each memory item is
  bound to function, bounded slice, basic block and instruction.
- The USA oracle reproduces entry bytes, the reader call, pool dispatch edges
  `0x8EA6 -> 0x8F12` and `0x8EC8 -> 0x8F22`, and the raw callback jump boundary.
  Debug/Release JSON hashes match; synthetic tests cover confirmed boundaries,
  caller/callee grouping, bindings, unresolved flow and unsupported addressing.

**Limitations:** the decoder remains a bounded opcode-family decoder. It does
not resolve register-based effective addresses, indirect targets or unknown
function boundaries, and it is not an emulator, whole-ROM discovery pass or
recompiler. The `0x8E90` and `0xA6A4` reports intentionally include only their
explicit windows and must not be read as complete function recovery.

### M11.5 third checkpoint — bounded dynamic trace at `0xA7D4`
**Status:** VERIFIED as a developer-only dynamic evidence PoC; no gameplay
runtime or full emulator was added.

- The isolated scenario uses the static `0xA6A4` slice but starts from the
  controlled/savestate-like PC `0xA7D4`. It initializes raw `A6=FF2954`, the
  record word at `+0x00` to `1`, and the raw pointer at `+0x22` to `0xA7E4`.
- The bounded backend executes exactly `A7D4, A7DA, A7DE, A7E2, A7E4`:
  three static blocks, one not-taken `BEQ` at `A7DA`, two RAM reads, one
  indirect jump and one `RTS`. Relevant `A6`/`A0` snapshots are retained only
  at the unresolved memory/control-flow sites.
- Static/dynamic comparison resolves three prior items: the effective RAM
  addresses at `A7D4` (`FF2954`) and `A7DE` (`FF2976`), plus indirect target
  `A7E2 -> A7E4`. Nine other static unresolved memory references remain
  unobserved and therefore unresolved.
- The deterministic `oasis.m68k.re-trace.v1` JSON and human report retain PC,
  block, branch, call/return, memory and indirect-target evidence. Synthetic
  and USA-ROM oracles reproduce the same five PCs, branch outcome, RAM reads,
  pointer value and resolved target.

**Backend limitation:** this is a bounded scenario interpreter for the exact
five-opcode path, not a general 68000 CPU or Mega Drive emulator. It models no
full call stack, writes, interrupts, peripherals or alternate path; execution
stops explicitly on an unsupported scenario PC. Full-game tracing remains out
of scope.

### M11.5 fourth checkpoint — USA retail versus USA Beta 1994-11-01
**Status:** VERIFIED bounded correspondence only. `oasis_re_diff` reports the
five requested pairs: exact `0x3820->0x37D0`, `0x60004->0x60004`,
`0x82AE->0x825E`, `0x7A28->0x79D8`, and structural `0xA6A4->0xA654` with
changed block ordinal 10. No semantic identity or behavior is inferred.

### M11.5 fifth checkpoint — changed block ordinal 10 detail
**Status:** CONFIRMED raw bounded instruction/CFG evidence. Retail
`[0xA786,0xA792)` and beta `[0xA736,0xA742)` are 12-byte, 3-instruction
blocks. Edges correspond as `A6BA->A786`/`A66A->A736`,
`A78E->A7D4`/`A73E->A784`, and `A78E->A792`/`A73E->A742`. Only
`2F3C0000A6BE` versus `2F3C0000A66E` differs and is `relocation_only`;
`4A46` and `6B000044` (condition `0xB`) are identical. Semantics remain
unknown.

### M11.5 first bounded ROM Atlas prototype
**Status:** VERIFIED as developer-only aggregation; no semantic names or
runtime behavior were added. Schema: `oasis.m68k.re-atlas.v1`.

- The typed manifest contains 13 entries: code `0x3820`, `0x60004`, `0x7A28`,
  `0x82AE`, `0x8E90`, `0x938E`, `0x9BF2`, `0xA6A4`, `0xD3B2`; tables
  `0x5CE96`, `0x96E8`, `0x96F8`, `0xC92C`. IDs are raw address IDs.
- Exact boundaries are claimed only for `[0x3820,0x3B3E)`,
  `[0xD3B2,0xD406)` and the documented 108-entry table
  `[0x5CE96,0x5D046)`. Other entries expose bounded evidence ends without
  claiming function ownership.
- The report reuses `re_program` for 13 direct call edges, function/block-bound
  ROM/RAM refs and unresolved/unsupported/indirect categories; `re_diff` for
  beta correspondence; and `re_trace` for raw A6A4 facts `A7D4->FF2954`,
  `A7DE->FF2976`, `A7E2->A7E4`. No whole-ROM scan was added.
- USA/Beta oracle result: 13 entries, 1314 confirmed classified bytes, 6560
  bounded evidence bytes and zero conflicts. Verified native statuses are
  limited to previously tested paths at `0x3820`, `0x7A28`, `0x82AE`, `0x938E`
  and `0x9BF2`; other entries remain unimplemented or unverified.
- Atlas-driven `oasis.m68k.re-ranking.v1` ranks all 577 static unresolved
  memory refs by mode/register/family/function frequency and raw candidate
  flags. USA result: displacement 446, `0x60004` 424, `A6` 387, immediate-based
  propagation 168, dynamic-scenario candidates 2; unsupported decoder items 4.

**Unknown:** table sizes at `0x96E8`, `0x96F8`, `0xC92C`, bounded ownership,
unresolved effective addresses and routine semantics remain unknown. Atlas is
not an emulator, recompiler, whole-ROM map or gameplay runtime dependency.

## ROM identification implementation
**Status:** VERIFIED.

Detector records byte size, Mega Drive header, Sega checksum, CRC32, SHA-1, SHA-256 and classification. Synthetic tests contain no original ROM bytes.
