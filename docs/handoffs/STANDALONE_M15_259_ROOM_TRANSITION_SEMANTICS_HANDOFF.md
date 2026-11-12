# STANDALONE M15.259 ROM SEMANTIC EXTRACTION & MULTI-SCENE TRANSITION HANDOFF

**Milestone:** SEGA-THOR — M15.259
**Track:** `HYBRID_ROM_TO_CPP`
**Branch:** `codex/helper10-99d6-bounded-scout`
**Baseline Git Commit:** `5390076dabb44ef869b884cac829913b8773bb1c`
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED (`81bac48afe74269444164870cf6b9f789ff68fd1`, M16.43)
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_room_transition_semantics_m15_259.json`
**Extraction Tool:** `tools/extract_rom_room_transition_semantics.py`

---

## 1. Phase 1 — Capability and Bottleneck Audit

An inventory of the Hybrid repository's reverse-engineering, static analysis, and dynamic oracle infrastructure was conducted across all 11 required extraction domains:

| Capability Domain | Existing Tools & Modules | Automation Level | Extraction Cost | Expected Standalone Reuse |
|---|---|---|---|---|
| **1. ROM Function Boundaries** | `tools/thor_recompiler/partitioner.py`, `tools/scan_all_rom_subroutines.py`, `tools/extract_rom_room_transition_semantics.py` | **Automated** (static entry/RTS/branch scan + table target resolution) | Low (< 2s full ROM sweep) | **High** — maps all 196 helpers + 573 room/transition functions |
| **2. Control-Flow Graphs (CFG)** | `tools/thor_recompiler/cfg.py`, `cfg_reconstructor.py`, `decoder.py`, `trace_m15_247_cfg_edges.cpp` | **Automated** for direct branches/calls; **Semi-automated** (GPGX trace-assisted) for indexed `jmp (a0)` / `jsr (a1)` | Low (< 3s static; ~15s dynamic trace) | **High** — directly drives C++ control-flow structuring |
| **3. Entity State Machines** | `tools/audit_entity_dispatch_table_m15_244.py`, `generate_entity_dispatch_m15_245.py`..`m15_256.py`, `m68k_00deec_instruction_step.cpp` | **Automated** for dispatch tables (`0x00DF54`, `0x0254EA`); **Manual/Semi-automated** for per-type AI substates | Medium (~10s static + GPGX fixture capture) | **High** — directly reusable for non-Town enemy types |
| **4. Animation Tables** | `src/tools/hybrid/m68k_008fa4_instruction_step.cpp`, `tools/disas_script.py` | **Automated** (table decoding via `0x008FA4` stepper contract) | Low (< 1s) | **High** — already powers Standalone `0x008FA4` animation stepper |
| **5. Sprite & DMA Descriptors** | `tools/generate_sat_oracle_fixtures.py`, `m68k_00ad76_*`, `m68k_00a9a0_*`, `sat_pipeline_contract.cpp` | **Automated** (descriptor decoding at `0x143E76`, bank formula `(sz_byte>>4)<<16`) | Low (< 2s) | **High** — already powers Standalone pixel-exact Town SAT/DMA |
| **6. VDP Update Sequences** | `tools/generate_tilemap_upload_oracle_fixtures.py`, `m68k_002512_*`, `m68k_00d168_*`, `m68k_00d398_*` | **Automated** (plane row/column upload & LIFO DMA queue contracts) | Low (< 2s) | **High** — reusable across all 186 single/dual-plane rooms |
| **7. Map Transitions & Room Descriptors** | **NEW in M15.259:** `tools/extract_rom_room_transition_semantics.py` (extending `screen_descriptor.cpp`) | **Fully Automated** across all 21 areas, 188 room slots, 448 transitions, and 33 spawn VM opcodes | Low (< 2s for 903 KB JSON) | **Critical / Immediate** — removes Standalone's #1 multi-scene blocker |
| **8. Collision Logic** | `m68k_009896_instruction_step.cpp`, `m68k_009a08_instruction_step.cpp`, `m68k_009bf2_instruction_step.cpp`, `m68k_00c1ce_instruction_step.cpp` | **Automated** (196-helper step contracts + `0x00C1CE` tile AABB extractor) | Low (< 1s) | **High** — already shared with Standalone terrain/AABB checks |
| **9. Input Handling** | `src/tools/hybrid/gameplay_session.cpp`, `m68k_0085e2_instruction_step.cpp` | **Automated** (deterministic poll journals + joypad bitmasks) | Low (< 1s) | **High** — identical joypad mask format across tracks |
| **10. PRNG Behavior** | `src/tools/hybrid/m68k_00e268_instruction_step.cpp` | **Automated** (bit-exact 16-bit LFSR/add-rotate contract at `0x00E268`) | Low (< 1s) | **High** — already 100% converged in Standalone |
| **11. Save/Load Behavior** | `src/tools/re_m12_save_slots_promote.py`, `src/core/runtime.cpp` | **Semi-automated / Manual** (SRAM slot layout classified; runtime serialization requires manual state mapping) | Medium | **Medium** — needed once multi-area progression is active |

---

## 2. Phase 2 — Standalone Gap Analysis (`C:/Github/Sega-Thor-Standalone` @ `81bac48`)

A read-only audit of the Standalone repository at commit `81bac48` (M16.43) and recent handoffs (`STANDALONE_M15_258_HANDOFF.md`) establishes the exact boundary between proven native functionality and checkpoint-dependent / hardcoded functionality:

1. **Cold Boot and Independent Initialization (`Checkpoint-dependent`):**
   - `StandaloneGameSimulation` initializes either from a savestate-derived `CheckpointInitialState` (`canonical_beach_frame3600.state` at Frame 595) or `TownCheckpointInitialState` (Frame 775).
   - `ControlledScreen` runs an isolated 2-screen vertical slice (`0x0100`/`0x0101`) with synthetic entity records, separate from `StandaloneGameSimulation`.
   - True cold boot or arbitrary room entry (`room_id = 0xMMRR`) without a savestate checkpoint is not yet wired into `StandaloneGameSimulation`.
2. **Room and Map Transitions (`Partially implemented` — Hardcoded 2-Transition Subset):**
   - `src/game/world/town_scene_loader.hpp` hardcodes only three map constants (`kBeachMapId = 0x003D`, `kTownMapId = 0x005A`, `kTownNorthMapId = 0x005B`) and three Y thresholds (`kBeachNorthExitThresholdY = 76`, `kTownNorthExitThresholdY = 108`, `kTownSouthExitThresholdY = 500`).
   - In `StandaloneGameSimulation::step_frame` (`src/game/standalone_game_simulation.cpp:395-403`), crossing `y <= 76` on Beach calls `transition_to_town(false)`, and crossing `y <= 108` in Town sets `map_id_ = world::kTownNorthMapId` and pushes `music_stop_cmd = 2`, **without loading any Room `0x0104` descriptor, dual-plane tilemap (`0x213FE8`), collision grid, water current mechanics (`0x02E9AA`), or Plane B waterfall scroll (`0x02EAE4`)**.
   - None of the 4 Beach doorway AABB triggers (`0x02E7B8..0x02E7F4 -> 0x00C1CE` leading to `0x0108`, `0x0109`, `0x0101`) or the remaining **446 world transitions** across the 21 areas are implemented.
3. **Autonomous Gameplay Across Multiple Scenes (`Partially implemented` — Single-Scene Proven):**
   - Within Town (`0x0103`), autonomous simulation is proven across 521 frames (`F595..F1115`) for combat causality and **bit-for-bit pixel-exact (`0 / 71,680` differing pixels, `0 / 65,536` VRAM bytes)** across all 21 consecutive combat frames (`F775..F795` in M16.43).
   - However, autonomous gameplay halts as soon as the player exits Town North at `F1112` into Room `0x0104` (`0x02E972`) and Room `0x0105` (`0x02EB3E`).
4. **Enemy AI and Combat (`Partially implemented` — Town Guard/Projectile/Archway Only):**
   - `town_entity_scheduler.cpp` and `attack_controller.cpp` faithfully implement Town Guard (`0x002C` state machine `0x0254EA`), Guard Projectile (`0x002E`), and Town Archway (`0x0060`), plus Town hit sparks (`$FF19C8`, `0x00C012`).
   - Even in Town (`0x0103`), `town_scene_loader.cpp` hardcodes raw ROM offsets (`0x046512` for Archway, `0x046522` for 14 guards, `0x002AF51E` and `0x00163BDA` for pattern streams) instead of interpreting the room's `0x04371E` spawn/resource bytecode script (`0x0464EE`) via `0x00F788`/`0x00F80E`.
5. **Animation and Sprite Systems (`Partially implemented` — Town/Beach Proven):**
   - `0x008FA4` animation script stepping, `0x00A6A0..0x00B72E` multi-piece SAT emission, weapon overlay (`0x00059D9C`), water ripples (`0x00B660`), and hitstun jitter (`0x00AB1A`) are proven byte-exact and pixel-exact in Town.
6. **DMA and VDP Behavior (`Partially implemented` — Single-Plane Proven, Dual-Plane Pending):**
   - `VdpDmaQueue` LIFO DMA (`0x00AC16`), dynamic ROM bank selection (`(sz_byte >> 4) << 16`), VBlank staging (`0x002130`), and `vdp_rasterizer.cpp` are 100% pixel-exact on single-plane Town (`layout_flags = 0x0100`).
   - Dual-plane room decompression in `0x00D406` (`layout_flags = 0x0101`, used immediately in Room `0x0104` at `0x02E972` where Plane B holds the animated waterfall backdrop) is not yet wired into `StandaloneGameSimulation`.
7. **Save/Load and Game Progression (`Not implemented`):**
   - Persistent bitflag tables (`$FF0DBC` global 32 bytes, `$FF0E1C` area 16 bytes, `$FF0E2C` room 16 bytes) and SRAM save slots (`0x00129A`) are not yet connected to room transitions or spawn script conditionals (`op 0x04`, `op 0x0F..0x12`, `op 0x15`).

### Top Three Gameplay Blockers
1. `MULTI_SCENE_ROOM_AND_MAP_TRANSITION_GRAPH_ABSENCE` — Lack of table-driven `0x00C92C` room descriptor loading, `0x00D406` dual-plane decompression, and `0x00CA38`/`0x00CA24`/`0x00CA10`/`0x00C9EC` transition execution blocks gameplay at `F1112` (`0x0103 -> 0x0104 -> 0x0105`).
2. `ROOM_SPAWN_AND_RESOURCE_BYTECODE_VM_ABSENCE` — Lack of the `0x04371E` / `0x00F788` / `0x00F80E` room spawn & preload DMA bytecode interpreter forces manual per-room ROM offset hardcoding (`0x046512`, `0x046522`).
3. `CHECKPOINT_DEPENDENT_ROOM_INITIALIZATION` — `StandaloneGameSimulation` relies on savestate checkpoint structs rather than initializing room state directly from `room_id = 0xMMRR` via `0x00C8AA` + `0x00CB60` + `0x00CCAC` + `0x00D406`.

---

## 3. Phase 3 — Automated ROM Knowledge Extraction

To remove the top Standalone bottlenecks in a single reusable step, we built `tools/extract_rom_room_transition_semantics.py` (459 lines), which extracts the complete **World Room Descriptor, Spawn VM, and Multi-Scene Map Transition Subsystem** into `data/thor-evidence/rom_room_transition_semantics_m15_259.json`.

### 3.1 Extraction Summary Metrics
- **Core Subsystem Routines Analyzed:** `15` (`0x00C8AA`, `0x00C90E`, `0x00C980`, `0x00C998`, `0x00C9B2`, `0x00C1CE`, `0x00C9EC`, `0x00CA10`, `0x00CA24`, `0x00CA38`, `0x00CB60`, `0x00CCAC`, `0x00D406`, `0x00F788`, `0x00F80E`)
- **Per-Room Handler Functions Analyzed:** `558` (`186` init handlers + `186` pre-VBlank tick handlers + `186` post-VBlank handlers)
- **Total ROM Functions Analyzed:** `573`
- **World Areas Extracted (`0x00C92C` & `0x04371E`):** `21` (`0x00`..`0x14`)
- **Total Room Table Slots Extracted:** `188` (`186` unique 26-byte descriptors; `186/186` [100.0%] verified calling `0x00D406`)
- **Total State Transitions Extracted:** `448` (`448/448` [100.0%] target room IDs and player spawn expressions resolved):
  * By Ingress Mode: `330` `CA24_FADE_SPAWN`, `70` `C9EC_PARAM_FADE`, `31` `CA38_SEAMLESS_SCROLL`, `17` `CA10_VERTICAL_FALL`
  * By Trigger Family: `228` `BOUNDARY_FLAG_40_41`, `154` `AABB_00C1CE`, `28` `COORD_THRESHOLD`, `17` `VERTICAL_FALL_CA10`, `21` `EVENT_OR_SCRIPT`
- **Tables Extracted:** `25` (`1` master area descriptor pointer table `0x00C92C`, `21` per-area room offset tables, `1` master spawn script pointer table `0x04371E`, `1` shared tile bank pointer table `0x05CE96` [65 entries], `1` spawn VM opcode jump table `0x00F7D0` [33 entries])

### 3.2 Core ROM Routines & Call Graph

| Routine Name | ROM Range | Bytes | Blocks | Edges | Callees | Role |
|---|---|---|---|---|---|---|
| `resolve_and_init_room` | `0x00C8AA..0x00C90E` | 100 | 3 | 2 | `jmp (a0)` (`+0x1A` of descriptor) | Clears `$FF0E2C` (16B) on room change and `$FF0E1C` (16B) on area change; writes `$FF0E40 = d0`, `$FF16EC = a1`; jumps to room init code at `+0x1A(a1)` |
| `lookup_room_descriptor` | `0x00C90E..0x00C92C` | 30 | 1 | 0 | — | Resolves `d0 = (area<<8)\|room` via `0x00C92C` into `a0` |
| `dispatch_room_post_vblank` | `0x00C980..0x00C998` | 24 | 2 | 1 | `jmp (a1)` (`a1 + word(0(a1))`) | Called at `0x008B60` and `0x00CD20`; skipped if `$FF17B8` bit 1 is set |
| `dispatch_room_pre_vblank_tick` | `0x00C998..0x00C9B2` | 26 | 2 | 1 | `jmp (a1)` (`a1+2 + word(2(a1))`) | Called at `0x008B52`; runs room mechanics and exit triggers |
| `check_room_exit_locks` | `0x00C9B2..0x00C9EC` | 58 | 3 | 6 | — | Checks `$FF17B9`, `$FF1987`, `$FF1985`, `$FF17B8:2`, `$FF1983`, `$FF1982`; if any active, executes `addq.l #4, a7; rts` to abort caller's exit checks |
| `check_player_tile_aabb` | `0x00C1CE..0x00C222` | 84 | 3 | 5 | — | Converts `d1=(tx1<<16)\|ty1`, `d2=(tx2<<16)\|ty2` to pixel box `[tx1*8, tx2*8+7] x [ty1*8, ty2*8+7]` and tests overlap with player `($8(a0), $C(a0)) +/- $46(a0)`; sets Carry (`CCR.C=1`) on hit |
| `transition_entry_param_fade` | `0x00C9EC..0x00CA10` | 36 | 1 | 1 | `0x00CA3E` | Sets `$FF19FE = d5`, `$FF17BE = 0xFF`, `d1 = d2 = ((d3-160)<<16)\|(d4-112)`, `d3 = (d3<<16)\|d4` |
| `transition_entry_vertical_fall` | `0x00CA10..0x00CA24` | 20 | 1 | 0 | `0x00CA24` | Sets vertical fall flags `$FF17BC = 0xFF`, `$FF17BB = 0xFF`, `$FF19FE = 2` |
| `transition_entry_fade_spawn` | `0x00CA24..0x00CA38` | 20 | 1 | 0 | `0x00CA38` | Computes `d1 = d2 = ((d3-160)<<16)\|(d4-112)` and `d3 = (d3<<16)\|d4` |
| `transition_entry_seamless_scroll` | `0x00CA38..0x00CB60` | 296 | 12 | 9 | `0x0017DA`, `0x002E78`, `0x00302E`, `0x00C90E`, `0x00CE72`, `0x060004` | Clears `$FF17BE`, remaps `d0 == 0x0004 -> 0x0611`, sends sound cmd `2`, saves `d1..d3` to `$FF1782..$FF178D`, flushes DMA queue `$FF1892`, fades palette |
| `transition_reset_player_and_spirit` | `0x00CB60..0x00CCAC` | 332 | 12 | 9 | `0x008C5A`, `0x0147E0` | Clears non-persistent entities (`0x008C5A`), sets player `$8(a0)=(px<<16)`, `$C(a0)=(py<<16)`, resets velocities (or sets fall `$10(a0)=0xF0`, `$52(a0)=0`, `$56(a0)=1`, state `6` if `$FF17BC/$FF17BB` set), repositions spirit `$FF1AA4` |
| `transition_load_room_and_spawns` | `0x00CCAC..0x00CE24` | 376 | 15 | 9 | `0x002E78`, `0x00302E`, `0x008E90`, `0x009D00`, `0x00A196`, `0x00C8AA`, `0x00C980`, `0x00CDCA`, `0x00F788`, `0x060004` | Resolves `$FF17B2` from `0x04371E`, clears `$FF17B6=0`, sets `$FF17B7=10`, calls `0x00C8AA` (which runs room init + `0x00D406`), runs `0x00F788` (initial spawns), `0x00C980`, `0x008E90`, `0x00A196`, starts BGM (`$FF16F4`) |
| `load_room_resources_and_tilemaps` | `0x00D406..0x00D6D0` | 714 | 25 | 21 | `0x0037D2`, `0x003820`, `0x00D74A`, `0x00D7B0`, `0x00D7C0`, `0x00F80E` | Copies descriptor header to `$FF16F0..$FF170E`, decompresses 4 shared tile banks (`0x05CE96`) to `$FF3FA8..$FF7FA7`, DMAs `0x2000` words to VRAM `0x0000`, calls `0x00F80E` (preload `0x0A` DMAs), decompresses map stream `+4(a1)` to `$FF3FA8`, configures Plane A (`$FF1716`) and optional Plane B (`$FF173E` when `+0x17(a1) != 0`), uploads initial tilemap rows/cols |
| `step_room_spawn_script_vm` | `0x00F788..0x00F80E` | 134 | 4 | 4 | `jsr (a1)` via `0x00F7D0` | Checks `$FF17B8:1` and `$FF17B6` wait timer; executes bytecode stream at `$FF17B2` until a yield opcode (`0x00`, `0x04`, `0x08`, `0x1E`, or full enemy cap) |
| `preload_room_spawn_script_vm` | `0x00F80E..0x00F854` | 70 | 6 | 5 | `jsr (a1)` via `0x00F7D0` | Called inside `0x00D406` before map stream decompression; executes leading opcodes `0x0A`, `0x12`, `0x13`, `0x14` |

### 3.3 Exact 26-Byte Room Descriptor Binary Layout (`$FF16EC`)

Every room `room_id = (area << 8) | room` resolves via `0x00C92C` (`base = u32_be(0x00C92C + 4*area); desc = base + 2*room + s16_be(base + 2*room)`) to a 26-byte header followed immediately at `+0x1A` by M68000 initialization instructions:

| Offset | Width | Field Name | WRAM Target | Semantics |
|---|---|---|---|---|
| `+0x00` | `int16_be` | `post_vblank_offset` | — | Signed displacement from `desc + 0x00` to the room's post-VBlank handler (`0x00C980`) |
| `+0x02` | `int16_be` | `pre_vblank_offset` | — | Signed displacement from `desc + 0x02` to the room's pre-VBlank tick handler (`0x00C998`) |
| `+0x04` | `uint32_be` | `map_stream_rom_addr` | `(a4)` in `0x00D406` | ROM address of `0x003820`-compressed room layout, collision, and palette bundle |
| `+0x08` | `uint8[4]` | `tile_bank_indices` | `$FF16FA..$FF16FD` | 4 indices into `0x05CE96`; each non-zero index `b` decompresses `u32_be(0x05CE96 + 4*b)` to `$FF3FA8 + i*0x1000` (4 KB each = 16 KB total, DMA'd to VRAM `0x0000..0x3FFF`) |
| `+0x0C` | `int8[4]` | `camera_zone_heights` | `$FF16FE..$FF1705` | 4 signed bytes multiplied by `8` (`asl.w #3`) to form camera vertical zone heights and cumulative midpoints (`$FF1706..$FF1715`) |
| `+0x10` | `uint16_be` | `field_10_bgm` | `$FF16F4` | Room BGM / audio command word (compared against `$FF16F2` at `0x00CD2C` and sent to `0x060004`) |
| `+0x12` | `uint16_be` | `field_12_env` | `$FF16F6` | Room environment / secondary audio parameter |
| `+0x14` | `uint16_be` | `field_14_pal` | `$FF16F8` | Room palette / lighting parameter |
| `+0x16` | `uint16_be` | `layout_flags` | `d6` in `0x00D406` | High byte (`+0x16`): Plane A enable (`0x01`); Low byte (`+0x17`): Plane B enable (`0x01` = Dual-Plane room, `0x00` = Single-Plane room) |
| `+0x18` | `uint16_be` | `field_18_flags` | `$FF16F0` | High byte (`+0x18` bit 7): palette/fade mode; Low byte (`+0x19` bit 6): transition sound hook `0x0017DA` |
| `+0x1A` | `code` | `init_handler` | `jmp (a0)` | M68000 room init code; prepares `d1`/`d2` camera offsets if needed, calls `jsr 0x00D406`, initializes room-specific WRAM registers (`$FF177A`, `$FF177C`, etc.), ends with `rts` |

### 3.4 Decompressed Map Stream Layout (`$FF3FA8` after `0x00D54A: jsr 0x003820`)

When `0x00D406` decompresses `map_stream_rom_addr` into `$FF3FA8`:
1. **Plane A Header & Metatiles (`0x00D554..0x00D5C2`):**
   - `byte 0` (`$FF3FA8`): `width_blocks = (byte0 & 0x7F)`, `width_px = width_blocks * 32` (`$20(a0)`). Bit 7 (`byte0 & 0x80`) indicates an extra half-size collision/attribute plane.
   - `byte 1` (`$FF3FA9`): `height_blocks = int8(byte1)`, `height_px = height_blocks * 32` (`$22(a0)`).
   - `0x00D7B0` computes `row_shift = d1` from `width_px` (`512 -> shift 6` [64x64 metatile grid, `data_bytes = height_px << 6 = 8192` bytes for 64x64 words]).
   - `plane_a_metatiles` start at `$FF3FAC` (`+4`), length `data_bytes`.
   - `terrain_collision` starts at `$FF3FAC + data_bytes` (`+8196` for a 64x64 room), length `data_bytes / 2` if bit 7 of `byte 0` is set (`4096` bytes for 64x64).
2. **Optional Plane B Header & Metatiles (`0x00D5C4..0x00D644`, executed when `layout_flags & 0x00FF != 0`):**
   - Immediately follows Plane A's collision block (`a2 = a3` after Plane A).
   - Reads a 4-byte Plane B header `(width_blocks_b, height_blocks_b, pad, pad)` followed by `plane_b_metatiles` (and optional Plane B collision if bit 7 of `width_blocks_b` is set), populating `$FF173E` (`Plane B camera struct`) and `$FF1770`!
3. **Scene Palette (`0x00D648..0x00D676`):**
   - Immediately follows the last plane's data (`a3`), copying 32 words (64 bytes = 2 palette lines) to `$FF132C` / `$FF1886`!

### 3.5 Complete Area 1 Playable Multi-Scene Chain (`0x0100 .. 0x0107`)

| Room ID | Descriptor | Map Stream | Tile Banks (`0x05CE96`) | Layout (`+0x16`) | Spawn Script (`$FF17B2`) | Preload DMAs (`0x0A`) | Outgoing Transitions (`jmp_pc`: Trigger -> Target @ Spawn, Camera) |
|---|---|---|---|---|---|---|---|
| `0x0100` (South Shore) | `0x02E57E` | `0x211F7A` | `[13,14,15,16]` | `0x0100` (1-Plane) | `0x046346` | `(0x2AF51E->0x4000)` | `0x02E5E0`: `player_y <= 108` -> `0x0101` @ `(160,296)`, cam `(0,184)` (`CA38`)<br>`0x02E5F8`: `player_y >= 308` -> `0x0008` @ `(336,552)`, cam `(176,440)` (`CA38`) |
| `0x0101` (Mid Beach) | `0x02E612` | `0x212684` | `[13,14,15,16]` | `0x0100` (1-Plane) | `0x04637C` | `(0x2AF51E->0x4000)`, `(0x1706B8->0x9200)` | `0x02E6AE`: `player_y <= 108` -> `0x0102` @ `(256,488)`, cam `(96,376)` (`CA38`)<br>`0x02E6C6`: `player_y >= 308` -> `0x0100` @ `(160,120)`, cam `(0,8)` (`CA38`)<br>`0x02E6D8`: AABB `[48,112..95,119]` -> `0x0102` @ `(168,488)` (`CA24`)<br>`0x02E6EA`: AABB `[224,112..271,119]` -> `0x0102` @ `(344,488)` (`CA24`) |
| `0x0102` (North Beach) | `0x02E748` | `0x212CBA` | `[13,14,15,16]` | `0x0100` (1-Plane) | `0x046408` | `(0x2AEBBC->0x4000)` | `0x02E810`: `player_y <= 76` -> `0x0103` @ `(160,488)`, cam `(0,376)` (`CA38`)<br>`0x02E828`: `player_y >= 500` \& AABB `[232,504..279,511]` -> `0x0101` @ `(160,120)`, cam `(0,8)` (`CA38`)<br>`0x02E83A`: `anim==0x20` \& AABB `[144,240..191,247]` -> `0x0108` @ `(160,232)` (`CA24`)<br>`0x02E84C`: `anim==0x20` \& AABB `[320,240..367,247]` -> `0x0109` @ `(160,232)` (`CA24`)<br>`0x02E85E`: `anim==0x20` \& AABB `[144,504..191,511]` -> `0x0101` @ `(72,144)` (`CA24`)<br>`0x02E870`: `anim==0x20` \& AABB `[320,504..367,511]` -> `0x0101` @ `(248,144)` (`CA24`) |
| `0x0103` (Town Combat) | `0x02E88A` | `0x213850` | `[13,14,15,16]` | `0x0100` (1-Plane) | `0x0464EE` | `(0x2AF51E->0x4000)`, `(0x163BDA->0x9200)` | `0x02E940`: `player_y <= 108` -> `0x0104` @ `(160,488)`, cam `(0,376)` (`CA38`)<br>`0x02E958`: `player_y >= 500` -> `0x0102` @ `(256,88)`, cam `(95,-24)` (`CA38`) |
| `0x0104` (Town North Waterfall) | `0x02E972` | `0x213FE8` | `[13,14,15,16]` | `0x0101` (**2-Plane**) | `0x04637A` | none (`0x00` halt) | `0x02EAC6`: `player_y <= 108` -> `0x0105` @ `(256,296)`, cam `(96,184)` (`CA38`)<br>`0x02EADE`: `player_y >= 500` -> `0x0103` @ `(160,120)`, cam `(0,8)` (`CA38`) |
| `0x0105` (Waterfall Gate Arena) | `0x02EB3E` | `0x214752` | `[13,14,15,16]` | `0x0100` (1-Plane) | `0x046620` | `(0x168072->0x4000)`, `(0x2B02B4->0x6600)` | `0x02ED02`: `player_y <= 108` -> `0x0107` @ `(160,232)`, cam `(0,120)` (`CA38`)<br>`0x02ED1A`: `player_y >= 308` -> `0x0104` @ `(160,120)`, cam `(0,8)` (`CA38`) |

---

## 4. Phase 4 — Independent Validation Against Pinned GPGX Execution

The extracted room transition graph, descriptor lookup (`0x00C90E`), exit lock guard (`0x00C9B2`), AABB trigger routine (`0x00C1CE`), pre/post-VBlank room dispatchers (`0x00C998`, `0x00C980`), and room-specific tick helpers (`0x02E704`, `0x02EDCA`, `0x02EEA0`) were validated against the pinned GPGX oracle (`build/scheduler-fixture-serializerfix-20261013/genesis_plus_gx_libretro.so`, SHA256 `ca4be3049e56b1fdfba9a7c727bebbdc645b3d50203d635a468048ba81165134`) across **all 6 canonical and longrun gameplay scenarios** (representing two distinct multi-room gameplay chains):

| Scenario | Frames | Traversed Room Chain | `0x00C90E` Lookups (Expected / Observed) | `0x00C998` Pre-VBlank Ticks | `0x00C980` Post-VBlank Calls (`0x00CD20` Init) | `0x00C980` Inhibit Ticks (`$FF17B8:1`) | `0x00C1CE` AABB Checks (`0x02E796`) | `0x00C9B2` Exit Lock Checks | `0x02E704` Water Ticks (`0101` / `0102`) | `0x02EDCA` Room `0105` Calls | Fallback / Mismatch Errors |
|---|---|---|---|---|---|---|---|---|---|---|---|
| `scenario_a` | 1,592 | `0x0100 -> 0x0101 -> 0x0102` | `2 / 2` (100%) | 772 | 772 (`2` init) | 30 | 463 | 0 | `190 / 463` | 0 | **0** |
| `longrun_alt_1800f` | 1,800 | `0x0100 -> 0x0101 -> 0x0102` | `2 / 2` (100%) | 876 | 876 (`2` init) | 30 | 567 | 0 | `190 / 567` | 0 | **0** |
| `scenario_b` | 600 | `0x0102 -> 0x0103` | `1 / 1` (100%) | 299 | 298 (`0` init*) | 10 | 288 | 0 | `0 / 289` | 0 | **0** |
| `scenario_c` | 1,800 | `0x0102 -> 0x0103 -> 0x0104 -> 0x0105` | `3 / 3` (100%) | 861 | 861 (`3` init) | 20 | 288 | 552 | `0 / 289` | 820 (205 ticks) | **0** |
| `longrun_combat_1800f` | 1,800 | `0x0102 -> 0x0103 -> 0x0104 -> 0x0105` | `3 / 3` (100%) | 861 | 861 (`3` init) | 20 | 288 | 552 | `0 / 289` | 820 (205 ticks) | **0** |
| `longrun_3600f` | 3,600 | `0x0102 -> 0x0103 -> 0x0104 -> 0x0105` | `3 / 3` (100%) | 1,360 | 1,360 (`3` init) | 20 | 288 | 1,051 | `0 / 289` | 2,816 (704 ticks) | **0** |

*(\*In `scenario_b`, the `0x0102 -> 0x0103` transition begins at frame 596 during the 10-tick fade/load sequence and the 600-frame scenario ends before `0x00CD20` is reached at frame 604.)*

### Coverage and Known Limitations
- **Static Extraction Coverage:** `100.0%` of the 21 area tables (`0x00C92C`, `0x04371E`), `188/188` room descriptor slots (`186` unique descriptors, `186/186` calling `0x00D406`), `448/448` room transition edges (target room IDs and player spawn expressions), and `33/33` spawn VM opcodes (`0x00F7D0`).
- **Dynamic GPGX Validation Coverage:** `6` rooms (`0x0100`, `0x0101`, `0x0102`, `0x0103`, `0x0104`, `0x0105`) and `5` distinct transition edges (`0x0100->0x0101`, `0x0101->0x0102`, `0x0102->0x0103`, `0x0103->0x0104`, `0x0104->0x0105`) are dynamically exercised across 11,192 GPGX frames in the 6 scenarios with `100.0%` agreement (`14/14` dynamic transition lookups, `0` fallbacks).
- **Known Limitations:**
  1. Transitions in the remaining 20 areas (`182` rooms outside `0x0100..0x0105`) are statically extracted and structurally verified from ROM instructions but do not yet have dedicated GPGX savestate replay traces.
  2. `21 / 448` transition edges classified as `EVENT_OR_SCRIPT` are triggered by boss-defeat flags (`$FF0DBC`), dialogue scripts, or warp events rather than purely geometric player coordinates.
  3. `parse_spawn_script_preview` statically extracts the linear prefix of each room's `0x04371E` spawn script (up to 48 opcodes before dynamic loops/conditionals); full runtime evaluation requires implementing the 33-opcode `IRoomSpawnScriptVm` interpreter described in Section 5.

---

## 5. Phase 5 — Standalone Implementation Handoff

### 5.1 Suggested Standalone C++20 Interfaces (`src/game/world/`)

```cpp
#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace oasis::game::world {

enum class TransitionIngressMode : std::uint8_t {
    SeamlessScroll_CA38, // 0x00CA38: d1=camA, d2=camB, d3=(px<<16)|py, $FF17BE=0
    FadeSpawn_CA24,      // 0x00CA24: d3=px, d4=py, cam=(px-160, py-112), $FF17BE=0
    VerticalFall_CA10,   // 0x00CA10: sets $FF17BC=$FF17BB=0xFF, $FF19FE=2, enters CA24
    ParamFade_C9EC,      // 0x00C9EC: sets $FF19FE=d5, $FF17BE=0xFF, cam=(px-160, py-112)
};

enum class TransitionTriggerType : std::uint8_t {
    CoordThreshold,      // cmpi.w #thr, player_x/y + bcc/bls/bcs/bhi
    TileAabb_00C1CE,     // jsr 0x00C1CE: [tx1*8, tx2*8+7] x [ty1*8, ty2*8+7] vs player +/- radius
    BoundaryFlag_40_41,  // player.$40 (X edge bit 3=W, 1=E) or player.$41 (Y edge bit 3=N, 1=S)
    VerticalFall_CA10,   // pitfall / z-drop completion trigger
    EventOrScript,       // gated on $FF0DBC / $FF0E1C / $FF0E2C bitflags
};

struct RoomDescriptorHeader26 {
    std::uint16_t room_id{};              // 0xMMRR = (area << 8) | room
    std::uint32_t descriptor_rom_addr{};  // $FF16EC
    std::uint32_t init_handler_pc{};      // descriptor_rom_addr + 0x1A
    std::uint32_t pre_vblank_tick_pc{};   // descriptor_rom_addr + 2 + pre_vblank_offset
    std::uint32_t post_vblank_pc{};       // descriptor_rom_addr + post_vblank_offset
    std::uint32_t map_stream_rom_addr{};  // +0x04: 0x003820 compressed layout/collision/palette
    std::array<std::uint8_t, 4> tile_bank_indices{};      // +0x08 -> $FF16FA (indexed via 0x05CE96)
    std::array<std::int16_t, 4> camera_zone_heights_px{}; // +0x0C * 8 -> $FF16FE
    std::uint16_t bgm_cmd_10{};           // +0x10 -> $FF16F4
    std::uint16_t env_param_12{};         // +0x12 -> $FF16F6
    std::uint16_t pal_param_14{};         // +0x14 -> $FF16F8
    std::uint16_t layout_flags_16{};      // +0x16 (0x0100 = Plane A only; 0x0101 = Plane A + Plane B)
    std::uint16_t flags_18{};             // +0x18 -> $FF16F0
    std::uint32_t spawn_script_rom_addr{};// $FF17B2 resolved via 0x04371E
};

struct DecodedRoomResources {
    RoomDescriptorHeader26 header{};
    std::array<std::vector<std::uint8_t>, 4> shared_tile_banks{}; // 4x 4KB -> VRAM 0x0000..0x3FFF
    std::uint16_t plane_a_width_px{};
    std::uint16_t plane_a_height_px{};
    std::vector<std::uint16_t> plane_a_metatiles{};
    std::vector<std::uint8_t> plane_a_collision{};
    bool has_plane_b{};
    std::uint16_t plane_b_width_px{};
    std::uint16_t plane_b_height_px{};
    std::vector<std::uint16_t> plane_b_metatiles{};
    std::array<std::uint16_t, 32> scene_palette_words{};
};

class IRoomDescriptorLoader {
public:
    virtual ~IRoomDescriptorLoader() = default;
    [[nodiscard]] virtual RoomDescriptorHeader26 lookup_room(
        std::span<const std::uint8_t> rom, std::uint16_t room_id) const = 0;
    [[nodiscard]] virtual DecodedRoomResources decode_room_00d406(
        std::span<const std::uint8_t> rom, std::uint16_t room_id) const = 0;
};

} // namespace oasis::game::world
```

### 5.2 Required Game-State Fields & Initialization Dependencies
To support multi-scene transitions (`0x0100 -> 0x0101 -> 0x0102 -> 0x0103 -> 0x0104 -> 0x0105`), `StandaloneGameSimulation` requires:
1. **Persistent Bitflag Stores (`0x00C8AA`):**
   - `std::array<std::uint8_t, 32> global_flags_ff0dbc_{}` (preserved across all rooms).
   - `std::array<std::uint8_t, 16> area_flags_ff0e1c_{}` (cleared when `(new_room_id >> 8) != (active_room_id_ >> 8)` at `0x00C8D0`).
   - `std::array<std::uint8_t, 16> room_flags_ff0e2c_{}` (cleared whenever `new_room_id != active_room_id_` at `0x00C8B6`).
   - `std::uint16_t active_room_id_{}` (`$FF0E40`).
2. **Dual-Plane Camera & Tilemap State (`0x00D406`):**
   - `PlaneCameraState plane_a_cam_{}` (`$FF1716`) and `PlaneCameraState plane_b_cam_{}` (`$FF173E`).
   - In Room `0x0104` (`0x02E972`, `layout_flags = 0x0101`), `init_handler` (`0x02E98C`) initializes `plane_b_cam_.y = (plane_a_cam_.y - 0x100 < 0) ? (plane_a_cam_.y + 0x200) : (plane_a_cam_.y - 0x100)`.
3. **Spawn Script VM State (`0x00F788` / `0x00F80E`):**
   - `std::uint32_t spawn_script_pc_{}` (`$FF17B2`), `std::uint32_t spawn_return_pc_{}` (`$FF1836`), `std::uint8_t spawn_wait_timer_{}` (`$FF17B6`), `std::uint8_t max_active_enemies_{10}` (`$FF17B7`), `std::array<std::uint16_t, 8> spawn_loop_counters_{}` (`$FF1826`).

### 5.3 Exact Update Ordering (`0x008B1E..0x008B8A`)
Per 30Hz game logic cycle:
1. **Tilemap Scroll Seam Streaming (`0x008B1E -> 0x00A4C8`):** Stream newly exposed Plane A / Plane B metatile rows/columns if camera moved.
2. **Input & System State (`0x008B22 -> 0x008BAC, 0x008B8E`).**
3. **Room Spawn & Event Bytecode VM (`0x008B2A -> 0x00F788`):** If `$FF17B8` bit 1 is clear and `$FF17B6 == 0`, step `$FF17B2` opcodes until yield.
4. **Player Physics & Action Step (`0x008B2E -> 0x00557A`).**
5. **Entity Scheduler & Collisions (`0x008B32..0x008B46 -> 0x00DEEC, 0x00E0BA, 0x00E0F4, 0x008E22, 0x008E90, 0x008DD4`).**
6. **Room Pre-VBlank Tick Handler (`0x008B52 -> 0x00C998`):**
   - Runs room-specific environmental mechanics:
     * Room `0x0101`/`0x0102` (`0x02E704`): water immersion depth `$39(a0)` and drift `add.l d0, $76(a0)`.
     * Room `0x0103` (`0x02E8B4`): Town water current push (`+0x10000` for `$1B0 <= y < $1C0`, `+0x20000` for `y < $1B0` on grounded entities in water).
     * Room `0x0104` (`0x02E9AA`): Waterfall river current push (`+$60000` to `$76(a0)`) and waterfall ledge drop (`d2 < 0xD0`).
     * Room `0x0105` (`0x02EC18`): Gate/boss arena lock (`$FF17B9`) and multi-part gate entity sync (`0x02ED20`, `0x02EDCA`, `0x02EEA0`).
   - Runs `0x00C9B2` exit lock check; if unlocked, evaluates the room's outgoing transition triggers (`COORD_THRESHOLD`, `AABB_00C1CE`, `BOUNDARY_FLAG_40_41`).
   - If a transition triggers, executes synchronous room load (`0x00CA38 -> 0x00CB60 -> 0x00CCAC -> 0x00C8AA -> 0x00D406 -> 0x00F80E -> 0x00F788 -> 0x00C980`).
7. **VBlank Synchronization (`0x008B56`) & Room Post-VBlank Handler (`0x008B60 -> 0x00C980`):**
   - Runs `0x00CEEA` (camera tracking) and `0x00D398` (or `0x00D168`/`0x00D266`/`0x00D2B4`/`0x00D398` in dual-plane Room `0x0104` at `0x02EAE4`).

### 5.4 Known Edge Cases & Acceptance Criteria
- **Edge Case 1 — Room `0x0004` Remap (`0x00CA3E`):** Any transition targeting `d0 == 0x0004` is remapped in hardware at `0x00CA3E` (`cmpi.w #4, d0; bne $ca48; move.w #$611, d0`) to Room `0x0611` (`0x031976`).
- **Edge Case 2 — `0x00C9B2` Caller Stack Pop (`0x00C9E8`):** When any of the 6 lock flags in `0x00C9B2` is non-zero, `0x00C9E8` executes `addq.l #4, a7; rts`, returning directly to `0x008B56` and skipping the remainder of the room's tick handler.
- **Edge Case 3 — `0x00C1CE` Inclusive Pixel Bounds (`0x00C1D2..0x00C1D6`):** `0x00C1CE` shifts `d1` and `d2` left by 3 (`* 8`) and adds `0x00070007` to `d2`, so tile box `(tx1, ty1)..(tx2, ty2)` tests inclusive pixel bounds `[tx1*8, tx2*8 + 7] x [ty1*8, ty2*8 + 7]` against `[px - radius, px + radius] x [py - radius, py + radius]` (`radius = $46(a0)`).
- **Edge Case 4 — Dual-Plane Room `0x0104` (`0x02E972`):** Has `layout_flags = 0x0101` (both Plane A and Plane B active) and an empty spawn script (`0x04637A` starts with `0x00` halt); its post-VBlank handler `0x02EAE4` scrolls Plane B horizontally (`+8` on even `$FF1971` frames) and vertically (`-6` every tick) to animate the waterfall.
- **Acceptance Criteria for Standalone Implementation:**
  1. Load Room `0x0100`, `0x0101`, `0x0102`, `0x0103`, `0x0104`, and `0x0105` via `IRoomDescriptorLoader` using `0x00C92C` and `0x04371E` without hardcoded per-room ROM offsets.
  2. Replace hardcoded Town `0x046512`/`0x046522`/`0x002AF51E`/`0x00163BDA` constants in `town_scene_loader.cpp` with `0x04371E` script lookup (`0x0464EE`) and `0x00F80E`/`0x00F788` opcode decoding (`0x0A`, `0x0B`, `0x03`, `0x15`, `0x12`, `0x01`), preserving 100% bit-exact Town parity (`F775..F795`).
  3. Extend autonomous multi-scene replay in `StandaloneGameSimulation` past `F1112` (`0x0103 -> 0x0104 -> 0x0105`), matching the GPGX transition frames and camera/player coordinates in `scenario_c`.

---

## 6. Phase 6 — Product-Level Progress Metrics

| Product Capability | Status | Evidence & Scope |
|---|---|---|
| **Boot Independence** | `Checkpoint-dependent` | `StandaloneGameSimulation` initializes from savestate checkpoints (`canonical_beach_frame3600.state` or `TownCheckpointInitialState`); `0x00C8AA`/`0x00D406`/`0x04371E` ROM boot tables are now extracted in M15.259 for direct room boot. |
| **Playable Areas** | `Partially implemented` | Beach (`0x0102`) and Town (`0x0103`) run in `StandaloneGameSimulation`; `0x0100`/`0x0101` run in `ControlledScreen`. All `188` room descriptors across all `21` areas are now extracted in `rom_room_transition_semantics_m15_259.json`. |
| **Player Control** | `Partially implemented` | Walk, run, jump, crouch, weapon swing (State `0x36`), hurt/knockback, and terrain/water collision (`0x009BF2`) are implemented and verified across Beach and Town. Special climbing/swimming/crawl substates in later dungeons remain pending. |
| **Combat** | `Partially implemented` | Sword slash poses, weapon overlays (`0x00059D9C`), dynamic bank DMA (`0x0D0000`), hit sparks (`$FF19C8`, `0x00C012`), 3D projectile hit detection (`0x00DF60`), and damage arithmetic are implemented and tested bit-exact across `F775..F795`. |
| **Enemy AI** | `Partially implemented` | Town Guard (`0x002C` state machine `0x0254EA`, rooftop ambush drop, patrol, attack, throw) and Projectile (`0x002E`) are implemented and tested across 521 frames. Non-Town enemy families (`0x0030` gate boss, soldiers, monsters) are not yet implemented in Standalone. |
| **Transitions** | `Partially implemented` | Only `0x0102 -> 0x0103` is fully wired in `StandaloneGameSimulation`; `0x0103 -> 0x0104` only sets `map_id_` without loading Room `0x0104`. All `448` ROM transitions across `21` areas are now extracted and validated in M15.259. |
| **Graphics** | `Partially implemented` | Nemesis `0x003820` decompression, LIFO `VdpDmaQueue` (`0x00AC16`), multi-piece SAT composer (`0x00A6A0..0x00B72E`), water CRAM cycling, and `vdp_rasterizer.cpp` are **implemented and tested pixel-exact (`0 / 71,680` px diff)** across 21 consecutive Town frames (`F775..F795`). Dual-plane (`0x0101`) room setup in `0x00D406` is pending in Standalone. |
| **Audio** | `Partially implemented` | Sound command queue (`0x060004`), priority arbitration (`0x06112E`), spatial attenuation (`0x00E7E8`), and Format-A decoding are implemented at the command/queue level; full Z80/YM2612 PCM synthesis is not yet active in Standalone. |
| **Save/Load** | `Not implemented` | Persistent world/area/room bitflag arrays (`$FF0DBC`, `$FF0E1C`, `$FF0E2C`) and SRAM save slot serialization (`0x00129A`) are not yet implemented in Standalone gameplay simulation. |
| **Original-Game Parity** | `Partially implemented` | Bit-for-bit pixel-exact (`0` pixel diff, `0` VRAM diff) across `F775..F795` (21 consecutive combat frames) and verified across 521 autonomous Town frames (`F595..F1115`); multi-scene parity beyond `F1115` awaits Room `0x0104`/`0x0105` integration. |

### Recommended Top Three Highest-Impact Tasks for the Standalone Agent
1. **Implement Table-Driven `IRoomDescriptorLoader` and `0x00D406` Dual-Plane Room Loader:** Consume `data/thor-evidence/rom_room_transition_semantics_m15_259.json` (or `0x00C92C` + `0x05CE96` directly from ROM) to load any of the 188 rooms—starting with Dual-Plane Room `0x0104` (`0x02E972`) and Room `0x0105` (`0x02EB3E`) so autonomous simulation continues seamlessly past `F1112`.
2. **Implement the `0x04371E` / `0x00F788` / `0x00F80E` Room Spawn & Preload Bytecode VM:** Replace the hardcoded Town ROM offsets (`0x046512`, `0x046522`, `0x002AF51E`, `0x00163BDA`) in `town_scene_loader.cpp` with the table-driven `0x04371E` bytecode interpreter (`op 0x0A`, `0x0B`, `0x03`, `0x15`, `0x12`, `0x01`, `0x0E`), unlocking automatic pattern DMA preloading and entity spawning across all 21 areas while preserving 21-frame pixel-exact Town parity.
3. **Implement the Unified 4-Mode Room Transition Controller (`0x00CA38`, `0x00CA24`, `0x00CA10`, `0x00C9EC`) and Trigger Evaluator (`0x00C9B2`, `0x00C1CE`, `$40/$41` Edge Flags):** Wire the extracted transition edges so both `scenario_a` (`0x0100 -> 0x0101 -> 0x0102`) and `scenario_c` (`0x0102 -> 0x0103 -> 0x0104 -> 0x0105`) execute end-to-end from cold room entry without savestate checkpoints.
