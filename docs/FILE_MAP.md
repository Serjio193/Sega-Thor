# M16 Standalone Native Runtime, Frame Sequencer & Interactive Simulation V1

- `src/game/enemy/boss_charge_engine.hpp` / `src/game/enemy/boss_charge_engine.cpp` / `include/game/enemy/boss_charge_engine.hpp`: ROM Boss Charge Attack, Water-Stop Gate, Elevation-Adjusted Gravity Bounce & Directional Knockback Velocity Engine (`0x00EE94..0x00F054`, 448 bytes, covering `0x00EE94` charge phase with `0x00EFA2 -> 0x00E36E` water stop & `0x00E7E8` SFX, `0x00EF7C` gravity bounce with `0.75*vz -> +$56` / `0.5*vz -> +$10`, unsigned `0x4000` threshold & `0x00EFCE` downward skip, `0x00F032` knockback normalization, and 64KB RAM window helpers) (M16.199).
- `tests/m16_199_boss_charge_engine_test.cpp`: Automated M16.199 test qualifying timer `0`/decrement transitions, `0x00EFA2 -> 0x00E36E` water stop, `0.75*vz` vs `0.5*vz` Z integration, unsigned `0x4000` bounce threshold, `0x00EFCE` downward skip, `0x00F032` knockback vectors, 64KB RAM roundtrips/guards, and optional local-ROM byte checks (M16.199).

- `src/game/physics/player_targeting.hpp` / `src/game/physics/player_targeting.cpp` / `include/game/physics/player_targeting.hpp`: ROM Player Ali Targeting, Cardinal Motion Table Loader, Fixed-Point Aim Vector Normalization & Knockback Launch Physics (`0x00E398..0x00E502`, 362 bytes, covering `target_player_ali_quadrant_angle` X-tie-break, `target_player_ali_and_load_motion_table` Y-tie-break & `max(dist, 16)` timer clamp, `normalize_target_aim_vector_divs` with `divs.w` truncation & `d4` `half_span` return, `launch_directional_projectile_physics` `(0x0300, 0x02E0)` + `0x00060000` gated by `d7 & 0xC000`, and 64KB RAM window helpers) (M16.198).
- `tests/m16_198_player_targeting_test.cpp`: Automated M16.198 test qualifying diagonal tie-break asymmetry (`0x00E398` X vs `0x00E3E8`/`0x00E45E` Y), 32-byte motion table decoding & `max(dist, 16)` clamp, `divs.w` truncation toward zero & `INT16_MIN` `asr.w #1`, `half_span` `d4` return, `0xC000` launch suppression & 8-way `(0x0300, 0x02E0)` velocities, 64KB RAM roundtrips/guards, and coexistence with `aabb_force_fields.hpp` (M16.198).

- `src/game/physics/prng_vector_engine.hpp` / `src/game/physics/prng_vector_engine.cpp` / `include/game/physics/prng_vector_engine.hpp`: ROM 16-Bit LFSR PRNG, 8-Way Fixed-Point Vector Engine, 4-Way Cardinal Resolver & Water Surface Probe (`0x00E25C..0x00E398`, 316 bytes + `0x009C40`/`0x009C7C` water/raft probe helpers, covering `step_entity_lifecycle_timer`, `Prng16State` `$FF1928` left-shift LFSR, `project_velocity_8way` `8.8 -> 16.16` `0..3`/`4..7`, `resolve_cardinal_direction_4way`, `probe_entity_water_surface` `(attr & 0x90) == 0x80` + `0x0056` rafts, and 64KB RAM window helpers) (M16.197).
- `tests/m16_197_prng_vector_engine_test.cpp`: Automated M16.197 test qualifying `0x00E2F2` jump table targets, timer `clr.w` + `0xFFFF` wrap, exact 16-bit LFSR sequences & limit-0 non-stepping, 8.8->16.16 8-way/0.75x projections, unsigned 32-bit `INT32_MIN` cardinal tie-breakers, `(attr & 0x90) == 0x80` + type-`0x0056` raft suppression in 64KB RAM, and header coexistence with `aabb_force_fields.hpp`/`projectile_spark_lifecycle.hpp` (M16.197).

- `src/game/physics/projectile_spark_lifecycle.hpp` / `src/game/physics/projectile_spark_lifecycle.cpp` / `include/game/physics/projectile_spark_lifecycle.hpp`: ROM Spark Effect Dispatcher, Projectile Gravity Fall, Lifetime Blink & Despawn Subsystem (`0x00E0BA..0x00E25C`, 418 bytes, covering `dispatch_spark_effects` `$FF2D8C` 6x90B, `dispatch_projectile_entities` `$FF1856` pre-pause decrement + `$FF2ABC` 8x90B, `0x00E144` single-slot Group 2 dispatch, `step_projectile_gravity_landing` `< $FF2ABC` parent unlatch, `step_projectile_gravity_blink_despawn` with `0x00E1CE -> 0x00E1D2` fallthrough and `> 300` non-writeback despawn, `step_projectile_bounce_phase`, and 64KB RAM window helpers) (M16.196).
- `tests/m16_196_projectile_spark_lifecycle_test.cpp`: Automated M16.196 test qualifying constants, spark negative/null gates, gravity/parent `< $FF2ABC` unlatch, fallthrough blink/despawn at frames 240..300/>300, `$FF1856` pre-pause decrement, non-decaying bounce phase `0x63`/2, 64KB RAM roundtrips/guards, co-inclusion with `aabb_force_fields.hpp`, and optional local-ROM parity (M16.196).

- `src/game/world/group2_entity_dispatcher.hpp` / `src/game/world/group2_entity_dispatcher.cpp` / `include/game/world/group2_entity_dispatcher.hpp`: ROM Master Group 2 Entity Tick Dispatcher, Post-Tick Isometric Bounding Box Calculator & 89-Entry Action Pointer Table (`0x00DEEC..0x00E0BA`, 462 bytes, covering `$FF17B8` bit-1 pause gate, 20-slot `$FF1AA4` sweep, 89-entry `kGroup2ActionHandlerAddrs` for even types `0x16..0xC6`, dummy `0x00E0B8` `rts`, unconditional `project_isometric_footprint` with `+$17` bit-0 X/Y swap into `+$42/+$44`, `dispatch_group2_entities` table/callback overloads, and 64KB RAM window helper) (M16.195).
- `tests/m16_195_group2_entity_dispatcher_test.cpp`: Automated M16.195 test qualifying constants, even `[0x0016, 0x00C6]` table bounds, isometric 3/4 footprint math with X/Y swap, `$FF17B8` bit-1 pause gate, 20-slot dispatch including self-despawning handlers and dummy `0x00E0B8`, callback/64KB RAM roundtrips, and optional local-ROM verification of all 89 entries plus `0x00E0B8 == 0x4E75` (M16.195).

- `src/game/physics/aabb_force_fields.hpp` / `src/game/physics/aabb_force_fields.cpp` / `include/game/physics/aabb_force_fields.hpp`: ROM 3D AABB Entity Force Vector Field, Conveyor Belt & Elevation Current Displacement Subsystem (`0x00DDA2..0x00DEEC`, 330 bytes, covering `BoundingBox3D::from_registers`/`to_registers`/`contains`, `apply_box_force_vector` `0x00DDA2`, `apply_box_force_vector_with_elevation_boost` `0x00DE00` with 32-bit `pos_z_fp != 0` 2x boost, `apply_conveyor_current_force` `0x00DE6C` with `+$3A` bits 0/3 immunity, and 64KB RAM window `$FF19E8` 21-slot helpers) (M16.194).
- `tests/m16_194_aabb_force_fields_test.cpp`: Automated M16.194 test qualifying constants, packed register roundtrips, 16.16 `pos_z_fp` high-word vs 32-bit subpixel `0x00008000` boost parity, inclusive signed AABB boundaries, `+$3A` bit 0/3 immunity, 21-slot `$FF19E8` cap, 32-bit wrapping accumulation, and 64KB RAM window caller replay (M16.194).

- `src/game/render/dirty_plane_blitter.hpp` / `src/game/render/dirty_plane_blitter.cpp` / `include/game/render/dirty_plane_blitter.hpp`: ROM Dynamic Partial Tilemap Plane VRAM Dirty Block Blitter, Viewport Bounds Clipper & Toroidal Stride Streamer (`0x00DC26..0x00DDA2`, 380 bytes, covering `compute_dirty_vram_write_cmd`, `plan_dirty_plane_rect` with 42x30 inclusive clipping / bit-1 unclipped branch / `+$24` top skip / `+$0C` ring offset, `vdp_sync_dirty_plane_rect` & `_from_tilemap` toroidal 64x32 streamer, `TilemapRectSyncRequest` overload, and 64KB RAM window helpers) (M16.193).
- `tests/m16_193_dirty_plane_blitter_test.cpp`: Automated M16.193 test qualifying constants, cross-header compatibility, VDP command math, 42x30 inclusive clipping, offscreen culling, top/left skip strides, unclipped 64x32 bit-1 streaming, col-64 trailing VDP command emission, `TilemapRectSyncRequest` end-to-end integration with M16.192, 64KB RAM roundtrips, and OOB guards (M16.193).

- `src/game/render/tilemap_rect_ops.hpp` / `src/game/render/tilemap_rect_ops.cpp` / `include/game/render/tilemap_rect_ops.hpp`: ROM Tilemap Rectangle Fill & Block Copy Subsystem (`0x00DB1A..0x00DC26`, 268 bytes, covering `parse_tilemap_rect_copy_args`, `tilemap_fill_rect_byte` via `$FF16F0` bit 7, `tilemap_copy_rect_byte` `0x00DBCA` via `a3 == $FF1716`, `tilemap_copy_rect_word`, `tilemap_copy_rect_word_only_and_sync` `0x00DB68`, `tilemap_copy_rect_word_and_sync` `0x00DB64` with decoupled `DirtyRectSyncCallback` for `0x00DC26`, and 64KB RAM window helpers) (M16.192).
- `tests/m16_192_tilemap_rect_ops_test.cpp`: Automated M16.192 test qualifying constants, BE16 arg parsing, inclusive `dbra` +1 fill/copy bounds, forward overlap parity, `$FF16F0` bit 7 vs `a3 == $FF1716` selection, `DirtyRectSyncCallback` contract, OOB guards, header co-inclusion, and optional local-ROM parity (M16.192).

- `src/game/world/entity_pool_alloc.hpp` / `src/game/world/entity_pool_alloc.cpp` / `include/game/world/entity_pool_alloc.hpp`: ROM Projectile/Effect Entity Pool Allocators & Sound Driver Command Dispatchers (`0x00D9A4..0x00DB1A`, 374 bytes, covering `decompress_palette_block` via `0x0037D2`, read-only scanners `alloc_projectile_entity_slot` `8x90B` / `alloc_spark_effect_slot` `6x90B` / `find_free_general_entity_slot_18`/`_17` `18/17x188B`, `alloc_spawn_npc_entity_slot_18`/`_17` with `0x002E` + `0x008D06` + `0x003C` + `$FF1850`/`$FF184F`, sound commands `0x00DAC2..0x00DB18`, and 64KB RAM window helpers) (M16.191).
- `tests/m16_191_entity_pool_alloc_test.cpp`: Automated M16.191 test qualifying constants, `0x00D9A4 -> 0x0037D2` palette DMA + OOB guard, read-only projectile/spark/general slot scanners with `$FF1850`/`$FF184F`, NPC `0x002E` spawn + `0x008D06` constructor, all 4 sound command dispatchers, and optional local-ROM parity (M16.191).

- `src/game/render/visible_plane_streamer.hpp` / `src/game/render/visible_plane_streamer.cpp` / `include/game/render/visible_plane_streamer.hpp`: ROM Visible Tilemap Plane VRAM Streamer & Full-Screen Blank Tile Plane Clearer (`0x00D7C0..0x00D9A4`, 484 bytes, covering `compute_vram_write_cmd`, `wrap_tile_origin`, `select_visible_stream_dimensions` with `41x29` vs `64x32` and `height==32 && width<=64` bit-1 latch, `vdp_fill_plane_blank` `0x0780`, `vdp_upload_visible_tilemap_plane` toroidal + 64x32 VDP ring streaming, and 64KB RAM window helper `vdp_upload_visible_tilemap_plane_from_ram`) (M16.190).
- `tests/m16_190_visible_plane_streamer_test.cpp`: Automated M16.190 test qualifying constants, VDP write command encoding, negative/positive toroidal tile wrapping, `41x29` vs `64x32` selection + bit-1 latch, `41x29` `0x0780` blank clearer & inactive fallback, `128x64` toroidal `41x29` upload, `32x32` full `64x32` VDP ring tiling, 64KB RAM roundtrip, and OOB guards (M16.190).

- `src/game/render/dual_plane_loader.hpp` / `src/game/render/dual_plane_loader.cpp` / `include/game/render/dual_plane_loader.hpp`: ROM Dual Tilemap Plane Header Unpacker, VRAM Layout Configurator, Stride Shift Calculator & Full Plane Initializer (`0x00D542..0x00D7C0`, 638 bytes, covering `compute_log2_byte_stride` `0x00D7B0`, `unpack_dual_plane_header`, `compute_camera_scroll_limits`, `init_dual_plane_layout` with two-pass `0x003820` decompression, VDP Reg 7/12/VRAM/VSRAM init, system/scene/extra CRAM loading `0x000476`/`0x05D046`/`$FF179E`, Plane A/B stream callback `0x00D7C0`, and 64KB RAM window helper `init_dual_plane_layout_from_ram`) (M16.189).
- `tests/m16_189_dual_plane_loader_test.cpp`: Automated M16.189 test qualifying constants, `compute_log2_byte_stride` + wrap-to-zero case, 4-byte header unpacking in tile units, camera scroll limits, single/two-pass initialization with `VdpState` + stream callback, 64KB RAM roundtrip with sentinel guards around `$FFFB5C`/`$FFFF5C`, and optional local-ROM Town/Sanctuary checks (M16.189).

- `src/game/render/scene_header_dma.hpp` / `src/game/render/scene_header_dma.cpp` / `include/game/render/scene_header_dma.hpp`: ROM Scene Script Header Unpacker, Dual-Plane Camera Initializer & 16KB VRAM DMA Upload Engine (`0x00D406..0x00D542`, 316 bytes, covering `parse_scene_header`, `unpack_elevation_bands` with `0xF000` terminal sentinel, `clear_camera_structs`, `align_packed_camera_coord` / `init_aligned_camera_coords` with `0xFFF8FFF8` negative clamp, `decompress_4_bg_patterns` skipping `0`, `queue_16kb_vram_dma` `(0x007F9FD4, 0x0000, 0x2000)`, `load_scene_header_and_init_vram`, and 64KB RAM window helpers targeting `$FF16F0..$FF173E`, `$FF3FA8`, `$FF1892`, and `$FF164E`) (M16.188).
- `tests/m16_188_scene_header_dma_test.cpp`: Automated M16.188 test qualifying constants, header parsing, signed elevation LUT generation, camera struct clearing and negative-clamp alignment, 4-slot BG decompression (including pattern 0 skip and short-buffer safety), 16KB VRAM DMA queuing, 64KB RAM window roundtrip and OOB queue guard, and optional local-ROM parity (M16.188).

- `src/game/render/bg_tile_dma.hpp` / `src/game/render/bg_tile_dma.cpp` / `include/game/render/bg_tile_dma.hpp`: ROM Background Tile Pattern Decompressor & 4KB VRAM DMA Streamer (`0x00D3B2..0x00D406`, 84 bytes, covering `resolve_bg_tile_pattern_address` at `0x05CE96`, `decompress_bg_tile_pattern` via `0x003820`, `queue_bg_tile_pattern_dma`, `decompress_and_dma_bg_tiles`, and 64KB RAM window helper `decompress_and_dma_bg_tiles_from_ram` targeting `$FF2FA8`, `$FF1892`, and `$FF164E`) (M16.187).
- `tests/m16_187_bg_tile_dma_test.cpp`: Automated M16.187 test qualifying constants, `0x05CE96` pointer resolution, Format A/B `0x003820` decompression, error/OOB safety, 8-byte DMA descriptor queuing (`0x007F97D4`, `0x0800`), V-Int bit 1 sync flag handling, 64KB RAM window boundary tests, and optional local-ROM parity (M16.187).

- `src/game/render/vdp_scroll_builder.hpp` / `src/game/render/vdp_scroll_builder.cpp` / `include/game/render/vdp_scroll_builder.hpp`: ROM VDP Dual-Plane H/V Scroll Table Builder, Scanline/Strip Mode Dispatcher & V-Int DMA Sync Trigger (`0x00D2B4..0x00D3B2`, 254 bytes, covering `vdp_update_vscroll_table`, `vdp_update_hscroll_only`, `vdp_update_hscroll_table`, `vdp_commit_scroll_dma_flags`, `update_vdp_scroll_tables`, and 64KB RAM window helpers for `$FF1344..$FF134A`, `$FFFB5C`, `$FFFF5C`, `$FF16F0`, `$FF164E`) (M16.186).
- `tests/m16_186_vdp_scroll_builder_test.cpp`: Automated M16.186 test qualifying constants, scroll mode bit codecs, FullScreen Plane A/B words, 29-entry 16px tile strip H-scroll, 232-entry scanline H-scroll, 20-entry 2-cell V-scroll, VScroll-first order & dirty flag clearing, `$FF16F0 -> $FF164E` commit, and 64KB RAM roundtrips chained with M16.185 (M16.186).

- `src/game/render/scroll_integrator.hpp` / `src/game/render/scroll_integrator.cpp` / `include/game/render/scroll_integrator.hpp`: ROM Plane A 512x256 Ring-Buffer Scroll Integrator, Tile Strip Dirty Marker & Toroidal Modulo Normalizer (`0x00D146..0x00D2B4`, 366 bytes, covering `accumulate_long_and_integrate`, `accumulate_word_and_integrate`, `check_enable_and_integrate`, `integrate_scroll_deltas`, `normalize_toroidal_coords`, and 64KB RAM window helpers for `$FF1716`/`$FF173E`) (M16.185).
- `tests/m16_185_scroll_integrator_test.cpp`: Automated M16.185 test qualifying constants, horizontal/vertical scroll deltas, 512x256 VDP ring accumulators, tile-strip shift suppression, +/-10000 coarse guards, fixed-point 16.16 accumulation, toroidal normalization, and 64KB RAM window roundtrips (M16.185).

- `src/game/camera/camera_controller.hpp` / `src/game/camera/camera_controller.cpp` / `include/game/camera/camera_controller.hpp`: ROM Camera Deadzone Tracking, Boundary Clamp & Screen Shake Controller (`0x00CEEA..0x00D146`, 604 bytes, Player Ali camera tracking `0x00CEEA`, horizontal deadzone `[120, 200]` with smooth +/-3 vs fast +/-30 catch-up clamping, vertical deadzone `[104, 128]` with smooth +/-3 vs fast +30/-6 catch-up clamping, instant snap mode `0x00CF0E` bypassing smoothing clamps, scroll bias direction flag `$FF1859` and target `$FF185A` dispatch on horizontal thresholds 80/240px and post-movement direction latching, elevation compensation with warp SFX bypass on `$FF17BB != 0`, direct follow mode `0x00D0C4` on room custom flag `$FF16F1` bit 3 locking to offsets `(player_x + offset_x - 160, effective_y + offset_y - 120)`, screen shake oscillation engine `0x00D068` on timer `$FF17C4` with alternating sign offsets `+timer` / `-timer` and viewport bounds clamping, viewport tile map boundary clamping `0x00D118` within `[0..(width - 40) * 8]` and `[0..(height - 28) * 8]`, and raw 64KB RAM window integration) (M16.184).
- `tests/m16_184_camera_controller_test.cpp`: Automated M16.184 test qualifying ROM constants, deadzone containment with zero drift, horizontal smooth clamping vs fast catch-up, vertical smooth clamping vs fast catch-up with -6 floor clamp, instant snap mode override, scroll bias direction and target updates, elevation compensation and warp SFX bypass, direct follow mode with offsets, screen shake oscillation sequence with alternating +/- offsets and boundary clamping, viewport tile map clamping, raw 64KB RAM window roundtrip, and cleanroom `thor::camera` namespace aliases (M16.184).

- `src/game/world/room_presentation_jingle.hpp` / `src/game/world/room_presentation_jingle.cpp` / `include/game/world/room_presentation_jingle.hpp`: ROM Room Entry Presentation Manager, Palette Crossfader & Footstep Audio Jingle (`0x00CD64..0x00CEEA`, 390 bytes, room entry presentation mode dispatch on $FF17BF and $FF17BE [Mode 1 Whiteout Flash 0x00CDEC filling 64 CRAM words with #$0EEE, Mode > 1 Smooth 33-step RGB crossfade 0x00CDA8 towards target white palette 0x05D686, Mode 0 Standard fade-in 0x00CE22 vs Instant warp with footsteps 0x00CD7E], 4-step footstep audio jingle 0x00CE72 emitting alternating sound commands $0090038E and $0090038F with 14-frame delays, post-fade finalization 0x00CE2E with room custom flag $FF16F1 bit 1 setting $FF164F = 0xFF, warp exit chime $00800230, stack pointer recovery from $FF1930, gameplay jump $8AA6, CRAM palette line 1 buffer copy 0x00CECC, and raw 64KB RAM window integration) (M16.183).
- `tests/m16_183_room_presentation_jingle_test.cpp`: Automated M16.183 test qualifying ROM constants, whiteout flash mode filling #$0EEE, 33-step smooth crossfade progression, standard fade mode, instant footsteps mode with 4-step sound command sequence, post-fade flag $FF164F and chime triggers, CRAM palette line 1 copy, Genesis RGB color interpolation math, raw 64KB RAM window integration, and cleanroom namespace aliases (M16.183).

- `src/game/world/spirit_warp_bgm.hpp` / `src/game/world/spirit_warp_bgm.cpp` / `include/game/world/spirit_warp_bgm.hpp`: ROM Companion Spirit Warp Repositioning, Room Collision Table Loader & BGM Engine Synchronizer (`0x00CC12..0x00CD64`, 338 bytes, spirit slot $FF1AA4 warp repositioning `0x00CC12` relative to Player Ali $FF19E8 with mode clearing, Bow unsummoning `0x0016`, Dytto/Efreet state sync `0x001C`, Shade 3D coordinate projection `0x001E`, state counter resets `0x001A`, room spirit forbid check $FF16F1 bit 0, 32-area room collision table loader `0x00CCA4` from ROM 0x04371E with 10-frame cadence and VDP flag bit 5, BGM track synchronization `0x00CD2C` with continue command 3 vs track switch commands to $60004, 21-slot Group 2 entity ground surface latch iterator `0x00CD4A`, and raw 64KB RAM window integration) (M16.182).
- `tests/m16_182_spirit_warp_bgm_test.cpp`: Automated M16.182 test qualifying ROM constants, Bow unsummoning, room forbid unsummoning, Dytto/Efreet state sync, Shade 3D projection, type 0x001A counter/mode resets, generic spirit Y-offset (-8px), 32-area collision pointer resolution, BGM track continue vs change commands, 21-entity active slot sweep, raw 64KB RAM window integration, and cleanroom namespace aliases (M16.182).

- `src/game/world/room_warp_engine.hpp` / `src/game/world/room_warp_engine.cpp` / `include/game/world/room_warp_engine.hpp`: ROM Room Warp Controller, Camera Centering, Cutscene Locks & Player Ali Spawn Reset Engine (`0x00C9B2..0x00CC12`, 608 bytes, cutscene script lock check `0x00C9B2` across 6 RAM lock flags [$FF17B9, $FF1987, $FF1985, $FF17B8 bit 2, $FF1983, $FF1982], camera centering calculation `0x00C9F8..0x00CA0C` and `0x00CA24..0x00CA38` for 320x224 display centered at target minus 160/112 writing to packed registers $FF178E/$FF1792, room ID override table `0x00CA3E` remapping Room 4 to 0x0611, player Ali entity slot $FF19E8 spawn reset `0x00CB64..0x00CC12` with 16.16 coordinates, zeroed velocity and elevation, default mode/facing/idle frames, conditional sky drop pose [elevation 240, mode 6, facing 6, anim counter 1] on active screen fade transition $FF17BC/$FF17BB, and raw 64KB RAM window integration) (M16.181).
- `tests/m16_181_room_warp_engine_test.cpp`: Automated M16.181 test qualifying ROM constants, cutscene lock check across all 6 lock flags and bitmasks, camera centering calculation with 16-bit wrapping and packed storage, room ID override remapping, default player spawn state initialization, sky drop mode on active fade transitions, warp state transitions, and raw 64KB RAM window integration (M16.181).

- `src/game/map/map_scene_dispatcher.hpp` / `src/game/map/map_scene_dispatcher.cpp` / `include/game/map/map_scene_dispatcher.hpp`: ROM Map & Scene Script Dispatcher, 21-Area Script Table & State Manager (`0x00C88C..0x00C9B2`, 294 bytes, scene script loading `0x00C8AA` with area transition state flag clearing [$FF0E2C..$FF0E3C area flags on scene change, $FF0E1C..$FF0E2C map flags and $FF0DB3..$FF0DB6 trigger flags on area change, $FF0E40 ID update], 21-area script pointer table `0x00C92C` for Areas 00..14 [0..20], scene script pointer calculation `0x00C90E` with 16-bit relative signed offsets, script init entrypoint `script_ptr + 0x1A`, pause-gated tick execution `0x00C980` and subtick execution `0x00C998` checking $FF17B8 bit 1, dialogue caller entity animation hook `0x00C88C` with $FF1882 non-null check and $FF193C hook address setting, raw 64KB RAM window integration, and cleanroom `thor::script` namespace aliases) (M16.180).
- `tests/m16_180_map_scene_dispatcher_test.cpp`: Automated M16.180 test qualifying ROM constants, 21-area table address resolution, relative signed offset calculation, scene change vs area change state flag transitions, script tick and subtick target calculation (positive and negative offsets), pause state evaluation, dialogue caller entity animation hook, raw 64KB RAM window integration, and cleanroom `thor::script` namespace aliases (M16.180).

- `src/game/dialogue/dialogue_task_compositor.hpp` / `src/game/dialogue/dialogue_task_compositor.cpp` / `include/game/dialogue/dialogue_task_compositor.hpp`: ROM Dialogue Entity Sprite Compositor, Prompt Arrow Indicator & Frame Sync Engine (`0x00C756..0x00C88C`, 310 bytes, dialogue entity task callback `0x00C756`, prompt arrow indicator emission with continue tile `0x86DC` and dismiss tile `0x96DC`, animated timer blink check `$FF1657 & 0x10 != 0`, dynamic arrow vertical position based on line counter [Y=332 for >= 3, 320 for 2, 308 for 1, 296 for 0], 4-line window text grid generation `0x00C7C0..0x00C836` using plane top base `((plane_top_base >> 5) | 0x8000)` and bottom base `((plane_bottom_base >> 5) | 0x8000)`, 8 sprite pieces per line of 4x4 tiles `0x0C00` stepping X by +32px and tile by +4, Sega Genesis 80-sprite hardware limit enforcement `0x00C838`, frame synchronization & render pacing `0x00C84E` with cadence reset to 2, palette cycling, and raw 64KB RAM window integration) (M16.179).
- `tests/m16_179_dialogue_task_compositor_test.cpp`: Automated M16.179 test qualifying ROM constants, prompt arrow dynamic Y calculation across line counts, prompt arrow continue vs dismiss tile selection, timer blinking visibility, 32-piece text line grid construction with top and bottom base attributes, 80-sprite hardware clamp, frame sync cadence stepping and V-Int wait loop, raw 64KB RAM window integration, and cleanroom `thor::dialogue` namespace aliases (M16.179).

- `src/game/dialogue/dialogue_scroll_dma.hpp` / `src/game/dialogue/dialogue_scroll_dma.cpp` / `include/game/dialogue/dialogue_scroll_dma.hpp`: ROM Dialogue Window 4-Line Circular Scroll Engine, VDP Direct Clearer & Glyph DMA Streamer (`0x00C566..0x00C756`, 496 bytes, cursor X reset `$FF187E = 0`, line counter increment `++$FF1880`, 4-line circular scroll loop with row offset decrements by 2, cyclic line recycling at offset 0 resetting to 48 and queueing 512-word blank DMA to VRAM plane base `$FF1872`/`$FF1874` [or `+0x400`], rolling line detection and direct VDP plane clearer `0x00C66A` writing 4 rows of 32 words `0x0000` when `offset < 8`, font glyph 16-word DMA entry generation `0x00C6BE` from `$FF2FA8 + (c - 0x20) * 32` into target VRAM line base `+ (cursor_x << 5)` with VDP sync flag `$FF164E |= 0x02`, blank plane 512-word DMA queueing `0x00C734` from blank ROM buffer `0x007F9DD4`, and raw 64KB RAM window integration) (M16.178).
- `tests/m16_178_dialogue_scroll_dma_test.cpp`: Automated M16.178 test qualifying ROM constants, line advance without scrolling for initial 3 lines, 4-line circular scrolling with offset decrements and modulo line recycling to 48, VDP line clearer record generation, glyph DMA 16-word entry construction and cursor advancement, blank DMA 512-word command creation, and raw 64KB RAM window integration (M16.178).

- `src/game/dialogue/dialogue_vm.hpp` / `src/game/dialogue/dialogue_vm.cpp` / `include/game/dialogue/dialogue_vm.hpp`: ROM Dialogue Script Virtual Machine & Message Window Presentation Controller (`0x00C2EC..0x00C566`, 634 bytes, string table resolution `0x051514` for 98 strings `0x00..0x61` via 16-bit relative signed offsets `0x051514 + id * 2 + offset`, bytecode instruction decoder `decode_dialogue_stream` supporting printable glyphs `byte >= 0x20` and control opcodes [Terminate `0x00`, Newline `0x01`, PromptNewline `0x02`, DelayFrames `0x03`, PromptWait `0x05`, ClearPage `0x06`, PromptClearPage `0x07`], interactive button wait loop `dialogue_wait_button` `0x00C536` with prompt icon flag `$FF1886` and joypad mask `0x70` [Button A `0x40`, Button B `0x10`, Button C `0x20`], window presentation controller `dialogue_box_exec_window` `0x00C326` and `dialogue_close_window` `0x00C45C` with entity lifecycle `$FF186C` and joypad debounce, debug loop `0x00C2EC` sweep across dialogues, and raw 64KB RAM window integration) (M16.177).
- `tests/m16_177_dialogue_vm_test.cpp`: Automated M16.177 test qualifying ROM constants, string table offset resolution, bytecode stream decoding into tokens, VM lifecycle and text emission, interactive joypad button actions (mask `0x70`), prompt opcodes (PromptNewline, PromptClearPage, PromptWait), DelayFrames execution stepper, debug mode trigger evaluation, and raw 64KB RAM window integration (M16.177).

- `src/game/font/font_glyph_packer.hpp` / `src/game/font/font_glyph_packer.cpp` / `include/game/font/font_glyph_packer.hpp`: ROM Dynamic 1-Bit Font Glyph Expansion, 4-Neighbor Outline Filter & 4bpp Tile Packer (`0x00C222..0x00C2EC`, 202 bytes, 14-word 1-bit scanlines expansion from ROM font table `0x0016C1CA` with glyph offset `glyph_index * 28`, 18x18 padded canvas at `$FF3028`, rows 0..1 and rows 16..17 zero-cleared via `0x00C2C8`, 4-neighbor cardinal outline filter `0x00C248` scanning 288 pixels from `$FF303A` for background pixels touching foreground `0xFF` and writing outline `0x0E`, four 8x8 Sega Genesis 4bpp tiles packed in column-major order to `$FF2FA8` [TL, BL, TR, BR, 128 bytes total] with nibble packing `(p0 << 4) | (p1 & 0x0F)` converting `0xFF` to color `0x0F`, and raw 64KB RAM window integration) (M16.176).
- `tests/m16_176_font_glyph_packer_test.cpp`: Automated M16.176 test qualifying ROM constants, geometry dimensions, blank glyph, single isolated pixel with 4 cardinal outline neighbors and diagonal zeros, full solid box glyph outline wrapping, quadrant extraction separation (TL, BL, TR, BR), nibble packing, ROM glyph rasterization matching hardware emulation, and raw 64KB RAM window integration (M16.176).

- `src/game/collision/player_rect_collision.hpp` / `src/game/collision/player_rect_collision.cpp` / `include/game/collision/player_rect_collision.hpp`: ROM Tile Rect vs Player Ali AABB Containment & Overlap Collision Testers (`0x00C17A..0x00C222`, 168 bytes, coordinate unpacking from packed registers `d1 = (x_min << 16) | y_min` and `d2 = (x_max << 16) | y_max`, tile scale left shift by 3 `asl.l #3` and extent bias `addi.l #$00070007, d2`, Player AABB construction from `$FF19E8` using world position `+0x08`/`+0x0C` and collision radius `+0x46`, strict containment test `0x00C17A` `player_inside_rect`, intersection/overlap test `0x00C1CE` `player_overlaps_rect`, CCR Carry flag signalling, register preservation `d0-d6/a0`, and raw 64KB RAM window integration) (M16.175).
- `tests/m16_175_player_rect_collision_test.cpp`: Automated M16.175 test qualifying ROM constants, coordinate unpacking and pixel bounds derivation, full containment, partial overlap across all 4 edges, inclusive edge touch and disjoint misses, negative coordinate evaluation, packed register inputs, and raw 64KB RAM window integration (M16.175).

- `src/game/hud/decimal_formatter.hpp` / `src/game/hud/decimal_formatter.cpp` / `include/game/hud/decimal_formatter.hpp`: ROM Fixed-Width & Space-Padded Decimal String Formatters (`0x00C0BE..0x00C17A`, 188 bytes, 5-byte buffer at `$FF192A..$FF192E`, fixed 5-digit zero-padded formatter `0x00C0BE` dividing successively by 10000, 1000, 100, 10 via `divu.w` with register `d0` stack preservation and pointer rewind `lea -4(a0), a0`, space-padded formatter `0x00C114` suppressing leading zeros with ASCII space `' '` before transitioning to digit conversion path on first non-zero quotient, unconditional units digit formatting, `DecimalFormatter` class, and raw 64KB RAM window integration) (M16.174).
- `tests/m16_174_decimal_formatter_test.cpp`: Automated M16.174 test qualifying ROM constants, fixed 5-digit zero-padded formatting across edge values (0, 1, 9, 10, 42, 99, 100, 999, 1000, 9999, 10000, 65535, interspersed zeros), space-padded formatting with leading zero suppression (e.g. 0 -> "    0", 42 -> "   42"), `DecimalFormatter` class methods, and raw 64KB RAM window integration (M16.174).

- `src/game/combat/hit_spark_generator.hpp` / `src/game/combat/hit_spark_generator.cpp` / `include/game/combat/hit_spark_generator.hpp`: ROM Combat Hit Spark Particle Generator & 3D Overlap Centerpoint Calculator (`0x00C012..0x00C0BE`, 172 bytes, ingress `0x00C012` invoked on unblocked combat hits `0x00BEFE` and shield clashes `0x00BFF6`, 4-slot ring buffer allocation `$FF19C8..$FF19E8` [8B stride, slots `$FF19C8`, `$FF19D0`, `$FF19D8`, `$FF19E0`] scanning for `slot[0] == 0` and dropping particle on buffer saturation, particle slot format `+0` type 2, `+2` particle X `mid_x`, `+4` projected Y `mid_y - mid_z`, `+6` lifetime 5 frames, 3D intersection volume midpoint projection: Z midpoint `z0 = max(hitbox.z_min, tz_min), z1 = min(hitbox.z_max, tz_max), mid_z = z0 + ((z1 - z0) >> 1)`, X midpoint `x0 = max(hitbox.x_min, tx_min), x1 = min(hitbox.x_max, tx_max), mid_x = x0 + ((x1 - x0) >> 1)`, Y midpoint `y0 = max(hitbox.y_min, ty_min), y1 = min(hitbox.y_max, ty_max), mid_y = y0 + ((y1 - y0) >> 1)`, projected 2D screen coordinate subtraction `spark_x = mid_x, spark_y = mid_y - mid_z`, lifetime update stepper, `HitSparkManager`, and raw 64KB RAM window integration) (M16.173).
- `tests/m16_173_hit_spark_generator_test.cpp`: Automated M16.173 test qualifying ROM constants, symmetric 3D midpoint projection, negative coordinate arithmetic shift right calculations, ring buffer slot allocation sequence (slots 0..3 and saturation drop), particle lifetime stepping and expiry slot-freeing, `HitSparkManager` lifecycle, and raw 64KB RAM window integration (M16.173).

- `src/game/combat/combat_hit_response.hpp` / `src/game/combat/combat_hit_response.cpp` / `include/game/combat/combat_hit_response.hpp`: ROM Entity Combat Hit Response, Knockback & Trajectory Physics Calculator (`0x00BE98..0x00C012`, 2 ingress entrypoints: `combat_hit_response_tracked` `0x00BE98` checking non-player and protection bit 0 of `$3A` to set HUD timer `$FF184F = 0x5A` and target `$FF1850 = target` before falling into `combat_hit_response_direct` `0x00BEBA`, shield guard checking `$6D(target)` with facing comparison `opp_facing = attacker_facing ^ 2 == target_facing` routing to blocked hit `0x00BFAE` or guard-break reset to unblocked hit `0x00BED4`, unblocked hit applying attacker backlink `+$7E`, stun cooldowns `+$82`, damage `+$86 = damage_word`, reaction mode `+$83 = 6`, stun clear `+$9D = 0`, hit facing `+$84`, setting bit 3 on `flags_37(attacker)` with hit SFX `a3[4]`, hit spark hook `0x00C012`, 4 knockback trajectory modes [Light grounded vs airborne air-juggle bit 7, VerticalPopup, Heavy, Severe] with 8-way directional velocity projection `0x00E2D4`, blocked hit applying durability check, attacker backlink, stun cooldown, damage word, shield clash SFX `0x00900202`, and zeroed knockback, and raw 64KB RAM window integration) (M16.172).
- `tests/m16_172_combat_hit_response_test.cpp`: Automated M16.172 test qualifying ROM constants, HUD boss/enemy health tracking logic, shield guard match, flank guard break, unblocked hit attribution, stun cooldowns, reaction mode, bit 3 attacker flag, 4 knockback trajectory modes (grounded vs airborne air juggle), vertical popup, heavy, severe, 8-way directional velocity projection, blocked hit handling with shield clash SFX and zeroed velocity, and raw 64KB RAM window integration (M16.172).

- `src/game/combat/attack_scanner_engine.hpp` / `src/game/combat/attack_scanner_engine.cpp` / `include/game/combat/attack_scanner_engine.hpp`: ROM Universal Attack Frame Scanner & 3D Hitbox Intersection Engine (`0x00BC8E..0x00BE98`, 4 ingress entrypoints: `scan_all_entities` `0x00BC8E` sweeping 21 Group 2 slots `$FF19E8` with callback `0x00BEBA`, `scan_group3_a` `0x00BCA4` sweeping 17 Group 3 slots `$FF1CD8` with callback `0x00BE98`, `scan_group3_b` `0x00BCBA` sweeping 17 Group 3 slots `$FF1CD8` with `$FF198B=2`, and `scan_single` `0x00BCD8` testing slot 0 `$FF19E8`, early phase suppression testing bit 3 of `flags_37(attacker)`, 44-byte animation frame search matching frame id at `+0x32(attacker)` terminated by negative word 0xFFFF, directional hitbox selection and left-facing mirroring `facing == 3` with `x_min = -x_max, x_max = -x_min`, Ali strength rank bonus from `+0x96` stat progression [10, 20, 30, 40, 50, 100, 200 -> max +11], damage/element/stun duration packing into `a4`, 3D AABB overlap check with target eligibility filters [active `type > 0`, self-skip `target != attacker`, invulnerable skip `flags_38 & 0x08 == 0`, recovery cooldown check `cooldown_82 == 0`], callback invocation, post-loop weapon durability reduction hook `0x008530` for Ali attack types 6 and 10 upon registered hit, and raw 64KB RAM window integration) (M16.171).
- `tests/m16_171_attack_scanner_engine_test.cpp`: Automated M16.171 test qualifying ROM constants, Ali strength rank bonus progression across all 7 thresholds, damage and stun duration packing into `a4` (Ali vs enemy), early phase suppression on bit 3 of `flags_37`, 44-byte descriptor frame search, directional hitbox selection and left-facing mirroring, candidate target filters and 3D AABB overlap evaluation, callback dispatch, weapon durability hook triggering, and raw 64KB RAM window integration (M16.171).

- `src/game/combat/weapon_hitbox_streamer.hpp` / `src/game/combat/weapon_hitbox_streamer.cpp` / `include/game/combat/weapon_hitbox_streamer.hpp`: ROM Weapon Attack Frame 3D Hitbox Streamer & Dual-Pool Scanner Engine (`0x00BAAC..0x00BC8E`, 12-byte weapon hitbox stream record decoding `[z_min, z_max, x_min, y_min, x_max, y_max]` indexed by frame offset `d0 * 12`, left-facing horizontal mirroring when facing left `facing == 3` with `x_min_mirrored = -x_max` and `x_max_mirrored = -x_min`, 3D world space coordinate projection with elevation and ground height, dual-pool entrypoints: `0x00BAAC` for player slot at `$FF19E8` [1 slot, `d7 = 0`], `0x00BAB8` for enemy pool at `$FF1CD8` [17 slots, `d7 = 16`], and `0x00BBA8` for unprotected/piercing enemy scan ignoring shield protection `+0x3A & 0x01`, target eligibility filtering [active `type > 0`, self-skip `target != self`, invulnerable skip `+0x38 & 0x08 == 0`, protection check `+0x3A & 0x01 == 0`], 3D AABB overlap evaluation, Carry flag signalling [hit sets Carry, miss clears Carry], and raw 64KB RAM window integration) (M16.170).
- `tests/m16_170_weapon_hitbox_streamer_test.cpp`: Automated M16.170 test qualifying ROM constants, 12-byte stream decoding and left-facing mirroring, 3D world space coordinate projection, target eligibility filters (active, self, invulnerable, protected vs unprotected), 3D AABB overlap evaluation and axis disjoint miss filtering, dual-pool scans (player scan, enemy scan, unprotected piercing scan), and raw 64KB RAM window integration (M16.170).

- `src/game/combat/combat_overlap_engine.hpp` / `src/game/combat/combat_overlap_engine.cpp` / `include/game/combat/combat_overlap_engine.hpp`: ROM Entity 3D AABB Combat Overlap & Multi-Pool Collision Iterator Engine (`0x00B79A..0x00BAAC`, directional combat hitbox calculation `0x00B79A..0x00B854` based on facing `+0x16` [0=Down: y_min=0, 1=Left: x_max=0, 2=Up: y_max=0, 3=Right: x_min=0] with radii `+0x42`/`+0x44`, vulnerability filtering [intangible/invulnerable `+0x38` bits 2/3 == 0, protected `+0x3A` bit 0 == 0, cooldown `+0x82` == 0, self-damage prevention `+0x7E != a0`], hit application [cooldown `+0x82 += 1`, attacker pointer `+0x7E = a6`, invulnerability timer `+0x83 = 12`, damage `+0x84 = self.damage`, status `+0x86 = 0x8001`, stun clear `+0x9D = 0`, knockback velocities `vel_x = 0x0200, vel_y = 0x01C0, vel_z = 0x00020000`], Group 2 3D AABB overlap scanner `0x00B856..0x00B920` [21 slots at `$FF19E8`, stride 188B, context save `$FF194A..$FF196A`, resume iterator `0x00B912`], Group 2 3D interval scanner `0x00B922..0x00B9DC` evaluating explicit vertical Z overlap `[z_low, z_high]`, Group 4 breakables/pickups scanner `0x00B9EC..0x00BAAC` [8 slots at `$FF2ABC`, stride 90B, context save `$FF196A`], and raw 64KB RAM window integration) (M16.169).
- `tests/m16_169_combat_overlap_engine_test.cpp`: Automated M16.169 test qualifying ROM constants, directional combat hitbox geometry across 4 facing directions (Down, Left, Up, Right), vulnerability filtering (intangible, invulnerable, protected, on-cooldown, self-damage), hit response application (cooldown bump, attacker attribution, knockback velocities, status/damage), Group 2 3D AABB scanner and coroutine iterator resume, Group 2 explicit Z interval scanner, Group 4 breakables scanner, and raw 64KB RAM window integration (M16.169).

- `src/game/render/dynamic_fx_renderer_p2.hpp` / `src/game/render/dynamic_fx_renderer_p2.cpp` / `include/game/render/dynamic_fx_renderer_p2.hpp`: ROM Entity Special Dynamic FX Halo, Swirl & SAT Piece Compositor Subsystem (Part 2) (`0x00B56E..0x00B79A`, bit 2 pulsing halo emitter `0x00B56E..0x00B5CE` with counter decrement at `+0x9A` or secondary `+0x03` clearing bit 2 of `+0x39` at `0x00B5C8` on zero, 21-entry offset table `0x00B5D0` with 255 flag skips and tiles 4..6, 3D elevation Z addition; bit 1 ground energy swirl emitter `0x00B610..0x00B65E` gated on grounded elevation `elev_z == 0`, modulo-9 counter advance, shadow preset Y shift `(preset >> 2) - 6` and 9-byte sequence table `0x00B660`; universal FX piece blitter `0x00B6A6..0x00B72E` reading frame definitions `0x0003F326` with base VRAM tile `0x055E` and priority mask `base_tile & 0x8000`, viewport clipping `[0x60, 0x160]` Y and `[0x60, 0x1C0]` X, 80-sprite link limit; static raw piece blitter `0x00B730..0x00B798` with `+0x80` coordinate bias) (M16.168).
- `tests/m16_168_dynamic_fx_renderer_p2_test.cpp`: Automated M16.168 test qualifying ROM constants, halo 21-entry table and swirl 9-byte sequence table, tile attribute priority mask formula, swirl Y offset formulas (primary and secondary), frame piece fetching and blit_fx_frame emission with 80-sprite cap, static raw piece blitter with 0x80 bias, bit 2 halo countdown, pause freeze, and completion bit clear, bit 1 grounded check and modulo-9 advance, and raw 64KB RAM window integration (M16.168).

- `src/game/render/dynamic_fx_renderer_p1.hpp` / `src/game/render/dynamic_fx_renderer_p1.cpp` / `include/game/render/dynamic_fx_renderer_p1.hpp`: ROM Entity Special Dynamic FX, Aura, Wave & Attachment SAT Renderer (Part 1) (`0x00B34C..0x00B56E`, master FX dispatcher `0x00B34C..0x00B382` checking global FX suppression `$FF1927.l` and entity mode flags `+0x39`, bit 0 aura/spark emitter `0x00B3D2..0x00B45C` with frame counter at `+0x9B` wrapping at 80 and 16-entry table `0x00B42C`, bit 3 wave ring emitter `0x00B45C..0x00B4FE` with frame counter at `+0x98` selecting small 8-entry table `0x00B4C8` for shadow preset `<= 8` or large 10-entry table `0x00B4E0` for preset `> 8`, bit 4 ground ripple/attachment emitter `0x00B4FE..0x00B56E` with 8-entry table `0x00B556` clearing bit 4 of `+0x39` at `0x00B54E` upon completion, viewport clipping `[0x60, 0x160]` Y and `[0x60, 0x1C0]` X, and 80-sprite hardware limit check) (M16.167).
- `tests/m16_167_dynamic_fx_renderer_p1_test.cpp`: Automated M16.167 test qualifying ROM constants, table entries and bounds, global FX suppression and zero-flags bypass, bit 0 aura counter step and 80-frame wrap with 255 flag skip, bit 3 wave ring small/large table selection and 8/10 wrap, bit 4 ground ripple counter step and completion bit clear, viewport clipping, 80-sprite limit capping, and raw 64KB RAM window integration (M16.167).

- `src/game/render/weapon_overlay_compositor.hpp` / `src/game/render/weapon_overlay_compositor.cpp` / `include/game/render/weapon_overlay_compositor.hpp`: ROM Entity Secondary Weapon & Auxiliary Overlay SAT Compositor (`0x00B15E..0x00B34C`, mode routing `0x00B15E` testing bit 9 of mode word `+0x3E` for DMA streaming `0x00B166` vs Non-DMA static `0x00B28E`, weapon frame cache check at `+0x5C` triggering DMA on frame change, descriptor table formula `0x0003F2FA + (type * 16) - 20`, aux VRAM tile base offset `+0x5A`, 3D elevation Z projection `+0x10`, viewport clipping `[0x60, 0x160]` Y and `[0x60, 0x1C0]` X, and 80-sprite hardware limit check) (M16.166).
- `tests/m16_166_weapon_overlay_compositor_test.cpp`: Automated M16.166 test qualifying ROM constants, mode routing bit 9, descriptor table offset formula, viewport clipping boundaries, DMA path frame caching and DMA trigger, Non-DMA static mode execution, 80-sprite hardware limit capping, and raw 64KB RAM byte buffer simulation with frame cache update (M16.166).

- `src/game/render/static_fast_sat_compositor.hpp` / `src/game/render/static_fast_sat_compositor.cpp` / `include/game/render/static_fast_sat_compositor.hpp`: ROM Static Entity & Shadowless Fast SAT Sprite Compositors (`0x00AD76..0x00B15E`, visibility check on `flags_36` bit 1 early exiting to RTS `0x00B010`, dynamic tail calls `0x00B012`, skipping dynamic DMA tile streaming/quota checks, evaluating 3D elevation Z at `+0x10`, screen viewport clipping `[0x60, 0x160]` for Y and `[0x60, 0x1C0]` for X, generating dynamic shadows via table `0x00ACFE` at ground plane `y + ground_h` when shadow flag bit 2 on `+0x3A` is set, shadowless fast SAT sprite composition `0x00B022..0x00B15E` for background objects/pickups/VFX with zero stack overhead and no shadow generation, buffer base subtract `suba.l #$FF13CC, a1` at `0x00B14A`, SAT tier mode selection `btst.b #0, $36(a6)` between Static `0x00AD76` and Dynamic `0x00A9A0`, and 80-sprite hardware limit check) (M16.165).
- `tests/m16_165_static_fast_sat_compositor_test.cpp`: Automated M16.165 test qualifying ROM constants, SAT tier mode selection, visibility check, viewport clipping boundaries (center, top-left, bottom-right, off-screen top/bottom/left/right), static entity SAT composition with pieces and dynamic shadow, visibility early exit, shadowless fast SAT composition, 80-sprite hardware limit capping, and raw 64KB RAM byte buffer simulation (M16.165).

- `src/game/render/master_sat_compositor.hpp` / `src/game/render/master_sat_compositor.cpp`: ROM Master Entity SAT Sprite Compositor, Screen Clipping & Dynamic Shadow Blitter (`0x00A9A0..0x00AD76`, visibility check on `flags_36` bit 1 early exiting to `0x00ACFC`, frame cache and VRAM DMA quota evaluation `0x00A9D6` comparing `anim_frame` vs `frame_cache` with deferred frame flag `$FF1651 = 0xFF` on quota overflow, 3D world to 2D screen coordinate projection `0x00AB00` with 3D elevation `elev_z`, Mega Drive screen viewport clipping `[0x60, 0x160]` for Y and `[0x60, 0x1C0]` for X, embedded 4x4 tile count table `0x00AD56`, dynamic shadow blitter `0x00ACD0` with 4-preset table `0x00ACFE` at ground plane `y + ground_h`, 80-sprite hardware limit capping, and SAT buffer writing into `$FF13CC`) (M16.164).
- `tests/m16_164_master_sat_compositor_test.cpp`: Automated M16.164 test qualifying ROM constants, 4 shadow preset table values, 16 4x4 tile count table entries, screen viewport clipping boundaries, world-to-screen coordinate projection with 3D elevation, frame cache hit/cold/exceeded evaluation, visibility early exit, 80-sprite hardware limit capping, and raw 64KB RAM byte buffer simulation with deferred flag set (M16.164).

- `src/game/render/demo_hud_spark_generator.hpp` / `src/game/render/demo_hud_spark_generator.cpp`: ROM Attract Demo Player Script Driver, HUD Frame Render Hook & Hit Spark Generator (`0x00A738..0x00A9A0` / `0x00A7E6..0x00A9A0`, demo status `$FF1656` bit 7 suppression check, Mode 3 [3 sprites at Y=0xE4, X=[0xF0, 0x110, 0x130], DMA from `0x000AA1FD` to VRAM `$F360` size `0xC0` words when `$FF17C0 != 1`] vs Mode 2 [2 sprites at Y=0xE4, X=[0x10C, 0x124], DMA from `0x000AA1AD` to VRAM `$F360` size `0x50` words when `$FF17C0 != 0xFF`], hardware 80-sprite limit check, camera plane selection via `$FF16F0` bit 7 between Layer 1 `$FF1716` and Layer 2 `$FF173E`, spark lifetime decrement and expiry clearing, screen projection `screen = world - cam + 120`, and margin 112 bounds clipping `[0..240]` Y and `[0..336]` X) (M16.163).
- `tests/m16_163_demo_hud_spark_generator_test.cpp`: Automated M16.163 test qualifying ROM constants, demo overlay suppression on bit 7, Mode 3 3-sprite emission and conditional DMA, Mode 2 2-sprite emission and conditional DMA, 80-sprite hardware limit capping, spark lifetime expiry and slot clearing, camera-relative in-bounds projection and emission, out-of-bounds dropping, spark pool stepping, and raw 64KB RAM byte buffer simulation (M16.163).

- `src/game/render/shadow_sat_pass.hpp` / `src/game/render/shadow_sat_pass.cpp`: ROM Shadow SAT Sprite Pass & Elevation Stacking Engine (`0x00A5A2..0x00A738` / `0x00A6A0..0x00A786`, pre-composition helper `0x00A342`, 4-pass multi-pool shadow rendering: Group 1 `$FF2954` [4 slots, 90B] testing `type > 0` and shadow bit 2 on `+0x3A`, Group 2 `$FF19E8` [21 slots, 188B] building 21-bit shadow inactive mask and selection sorting active shadows in descending Y order with sentinel -1, Group 4 `$FF2ABC` [8 slots, 90B], Group 3 `$FF2D8C` [6 slots, 90B]; flags_36 bit 0 SAT routing to Custom SAT `0x00A9A0` vs Normal SAT `0x00AD76`; elevation stacking engine `ground_y = y + ground_h`, `sprite_screen_y = y + ground_h - z`, and shadow scale tiers [Full, Medium, Small, Tiny]; SAT link terminator `move.b #0, -5(a6, d7.w)` with offset-0 advance to 8) (M16.162).
- `tests/m16_162_shadow_sat_pass_test.cpp`: Automated M16.162 test qualifying ROM constants, shadow active filtering and 21-bit inactive mask construction, descending Y-depth selection sort parity, elevation stacking ground contact and 4 shadow scale tiers, multi-pool front-to-back pass ordering, and raw 64KB RAM byte buffer simulation with SAT link termination (M16.162).

- `src/game/render/pass_b_sat_dispatcher.hpp` / `src/game/render/pass_b_sat_dispatcher.cpp`: ROM Entity Pass B SAT Sprite Dispatcher & Camera Window Filter (`0x00A524..0x00A69A`, reusing cached active entity bitmask from `$FF188E.l` without slot rescanning, camera window frustum culling filter `CameraWindowFilter` marking off-screen entities into cached mask, 5-pass multi-pool render sequence: Group 1 `$FF2954` [4 slots, 90B], Special Pool `$FF19C8` [4 slots, 8B, jump table `0x00A69A`], Group 2 `$FF19E8` [21 slots, 188B, descending-Y selection sort], Group 4 `$FF2ABC` [8 slots, 90B], Group 3 `$FF2D8C` [6 slots, 90B]; flags_36 bit 0 SAT routing to Custom SAT `0x00A9A0` vs Normal SAT `0x00AD76`; foreground player override `$FF1995` to `0x00A9AA`/`0x00AD80`; SAT link terminator `move.b #0, -5(a6, d7.w)` with offset-0 advance to 8, and VDP sync mask `$FF164E |= 0x03`) (M16.161).
- `tests/m16_161_pass_b_sat_dispatcher_test.cpp`: Automated M16.161 test qualifying ROM constants, cached active bitmask reuse skipping pre-marked slots, descending Y-depth selection sort, camera window bounds filtering with margins, multi-pool front-to-back pass ordering with special pool target `0x00A69A`, and raw 64KB RAM byte buffer simulation (M16.161).

- `src/game/render/dialogue_sprite_subsystem.hpp` / `src/game/render/dialogue_sprite_subsystem.cpp`: ROM Dialogue Entity Sprite Subsystem & Special Mode Presets (`0x00A2F0..0x00A524`, primary template `0x00A438` [9 entries] and secondary template `0x00A480` [9 entries], 6-sprite dialogue border loop with horizontal offset `$FF185A`, 3-sprite speaker portrait loop with mirror base `0x158 - offset_x` gated on `$FF184F.b`, 1-sprite flashing cursor at Y=0x118, X=`offset_x + 0x68`, attr=0x85C7, size=0x0A gated on `$FF1856.w` with 80-sprite limit check, cursor frame animation and VRAM `$B8E0` DMA queue `$FF1892` [size $90 words, ROM base `frame_id * 144 + 0x000AAEEB`], static sprite reservation pass `0x00A4C8` [base 48B/6 sprites, portrait +24B/+3 sprites, cursor +8B/+1 sprite], and special entity preset evaluation `0x00A316..0x00A336`) (M16.160).
- `tests/m16_160_dialogue_sprite_subsystem_test.cpp`: Automated M16.160 test qualifying ROM constants, authentic primary/secondary 9-sprite templates, static SAT reservation byte and sprite counts, cursor VRAM DMA command calculation, 10-sprite composition with offset_x and portrait mirror positioning, 80-sprite hardware cursor suppression, special entity preset actions, and raw 64KB RAM byte buffer simulation with DMA queueing (M16.160).

- `src/game/render/master_sat_dispatcher.hpp` / `src/game/render/master_sat_dispatcher.cpp`: ROM Entity Pass A Master SAT Sprite Dispatcher (`0x00A196..0x00A2EE`, clearing composite flag `$FF1651`, demo bypass check `$FF1996` to hook `0x00A336`, 5 front-to-back render passes: Group 1 `$FF2954` [4 slots, 90B], Special Pool `$FF19C8` [4 slots, 8B, jump table `0x00A33C`], Group 2 `$FF19E8` [21 slots, 188B, Y-depth selection sort with sentinel -1], Group 4 `$FF2ABC` [8 slots, 90B], Group 3 `$FF2D8C` [6 slots, 90B]; flags_36 bit 0 SAT routing to Custom SAT `0x00A9A0` vs Normal SAT `0x00AD76`; foreground player override `$FF1995` to `0x00A9AA`/`0x00AD80`; SAT link terminator `move.b #0, -5(a6, d7.w)` with offset-0 advance to 8, and VDP sync mask `$FF164E |= 0x03`) (M16.159).
- `tests/m16_159_master_sat_dispatcher_test.cpp`: Automated M16.159 test qualifying ROM constants, Group 2 inactive bitmask calculation, Group 2 descending Y-depth selection sorting and negative Y (< 0) sentinel skipping, multi-pool front-to-back pass ordering, target resolution (Normal SAT, Custom SAT, Special Jump Table), foreground player override, demo mode bypass, and raw 64KB RAM byte buffer integration with SAT link termination (M16.159).

- `src/game/player/vertical_entity_sweeps.hpp` / `src/game/player/vertical_entity_sweeps.cpp`: ROM vertical (±Y) solid dynamic entity collision sweeps (`0x009FA0..0x00A196`, +Y sweep `0x009FA0` testing `target.y > self.y` and clamping `dy = (target.y - target.rad_y) - (self.y + self.rad_y)`, -Y sweep `0x00A09A` testing `target.y < self.y` and clamping `dy = (self.y - self.rad_y) - (target.y + target.rad_y)`, 3D AABB overlap with X bounds `x + ground_h ± rad_x` and Z bounds `z + ground_h + [0..height]`, setting dynamic hit bit 7 on `+0x41`, writing collided pointer to `+0x68`, reaction code 3 to `+0x6C`, and pushing target `vel_y += 1` / `vel_y -= 1` on flush contact `dy == 0` when push flags `+0x37` bits 5 and 6 allow) (M16.158).
- `tests/m16_158_vertical_entity_sweeps_test.cpp`: Automated M16.158 test qualifying ROM constants, intangible self bypass, +Y sweep distance clamping, flush contact push `vel_y += 1`, push guards (cannot push, cannot be pushed), X/Z disjoint miss filtering, target eligibility filtering (inactive, intangible, ignore collision, behind self), -Y sweep distance clamping and push `vel_y -= 1`, and raw 188-byte entity slot byte integration (M16.158).

- `src/game/player/horizontal_entity_sweeps.hpp` / `src/game/player/horizontal_entity_sweeps.cpp`: ROM horizontal (±X) solid dynamic entity collision sweeps (`0x009DD6..0x009F9E`, +X sweep `0x009DD6` testing `target.x > self.x` and clamping `dx = (target.x - target.rad_x) - (self.x + self.rad_x)`, -X sweep `0x009EBA` testing `target.x < self.x` and clamping `dx = (self.x - self.rad_x) - (target.x + target.rad_x)`, 3D AABB overlap with Y bounds `y + ground_h ± rad_y` and Z bounds `z + ground_h + [0..height]`, setting dynamic hit bit 7 on `+0x40`, writing collided pointer to `+0x68`, reaction code 3 to `+0x6C`, and pushing target `vel_x += 1` / `vel_x -= 1` on flush contact `dx == 0` when push flags `+0x37` bits 5 and 6 allow) (M16.157).
- `tests/m16_157_horizontal_entity_sweeps_test.cpp`: Automated M16.157 test qualifying ROM constants, intangible self bypass, +X sweep distance clamping, flush contact push `vel_x += 1`, push guards (cannot push, cannot be pushed), Y/Z disjoint miss filtering, target eligibility filtering (inactive, intangible, ignore collision, behind self), -X sweep distance clamping and push `vel_x -= 1`, and raw 188-byte entity slot byte integration (M16.157).

- `src/game/player/world_boundary_clamps.hpp` / `src/game/player/world_boundary_clamps.cpp`: ROM 4-direction world boundary clamp subsystem (`0x009D4E..0x009DD6`, right +X clamp `0x009D4E` checking `(width_tiles << 3) - 1 - (x + rad_x)`, left -X clamp `0x009D70` checking `x - rad_x` with negative test `bmi`, bottom +Y clamp `0x009D92` checking `(height_tiles << 3) - 1 - (y + rad_y)`, top -Y clamp `0x009DB4` checking `y - rad_y` with negative test `bmi`, setting boundary collision flag `0x48` [wall bit 6 | boundary bit 3] on `+0x40` or `+0x41` when step is clamped to 0, and slot stepping integration helpers) (M16.156).
- `tests/m16_156_world_boundary_clamps_test.cpp`: Automated M16.156 test qualifying ROM constants, right +X clamp (within bounds, partial clamp, exact boundary hit with flag 0x48), left -X clamp (within bounds, partial clamp, negative coordinate zero-clamp with flag 0x48), bottom +Y clamp (within bounds, partial clamp, exact boundary hit with flag 0x48), top -Y clamp (within bounds, partial clamp, negative coordinate zero-clamp with flag 0x48), and 188-byte entity slot stepping (M16.156).

- `src/game/player/bounding_box_pushable_latch.hpp` / `src/game/player/bounding_box_pushable_latch.cpp`: ROM 2D bounding tile box sampler, type 0x56 pushable block scanner & ground latch (`0x009BC2..0x009D4E`, bounding box dispatch `0x009BC2` selecting layer 1 vs 2, 2D bounding box tile loop `0x009BF2` accumulating `or.b` into `d4` and `and.b` into `d5`, point tile layer reader `0x009C40`, pushable block solid collision scanner `0x009C7C` checking 6 slots at `$FF2D8C` for type `0x0056` with 2D AABB overlap setting Carry flag, and ground surface latch `0x009D00` storing `+0x6F`/`+0x6E`, mapping surface type via LUT `0x0096E8` into `+0x70`, and probing elevation into `+0x14`) (M16.155).
- `tests/m16_155_bounding_box_pushable_latch_test.cpp`: Automated M16.155 test qualifying ROM constants, 2D bounding tile box OR/AND accumulation across spans, layer dispatch selection, point tile reading, pushable block AABB collision detection and miss filtering (wrong type, out-of-range, 6-slot limit), raw byte pool scanning, and ground surface latching into entity slot (M16.155).

- `src/game/player/slope_elevation_engine.hpp` / `src/game/player/slope_elevation_engine.cpp`: ROM ramp/bridge slope solver, fractional height interpolator (mulu/divu) & tile elevation engine (`0x0099F0..0x009BC0`, custom elevation probe `0x009AD0`, entity elevation probe `0x009AD6`, 16-byte LUT2 mapping `0x0096F8`, void/pit detection `0xFF`, flat surface lookup `0x009BB0`, fractional height slope interpolation `0x009B26..0x009B9C` using `mulu`/`divu` on height delta and sub-tile Y span, ramp search handler 1 `0x0099F0`, ramp search handler 2 `0x009A60`, and entity slot step integration) (M16.154).
- `tests/m16_154_slope_elevation_engine_test.cpp`: Automated M16.154 test qualifying ROM constants, LUT2 surface code mapping, flat surface elevation reads, void/pit detection and fallback, slope fractional interpolation across sub-tile Y positions and multi-tile ramps, upper-tile lower-surface fallback to flat, layer 1 vs layer 2 entity elevation probe selection, ramp search handlers 1 and 2 clearance tests and failure branches, and 188-byte entity slot stepping (M16.154).

- `src/game/player/point_tile_integrator.hpp` / `src/game/player/point_tile_integrator.cpp`: ROM point tile solidity, air/elevation integrator & tile attribute sampler (`0x0098EC..0x0099EE`, point tile solidity probe `0x0098EC`, elevation probe `0x009AD0`, ramp flag `0x90` mask, pit/void wall blocking, enter ground from void, slope integration with sub-tile boundary step checks, sample point tile attribute `0x0099B8`, and read point tile raw `0x0099D6`) (M16.153).
- `tests/m16_153_point_tile_integrator_test.cpp`: Automated M16.153 test qualifying ROM constants, point tile attribute masking and stride indexing, raw tile reading, ramp detection, clearance checks (`air_h + delta < 0`), void pit blocking, enter ground from void, flat pass-through, ascending/descending slope integration across sub-tile boundaries, and 188-byte entity slot integration (M16.153).

- `src/game/player/flying_raycaster.hpp` / `src/game/player/flying_raycaster.cpp`: ROM flying/projectile dual-axis (±X, ±Y) tilemap point raycaster & collision dispatcher (`0x009708..0x0098EC`, direction codes `0x11`/`0x13` on `+0x40` and `0x12`/`0x10` on `+0x41`, 32-bit subpixel accumulation, 8-pixel tile boundary distance stepping, point wall probe `0x0098EC`, wall hit bit 6, final position writes, velocity clears `+0x72`/`+0x76`, surface height sampling `+0x14`, and return dispatch `0x009382`) (M16.152).
- `tests/m16_152_flying_raycaster_test.cpp`: Automated M16.152 test qualifying ROM constants, positive/negative horizontal raycasting with wall contact and boundary distances, positive/negative vertical raycasting with wall contact, dual-axis stepping and surface height probe gating (`a3 != 0`), boundary clamp and entity sweep hooks, and 188-byte entity slot simulation (M16.152).
- `src/game/player/vertical_wall_probe.hpp` / `src/game/player/vertical_wall_probe.cpp`: ROM vertical (±Y) tilemap wall collision, row sampler & height step subsystem (`0x0094D2..0x0096E8`, checks intangible bit 0 on `+0x38`, -Y single row span sampling `0x0096BA`, +Y multi-row solidity scanner `0x009678`, translates target surface via LUT `0x0096E8`, evaluates height delta `d6 - d7`: wall collision `+0x41 |= 0x40`, incline climb disable check, flat slope bit check, ledge drop trigger `+0x41 |= 0x20` with elevation table `$FF1706` delta to `$FF196E` and `+0x76` vel_y clear, and tile row span sampling `0x0096BA`) (M16.151).
- `tests/m16_151_vertical_wall_probe_test.cpp`: Automated M16.151 test qualifying ROM constants, 16-byte terrain LUT lookups, row column span calculation, tile sampling OR/AND accumulation, multi-row solidity scanner step and wrap, intangible bypass, wall collisions (delta >= 2, target void), incline climb disable and wall bits, flat step slope and wall bits, ledge drop trigger with drop height elevation and vel_y clear, ledge drop guards (ineligible, airborne, ramp, extent limit), and 188-byte entity slot integration (M16.151).
- `src/game/player/horizontal_wall_probe.hpp` / `src/game/player/horizontal_wall_probe.cpp`: ROM horizontal tilemap wall collision, surface height step & ledge drop detector (`0x00938E..0x0094D2`, checks intangible bit 0 on `+0x38`, scans tile column solidity across entity Y radius `0x009460`, translates target surface via LUT `0x0096E8`, evaluates height delta `d6 - d7`: wall collision `+0x40 |= 0x40`, incline climb disable check, flat slope bit check, ledge drop trigger `+0x40 |= 0x20` with elevation table `$FF1706` delta to `$FF196E` and `+0x76` vel_y clear, and tile column span sampling `0x0094A2`) (M16.150).
- `tests/m16_150_horizontal_wall_probe_test.cpp`: Automated M16.150 test qualifying ROM constants, 16-byte terrain LUT lookups, column row span calculation, tile sampling OR/AND accumulation, intangible bypass, wall collisions (delta >= 2, target void), incline climb disable and wall bits, flat step slope and wall bits, ledge drop trigger with drop height elevation and vel_y clear, ledge drop guards (ineligible, airborne, ramp), and 188-byte entity slot integration (M16.150).
- `src/game/player/vertical_raycaster.hpp` / `src/game/player/vertical_raycaster.cpp`: ROM vertical (±Y) tilemap raycaster, collision sweep & entity return dispatcher (`0x0091F6..0x00938E`, direction codes `0x12`/`0x10`, 32-bit subpixel Y accumulation, 8-pixel tile row stepping, wall collision stops, surface height updates at `+0x14`, 16-byte terrain LUT lookup at `+0x70`, `+0x76` velocity clear, and `$FF193C` recovery vector dispatch) (M16.149).
- `tests/m16_149_vertical_raycaster_test.cpp`: Automated M16.149 test qualifying ROM constants, 16-byte terrain LUT lookups, tile row distance formulas for down and up, positive/negative Y movement with wall collisions, subpixel accumulation, boundary clamps, resting fallback, 188-byte entity slot integration, and entity return dispatcher (M16.149).
- `cmake/project_tests_m16_b.cmake`: CMake test suite list part B for M16 milestones (M16.90 through M16.149), maintaining all CMake files <= 500 lines (M16.149).
- `src/game/player/horizontal_raycaster.hpp` / `src/game/player/horizontal_raycaster.cpp`: ROM horizontal (±X) tilemap raycaster, surface height probe & 0x0096E8 terrain LUT (`0x009084..0x0091F6`, direction codes `0x11`/`0x13`, 32-bit subpixel accumulation, 8-pixel tile stepping, wall collision stops, surface height updates at `+0x14`, 16-byte terrain LUT lookup at `+0x70`, and `+0x72` velocity clear) (M16.148).
- `tests/m16_148_horizontal_raycaster_test.cpp`: Automated M16.148 test qualifying ROM constants, 16-byte terrain LUT lookups, tile distance formulas for right and left, positive/negative X movement with wall collisions, subpixel accumulation, boundary clamps, resting fallback, and 188-byte entity slot integration (M16.148).
- `src/game/player/entity_sim_anim_preflight.hpp` / `src/game/player/entity_sim_anim_preflight.cpp`: ROM universal entity simulation machine, animation frame stepper & terrain pre-flight (`0x008F22..0x009084`, freeze/slow-motion sub-timers and global `$FF17B8` velocity scaling, state transition keyframe advancement with loop point detection, and terrain collision pre-flight layer selection and X-velocity branching) (M16.147).
- `tests/m16_147_entity_sim_anim_preflight_test.cpp`: Automated M16.147 test qualifying ROM constants, freeze sub-timers `+0x9C`/`+0x9D`, global freeze/slow-motion scaling for enemies vs player preservation, animation state transitions and keyframe duration decoding with loop cycle triggers, and terrain collision pre-flight layer/flying/velocity branches (M16.147).
- `src/game/player/anim_palette_entity_stepper.hpp` / `src/game/player/anim_palette_entity_stepper.cpp`: ROM animated palette CRAM sequencer & 3-group entity master stepper (`0x008E22..0x008F22`, chained palette animation frame countdowns and CRAM buffer sync flag, 3-group entity stepping at `$FF2954`, `$FF19E8`, `$FF2D8C` with `$FF193C` recovery vectors, entity state reset helper, and secondary entity bit 4 activation) (M16.146).
- `tests/m16_146_anim_palette_entity_stepper_test.cpp`: Automated M16.146 test qualifying ROM constants, animated palette timer countdowns and frame advances with sentinel looping, multiple chained palette entries, entity state reset with timer and flashing clears, secondary dispatch bit 4 setting, and 3-group entity stepping with active filtering and recovery vector assignment (M16.146).
- `src/game/player/entity_constructor_lifecycle.hpp` / `src/game/player/entity_constructor_lifecycle.cpp`: ROM master entity subsystems clear, universal descriptor-driven entity constructor, script pointers re-init & HUD spirit glow pulse controller (`0x008C5A..0x008E22`, clearing 5 entity pools preserving player and spirit slots, 32-byte ROM descriptor table parsing at `0x0003F2FA`, populating 16 descriptor fields, and HUD spirit glow ramp/pulse/fade logic) (M16.145).
- `tests/m16_145_entity_constructor_lifecycle_test.cpp`: Automated M16.145 test qualifying ROM constants, 90B/188B/8B slot clearing, EntityPoolSet clear preserving player/spirit slots, descriptor table reading with ROM and table buffers, universal constructor field population for small and large slots, script pointers re-init, and spirit glow ramp/pulse/fade transitions (M16.145).
- `src/game/player/master_frame_sequencer.hpp` / `src/game/player/master_frame_sequencer.cpp`: ROM master gameplay frame sequencer, screen shake dispatcher & joypad menu hotkeys (`0x008AA6..0x008C5A`, 15-phase frame pipeline, demo active/playing checks with title return, global tick `$FF0D76 += 2`, anim subtick `$FF1971 += 1`, slow-motion gate, screen shake / palette flash dispatcher `0x008B8E`, and joypad menu hotkeys `0x008BAC` with release reset and 6-button controller shortcuts) (M16.144).
- `tests/m16_144_master_frame_sequencer_test.cpp`: Automated M16.144 test qualifying ROM constants, screen shake dispatcher bit 7 branch logic, joypad button release detection and input ring count clearing, Start and 6-button hotkey menus with state guards and input lock suppression, and master gameplay frame sequencing (M16.144).
- `src/game/player/ali_rapid_tripletap_demo.hpp` / `src/game/player/ali_rapid_tripletap_demo.cpp`: ROM Ali rapid-mash, triple-tap motion detectors & demo stream asset table (`0x008910..0x008AA6`, 13-frame Button A mash counter with `$FF16EA` clear, 25-frame 3-tap cardinal matcher per `0x0087F4` facing direction with neutral skip and mismatch abort, 9-entry demo stream offset table relative to `0x1698FE`, 0..8 demo index cycle, and 0x1FFE-word in-place delta reconstruction) (M16.143).
- `tests/m16_143_ali_rapid_tripletap_demo_test.cpp`: Automated M16.143 test qualifying ROM constants, direct rapid-mash spans and 13-frame boundary checks, rapid-mash InputHistoryRing64 integration, direct triple-tap sequences for all 4 facings with mismatch rejection, 25-frame max window limit, triple-tap ring integration with count clear, demo stream table lookup, demo index cycling and DemoPlaybackState transitions, and in-place delta prefix sum reconstruction (M16.143).
- `src/game/player/ali_chained_super_motion.hpp` / `src/game/player/ali_chained_super_motion.cpp`: ROM chained F-B-F + 360-spin super-motion recognizer (`0x008814..0x00890E`, full-depth ring scan up to `$FF16EA`, perpendicular D-pad input rejection, 3-step F-B-F recognition from `0x0087F4`, seamless ring-index reuse for Phase 2 360° spin start, CW/CCW rotation tracking via `0x008768`, and `$FF16EA` count clearing on match) (M16.142).
- `tests/m16_142_chained_super_motion_test.cpp`: Automated M16.142 test qualifying ROM constants, direct chained CW/CCW super-motion vectors for all 4 facings, perpendicular rejection in Phase 1, seamless Phase 1 -> Phase 2 frame overlap, and InputHistoryRing64 integration with frame counter clear (M16.142).
- `src/game/player/ali_command_motions.hpp` / `src/game/player/ali_command_motions.cpp`: ROM 360-spin & F-B-F command-motion recognizers (`0x0086B4..0x008813`, 64-entry input ring buffer at `$FF1668`, 30-sample backwards scan, 16-entry LUT `0x008768` mapping D-pad to cardinals ignoring diagonals, CW/CCW 4-cardinal step 360-spin recognition, and facing-dependent F-B-F recognition with perpendicular D-pad input rejection and `$FF16EA` count clearing on match) (M16.141).
- `tests/m16_141_command_motions_test.cpp`: Automated M16.141 test qualifying ROM constants, direct CW/CCW 360-spin vectors with noisy diagonals and neutral frames, direct F-B-F sequences for all 4 facings with perpendicular rejection, and InputHistoryRing64 integration with frame counter clear (M16.141).
- `src/game/player/ali_weapon_durability.hpp` / `src/game/player/ali_weapon_durability.cpp`: ROM weapon durability decrement, auto-revert to Dagger & 16-way D-pad velocity resolver (`0x008530..0x0086B2`, consumable IDs 2..12 durability step, zero-durability slot clear and Dagger revert with HUD toast `$FF1856 = 0x5A` and SFX `0x00C00269`, threshold reset guard `$FF0E3C > $FF1A70` tripping `$FF0DBB = 0xFF`, and 16-way D-pad 16.16 velocity and facing resolution via jump table `0x0085FA`) (M16.140).
- `tests/m16_140_weapon_durability_dpad_test.cpp`: Automated M16.140 test qualifying ROM constants, direct durability step vectors, full inventory state transition and slot clear revert, threshold guard logic, and all 16 D-pad input combinations (M16.140).
- `src/game/combat/combat_hit_apply.hpp` / `src/game/combat/combat_hit_apply.cpp`: ROM hit-apply, guard deflect, 4-way knockback & hit-spark (`0x00BE98..0x00C0BC`, enemy gauge latching `$FF184F = 0x5A` and `$FF1850 = target` when `target != Ali` and `!(target.flags_3a & 0x01)`, guard / shield deflection check with opposite facing `target.facing == (attacker.facing ^ 2)` preserving guard and triggering block SFX `0x00900202` on bit 3 of `attacker.flags_37` with special recoil cancel `0x008530` for Ali attacks 6 & 10 vs guard break on facing mismatch, normal damage application with hitstop and reaction stun timer 6, 4-tier knockback table with airborne tier-0 fallback to tier-2 with bit 15 set, 4-way velocity signs via `0x00E2D4`, and 4-slot 3D intersection hit-spark spawner at `$FF19C8` with clamped AABB midpoint projection `mid_y - mid_z`) (M16.139).
- `tests/m16_139_combat_hit_apply_test.cpp`: Automated M16.139 test qualifying enemy gauge latching and Ali/hidden bypass, 4-slot hit-spark pool allocation and clamped AABB projection math, guard deflection vs guard break, 4 knockback tiers and airborne fallback, and cardinal velocity direction signs (M16.139).
- `src/game/combat/combat_hit_dispatch.hpp` / `src/game/combat/combat_hit_dispatch.cpp`: Combat hitbox dispatcher & Ali damage scaling (`0x00BC8E..0x00BE96`, four entrypoints `scan_primary_pool` [21 slots, callback `0xBEBA`], `scan_fx_pool` [17 slots, callback `0xBE98`], `scan_fx_pool_marked` [17 slots, `$FF198B = 2`], `scan_ali_only` [1 slot], 44-byte combat record search, facing 3 Left X-mirroring, Ali cumulative kill count bonus up to +11, hitstop `(hit_flags & 0x0F) + 1`, packed damage word, 3D AABB overlap with hitstop check, and post-loop special recoil cancel `0x008530` for Ali attack indices 6 and 10) (M16.138).
- `tests/m16_138_combat_hit_dispatch_test.cpp`: Automated M16.138 test qualifying damage word scaling and hitstop packing, early exit on `flags_37` bit 3, frame record lookup, 4 entrypoints pool routing, target hitstop filtering, and special recoil cancel triggers (M16.138).
- `src/game/combat/weapon_hitbox_builder.hpp` / `src/game/combat/weapon_hitbox_builder.cpp`: Facing-mirrored 3D weapon hitbox builder & FX-pool scanner (`0x00B9EC..0x00BB1E`, 8-slot FX pool `$FF2ABC` with stride `0x5A`, resumable FX iterator, 12-byte local hitbox table indexed by `(facing == 3 ? 1 : facing) * 12`, Left facing X mirroring `[-dx2, -dx1]`, entity translation `(x, y + y_off, z + y_off)`, candidate pool scanning for all slots `$FF19E8` and enemy slots `$FF1CD8` [17 slots]) (M16.137).
- `tests/m16_137_weapon_hitbox_builder_test.cpp`: Automated M16.137 test qualifying ROM constants, facing 3 X-axis mirroring and elevation translation vectors, ROM table lookup at `0x0586E8`, 8-slot FX pool resumable iteration with save/resume, and enemy pool scan filtering (M16.137).
- `src/game/combat/resumable_hitbox_iter.hpp` / `src/game/combat/resumable_hitbox_iter.cpp`: Resumable 21-slot masked 3D hitbox iterator (`0x00B852..0x00B99E`, 21 entity slots at `$FF19E8` with stride `0xBC`, `find_first_3d_point_hit` with caller coordinate/elevation add, `find_next_3d_hit` resuming past previous hit, `find_first_3d_box_hit` with 3D Z-interval overlap, register save `$FF194A..$FF196A`) (M16.136).
- `tests/m16_136_resumable_hitbox_iter_test.cpp`: Automated M16.136 test qualifying ROM constants, evidence reference multi-hit matching, step-by-step resumable iteration, self-index exclusion, inactive/negative state skip, relative caller offsets/elevation, and 3D box Z-interval overlap (M16.136).
- `src/game/player/ali_jump_pickup_probe.hpp` / `src/game/player/ali_jump_pickup_probe.cpp`: Ali jump trigger, pickup probe & 4-dir interaction box (`0x00822C..0x008350`, running jump state `22` with preserved XY vel vs standing jump state `16` with zeroed XY vel, `vz = 0x00070000`, ±6px pickup entity `0x08` probe to `$FF1976`, 4-dir interaction bounding boxes for Up/Right/Down/Left) (M16.135).
- `tests/m16_135_ali_jump_pickup_probe_test.cpp`: Automated M16.135 test qualifying ROM constants, 4-dir interaction box bounds and point containment, cutscene lock guard, running vs standing jump state transitions and velocity behavior, and ±6px pickup probe collection and entity despawn (M16.135).
- `src/game/player/ali_input_dispatch.hpp` / `src/game/player/ali_input_dispatch.cpp`: Ali 5-frame input-buffer action & spirit-ball dispatcher (`0x007D8A..0x007E9C`, 64-frame input ring buffer `$FF1668` wrap mask `0x7E`, input lock `$FF1983`, idle flag `$FF1986 = 0xFF`, priority Button C [summon state 20, SFX `0x00900359`, guards for SP, spirit ID, sub_action 4, `$FF1985`] -> Button A [buffered or held attack] -> Button B [jump]) (M16.134).
- `tests/m16_134_ali_input_dispatch_test.cpp`: Automated M16.134 test qualifying ROM constants, input ring sampling & wrap-around, priority ordering (C > A > B), summon guards (SP, active spirit, cutscene lock), attack buffered vs held, and idle tracking flag (M16.134).
- `src/game/combat/kill_counter_vm_tick.hpp` / `src/game/combat/kill_counter_vm_tick.cpp`: Pickup overflow reclaimer, kill-counter & event VM wait-tick (`0x00F64C..0x00F7CE`, 8 pickup slots at `$FF2ABC` with stride 90B and VRAM `0x0694 + 9*i`, pass 1 offscreen non-bit7, pass 2 on-screen non-bit7, fallback bit 5 non-discardable check with `jmp $206` or suppression, bonus drop flag `$FF0DBB` -> `0x1101`, Ali attacker saturating kill counter `$FF1A7C`, and event VM pause/wait-timer decrement) (M16.133).
- `tests/m16_133_kill_counter_vm_tick_test.cpp`: Automated M16.133 test qualifying ROM constants, pickup slot reclamation passes 1 & 2, non-discardable key item overflow vs food drop suppression, Ali saturating kill counter increment, and event VM pause guard & wait-timer tick (M16.133).
- `src/game/combat/enemy_loot_drop.hpp` / `src/game/combat/enemy_loot_drop.cpp`: Enemy loot & 2-tier drop-table roller (`0x00F476..0x00F53A`, fixed drop override `+0x8C` [sentinel `0xFFFF` drops nothing], `no_drop_flag` `+0x6F` bit 7, 2-tier `[prob1, item1, prob2, item2]` roll, pickup entity `0x0008` with `vz = 0x00080000`, `z + 1.0 px`, timer `prng & 0x3F`, item attribute table `0x0005CE16` bit 7 sub-action 2) (M16.132).
- `tests/m16_132_enemy_loot_drop_test.cpp`: Automated M16.132 test qualifying ROM constants, fixed drop override, 2-tier table probabilities, no-drop suppression flag, item attribute table bit 7, and pickup entity serialization (M16.132).
- `src/game/input/mash_tripletap_detectors.hpp` / `src/game/input/mash_tripletap_detectors.cpp`: Rapid-mash & triple-tap input ring detectors (`0x008910..0x0089A8`, rapid-mash bit 4 >= 3 in 13 frames, triple-tap 3 matching cardinal D-pad frames in <= 25 frames ignoring neutral 0 and aborting on mismatch, clearing `$FF16EA` on success) (M16.131).
- `tests/m16_131_mash_tripletap_test.cpp`: Automated M16.131 test qualifying ROM constants, facing direction mapping, rapid-mash span/ring detection, triple-tap bytes/words/ring detection, and ring count clearing (M16.131).
- `src/game/combat/bomb_detonation.hpp` / `src/game/combat/bomb_detonation.cpp`: Bomb detonation blast spawner & screen-shake (`0x00ED04..0x00EDA8`, `$FF198F = 0xFF`, clear Ali lock `+0x06 = 0`, blast entity `0x003A`, Z -4.0 clamp >= 0, palette `Ali +0x5A` masked `0x97FF`, damage `0x8007`, hit-type `2`, flags `0x36 |= 8`, `0x38 |= 8`, `0x3C |= 2`, state `8`, dual SFX `0x00B00225`/`0x00600382`, screen shake `$FF17C4 = 4`) (M16.130).
- `tests/m16_130_bomb_detonation_test.cpp`: Automated M16.130 test qualifying bomb detonation flags, blast hazard entity conversion, coordinate and palette inheritance, damage word `0x8007`, dual SFX trigger, and 4-frame screen-shake (M16.130).
- `src/game/combat/projectile_hit_knockback.hpp` / `src/game/combat/projectile_hit_knockback.cpp`: Projectile hit knockback, enemy gauge timer & gravity (`0x00EB40..0x00EC58`, 10f hitstun, 0x0400/0x03C0 normal or 0x0200/0x01E0 light knockback, +2.0 Z velocity, airborne bit 15 on +0x86, 90f Ali enemy gauge, 8f latch attach, 0.375 px/f^2 projectile gravity, ground drop/cull) (M16.129).
- `tests/m16_129_projectile_hit_knockback_test.cpp`: Automated M16.129 test qualifying Z gravity step and ground clamp, normal/light target knockback, airborne bit 15, Ali 90f enemy gauge timer, 8f latch attachment, and 10px offscreen culling (M16.129).
- `src/game/combat/projectile_latch_burst.hpp` / `src/game/combat/projectile_latch_burst.cpp`: Death explosion burst, carried latch follow & projectile probe (`0x00EA04..0x00EB40`, jitter `(prng & 0x1F) - 16` + dual SFX `0x0090021B`/`0x00600382`, latch XYZ parent binding, wall bounce SFX `0x00700378`, 6px probe, target mailbox `+0x7E`) (M16.128).
- `tests/m16_128_projectile_latch_burst_test.cpp`: Automated M16.128 test qualifying burst timer countdown and jitter math, carried latch timer states and parent binding, 6px probe box, and wall bounce/shield/intangible/hitstun probe outcomes (M16.128).
- `src/game/combat/explosion_spawner.hpp` / `src/game/combat/explosion_spawner.cpp`: Explosion spawner, 7-frame SFX cooldown & Shade phase sync (`0x00E90E..0x00EA02`, entity type `0x0034`, sprite `0x055E`/`0x855E`, 7-frame SFX cooldown with `0x0090021B` + `0x00600382`, and Shade `0x1E` phase flag sync on `0x36`/`0x38`) (M16.127).
- `tests/m16_127_explosion_spawner_test.cpp`: Automated M16.127 test qualifying SFX cooldown 7f window and 16-bit wrap/negative, Shade spirit flags 0x36/0x38, normal & hi explosion dispatch, and 0xBC entity slot byte writing (M16.127).
- `src/game/audio/spatial_audio.hpp` / `src/game/audio/spatial_audio.cpp`: Spatial 2D audio attenuation & stereo panning (`0x00E78E..0x00E90C`, Manhattan/aspect distance metric `max(|dx|, |dy| + (|dy| >> 2))`, 3-tier bank-2 attenuation/cull & stereo pan, high-res attenuation/cull & stereo pan, low-sound pass-through) (M16.126).
- `tests/m16_126_spatial_audio_test.cpp`: Automated M16.126 test qualifying low sound pass-through, FM/PSG bank 2 distance attenuation & stereo panning, and high-res distance attenuation & panning (M16.126).
- `src/game/math/isqrt_atan2.hpp` / `src/game/math/isqrt_atan2.cpp`: Integer square root (Heron's method, `0x0400` cap, `0x00E6BA..0x00E6EE`) and 256-step `atan2` angle LUT (`0x00E6F0..0x00E78C`, 9-word tangent LUT `0x0005D906`, `0`=Right, `64`=Up, `128`=Left, `192`=Down) (M16.125).
- `tests/m16_125_isqrt_atan2_test.cpp`: Automated M16.125 test qualifying `isqrt_u32`, 8-step `atan_q1`, 256-step `atan2_256` quadrants/octants, and `atan2_to_point` / `atan2_to_player` (M16.125).
- `src/game/player/ali_attack_dispatch.hpp` / `src/game/player/ali_attack_dispatch.cpp`: Ali attack action & weapon-class state dispatcher (`0x008002..0x00814C`: ground & airborne attacks for dagger `0x0A`, sword `0x0C`, bow `0x0E`, bomb `0x10`, 21f air-kick stun, bow SFX `0x00600377`, input ring clear) (M16.124).
- `tests/m16_124_ali_attack_dispatch_test.cpp`: Automated M16.124 test qualifying ground/airborne attack dispatch, sword thrust/spin, bow normal/hyper fire & SFX, bomb throw, active projectile guard, and air-kick stun (M16.124).
- `src/game/entities/viewport_culler_landing.hpp` / `src/game/entities/viewport_culler_landing.cpp`: Airborne XY integration & `15`-frame landing stun / `0x00800270` thud (`0x00E522..0x00E5AC`, reusing `step_z_gravity()`), `320x224 + margin` viewport cullers (`0x00E5AE..0x00E66C`), and `94`-word `0xBC` entity struct copy (`0x00E66E..0x00E67E`) (M16.123).
- `tests/m16_123_viewport_culler_landing_test.cpp`: Automated M16.123 test qualifying airborne XY/spin, landing stun & thud SFX, grounded codes `0`/`3`, `320x224 + margin` cullers, and `0xBC` struct copy (M16.123).
- `src/game/player/weapon_equip_summon.hpp` / `src/game/player/weapon_equip_summon.cpp`: Spirit summon trigger (`0x007E9E..0x007EC2`: state `0x14`, SFX `0x00900359`) and weapon quick-equip class mapper (`0x007EC4..0x007F80`: `0x0A`/`0x0C`/`0x0E`/`0x10`, 60f toast, `0x00C0025D`/`0x00A00253`), reusing `secondary_weapon.hpp` (M16.122).
- `tests/m16_122_weapon_equip_summon_test.cpp`: Automated M16.122 test qualifying `start_spirit_summon`, weapon class mapping `1..13`, slot toggle-to-0, 60f toast, and empty-slot error SFX (M16.122).
- `src/game/ui/debug_warp_menu.hpp` / `src/game/ui/debug_warp_menu.cpp`: Debug stage/room warp menu (`0x004C9C..0x004F42`, `10f`/`3f` key repeat, non-empty area skip, room clamp, `0x006003A6` / `0x00A00256` / `0x00A00347` SFX) and `0x0005D918` 32-area ROM spawn table parser (`(tile_x, tile_y) << 3`) (M16.121).
- `tests/m16_121_debug_warp_menu_test.cpp`: Automated M16.121 test qualifying ROM spawn table parsing, room/area wrap, empty-area skip, key repeat, confirm, and cancel (M16.121).
- `src/game/entities/ai_steering_gravity.hpp` / `src/game/entities/ai_steering_gravity.cpp`: Enemy AI steering (`0x00E398..0x00E45C`), 16.16 normalized `2.0 px/frame` aim (`0x00E45E..0x00E4CE`), knockback launch (`0x00E4D0..0x00E500`), and Z-gravity (`0x00E502..0x00E520`), reusing `velocity_vector_math.hpp` (M16.120).
- `tests/m16_120_ai_steering_gravity_test.cpp`: Automated M16.120 test qualifying Manhattan/4-way steering, 16.16 aim vector, knockback launch, and Z-gravity `0 -> -1` skip & landing (M16.120).
- `src/game/video/sprite_dma_throttle.hpp` / `src/game/video/sprite_dma_throttle.cpp`: Entity sprite-DMA budget throttle (`0x00A9AE..0x00A9F8`, facing `3` -> `1`, `$FF1651 = 0xFF` throttle) and shadow visibility (`0x00AA78..0x00AAA5`) (M16.119).
- `tests/m16_119_sprite_dma_throttle_test.cpp`: Automated M16.119 test qualifying uncached force-DMA, over-budget throttling, facing-3 mirror mapping, and shadow visibility (M16.119).
- `src/game/ui/hud_sat_emitter.hpp` / `src/game/ui/hud_sat_emitter.cpp`: HUD frame SAT & item-toast DMA emitter (`0x00A342..0x00A436`: 6 primary HUD sprites, 3 enemy-gauge sprites mirrored at `0x0158 - slide_x`, 3x3 toast sprite `0x85C7`, 80-sprite cap, and `0x90`-word DMA from `0x000AAEBB + item_id * 0x90` to `0xB8E0`) (M16.118).
- `tests/m16_118_hud_sat_emitter_test.cpp`: Automated M16.118 test qualifying HUD slide mirror math, toast sprite coordinates, 80-sprite cap, and toast icon DMA descriptor (M16.118).
- `src/game/video/ysort_sat_pipeline.hpp` / `src/game/video/ysort_sat_pipeline.cpp`: Master Y-sorted SAT render pipeline (`0x00A196..0x00A27E`: 5 passes — 4 high-prio FX, 4 hit-sparks, 21 descending-Y `> -1` entities, 8 ground/shadow FX, 6 floor FX) (M16.117).
- `tests/m16_117_ysort_sat_pipeline_test.cpp`: Automated M16.117 test qualifying the 5 render passes, descending-Y sort, inactive entity skipping, and `y < -1` sentinel skipping (M16.117).
- `src/game/world/aabb_solid_sweep_3axis.hpp` / `src/game/world/aabb_solid_sweep_3axis.cpp`: 3D AABB solid sweep for `-X` (`0x009EBA`), `+Y` (`0x009FA0`), and `-Y` (`0x00A09A`) across 21 `0xBC`-stride slots with zero-gap `±1` push, reusing `SolidSweepBlocker` (M16.116).
- `tests/m16_116_aabb_solid_sweep_3axis_test.cpp`: Automated M16.116 test qualifying `-X`, `+Y`, `-Y` gap clamping, Z non-overlap skip, and zero-gap `±1` push (M16.116).
- `src/game/ui/save_prompt.hpp` / `src/game/ui/save_prompt.cpp`: In-game save prompt & save-allowed guard (`0x005472..0x005578`, `can_open_save_prompt`, `step_save_prompt` `3=YES`/`4=NO`, toggle `0x006003A5`, confirm `0x00A003AA`, cancel `0x00A00347`, error `0x00A00253`) (M16.115).
- `tests/m16_115_save_prompt_test.cpp`: Automated M16.115 test qualifying the save-allowed guard and YES/NO prompt state machine (M16.115).
- `src/game/player/consumable_effects.hpp` / `src/game/player/consumable_effects.cpp`: Player-facing consumable item validation (`0x005236..0x00530A`) and HP/SP/Spirit restore (`0x00530C..0x005472`) delegating to `oasis::game::items` (M16.114).
- `tests/m16_114_consumable_effects_test.cpp`: Automated M16.114 test qualifying Area `0x10` block, Spirit Gem unlock/SP/active checks, and 25%/50%/75%/100% HP/SP/Elixir restore (M16.114).
- `src/game/ui/food_menu.hpp` / `src/game/ui/food_menu.cpp`: 4x4 food/consumable menu cursor wrap (`0x004906..0x00494A`), drop guards (`0x004896..0x004904`, `terrain_blocked`, `item_id != 0`, `0x05CE16` bit 5 undroppable), and consume timer constants (`90` frames, SFX `0x00A00256`) (M16.113).
- `tests/m16_113_food_menu_test.cpp`: Automated M16.113 test qualifying 4x4 cursor wrap and undroppable/terrain drop guards (M16.113).
- `src/game/ui/ring_menu_drop.hpp` / `src/game/ui/ring_menu_drop.cpp`: Ring menu 4x2 cursor wrap (`0x00462E..0x004670`, SFX `0x006003A6`) and item drop handler (`0x004674..0x0046E2`, `protected_slot` / `terrain_blocked` / `item_id <= 1` guards, `(item_id | 0x80, qty)`) (M16.112).
- `tests/m16_112_ring_menu_drop_test.cpp`: Automated M16.112 test qualifying 4x2 cursor wrap and item-drop validation (M16.112).
- `src/game/ui/ring_menu_loop.hpp` / `src/game/ui/ring_menu_loop.cpp`: Ring/spirit menu entry guard (`0x004416..0x004438`, `$FF1A6E == 0 && $FF19EC <= 2`, error SFX `0x00A00253`) and input decoder (`0x004480..0x00455C`, `$FF1996` A/BC swap, close SFX `0x00A00347`) (M16.111).
- `tests/m16_111_ring_menu_loop_test.cpp`: Automated M16.111 test qualifying entry guard and normal/swapped button decoding (M16.111).
- `src/game/ui/decimal_ascii.hpp` / `src/game/ui/decimal_ascii.cpp`: 5-digit decimal-to-ASCII formatter (`0x00C0BE..0x00C112` zero-padded `"00000".."65535"`, `0x00C114..0x00C178` space-padded `"    0".."65535"`) (M16.110).
- `tests/m16_110_decimal_ascii_test.cpp`: Automated M16.110 test qualifying `0`, `105`, `9999`, `10000`, and `65535` in both zero-padded and space-padded modes (M16.110).
- `src/game/ui/glyph_outliner.hpp` / `src/game/ui/glyph_outliner.cpp`: 1-bpp dialogue glyph outliner & 4-tile VDP rasterizer (`0x00C222..0x00C2EA`, `18x17` 306B canvas, 4-neighbor `0x0E` outline dilation, column-major TL `0x13`/BL `0xA3`/TR `0x1B`/BR `0xAB` 128B 4-bpp tiles) (M16.109).
- `tests/m16_109_glyph_outliner_test.cpp`: Automated M16.109 test qualifying single-pixel 4-neighbor outline dilation and 128-byte 4-bpp VDP tile packing (M16.109).
- `src/game/ui/inventory_grid_ui.hpp` / `src/game/ui/inventory_grid_ui.cpp`: Inventory/Ring 4x4 grid UI layout (`0x0042A6..0x004360`, `grid_slot_to_xy`, `build_inventory_grid_icon_plan` with 8 inventory slots, `9`-tile stride, `+0x18` Y bias, spirit `0x20` & special `0x29` in `$FF2FA8`) (M16.108).
- `tests/m16_108_inventory_grid_ui_test.cpp`: Automated M16.108 test qualifying 4x4 grid slot coordinates (`0`, `3`, `4`, `7`, `15`) and icon layout plan (M16.108).
- `src/game/world/player_tile_box.hpp` / `src/game/world/player_tile_box.cpp`: Player tile-box containment (`0x00C17A..0x00C1CC`) and overlap (`0x00C1CE..0x00C220`) checks with `8px` tile shift and `+7px` inclusive tile max bias (`0x00070007`) (M16.107).
- `tests/m16_107_player_tile_box_test.cpp`: Automated M16.107 test qualifying containment and overlap boundaries including exact `+7px` edge cases (M16.107).
- `src/game/ui/menu_sprite_dma.hpp` / `src/game/ui/menu_sprite_dma.cpp`: Inventory/menu sprite & font DMA strip emitter (`0x004000..0x00417A`, `kSpriteTileCountLut16` `0x0040CE`, `remap_menu_ascii`, `append_menu_sat_sprite` 80-sprite cap, `emit_item_and_digit_pair` `0x86DD`/`0x85D0`, `emit_font_strip_plan` `<= 4`-tile chunks + `0x152F5A`/`0x15375A` DMA descriptors) (M16.106).
- `tests/m16_106_menu_sprite_dma_test.cpp`: Automated M16.106 test qualifying `kSpriteTileCountLut16`, ASCII remap (`0x2B..0x2F`), `<= 4`-tile font strip chunking, and item/digit SAT pair with 80-sprite cap (M16.106).
- `src/game/world/room_height_tileset.hpp` / `src/game/world/room_height_tileset.cpp`: Room height-band table builder (`0x00D43A..0x00D46A`, `0xF000` sentinel), 8px camera quantization (`0x00D48A..0x00D4C4`), and VScroll buffer flush (`0x00D344..0x00D396`) (M16.105).
- `tests/m16_105_room_height_tileset_test.cpp`: Automated M16.105 test qualifying height-band tables, 8px quantization, and `$FFFF5C`/`$FF1348` VScroll flush (M16.105).
- `src/game/world/plane_scroll_sync.hpp` / `src/game/world/plane_scroll_sync.cpp`: Plane scroll delta & VDP H/V-scroll buffer sync (`0x00D188..0x00D342`: 8px tile crossings, `0x1FF`/`0xFF` wrap, `±10000` anti-overflow, `wrap_camera_to_map_bounds`, `flush_hscroll_to_vdp_buffer`) (M16.104).
- `tests/m16_104_plane_scroll_sync_test.cpp`: Automated M16.104 test qualifying tile-crossing deltas, `0x1FF`/`0xFF` masks, `±10000` wrap, map-bounds wrap, and Plane A/B HScroll flush (M16.104).
- `src/game/camera/camera_follow.hpp` / `src/game/camera/camera_follow.cpp`: Camera deadzone follow (`0x00CEEA..0x00D114`: `[0x78,0xC8]`/`[0x68,0x80]`, `±3`/`±0x1E` ramp, HUD slide `0x50`/`0xF0`, locked-center mode, `$FF17C4` screen shake) (M16.103).
- `tests/m16_103_camera_follow_test.cpp`: Automated M16.103 test qualifying deadzone, smooth catch-up, HUD slide triggers, locked-center mode, and screen shake (M16.103).
- `src/game/world/room_warp_sequence.hpp` / `src/game/world/room_warp_sequence.cpp`: Room update hook guard (`0x00C980`/`0x00C998`), 6-flag warp lock guard (`0x00C9B2..0x00C9EA`), and warp transition sequence (`0x00C9EC..0x00CB0A`: `0x0004 -> 0x0611`, camera centering, checkpoint bit 6, flag reset, fade `4`) (M16.102).
- `tests/m16_102_room_warp_sequence_test.cpp`: Automated M16.102 test qualifying the 6-flag warp lock guard, `0x0004 -> 0x0611` redirect, camera centering, checkpoint trigger, and flag reset (M16.102).
- `src/game/ui/dialogue_sat_builder.hpp` / `src/game/ui/dialogue_sat_builder.cpp`: Dialogue cursor & 4x8 border SAT builder (`0x00C756..0x00C84C`, `0x86DC`/`0x96DC` blinking cursor, `0x0C00` border bars, `80`-sprite cap) (M16.101).
- `tests/m16_101_dialogue_sat_builder_test.cpp`: Automated M16.101 test qualifying cursor blinking/choice positioning, 8-sprite bar emission, and 80-sprite SAT cap (M16.101).
- `src/game/world/room_area_loader.hpp` / `src/game/world/room_area_loader.cpp`: Room & area transition loader (`0x00C8AA..0x00C96B`, 16-entry Area Table, `resolve_room_header_addr`, `transition_room_flags`) (M16.100).
- `tests/m16_100_room_area_loader_test.cpp`: Automated M16.100 test qualifying the 16 area pointers, `entry + int16_be` header resolution, same/cross-area flag clearing, and `0x001B` ROM parity (M16.100).
- `src/game/video/vdp_dma_exec.hpp` / `src/game/video/vdp_dma_exec.cpp`: Z80-safe VDP DMA command word serializer (`0x0027FC..0x002896`: regs `0x94..0x97` + VRAM command) and HUD portrait DMA descriptor builder (`0x002632..0x002678`, `0x000AA3EC + portrait * 0x60` -> `0xF1C0`, `0x60` words) (M16.99).
- `tests/m16_99_vdp_dma_exec_test.cpp`: Automated M16.99 test qualifying DMA register sequence, VRAM command words, and HUD portrait dirty-check & address math (M16.99).
- `src/game/world/terrain_elevation_probe.hpp` / `src/game/world/terrain_elevation_probe.cpp`: Terrain elevation & ledge probe (`0x00938E..0x0096F7`, 16-byte Gray-code LUT `0x0096E8`, span OR/AND aggregation, step-up `bit 5`, blocked `bit 6`, ledge-drop `drop_height`) (M16.98).
- `tests/m16_98_terrain_elevation_probe_test.cpp`: Automated M16.98 test qualifying the Gray-code LUT, passable flat tiles, mixed/invalid blocking (`bit 6`), step-up blocking (`bit 5`), and ledge-drop `drop_height` (M16.98).
- `src/game/world/vdp_plane_scroll.hpp` / `src/game/world/vdp_plane_scroll.cpp`: Incremental row/col VDP plane scroller (`0x0023CC..0x002510` 29-tile column streamer & `0x002512..0x002630` 41-tile row streamer with dual source/VDP `64x32` wrap) (M16.97).
- `tests/m16_97_vdp_plane_scroll_test.cpp`: Automated M16.97 test qualifying positive/negative column & row scroll deltas and source/VDP nametable wrap-around (M16.97).
- `src/game/ui/vint_hud_slide.hpp` / `src/game/ui/vint_hud_slide.cpp`: V-Int tail sound driver tick (`0x060000`) and HUD slide offset state machine (`0x002380..0x0023CA`, `$FF1859`/`$FF185A`, stops `0x20`/`0x138`, wrap `0x158`) (M16.96).
- `tests/m16_96_vint_hud_slide_test.cpp`: Automated M16.96 test qualifying constants, forward/reverse HUD slide stops and wraps, and `VintTailState` tick (M16.96).
- `src/game/world/vdp_plane_blitter.hpp` / `src/game/world/vdp_plane_blitter.cpp`: Wrap-around VDP Plane A/B blitter (`0x00D800..0x00D94E`, `(64, 32)` vs `(41, 29)` selection and dual-modulo `64x32` nametable copy) (M16.95).
- `tests/m16_95_vdp_plane_blitter_test.cpp`: Automated M16.95 test qualifying full-plane vs viewport dimension selection and dual-modulo wrap blitting (M16.95).
- `src/game/entities/entity_anim_sequencer.hpp` / `src/game/entities/entity_anim_sequencer.cpp`: 4-byte entity animation keyframe sequencer (`0x008FF0..0x009041`, `AnimKeyframe4`, `EntityAnimState`, `step_anim_keyframe`, `step_anim_tick`) (M16.94).
- `tests/m16_94_entity_anim_sequencer_test.cpp`: Automated M16.94 test qualifying keyframe duration math, `swing_hit_once` clear, `last_frame` wrap, and `anim_completed` flag (M16.94).
- `src/game/world/room_layer_loader.hpp` / `src/game/world/room_layer_loader.cpp`: Room tilemap & collision layer descriptor builder (`0x00D500..0x00D7BF`, `compute_row_shift` `0x00D7B0`, `decode_room_layer_header`) (M16.93).
- `tests/m16_93_room_layer_loader_test.cpp`: Automated M16.93 test qualifying `compute_row_shift`, 2x1 room header layout, and dual-plane collision offsets (M16.93).
- `src/game/entities/prng_lfsr.hpp` / `src/game/entities/prng_lfsr.cpp`: 16-bit LFSR PRNG (`$FF1928`, seed `0x7F28`, `next_rng16` `0x00E268`, `rand_u16` `0x00E2A2`, `rand_u8` `0x00E2BA`) (M16.92).
- `tests/m16_92_prng_lfsr_test.cpp`: Automated M16.92 test qualifying the exact 8-output sequence from `0x7F28` and bounded `rand_u16`/`rand_u8` helpers (M16.92).
- `src/game/entities/entity_pool_allocators.hpp` / `src/game/entities/entity_pool_allocators.cpp`: 4 fixed-stride WRAM entity/effect pool allocators (`Effect8` `$FF2ABC`, `Special6` `$FF2D8C`, `FullEntity18` `$FF1C1C`, `EnemyEntity17` `$FF1CD8`, `0x0694 + idx * 9`, `$FF184F` HUD invalidation) (M16.91).
- `tests/m16_91_entity_pool_allocators_test.cpp`: Automated M16.91 test qualifying pool capacities, RAM addresses, Effect8 VRAM tiles, and `$FF184F` invalidation on slot reuse (M16.91).
- `src/game/video/vdp_clear_and_palette_cycle.hpp` / `src/game/video/vdp_clear_and_palette_cycle.cpp`: VDP Full Clear table (`0x002B28`), Armlet Glow state machine (`0x008DD4`), and CRAM Palette Cycler (`0x008E22`) (M16.90).
- `tests/m16_90_vdp_clear_and_palette_cycle_test.cpp`: Automated M16.90 test qualifying 6 VDP clear regions, armlet glow ramp/pulse/decay, and CRAM keyframe cycling (M16.90).
- `src/game/entities/solid_entity_sweep.hpp` / `src/game/entities/solid_entity_sweep.cpp`: 3D AABB solid entity `+X` sweep (`0x009DD6`, 21-slot `$FF19E8` stride `0xBC`, `$40` bit 7, `3`-frame `$6C` push timer, `+1px` pushable nudge) (M16.89).
- `tests/m16_89_solid_entity_sweep_test.cpp`: Automated M16.89 test qualifying eligibility flags, 3D AABB separation, 21-slot bound, push timer, and pushable nudge (M16.89).
- `src/game/world/terrain_probe_and_clamp.hpp` / `src/game/world/terrain_probe_and_clamp.cpp`: Terrain surface effect LUT (`0x0096E8`, `kTerrainSurfaceLut16`), layer descriptor selector (`$FF1766`/`$FF1770`), and 4-side room boundary clamp (`0x009D4E..0x009DD4`, `0x48` edge-wall flag) (M16.88).
- `tests/m16_88_terrain_probe_and_clamp_test.cpp`: Automated M16.88 test qualifying all 16 LUT entries and 4-side + corner room boundary clamping (M16.88).
- `src/game/entities/velocity_vector_math.hpp` / `src/game/entities/velocity_vector_math.cpp`: 8-direction 16.16 fixed-point velocity vector (`0x00E2D4`, `0.75` diagonal `(v >> 1) + (v >> 2)`) and cardinal facing resolver (`0x00E338`) (M16.87).
- `tests/m16_87_velocity_vector_math_test.cpp`: Automated M16.87 test qualifying all 8 velocity directions, mod-8 wrapping, and cardinal facing tie-breaking (M16.87).
- `src/game/entities/entity_struct_layout.hpp` / `src/game/entities/entity_struct_layout.cpp`: Universal entity constructor (`0x008D06`), pointer re-init (`0x008DA4`), and `0x03F2FA` 32-byte master descriptor decoder (`EntityDescriptor32`, `EntityConstructorState`, `0x5D`/`0x2C` clear words, `< $FF2954` `secondary_ai_ptr` gating) (M16.86).
- `tests/m16_86_entity_struct_layout_test.cpp`: Automated M16.86 test qualifying 16-field descriptor decoding, full vs lightweight slot constructor, `0x008DA4` pointer re-init, and Ali ID 2 ROM parity (M16.86).
- `src/game/audio/sound_channels.hpp` / `src/game/audio/sound_channels.cpp`: Sound driver 11-channel FM/PSG/SFX RAM map (`$FF0022..$FF07BB`, `0xCA`/`0x8A` strides), `stop_all` (`0x0604E6`), and `start_fade_all` (`0x060516`, step `2`, timer `0x7E`, `+0x75` bit 4) (M16.85).
- `tests/m16_85_sound_channels_test.cpp`: Automated M16.85 test qualifying all 11 channel RAM addresses, `start_fade_all` attenuation ramp, and `stop_all` mute/clear (M16.85).
- `src/game/ui/hud_gauge_bars.hpp` / `src/game/ui/hud_gauge_bars.cpp`: HUD HP/SP/Enemy Gauge Bar VDP Renderer (`0x00267C..0x0027A4`, `compute_gauge_fill_rows`, differential row-delta writes, `0x00EEEE00` erase mask) (M16.84).
- `tests/m16_84_hud_gauge_bars_test.cpp`: Automated M16.84 test qualifying gauge fill-row math, differential caching, erase OR-mask, and enemy timer/demo gating (M16.84).
- `src/game/player/special_move_inputs.hpp` / `src/game/player/special_move_inputs.cpp`: Ali command-input special moves (`0x0086B4` 360° Spin Slash via LUT `0x008768` & `0x008778` F-B-F Lunge via `0x0087F4`) (M16.83).
- `tests/m16_83_special_move_inputs_test.cpp`: Automated M16.83 test qualifying CW/CCW 360° spin, F-B-F lunge, perpendicular abort, and `$FF16EA` count clear (M16.83).
- `src/game/combat/hit_reactions_and_sparks.hpp` / `src/game/combat/hit_reactions_and_sparks.cpp`: Hit-reaction knockback physics (`0x00BF26`, profiles `0/2/4/6`), shield parry recoil (`0x00BFAE`, SFX `0x00900202`, weapon `6`/`10`), and 4-slot hit-spark pool (`0x00C012`, `$FF19C8`, `timer = 5`) (M16.82).
- `tests/m16_82_hit_reactions_and_sparks_test.cpp`: Automated M16.82 test qualifying all 4 hit-reaction profiles, shield parry recoil, and 4-slot hit-spark lifecycle (M16.82).
- `src/game/combat/combat_hitbox_kernel.hpp` / `src/game/combat/combat_hitbox_kernel.cpp`: Combat hitbox collision & kill-rank scaling (`0x00BCBA..0x00BF26`, 7-tier `+11` max bonus, shield block `0x00BFAE`, `90`-frame enemy HP HUD `$FF184F`, `6`-frame hit-flash `$83`, reaction `0/2/4/6`) (M16.81).
- `tests/m16_81_combat_hitbox_kernel_test.cpp`: Automated M16.81 test qualifying kill-rank bonus tiers, dir 3->1 mirror, shield block, and HUD/flash/reaction outputs (M16.81).
- `src/game/video/vint_dma_and_input_ring.hpp` / `src/game/video/vint_dma_and_input_ring.cpp`: V-Int LIFO DMA queue (`0x002112`, `$FF164E` bit 1), 64-frame input history ring (`0x0021B2`, `$FF1668`, `0xE0000202` soft-reset), and V-Int frame/countdown timers (`0x0021EC`) (M16.80).
- `tests/m16_80_vint_dma_and_input_ring_test.cpp`: Automated M16.80 test qualifying LIFO DMA flush, 64-frame input ring wrap/clamp, soft-reset combo, and frame timers (M16.80).
- `src/game/video/palette_fade.hpp` / `src/game/video/palette_fade.cpp`: CRAM Palette Fade Engine (`0x002D58..0x00302D`), `scale_bgr333_color` (`0x002FDA`), 64-entry `$FF134C`/`$FF18A6` fade-in/fade-out, and `$FF164D` CRAM DMA / unblank flags (M16.79).
- `tests/m16_79_palette_fade_test.cpp`: Automated M16.79 test qualifying `0x0EEE` BGR333 ramp, fade-in delay/unblank, and fade-out backup restoration (M16.79).
- `src/game/audio/sound_test_ost.hpp` / `src/game/audio/sound_test_ost.cpp`: Secret Sound Test (`0x03E7F4..0x03F047`), 34-track Yuzo Koshiro named OST registry (`0x03ED7E`), and `SoundTestMenuState` (`BGM`/`SFX`/`PCM`/`EXIT`) (M16.78).
- `tests/m16_78_sound_test_ost_test.cpp`: Automated M16.78 test qualifying all 34 OST tracks, unlock chord `0x90000000`, and `SoundCommandQueue` dispatch (M16.78).
- `src/game/ui/continue_and_checkpoint.hpp` / `src/game/ui/continue_and_checkpoint.cpp`: Continue Game SRAM load (`0x00161C`/`0x0016F4`, primary/backup fallback, 376B restore, `30` invuln frames) and Checkpoint Snapshot / Retry Restore (`0x0017DA`/`0x001828`) (M16.77).
- `tests/m16_77_continue_and_checkpoint_test.cpp`: Automated M16.77 test qualifying SRAM primary/backup fallback, soft-reset on double corruption, and checkpoint snapshot/retry restore (M16.77).
- `src/game/ui/new_game_init.hpp` / `src/game/ui/new_game_init.cpp`: New Game initialization (`0x001300..0x00144F`: `HP=200`, `ATK=10`, `Room=0x001B`, `Pos=(256,80)`) and 4 secret start chords (`0xD6000100`, `0xC9000200`, `0xCA000100`, `0x90000000`) (M16.76).
- `tests/m16_76_new_game_init_test.cpp`: Automated M16.76 test qualifying normal New Game stats/spawn and all 4 secret start chords (M16.76).
- `src/game/ui/file_select_menu.hpp` / `src/game/ui/file_select_menu.cpp`: Save/Load File Select Menu (`0x0004C6..0x000D30`), 2x3 grid navigation (`0x0006F6`), New/Continue/Copy/Erase flows (`0x000B04`/`0x000C46`), and `$2025C9` last-slot persistence (M16.75).
- `tests/m16_75_file_select_menu_test.cpp`: Automated M16.75 test qualifying 2D grid navigation, Copy/Erase guards, New/Continue dispatch, and Cancel to Title (M16.75).
- `src/game/ui/world_map_and_records.hpp` / `src/game/ui/world_map_and_records.cpp`: World Map pin math (`0x004B4E`/`0x004BAE`/`0x004BE8`), `"Rank00"`/`"Race00"` odd-byte SRAM clear-record codecs (`0x001DDC..0x001ECE`), and Attract/Demo stream lookup + delta-decoder (`0x008A94`/`0x008A30`) (M16.74).
- `tests/m16_74_world_map_and_records_test.cpp`: Automated M16.74 test qualifying overworld/dungeon pin math, `"Rank00"`/`"Race00"` SRAM codecs, and attract demo lookup/delta-decoding (M16.74).
- `src/game/player/player_action_machine.hpp` / `src/game/player/player_action_machine.cpp`: Complete 59-state Ali player table (`0x0059BA..0x005A2F`), pre-dispatch HP/SP warnings/regen/hazard/respawn tick (`0x005580..0x0057AE`), and death/auto-revive/game-over state machine (`0x005F92`, `0x00611E`) (M16.73).
- `tests/m16_73_player_action_machine_test.cpp`: Automated M16.73 test qualifying 59-state lookup, pre-dispatch timers/regen, and `0x5E -> 0x6C` / `0x60` transitions (M16.73).
- `src/game/ui/game_mode_flow.hpp` / `src/game/ui/game_mode_flow.cpp`: Top-level game mode state machine (`0x00045E`, `$FF10AC`/`$FF10AE`), Title Screen (`0x0032F8`: `720`-frame attract timeout, `128`-frame blink duty, 4 Start chords), and Best 5 Players Clear Records (`0x03E4FA`, BGM `0x0121`, SRAM magic clear) (M16.72).
- `tests/m16_72_game_mode_flow_test.cpp`: Automated M16.72 test qualifying `0x00045E` mode table lookup, `$FF10AC`/`$FF10AE` latch, Title Screen timeout/blink/chords, and Clear Records screen flow (M16.72).
- `src/game/dialogue/dialogue_engine.hpp` / `src/game/dialogue/dialogue_engine.cpp`: Dialogue/text engine (`0x00C386..0x00C755`), 116-entry `0x051514` string table lookup, `0x16943C` font glyph indexing, and 7 control codes (`0x00..0x07`) (M16.71).
- `tests/m16_71_dialogue_engine_test.cpp`: Automated M16.71 test qualifying `0x051514` string lookup, glyph offset math, and frame-by-frame control code execution (M16.71).
- `src/game/events/event_vm.hpp` / `src/game/events/event_vm.cpp`: Event VM 31-opcode ISA (`0x00F788..0x00F80D`, opcodes `0x00..0x1E`) and Opcode `0x12` conditional branch evaluator (`0x00FAB2..0x00FC6B`, 12 sub-actions, 6 comparators) (M16.70).
- `tests/m16_70_event_vm_test.cpp`: Automated M16.70 test qualifying all 31 opcode targets, 6 comparators, 12 Opcode `0x12` sub-actions, and multi-frame script execution (M16.70).
- `tests/m16_69_bgm_track_table_test.cpp`: Automated M16.69 test qualifying `0x0638D4` BGM sequence offset table resolution, `Z80BusHandshakeState` (`0x060B68..0x060BC4`), and `SoundDriverState::dispatch_queue` BGM ROM address tracking (M16.69).
- `src/game/enemy/heavy_enemy_dispatch.hpp` / `src/game/enemy/heavy_enemy_dispatch.cpp`: Heavy/Boss enemy families (`0x0068`, `0x0074`, `0x0076`, `0x0080`) `$66(A6)` substate dispatch specs, `& 0x7F` masking, init timer/state/action setup, and `HeavyEnemySubstateController` (M16.68).
- `tests/m16_68_heavy_enemy_dispatch_test.cpp`: Automated M16.68 test qualifying `0x0068`/`0x0074`/`0x0076`/`0x0080` family specs, `& 0x7F` masking, init timer/state/action setup, and substate wrapping (M16.68).
- `src/game/enemy/dungeon_enemy_dispatch.hpp` / `src/game/enemy/dungeon_enemy_dispatch.cpp`: Dungeon enemy families (`0x006C..0x0072`) `$66(A6)` substate dispatch specs, `0x006C` variant table `0x01F5C8` (`[3, 6, 9, 12]`), `0x0072` jump table `0x01D152`, and `DungeonEnemySubstateController` (M16.67).
- `tests/m16_67_dungeon_enemy_dispatch_test.cpp`: Automated M16.67 test qualifying `0x006C..0x0072` family specs, `0x006C` variants, `0x0072` substate target PCs, and `$66(A6)` clamping/wrapping (M16.67).
- `src/game/interaction/interaction_dispatcher.hpp` / `src/game/interaction/interaction_dispatcher.cpp`: Universal Interaction Dispatcher (`0x008330`, `0x00B852` 16 px proximity, `0x00E338` facing) routing `0x0032`, `0x0020`, `0x0094`/`0x0096`/`0x00BE` (`0x018EBE` + hooks `0x24`/`0x2F`/`0x30`/`0x39`), and `0x00B0` Signpost (`0x00839E`, SFX `0xA00264`, freeze `0x3F`) (M16.66).
- `tests/m16_66_interaction_dispatcher_test.cpp`: Automated M16.66 test qualifying `0x008330` proximity/facing gate, entity-type routing, and Story NPC post-dialogue hooks (M16.66).
- `src/game/spirits/shade_bow_controllers.hpp` / `src/game/spirits/shade_bow_controllers.cpp`: Shade (`0x001C`: Shadow Tether `7` SP, Shadow Surge, 40-frame SP drain) and Bow (`0x001E`: Bite `4` SP, Pollen / Vine `9` SP, Plant Storm `28` SP, 30-frame SP drain) controllers (M16.65).
- `tests/m16_65_shade_bow_abilities_test.cpp`: Automated M16.65 test qualifying Shade and Bow ability SP guards, state/action transitions, and 40/30-frame SP drain despawn (M16.65).
- `src/game/spirits/efreet_controller.hpp` / `src/game/spirits/efreet_controller.cpp`: Shared spirit SP-cost guard (`0x011B28`) and Efreet (`0x001A`) controller implementing Fire Dash (`8` SP), Flame Pillar (`6` SP), Meltdown (`25` SP), Dismiss (`0` SP), and 35-frame SP drain (M16.64).
- `tests/m16_64_efreet_abilities_test.cpp`: Automated M16.64 test qualifying `0x011B28` SP guard, Efreet abilities, and 35-frame SP drain despawn (M16.64).
- `src/game/items/item_attribute_tables.hpp` / `src/game/items/item_attribute_tables.cpp`: ROM 64-byte Item Attribute Table (`0x05CE16` bit 5 non-discardable), Pickup Fanfare Tier Table (`0x05CE56`/`0x007A6C`), and pickup persistent flag setter (`0x007AF0`) (M16.63).
- `tests/m16_63_item_attribute_tables_test.cpp`: Automated M16.63 test qualifying `0x05CE16` attributes, `0x05CE56` tiers 0..4, `0x007AF0` flag setter, and `discard_inventory_slot` key-item protection (M16.63).
- `tests/m16_62_spirits_sp_drain_test.cpp`: Automated M16.62 test qualifying `45/35/40/30`-frame SP drain intervals, despawn handlers (`0x012314..0x013F22`), state tables (`0x011BAC..0x0138E6`), and `ActiveSpiritController::step_sp_drain` clean despawn (M16.62).
- `src/game/audio/sound_command_dispatch.hpp` / `src/game/audio/sound_command_dispatch.cpp`: ROM sound driver entrypoints (`0x060000..0x060008` -> `0x060090`, `0x06042A`, `0x061232`) and `d0.L` command classifier/dispatcher (`0x00..0x08` & `0x0100..0x0500`) (M16.61).
- `tests/m16_61_sound_command_dispatch_test.cpp`: Automated M16.61 test qualifying ROM `0x060000..0x060008` branch targets, command classification (`0x80034F`, `0x800399`, `0x8004D9`, `0xA00234`, `0x0002`), and `SoundCommandQueue` dispatch (M16.61).
- `src/game/enemy/enemy_ai_0030_0048.hpp` / `src/game/enemy/enemy_ai_0030_0048.cpp`: Entity `0x0048` Charger (`0x01778C`, AABB table `0x01783E`) and Entity `0x0030` Room `0x0105` Gate Boss (`0x015AE2`, 5 linked child slots) AI controllers (M16.60).
- `tests/m16_60_enemy_ai_0030_0048_test.cpp`: Automated M16.60 test qualifying ROM `0x01783E` AABB bytes, `Charger0048Ai` 4-substate cycle, and `GateBoss0030Ai` 5-child spawn/defeat lifecycle (M16.60).
- `src/game/ui/ring_menu_and_rank.hpp` / `src/game/ui/ring_menu_and_rank.cpp`: Pause Ring Menu (`0x0042E8`/`0x004388`/`0x00155D76`, 8 weapon + 16 item slots, 24px spacing) and `PlayerRankStats` (`0x007BF6` `198 + 2 * min(rank, 200)`, `0x007BCA` `+5` SP, `0x007BD4` gem counters) (M16.59).
- `tests/m16_59_ring_menu_and_rank_test.cpp`: Automated M16.59 test qualifying ring menu cursor wrapping/WRAM sync, rank/MaxHP scaling cap 200, SP pickup, and gem counters (M16.59).
- `src/game/spirits/all_spirits_registry.hpp` / `src/game/spirits/all_spirits_registry.cpp`: All-4-spirits `SpiritResourceSpec` registry (`0x014522`, `0x014668..0x014798`), generic Nemesis VRAM uploader, and `ActiveSpiritController` (M16.58).
- `tests/m16_58_all_spirits_resources_test.cpp`: Automated M16.58 test qualifying ROM DMA branches, Nemesis decompression into VRAM for all 4 spirits, and ring `0x1A..0x1D` summon integration (M16.58).
- `src/game/items/item_effects.hpp` / `src/game/items/item_effects.cpp`: Item eligibility (`0x005236`), effect dispatcher (`0x00530C`/`0x00532C` for `0x1A..0x2A`), inventory slot consumption, and Dytto/all-spirits summon trigger (M16.57, M16.58).
- `tests/m16_57_item_effects_and_summon_test.cpp`: Automated M16.57 test qualifying `0x005236` eligibility, `0x00530C` HP/SP/Dual/Ring effects, consumable stack decrement, and Dytto Ring (`0x1A`) summon (M16.57).
- `src/game/save/sram_save_system.hpp` / `src/game/save/sram_save_system.cpp`: 596-byte (`$FF0BFE..$FF0E51`) persistent WRAM serializer, 4+4 odd-byte SRAM bank (`0x001182`/`0x00129A`/`0x0012E8`), and `StandaloneGameSimulation` state capture/restore helpers (M16.56).
- `tests/m16_56_sram_save_and_inventory_test.cpp`: Automated M16.56 test qualifying ROM magic `0x001754`, 596B WRAM inventory/flag offsets, 4+4 SRAM slot scan/checksum/backup repair, and `StandaloneGameSimulation` save/load round-trip (M16.56).
- `src/game/entity/rom_entity_tables.hpp` / `src/game/entity/rom_entity_tables.cpp`: ROM 32-byte Entity Descriptor loader (`0x03F2FA`, `0x0002..0x00C6`) and Master Update Dispatch Table lookup (`0x00DF54..0x00E0B8`, `0x0016..0x00C6`) (M16.55).
- `src/game/combat/elemental_damage.hpp` / `src/game/combat/elemental_damage.cpp`: Universal Damage & Elemental Bit-13 / Defense `$92(A6)` pipeline (`0x017E30`) with death State 2 transition (`0x008EF8`) (M16.55).
- `tests/m16_55_entity_master_tables_test.cpp`: Automated M16.55 test qualifying `0x03F2FA` descriptors, `0x00DF54` update handlers, `0x017E30` elemental/defense damage, and `DyttoProjectile` (`0x0024`) integration (M16.55).
- `src/game/spirits/dytto_abilities.hpp` / `src/game/spirits/dytto_abilities.cpp`: Dytto 3-way spread (`0x015214`), magic burst (`0x015136`), `0x0024` projectile flight/collision, Water Elemental damage (`0x2008` / `0xA008` at `0x014E86`), and knockback deceleration (`0x014EE6`) (M16.54).
- `tests/m16_54_dytto_abilities_and_town_sat_test.cpp`: Automated M16.54 test qualifying `State 0x00` PRNG dispatch, 3-way spread, magic burst, Water damage (`0x2008`/`0xA008`), Ali contact buff, knockback recovery, and Town-mode SAT rendering of Dytto (M16.54).
- `src/game/render/dytto_sat_renderer.hpp` / `src/game/render/dytto_sat_renderer.cpp`: Dytto Nemesis sprite decompression (`0x00141580` -> VRAM `0x9A00` / tile `0x04D0` via `0x00003820`) and SAT sprite emission for State `0x1A` (Appear) and State `0x04` (Follow) (M16.53).
- `tests/m16_53_dytto_vram_sat_render_test.cpp`: Automated M16.53 test qualifying `0x00141580` VRAM `0x9A00` upload, State `0x1A` burst/pulse SAT entries, State `0x04` H-flip follow SAT entries, and `VdpRasterizer` RGB24 rendering (M16.53).
- `src/game/spirits/spirit_awakening.hpp` / `src/game/spirits/spirit_awakening.cpp`: Spirit Awakening Dispatcher (`0x018CAA`) and `ShrineEventConfig` table for Dytto (`0x0106`), Efreet (`0x0206`), Shade (`0x0805`), Bow (`0x0A10`), and Agito (`0x090B`) (M16.52).
- `src/game/spirits/dytto_controller.hpp` / `src/game/spirits/dytto_controller.cpp`: Native Dytto (`0x0018`) spirit controller implementing `0x014668` spawn, State `0x1A` 420-frame awakening timeline (`0x014F60`), and State `0x04` follow-player steering & `ASR.L #7` damping (`0x0150A0`) (M16.52).
- `tests/m16_52_spirit_awakening_dytto_test.cpp`: Automated M16.52 test qualifying `0x018CAA` dispatcher ROM bytes, `$FF0DBC:3`/`$FF0E1D:2`/`$FF0D7A:0` flags, Dytto `0x0018` spawn, State `0x1A` timeline, and State `0x04` follow steering (M16.52).
- `data/standalone-fixtures/sanctuary_multi_room_m16_51.json`: Machine-readable specification and differential fixture for Spirit Sanctuary multi-room network (Area 0x0D Entrance Hall 0x0D00, descriptor 0x037B44, stream 0x259F78, tile banks [86, 87, 88, 0]; Inner Chamber 0x0D01 / 0x0F01, descriptor 0x02E48C, stream 0x25A3C4; authentic doorway transitions 0x02E452 and 0x02E524; camera bounds; continuous progression chain) (M16.51).
- `tests/m16_51_sanctuary_playability_test.cpp`: Automated M16.51 test qualifying multi-room Spirit Sanctuary world structure, generic room loading, bidirectional in-sanctuary doorway transitions, camera boundaries, and continuous unbroken progression from Beach to Sanctuary without state injection (M16.51).
- `data/standalone-fixtures/princess_rescue_and_sanctuary_m16_50.json`: Machine-readable specification and differential fixture for entity 0x0020 (Princess Gwyn / Water Spirit), dispatcher at 0x00835E, cutscene 0x018CAA / 0x018CDE, Story Flag 3 set point at 0x018D38, dialogue Message ID 0 (3-page Dytto reveal), and authentic Spirit Sanctuary transition (M16.50).
- `tests/m16_50_princess_rescue_test.cpp`: Automated M16.50 test qualifying Princess Gwyn rescue event on Screen(1,6), dialogue progression, authentic setting of Global Flag 3 by gameplay, and subsequent cross-area warp to Spirit Sanctuary Room 0x0D00 without state injection (M16.50).
- `data/standalone-fixtures/cross_area_spirit_sanctuary_m16_49.json`: Machine-readable specification and differential fixture for Area 13 Screen 0 descriptor at `0x037B44`, extended tile banks `[86, 87, 88, 0]`, cross-area warp trigger at `0x02EFC0..0x02EFCE`, cutscene timer `$FF1971: 0x22`, warp destination `(256, 768)`, Area 13 spawn script `0x04382E`, and story flag blocker analysis (M16.49).
- `tests/m16_49_cross_area_sanctuary_test.cpp`: Automated M16.49 test qualifying commit ancestry and regression audit, Room 0x0D00 descriptor reverse engineering, extended tile banks and stream parsing, cross-area transition from Screen(1,6), spawn script execution, and story flag blocker vs component injection (M16.49).
- `data/standalone-fixtures/npc_interaction_differential_m16_48.json`: Machine-readable specification and differential fixture for authentic dialogue table `0x051514`, Message IDs 4 and 5, Room `0x0100` and `0x0108` spawn scripts, Pier Villager and Village Elder interaction rules, and Event VM flag opcodes `0x0F`, `0x10`, `0x11` (M16.48).
- `tests/m16_48_npc_dialogue_parity_test.cpp`: Automated M16.48 test qualifying Village Elder differential parity, dialogue presentation geometry and font glyphs, second ROM-grounded interaction (Pier Villager, Msg 4), and Event VM flag opcodes `0x0F`, `0x10`, `0x11` (modes 0/1/2 across all banks) (M16.48).
- `src/game/event/event_vm.hpp` / `src/game/event/event_vm.cpp`: Reusable ROM-backed dialogue decoder (`resolve_dialogue_address`, `decode_dialogue_from_rom`, `decode_dialogue_by_id` using table `0x051514`) and Event & Room Spawn Script VM executing bytecode instructions `0x00`, `0x07`, `0x08`, `0x0A`, `0x0F`, `0x10`, `0x11`, `0x14`, `0x15`, `0x16` with multi-bank event flag tracking and modification (M16.47, M16.48).
- `src/game/npc_dialogue_handler.hpp` / `src/game/npc_dialogue_handler.cpp`: Authentic ROM-grounded NPC interaction handler evaluating proximity (`dx <= 28 && dy <= 28`), player movement lock state `0x36`, controller button dialogue advancement, dynamic entity message ID dispatch, and Event Flag lifecycles (M16.47, M16.48).
- `src/game/render/dialogue_window.hpp` / `src/game/render/dialogue_window.cpp`: Authentic dialogue window presentation model with multi-page progression, VDP nametable commitment, and RGB24 text overlay rendering (M16.47).
- `data/standalone-fixtures/npc_interaction_differential_m16_47.json`: Machine-readable specification and differential fixture for authentic dialogue table `0x051514`, Message ID 5 (`0x05178C`), Room `0x0108` spawn script `0x0466B2`, and NPC `0x003C` interaction rules (M16.47).
- `tests/m16_47_npc_interaction_event_test.cpp`: Automated M16.47 test qualifying authentic dialogue decoding, Room `0x0108` spawn script execution, dialogue window rendering, and checkpoint-free native end-to-end interactive NPC gameplay flow (M16.47).
- `tests/m16_46_world_transition_parity_test.cpp`: Automated M16.46 test qualifying Group 1 transition parity, scripted door warp & exit (Beach Screen 1,2 <-> West Village House Interior Screen 1,8), NPC 0x003C spawn, camera clamping, transition generalization, and holdout snapshot provenance (M16.46).
- `tools/generate_rom_world_graph.py`: Standalone build-time Python tool extracting Group 1 screen descriptors and transitions directly from ROM without hybrid repo dependency (M16.46).
- `data/standalone-fixtures/gpgx_transition_differential_m16_46.json`: Machine-readable comparison fixture documenting 5 dynamically confirmed and 13 statically verified transitions plus scripted doors (M16.46).
- `src/game/world/world_graph.hpp` / `src/game/world/world_graph.cpp`: Verified Group 1 room transition world graph (`WorldTransitionEdge`, `ScreenTransitionTarget`, `get_group1_world_edges`, `get_group1_screen_bounds`, `evaluate_world_transition`) implementing authentic geometric boundary and doorway transitions across screens `(1,0)`..`(1,8)` (M16.45, M16.46).
- `src/game/scene_transition_helpers.hpp` / `src/game/scene_transition_helpers.cpp`: Scene transition helper routines (`assign_default_town_entities`, `make_scene_camera_config`) providing reusable entity initialization and camera configuration across multi-screen traversals (M16.45).
- `tests/m16_45_world_graph_playability_test.cpp`: Automated M16.45 test qualifying Group 1 world graph transition verification, 3,600-frame (60.0s) native-only playability across 8 distinct screens (`0x0100`..`0x0107`) with 19 transitions, and 100% preservation of 21-frame exact prefix (F775..F795) and holdout baseline (21,110 diff px) (M16.45).
- `tests/m16_44_standalone_playability_audit_test.cpp`: Automated M16.44 test qualifying 1,800-frame (30.0s) checkpoint-free native-only end-to-end playability across 3 connected screens (`0x003D <-> 0x005A <-> 0x005B`), 21-frame continuous pixel-exact prefix (`F775..F795`), holdout regression repair (`21,383 -> 21,110`), and ROM `0x025B0E` / `0x058012` mutation guards (M16.44).
- `src/game/world/town_scene_loader.hpp` / `src/game/world/town_scene_loader.cpp`: Multi-screen ROM world loader (`load_world_screen_from_rom` for `0x003D`, `0x005A`, `0x005B`), autonomous ROM guard defense/AI timer initialization (`make_guard_from_rom`), Plane A nametable population (`populate_plane_a_from_scene`), and checkpoint-free VDP cold-boot initializer (`initialize_native_vdp_from_rom`) (M16.44).
- `tests/m16_43_f791_sat_divergence_test.cpp`: Automated M16.43 test qualifying elimination of F791..F795 divergences (0 diff pixels, 0 diff VRAM across all 21 frames F775..F795), hit spark effect emission, hitstun jitter synchronization, full 14-snapshot regression parity, and 5 focused mutation guards (M16.43).
- `tests/m16_42_water_ripple_sat_scheduler_test.cpp`: Automated M16.42 test qualifying elimination of F787 and F788 divergences (0 diff pixels, 0 diff VRAM), continuous 16-frame bit-for-bit exact prefix expansion (F775..F790), universal Pattern RAM accuracy across F775..F795, first post-prefix divergence at F791 (123 diff pixels), full 14-snapshot regression parity, and 5 focused mutation guards (M16.42).
- `tests/m16_41_sword_attack_dma_test.cpp`: Automated M16.41 test qualifying sword attack animation reconstruction, dynamic DMA queues, 12-frame exact prefix, and mutation guards (M16.41).

- `tests/m16_37_frame_aligned_pixel_exact_test.cpp`: Automated M16.37 test qualifying frame-aligned simultaneous native-vs-original VDP and RGB24 pixel comparison across 14 snapshot pairs (canonical F775..F1115 + holdout F775..F1125), relabeling M16.36's 17,698 bytes as temporal change, verifying ROM `0x00A380` DMA queue contracts, validating 402 hardware sprites, and running negative mutation guards (M16.37).
- `tests/m16_36_vram_and_entity_matrix_test.cpp`: Automated M16.36 test qualifying temporal VRAM classification across 14 snapshots (17,698 differing bytes relative to F775), first-cause attribution, hardware-visible sprite evaluation contract (14/14 snapshots exact, 402 sprites), complete 20-entity slot matrix across F776..F1125 (51,476 field comparisons matched 100%), PRNG call-order convergence, canonical 521 replay regression prevention, and negative mutation guards (M16.36).
- `cmake/project_tests_m16.cmake`: Modular CMake test registration file containing target declarations for M16 tests (oasis_m16_27 through oasis_m16_37) keeping `cmake/project_tests.cmake` strictly <= 500 lines (M16.37).
- `tests/m16_35_prng_convergence_test.cpp`: Automated M16.35 test qualifying PRNG call-order convergence (350/350 frames), Manhattan distance decision metric, rooftop guard drop physics, pre-move perception cadence, holdout entity parity, and VRAM differentials (M16.35).

- `tests/pixel_differential_test.cpp`: Automated scanline-by-scanline RGBA32 multi-layer pixel differential test comparing native VDP rasterization against genuine GPGX Waterbox video output, establishing 98.03% pixel match (70,268 / 71,680 pixels) on Frame 1 within 1 DAC step (M16.8).
- `tests/alternative_trace_test.cpp`: Unit test evaluating alternative input journal trajectory (Left -> Stop -> Up -> Stop) across 61 frames, achieving 100% Plane B parity (M16.8).
- `data/standalone-fixtures/beach_alternative_trace_oracle.json`: Independent 61-frame ROM execution oracle fixture mined via GPGX libretro core under WSL for alternative input sequence (M16.8).
- `src/game/world/tilemap_streamer.hpp` / `src/game/world/tilemap_streamer.cpp`: Authentic Beyond Oasis Plane A dynamic tilemap streamer implementing column streaming (routine 0x0023CC, 29 rows), row streaming (routine 0x002512, 41 words), 64x64 Screen {1, 0} beach map decompression, and 2-frame presentation pipeline delay, achieving 100.0% exact parity across all 121 frames on PLANE_A_EXACT (M16.7).
- `data/standalone-fixtures/beach_graphics_oracle_v3.json`: Versioned 121-frame graphics oracle fixture mined via GPGX libretro core under WSL with activated Waterbox render flags (`cinterface_render_bgw/bgb/bga/obj`), containing normalized 320x224 RGBA32 framebuffers ($286,720$ bytes), active SAT link chain count and hashes, Plane A and Plane B name table hashes, HUD sprite hashes, and SHA-256 hashes of Work RAM, SAT, and framebuffers (M16.7).
- `data/standalone-fixtures/beach_plane_a_map_64x64.bin`: Authentic 64x64 Screen {1, 0} beach Plane A tilemap (8,192 bytes) extracted from RAM at $00FF3FAC (M16.7).
- `src/game/render/hud_overlay.hpp` / `src/game/render/hud_overlay.cpp`: Authentic Beyond Oasis HUD overlay subsystem based on ROM routine 0x003FA0, allocating health and magic gauge frames and dynamic bar sprites across SAT slots 0..5 with 100.0% exact parity (M16.6).
- `src/game/render/sprite_compositor.hpp` / `src/game/render/sprite_compositor.cpp`: Modular C++20 sprite compositor implementing ISpriteCompositor, Sprite Mode 0 (0x00AD76, world-space entity mapping with camera subtraction and flips), Sprite Mode 1 (0x00A9A0, attached / weapon / child / mirror mapping), active hardware link chain traversal (link != 0 until link == 0), and big-endian Genesis VDP VRAM commit (M16.6).
- `data/standalone-fixtures/beach_canonical_120_oracle.json`: Independent 121-frame ROM execution oracle fixture mined via GPGX libretro core under WSL containing frame numbers, input polls, raw inputs, fixed and pixel coordinates, camera world coordinates, VDP register states, HScroll/VScroll values, Slot 7 entity state, active SAT chain count and hashes, Plane A and Plane B name table hashes, HUD sprite hashes, and SHA-256 hashes of Work RAM, SAT, and framebuffers (M16.6).
- `tests/differential_oracle_comparison_test.cpp`: Automated 121-frame 12-domain differential comparison test evaluating PLAYER_EXACT (121/121, 100%), CAMERA_EXACT (121/121, 100%), SCROLL_EXACT (121/121, 100%), ENTITY_EXACT (121/121, 100%), HUD_PIXELS_EXACT (121/121, 100%), PLANE_B_EXACT (121/121, 100%), SPRITE_PIXELS_EXACT (121/121, 100%), PLANE_A_EXACT (29/121, 24.0%), SAT_ACTIVE_CHAIN_EXACT, SAT_RAW_EXACT, VRAM_RELEVANT_EXACT, and FULL_FRAME_EXACT with ROM PC attribution and native C++ owner identification (M16.6).
- `src/game/camera/camera_controller.hpp` / `src/game/camera/camera_controller.cpp`: Native Beyond Oasis camera controller implementing authentic Work RAM struct coordinates ($FF1716 X, $FF171A Y starting at 96, 3), authentic 0x00CEEA acceleration formulas (deadzones [120..200]x[104..128], delta <= 3 px clamp, d0 - 30 accelerated catch-up), 30Hz execution cadence, 2-frame VBlank latch for VRAM 0xDC00 and VSRAM, and boundary clamping (M16.5).
- `src/genesis/vdp_state.hpp` / `src/genesis/vdp_state.cpp`: Explicit Mega Drive VDP hardware state model (registers, VRAM, CRAM, VSRAM, memory layout decoders).
- `src/genesis/vdp_rasterizer.hpp` / `src/genesis/vdp_rasterizer.cpp`: Native C++ VDP rasterizer compositing Plane A, Plane B, Window, Sprites, and Backdrop into RGB24 and RGBA32 buffers with support for `ColorDacMode::GenesisPlusGx` and `ColorDacMode::ModelFullRange`.
- `src/game/standalone_game_loop.hpp`: Decoupled simulation boundary interfaces (`FrameInput`, `SimulationState`, `IGameSimulation`, `FrameSequenceSource`).
- `src/game/render/sat_pipeline_contract.hpp` / `src/game/render/sat_pipeline_contract.cpp`: Verified Mega Drive VDP Sprite Attribute Table pipeline contract defining `MegaDriveSatEntry`, viewport culling (320x224), piece offsets, link chain termination, and big-endian serialization.
- `src/game/standalone_game_simulation.hpp` / `src/game/standalone_game_simulation.cpp`: Native gameplay simulation vertical slice advancing player state, reverse-engineered direction and speed physics, authentic 30Hz movement cadence, checkpoint restoration, camera tracking with 30Hz cadence gating, VDP plane scrolling, secondary scene entities, authentic Slot 7 chest interaction (routine 0x0063DA -> 0x006430), trajectory replay mode, and publishing updated SAT entries via `SatPipelineContract` to VDP state (M16.5).
- `src/tools/standalone_first_pixel.cpp`: Native VDP rasterizer verification CLI with `--dac`, `--fixture`, `--output`, `--compare`, and `--diff`.
- `src/tools/standalone_player.cpp`: Dual-mode (Replay / Simulation) windowed player using Win32 GDI `NativeWindow` with live controller input, 60 FPS pacing, pause/resume, step forward/backward, reset, mode toggling, camera tracking toggle, dynamic multi-entity and camera title bar HUD with tracking status (`Track=ON/OFF`), and `--headless` mode (M16.5).
- `tests/camera_controller_test.cpp`: Unit tests for camera initial state, authentic (96, 3) origin, deadzone, tracking and smoothing, boundary clamping, world-to-screen transforms, and VDP scroll table writes (M16.5).
- `tests/vdp_rasterizer_test.cpp`: Unit tests for VDP state parsing, plane priority, model and genuine emulator oracle matching, determinism, and RGBA32 formatting.
- `tests/standalone_sequencer_test.cpp`: Unit tests for sequence chronological ordering, sprite animation transition detection, pause/step state machine, and error handling.
- `tests/standalone_gameplay_test.cpp`: Unit tests for input nibble mapping, controller button edges, sub-pixel position integration, checkpoint initial state adapter, SAT pipeline contract integration, multi-entity updates and SAT emission, simulation determinism, 61-frame exact canonical beach oracle trajectory, rasterization of simulation output, 120-frame camera tracking simulation, and authentic chest interaction (M16.5).
- `tests/sat_pipeline_contract_test.cpp`: Unit tests for initial state, single piece packing, link chain traversal, clipping/culling, 80-sprite capacity limit, and shadow emission.
- `data/standalone-fixtures/`: Authentic frame state JSONs and reference `.rgb24` framebuffers for Frame 779, Frame 200, 17-frame sequence 770..786, canonical beach baseline (`beach_frame_000.json`), 60-frame canonical trajectory (`beach_canonical_60_frames.json`), and 121-frame multi-domain oracle (`beach_canonical_120_oracle.json`).

# M12 Controlled Entity / Player Provenance V1

`docs/reports/THOR_ACCEPTED_ARTIFACT_BACKLOG_20260928.md` inventories existing
accepted evidence that could contribute ROM properties through the separate
`ACCEPTED_ARTIFACT` layer. It records exact W5 resource results and separates
ROM-backed graphics candidates from runtime-only SAT/VDP evidence.

## M14 canonical full-ROM address map V1

`src/tools/thor_evidence/canonical_full_rom_map.py` is the semantic-neutral
full-ROM interval map API. It imports the existing emission partition, enforces
canonical ROM identity and exact ownership accounting, supports byte lookup,
boundaries, deterministic map operations, promotion gates, map deltas, and
unknown-range ranking.

`tests/canonical_full_rom_map_test.py` covers lookup, coverage-preserving
closure operations, promotion rejection, and fail-closed baseline import.

`src/tools/thor_evidence/m14_reconcile_ownership.py` locates an accepted
post-Stage7 generation, verifies the exact ownership transition and byte
roundtrips, and regenerates the canonical report from authoritative SQLite
emission rows. It rejects stale Stage7/full-ROM receipts and ownership
accounting mismatches.

`src/tools/thor_evidence/m14_runtime_validation.py` validates sealed M14.1
runtime receipts, computing ownership and coverage from the authoritative
SQLite emission table.

`src/tools/thor_evidence/knowledge_generation.py` records deterministic
derivations, proposal operations, and generation lineage in the canonical
knowledge SQLite schema.

## M12 generic recursive ROM closure V1

- `src/tools/thor_evidence/generic_recursive_closure.py` — normalized
  post-run, subsystem-neutral provenance graph, register/memory backtrace,
  call/table discovery, fail-closed queue, exact range round-trip promotion,
  and deterministic artifact emission.
- `tests/generic_recursive_closure_test.py` — synthetic generic regression for
  RAM selector → indexed ROM table → ROM value → A0 → renderer/SAT tail and
  unresolved-evidence queue behavior.

## M13 generic recursive closure live integration V1

- `tools/bizhawk-native-ring/generic_flow_normalizer.py` — converts native
  FLOW V2 records into versioned, subsystem-neutral instruction, memory,
  ROM-read, control-flow, call, return and indirect-target collections while
  reporting evidence fields absent from the native ABI.
- `src/tools/thor_evidence/rom_read_consumer_index.py` — imports canonical-ROM
  receipt-bound FLOW V2 bus reads into a deterministic per-read JSONL corpus
  and reader-to-ROM-range index; caller ancestry remains explicitly unknown.
- `tests/rom_read_consumer_index_test.py` — synthetic FLOW import, physical
  mapping and duplicate event regression.
- `tools/bizhawk-native-ring/live_forward_generic_closure_stage.py` — seals a
  normalized FLOW corpus, checks canonical ROM identity, runs closure, emits
  the M13 receipt and deterministic capture-gap ranking.
- `tools/bizhawk-native-ring/generic_closure_handoff.py` — coordinator handoff
  that continues to ASM closure on PASS/NO_DELTA/SKIPPED results and stops on
  integrity failure.
- `tools/bizhawk-native-ring/pipeline_outcomes.py` — typed stage result and
  scheduling-effect policy; preserves semantic unresolved reasons separately
  from fatal failures.
- `tools/bizhawk-native-ring/semantic_stage_scheduler.py` — executes ordered
  semantic stages, continues recognized unresolved outcomes into generic
  closure, and returns fatal stops without downstream scheduling.
- `tools/bizhawk-native-ring/live_forward_cleanup_acceptance.py` — verifies
  generic-closure deterministic replay and gap-ranking receipts before raw-FLOW
  reclaim is allowed.
- `tests/live_forward_generic_closure_stage_test.py` — sealed-corpus identity,
  missing-corpus and zero-promotion gap-plan regressions.
- `tests/generic_flow_normalizer_test.py` — FLOW V2 identity, CPU/domain,
  opcode verification, memory linkage, control-flow and explicit missing
  register-snapshot coverage.
- `docs/reports/THOR_M13_3_GENERIC_EVIDENCE_CONTRACT_V2_V1.md` — live V2
  coverage, closure metrics, retained evidence identity, and unresolved
  acceptance gaps.
- `tests/live_forward_controlled_entity_stage_test.py` — semantic unresolved
  continuation through generic/ASM/full audit/cleanup and fatal-stop boundary.
- `tests/live_forward_postrun_progress_test.py` — distinct unresolved terminal
  UI state and truthful final pipeline labeling.
- `tests/live_forward_absorption_cleanup_test.py` — requires validated generic
  closure, ranked-gap and deterministic-replay artifacts before deletion.
- `docs/reports/THOR_M13_2_GENERIC_CLOSURE_GAP_DRIVEN_IMPROVEMENT_V1.md` —
  artifact-derived baseline, replay metrics, blocker ranking, and next-capture
  plus raw-retention contract.
- `docs/reports/THOR_PER_READ_ROM_CONSUMER_EVIDENCE_20261001.md` — restored
  receipt-bound per-read ROM consumer evidence, hashes, target intersection,
  and capture limitations.
- `docs/reports/carver-family-index-v1.json` — fail-closed exact Carver family
  membership index and reader intersections; unresolved families retain no
  synthesized intervals.
- `docs/reports/carver-family-index-v2.json` — audit of historical Carver
  family-ID derivation and blocked independent re-derivation, with provenance
  and a separate reader-driven pivot candidate.
- `docs/reports/THOR_002888_CONSUMER_CONTRACT_20261001.md` — static and
  runtime audit of the reusable one-word queue consumer at `0x002888`, kept
  separate from any UNKNOWN-family parser or classification claim.

## Structural consumer-family evidence plane

- `src/tools/thor_evidence/static_family_direct_root.py` — versioned removable
  structural ledger for exact direct-literal ROM consumer roots.
- `src/tools/thor_evidence/gfx3820_3c07c_structural_closure.py` and
  `tests/gfx3820_3c07c_structural_closure_test.py` — two-run machine-code
  source-chain verification, exact `0x3820` boundary accounting, direct-root
  contribution and removal replay.
- `docs/reports/THOR_GFX3820_3C07C_STRUCTURAL_CLOSURE_20261004.md` and
  `docs/reports/THOR_GFX3820_3C07C_STRUCTURAL_20261004/` — accepted 8,564-byte
  structural contribution, A/B source-chain artifacts, before/after/removal
  views, and closure receipt.

- `docs/reports/THOR_STRUCTURAL_FAMILY_RANKING_AFTER_FIXED1208_REJECTION_20261004.md` —
  pre-closure ranking snapshot against the 1,240,094-byte AUTO52 view; records
  FIXED_1208 physical-mapping rejection and later GFX3820_3C07C closure.

- `docs/reports/THOR_STRUCTURAL_FAMILY_RANKING_POST_AUTO51_20261002.md` — exact
  candidate unions re-intersected with the accepted 1,207,195-byte view;
  selects AUTO8 by unique delta per estimated proof cost.

- `src/tools/thor_evidence/gfx_static_stream_pointers_auto51_closure.py` —
  closes the finite AUTO51 pointer inventory with exact ROM references,
  independent A/B enumeration, decoder ends, overlap, and removal replay.
- `tests/gfx_static_stream_pointers_auto51_closure_test.py` — AUTO51 census,
  pointer membership, ledger integrity, and removal acceptance checks.
- `docs/reports/THOR_GFX_STATIC_STREAM_POINTERS_AUTO51_20261002/` and
  `docs/reports/THOR_GFX_STATIC_STREAM_POINTERS_AUTO51_20261002.md` —
  machine census, accepted contribution, structural views, and closure report.

- `src/tools/thor_evidence/m68k_auto50_multi_resource_closure.py` — closes
  AUTO50's three descriptor/table/direct-loader mechanisms with deterministic
  A/B enumeration, Python/native decoder agreement, exact overlap, and
  independently removable contributions.
- `tests/m68k_auto50_multi_resource_closure_test.py` — AUTO50 census,
  accounting, source evidence, and full/independent removal acceptance tests.
- `docs/reports/THOR_M68K_AUTO50_STRUCTURAL_20261002/` and
  `docs/reports/THOR_M68K_AUTO50_STRUCTURAL_20261002.md` — machine censuses,
  accepted subfamily contributions, structural views, and closure report.

- `src/tools/thor_evidence/static_family_descriptor_stream.py` — v3 removable
  ledger schema for exact descriptor records plus variable-length decoder
  streams and explicit provenance edges.
- `src/tools/thor_evidence/gfx_descriptor_streams_auto49_closure.py` — fresh
  A/B AUTO49 descriptor census, independent parser boundary checks, exact
  overlap accounting, and removal replay.
- `tests/gfx_descriptor_streams_auto49_closure_test.py` — acceptance and
  fail-closed schema regressions for the AUTO49 family.
- `docs/reports/THOR_GFX_DESCRIPTOR_STREAMS_AUTO49_20261002/` — machine
  enumeration, accepted contribution, structural views, and closure receipt.

- `docs/reports/THOR_STRUCTURAL_FAMILY_RANKING_POST_1M_20261002.md` — historical
  pre-AUTO49 ranking against the then-current 1,030,892-byte view; retained for
  chronology and reproduction, not a current ranking authority.
- `docs/reports/THOR_STRUCTURAL_FAMILY_RANKING_POST_AUTO49_20261002.md` — exact
  remaining-family intervals re-intersected with the 1,103,176-byte view;
  selects AUTO50 by unique delta per estimated proof cost.
- `docs/reports/THOR_STRUCTURAL_FAMILY_RANKING_POST_AUTO50_20261002.md` — exact
  candidate unions re-intersected with the accepted 1,179,195-byte view;
  selects AUTO51 by unique delta per estimated proof cost.
- `docs/reports/THOR_STRUCTURAL_FAMILY_RANKING_POST_AUTO8_20261002.md` — exact
  remaining-family intervals re-intersected with the 1,228,211-byte view;
  selected AUTO52, now accepted.
- `docs/reports/THOR_STRUCTURAL_FAMILY_RANKING_POST_AUTO52_20261003.md` — exact
  remaining-family intervals re-intersected with the accepted 1,240,094-byte
  view; ranking and stale-baseline audit. Its FIXED_1208 candidate was later
  rejected by the source-backing audit.
- `src/tools/thor_evidence/fixed_1208_source_audit.py` — independent raw-root
  and decoded-instruction census for the eight 1,208-byte candidate slots;
  fails closed when logical address backing is not physical ROM.
- `tests/fixed_1208_source_audit_test.py` — checks A/B equality, exact
  candidate arithmetic, and rejection of unresolved SRAM-overlaid mapping.
- `docs/reports/THOR_FIXED_1208_SOURCE_AUDIT_20261004/` and
  `docs/reports/THOR_FIXED_1208_SOURCE_AUDIT_20261004.md` — machine A/B rows,
  source hashes, exact candidate ranges, SRAM mapping blocker, and closure
  outcome; no contribution was accepted.

- `docs/reports/THOR_LARGE_STRUCTURAL_FAMILY_CANDIDATES_20261001.md` — ranked
  exact structural-family candidates through post-AUDIO_MODE1 re-ranking;
  Family #4 `GFX_LEVEL_RESOURCE_FIELD1` was the latest TOP-1 and is now closed.

- `src/tools/thor_evidence/static_family_evidence.py` — provenance-bound,
  independently removable `STATICALLY_ENUMERATED_CONSUMER_SOURCE` ledger and
  derived range view; never inputs the runtime property map, `TOTAL_PROVEN`,
  canonical ownership, or `SOURCE_OWNED`.
- `tests/static_family_evidence_test.py` — deterministic generation, exact
  physical union, fail-closed contract validation, source hash, mixed-ROM,
  authority-separation, and removal/rebuild regressions.
- `src/tools/thor_evidence/static_family_variable.py` — versioned variable-
  length family contribution, exact range union, provenance graph validation,
  source verification, derived view, and independent removal support.
- `src/tools/thor_evidence/gfx_screen_roots_structural_closure.py` —
  deterministic A/B census verifier and `GFX_SCREEN_ROOTS` family closure.
- `tests/static_family_variable_test.py` — exact interval, deterministic hash,
  ambiguous descriptor, boundary, and root-domain regression cases.
- `docs/reports/THOR_GFX_SCREEN_ROOTS_20261001/` — Debug/Release census
  inputs, per-stream machine enumeration, accepted contribution, before/after
  and removal views, and closure acceptance report.
- `docs/reports/THOR_GFX_SCREEN_ROOTS_STRUCTURAL_CLOSURE_20261001.md` — human
  summary of exact family accounting, provenance, and validation.
- `src/tools/thor_evidence/static_family_table_variable.py` — v3 table-rooted
  variable-length contribution validator and removable structural view.
- `src/tools/thor_evidence/audio_mode0_structural_closure.py` — independent
  descriptor-census regeneration, exact Mode-0 boundary proof, overlap
  accounting, and removal replay.
- `tests/static_family_table_variable_test.py` — selector completeness,
  exact length boundary, descriptor uniqueness, provenance hash, and removal
  regressions.
- `docs/reports/THOR_AUDIO_MODE0_STRUCTURAL_20261002/` — dual parser census,
  67-resource enumeration, accepted contribution, structural views, and
  machine acceptance report.
- `docs/reports/THOR_LARGE_STRUCTURAL_FAMILY_2_20261002.md` — candidate rank,
  proof-cost ratio, exact contract, and closure result for family #2.
- `src/tools/thor_evidence/static_family_pointer_table.py` — v4 absolute
  pointer-table ledger for variable-length parser-terminated streams; validates
  exact table census, target mapping, boundary evidence, provenance, source
  hashes, and removable structural views alongside v1/v2/v3 contributions.
- `src/tools/thor_evidence/gfx3820_indexed_pointer_closure.py` — canonical
  ROM table and `0x00D3B2` opcode-contract validation, 0x3820 exact
  source-consumed A/B enumeration, overlap accounting, source verification,
  and byte-exact removal replay for the 107-stream family.
- `tests/gfx3820_indexed_pointer_closure_test.py` — exact census, structural
  accounting/removal, missing-boundary, pointer/provenance, and hash-drift
  fail-closed cases.
- `docs/reports/THOR_GFX3820_INDEXED_STRUCTURAL_20261002/` — two matching
  complete censuses, 107-resource machine enumeration, accepted contribution,
  before/after/removal structural views, and closure receipt.
- `docs/reports/THOR_GFX3820_FILE_BACKEND_ASSESSMENT_20261004.{md,json}` —
  discovery-only assessment of the accepted 107-ID graphics family as a
  future file-backed native resource-loader target; stores hashes and ranges,
  not extracted assets.
- `docs/reports/THOR_CROSS_POOL_OBJECT_TARGET_QUERY_CANDIDATE_20261004.md` —
  raw `0xB922` cross-pool query contract, native replacement boundary, and
  differential-oracle limitations.
- `docs/reports/THOR_B922_ORACLE_ATTEMPT_AND_ANIMATION_PIVOT_20261004.md` —
  bounded full-oracle attempt result, static CFG evidence, animation pivot
  limits, and unchanged authority totals.
- `build/b922-oracle-attempt/fixed-count-frontier.json` — ignored summary of
  the accepted fixed-count record family's exact ID/alias and parser contract
  as the next externalization candidate.
- `build/b922-oracle-attempt/` — ignored bounded M68K slice plus fresh
  selector/descriptor and relative-table analysis artifacts for the single
  `0xB922` oracle attempt and subsequent animation-discovery pivot.
- `src/tools/generate_gfx_screen_roots_manifest.py` — validates the accepted
  root/descriptor/stream census and emits the provenance-bound screen-root
  manifest draft.
- `tests/gfx_screen_resource_reference.cpp` — finalizes decoder hashes with
  the native 0x3820 decoder and verifies all ROM fallbacks, exact-copy file
  overrides, and invalid-override fallback.
- `docs/reports/THOR_GFX_SCREEN_ROOTS_FILE_BACKEND_20261004.md` — accepted
  stream-ID contract, alias accounting, portability acceptance, authority
  limits, and next direction.
- `docs/reports/THOR_LARGE_STRUCTURAL_FAMILY_3_20261002.md` — exact acceptance
  accounting and provenance summary for the indexed graphics family.
- `src/tools/thor_evidence/static_family_gfx_level_field1.py` — validates the
  exact 99-row field1 table census and the removable 35-stream accepted ledger.
- `src/tools/thor_evidence/gfx_level_resource_structural_closure.py` — runs
  independent native decoder censuses, checks field1 target/alias mapping,
  pins parser boundaries and source hashes, and emits exact overlap and removal
  replay views for Family #4.
- `tests/gfx_level_resource_structural_closure_test.py` — table pointer drift,
  parser boundary disagreement, alias accounting, and claim-boundary regressions.
- `docs/reports/THOR_GFX_LEVEL_RESOURCE_FIELD1_20261002/` — identical A/B
  machine censuses, 35-row enumeration, accepted evidence contribution,
  before/after/removal views, and machine closure receipt.
- `docs/reports/THOR_LARGE_STRUCTURAL_FAMILY_4_20261002.md` — Family #4 exact
  boundary, overlap, deterministic regeneration, and removal acceptance report.
- `src/tools/thor_evidence/static_family_fixed_count.py` — validates the
  removable structural evidence ledger for the 99-entry count-bounded M68K
  record family without entering the runtime property plane.
- `src/tools/thor_evidence/fixed_count_record_structural_closure.py` — performs
  independent AUTO11/raw-table enumerations, pins source hashes, recomputes
  structural overlap, and verifies removal replay for
  `M68K_FIXED_COUNT_RECORD_FIELD0`.
- `tests/fixed_count_record_structural_closure_test.py` — validates the
  emitted census/contribution contract and fail-closed mutations.
- `tests/fixed_count_resource_reference.cpp` — canonical-ROM acceptance for
  96 row resolutions, 78 stream fallbacks, exact copies, aliases, and rejection
  fallback cases.
- `docs/reports/THOR_M68K_FIXED_COUNT_FILE_BACKEND_20261004.md` — portable
  loader contract, exact-copy acceptance, and authority separation.
- `docs/reports/THOR_MAIN_POOL_SELECTOR_FIXED_COUNT_LINK_20261004.md` — static
  main-pool selector to fixed-count stream/consumer-field linkage.
- `docs/reports/THOR_M68K_FIXED_COUNT_FIELD0_20261002/` — A/B censuses,
  machine enumeration, accepted structural contribution, structural views,
  and closure receipt for the fixed-count family.
- `docs/reports/THOR_M68K_FIXED_COUNT_FIELD0_20261002.md` — exact family
  accounting, boundary contract, and authority-separation summary.
- `src/tools/thor_evidence/gfx_direct_static_structural_closure.py` — validates
  three static source roots, A0-preserving `0x3820` call chains, independent
  decoder boundaries, family accounting, and removal replay for the direct
  graphics stream family.
- `tests/gfx_direct_static_structural_closure_test.py` — checks identical
  independent censuses, exact provenance edges, structural delta, source hashes,
  and byte-exact removal receipts.
- `docs/reports/THOR_GFX_DIRECT_STATIC_CONSUMER_STREAMS_20261002/` — A/B
  machine censuses, exact enumeration, accepted structural contribution,
  resulting/removal views, and closure receipt.
- `docs/reports/THOR_GFX_DIRECT_STATIC_CONSUMER_STREAMS_20261002.md` — exact
  direct-root/continuation contract, instance boundaries, and claim limits.

`src/tools/m12_dynamic_sat_shadow_discovery.py` is the bounded, payload-free
M12 analyzer for distinguishing fixed/template writes to the accepted
`0x00FF13CC` SAT shadow from post-template dynamic patches and for inventorying
alternative SAT DMA groups. It emits the five `postrun_*` JSON artifacts under
ignored `build/thor-evidence/` output directories.
`tests/m12_dynamic_sat_shadow_discovery_test.py` covers writer-contract and
callback-alias fail-closed behavior. The result report is
`docs/reports/THOR_M12_DYNAMIC_SAT_SHADOW_MUTATION_DISCOVERY_V1.md`.

`docs/reports/THOR_M12_UPSTREAM_CONTROLLED_OBJECT_DISCOVERY_V1.md` records the
fail-closed upstream classification of `0xFF13CC` as a SAT shadow buffer. It
separates the exact fixed ROM render-record producer and DMA/SAT relation from
the still-unproven gameplay RAM object, X/Y source, and input provenance.

`src/tools/re_bizhawk_m12_targeted_controlled_entity.lua` is the bounded,
developer-only real BizHawk observer for the targeted controller/entity capture;
it records deliberate input transitions, frame-end events, PC-window register
snapshots, and selected bus events while keeping Worker capture disabled.
`tools/bizhawk-native-ring/run_targeted_controlled_entity_capture.py` launches
it in an isolated install, and
`tools/bizhawk-native-ring/analyze_targeted_controlled_entity_capture.py`
performs the fail-closed post-run acceptance audit. Raw capture artifacts remain
under ignored `build/thor-evidence/` directories.

`tools/bizhawk-native-ring/live_forward_controlled_entity_stage.py` is the
developer-only fail-closed stage after `GAMEPLAY RAM / ENTITY CANDIDATES` and
before `ASM CLOSURE`. It consumes accepted gameplay candidate/SAT artifacts and
preserved FLOW records, identifies exact controller-port and input-RAM events,
requires same-control-flow input consumers before accepting an entity update,
recovers only evidence-backed fields, joins proven RAM→SAT→DMA/hardware pieces,
and gates `CONTROLLED_ENTITY`/`PLAYER` labels. It emits
`postrun_controlled_entity_analysis.json`, `postrun_controlled_entity_fields.json`,
`postrun_input_entity_chains.json`, `postrun_controlled_entity_sat_chain.json`,
and `postrun_controlled_entity_receipt.json`; it never changes SOURCE_OWNED.
`tests/live_forward_controlled_entity_stage_test.py` covers event identity,
exact input consumers, field/SAT continuation, slot-reuse fail-closed behavior,
the PLAYER gate, deterministic analysis, zero ownership delta, and automatic
stage ordering.

# M12 Post-Run Gameplay RAM / Entity Candidates V1

`tools/bizhawk-native-ring/live_forward_gameplay_stage.py` consumes preserved
FLOW V2 M68K RAM bus evidence plus accepted Sprite/SAT provenance to discover
fail-closed RAM structure candidates, exact or hypothesized strides, runtime
slot lifetimes, update-loop witnesses, RAM-to-SAT field chains, and
candidate-to-hardware-sprite relations. It emits six deterministic post-run
artifacts and keeps SOURCE_OWNED unchanged.
`tests/live_forward_gameplay_stage_test.py` covers stride proof, slot iteration,
lifetime/reuse, exact and partial RAM-to-SAT provenance, one-to-many ownership,
overlap deduplication, label exclusion, deterministic receipts, zero ownership
delta, and automatic coordinator ordering.

# M12 Post-Run Sprite / SAT Analysis Stage V1

`tools/bizhawk-native-ring/live_forward_sprite_stage.py` implements the repository-native post-run sprite and SAT analysis stage (placed between Stage 8 `VDP / DMA ANALYSIS` and Stage 10 `ASM CLOSURE`), consuming preserved W3 V2 FLOW records, decoding Genesis Sprite Attribute Table (SAT) entries and link lists, handling H40/H32 mode base addresses, tracking scanline raster limits (20 sprites / 320 pixels per line, X=0 masking, clipping), tracking exact DMA-to-SAT provenance, verifying regression against contract and durable real oracle `sprite_oracle_v1_f779` (frame 779) with 0 conflicts, enforcing zero SOURCE_OWNED delta, and emitting the 6 authoritative artifacts (`postrun_sprite_analysis.json`, `postrun_sat_state.json`, `postrun_sprite_frames.json`, `postrun_sprite_provenance.json`, `postrun_sprite_candidates.json`, `postrun_sprite_receipt.json`).
`tools/bizhawk-native-ring/capture_real_sprite_oracle.py` provides standalone capture and verification tooling for the durable real sprite oracle, extracting hardware VRAM/CRAM/VSRAM/SAT state from BizHawk and generating oracle manifests and receipts.
`tests/live_forward_sprite_stage_test.py` covers 19 comprehensive unit tests (Tests A–S) verifying exact SAT base decode, entry decode, link traversal, termination, cycle detection, partial SAT handling, direct vs persisted provenance, DMA-to-SAT linkage, worker deduplication, sprite geometry, flip, palette, clipping, scanline raster limits, no logical entity naming, deterministic receipts, zero source-owned delta, automatic pipeline progression, and durable real sprite oracle regression.

# M12 Post-Run VDP / DMA Analysis Stage V1

`tools/bizhawk-native-ring/live_forward_vdp_stage.py` implements the repository-native post-run VDP and DMA analysis stage (placed between Stage 7 `AUDIO ANALYSIS` and Stage 9 `ASM CLOSURE`), consuming preserved W3 V2 FLOW records, decoding Genesis VDP control commands, register state, DMA operations (68K_BUS, FILL, COPY), memory-to-VDP producer provenance, candidate ROM graphics ranges, verifying S8 frame-779 regression with zero conflicts, and emitting the 6 authoritative artifacts (`postrun_vdp_analysis.json`, `postrun_vdp_registers.json`, `postrun_vdp_dma.json`, `postrun_vdp_provenance.json`, `postrun_vdp_candidates.json`, `postrun_vdp_receipt.json`).
`tests/live_forward_vdp_stage_test.py` covers 14 comprehensive tests (Tests A–N) verifying W3 V2 record consumption, complete two-word VDP command decoding, incomplete command fail-closed handling, register write decoding, VRAM/CRAM/VSRAM target distinction, exact DMA parameter derivation, non-exact DMA classification, equality-only causality rejection, frame/segment identity enforcement, S8 frame-779 regression with 0 conflicts, candidate ROM range clustering without promotion, source-owned invariance, idempotent deterministic receipt emission, automatic pipeline progression, and the 500-line limit across all governed files.

# M12 Post-Run Audio Analysis Stage V1

`tools/bizhawk-native-ring/live_forward_audio_stage.py` implements the repository-native post-run audio analysis stage (placed between Stage 6 `CONTROL PROVENANCE` and Stage 8 `ASM CLOSURE`), consuming preserved W3 V2 FLOW records, performing exact resource recognition and W5 roundtrip decode/encode verification for accepted resources (Resource 1 `0x0BD540..0x0BF768` and Resource 2 `0x0BC95C..0x0BD540`), verifying Level 3 strict causal chains with provenance tags, clustering newly discovered banked ROM reads into non-promoted candidate ranges, and emitting the 4 authoritative artifacts (`postrun_audio_analysis.json`, `postrun_audio_candidates.json`, `postrun_audio_provenance.json`, `postrun_audio_receipt.json`).
`tests/live_forward_audio_stage_test.py` covers 11 comprehensive tests (Tests A–K) verifying clean execution, exact resource recognition, byte-exact W5 roundtrip, Level 3 strict causal provenance, candidate clustering without promotion, source-owned invariance, zero fallback defaults, deterministic receipts generation, pipeline progression, schema validity, and the 500-line limit across all governed files.

# M12 Post-Run Pipeline Modernization V1 receipts architecture

`tools/bizhawk-native-ring/live_forward_postrun_receipts.py` generates all eight authoritative post-run receipts (`postrun_map_refresh_receipt.json`, `postrun_control_provenance_receipt.json`, `postrun_audio_receipt.json`, `postrun_vdp_receipt.json`, `postrun_asm_closure_receipt.json`, `postrun_full_rom_audit.json`, `postrun_cleanup_receipt.json`, `postrun_final_receipt.json`) deterministically from pipeline artifacts without fallback defaults, authoritatively deriving Stage 7 promotion deltas and pipeline deltas.
`tests/live_forward_postrun_receipts_test.py` covers unit tests verifying deterministic generation, schema conformance, fail-closed validation, non-zero Stage 7 promotion derivation, and synthetic zero-promotion fail-closed semantics.

# M12 W5d functional audio validation and listening test V1

`src/tools/thor_evidence/w5_audio_functional.py` implements the functional audio validation and listening pipeline, including raw PCM and WAV preview export, timing analysis, checksum-conserved semantic mutation within the early-boot witness window, emulator execution, standalone prediction vs hardware DAC verification, and report generation.
`tests/w5_audio_functional_test.py` covers 11 focused functional tests (Tests A–K) verifying header rejection, exact stream boundaries, deterministic PCM SHA, isolation, WAV format fidelity, timing classification, semantic mutation encoding, temp ROM checksum conservation, modified emulator DAC stream prediction with 0 mismatches, and canonical ROM immutability.

# M12 W5c exact audio resource ownership and canonical promotion V1

`src/tools/thor_evidence/w5_audio_promote.py` implements the canonical promotion engine for reconstructed audio resources `AUDIO_RESOURCE_FORMAT_A_0001` and `AUDIO_RESOURCE_FORMAT_A_0002` into the canonical ROM knowledge map, handling set-theoretic overlap reconciliation, parent emission tri-split, KnowledgeStore insertion, idempotence validation, and map/report re-export.
`src/tools/thor_evidence/w5_audio_ownership_receipt.py` generates the 7 required W5c acceptance artifacts in `build/m12-w5-acceptance/`.
`tests/w5_audio_ownership_test.py` covers 20 unit and integration tests (Tests A–T) verifying ownership eligibility, overlap reconciliation, partition split, dry-run invariants, canonical tooling execution, idempotence, diff accuracy, fail-closed boundaries, and ADR compliance.

# M12 W5 exact audio resource format and round-trip V1

`src/tools/thor_evidence/w5_audio_format.py` provides the data model, delta lookup tables, bank base mapping, descriptor parsing, and canonical Intermediate Representation (IR) for Beyond Oasis non-linear delta-PCM audio (Format A).
`src/tools/thor_evidence/w5_audio_decode.py` implements pure deterministic decoding of compressed audio data into canonical IR tokens and PCM sample streams for both Mode 0 (standard) and Mode 1 (hold/interpolate).
`src/tools/thor_evidence/w5_audio_encode.py` implements pure deterministic encoding from canonical IR tokens back to byte-identical original ROM binary streams without reading original ROM bytes.
`src/tools/thor_evidence/w5_audio_pipeline.py` orchestrates bank descriptor discovery, resource selection, decode, encode, byte-identical round-trip verification, witness alignment, and emission of the 9 acceptance artifacts in `build/m12-w5-acceptance/`.
`tests/w5_audio_format_test.py` covers 18 unit and integration tests (Tests A–R) verifying format semantics, bank parsing, boundary proof, witness alignment, byte-identical round-trip, multi-bank generality, and gate audit.

# M12 W4 active audio resource analysis V1

`src/tools/thor_evidence/w4_audio_sink.py` tracks hardware audio ports (YM2612 ports 0x4000-0x4003, PSG port 0x7F11), enforces port-local address latch pairing (0x4000->0x4001, 0x4002->0x4003) without cross-port leakage, and identifies DAC data (reg 0x2A) and enable (reg 0x2B) registers.
`src/tools/thor_evidence/w4_z80_dataflow.py` tracks exact Z80 register, stack, and memory dataflow provenance, supporting instruction transfers with conservative termination on unsupported opcodes.
`src/tools/thor_evidence/w4_audio_provenance.py` implements the 4 causal levels (Level 0 Temporal, Level 1 Memory last-writer, Level 2 Dataflow, Level 3 Strict Causal Chain), driver hotspot analysis, ROM clustering, candidate resource classifications, and directed provenance graph generation.
`src/tools/thor_evidence/w4_audio_pipeline.py` provides the end-to-end pipeline generating the 9 canonical acceptance artifacts in `build/m12-w4-acceptance/`.
`tests/w4_audio_analysis_test.py` covers unit tests for port-local pairing, DAC/PSG identification, last-writer verification, Z80 dataflow provenance, and artifact generation.

# M12 W3 Z80 co-capture + cross-CPU timeline

`tools/bizhawk-native-ring/live_forward_z80_trace.c` implements native Z80 instruction
capture and Z80 bus event sidebands (RAM, banked ROM, YM2612, PSG, bank register).
`tools/bizhawk-native-ring/genesis-plus-gx-z80-cocapture-v1.patch` and
`tools/bizhawk-native-ring/bizhawk-2.11.1-z80-cocapture-v1.patch` contain the external
GPGX and BizHawk patch chains for Z80 execution loop, memory, and bank hooks.
`tests/live_forward_z80_test.cpp` provides the 22 required C tests A–V for Z80 capture,
banked ROM physical resolution, cross-CPU master_time ordering, and M68K regression.
`src/tools/thor_evidence/w3_z80_evidence.py` provides lossless V2 48-byte record decoding,
stream iteration, and Genesis Z80 bank register tracking.
`src/tools/thor_evidence/w3_cross_cpu_timeline.py` builds the cross-CPU common master_time
timeline and validates exact causal interactions across 68K and Z80.
`src/tools/thor_evidence/w3_z80_classification.py` classifies low-level primitive facts
(Z80 RAM, Banked ROM, YM2612, PSG, 68K Z80-window writes) without song/instrument semantics.
`tests/w3_z80_evidence_test.py` validates the Python analysis layer with 13 unit tests.

# Runtime Z80 ROM execution-origin tracking

- `tools/bizhawk-native-ring/graphics_caller_census.c/.h` — bounded,
  non-authoritative runtime event buffer for attempted `0x3820` caller census.
  The current JSON integration is blocked; see the 2026-09-30 census report.

# Ephemeral graphics resource transaction

`tools/bizhawk-native-ring/graphics_transaction_bridge.c/.h` provide bounded
per-byte decoder-output origin tracking, overwrite invalidation and partial
RAM-to-VDP publication records. JSON receipt serialization is isolated in
`graphics_transaction_bridge_json.c`; `graphics_transaction_sha256.c/.h`
provide the local hash primitive used for decoder output and published spans.
`graphics_publication_path_diag.c/.h` hold a bounded recent-event diagnostic
for CPU VDP writes and DMA path families; it is observational only and does not
promote map properties. `tools/bizhawk-native-ring/export_graphics_transaction_tiles.py`
validates captured decoder/transfer hashes and exports only byte ranges that
were actually published into a grayscale tile diagnostic sheet.
`graphics_transaction_runtime.c` owns the emulator-facing bounded collector
instance and its narrow lifecycle/VDP API. Its caller-scoped observer records
only a `0x03C07C` call into `0x3820`, checking the immediate JSR handoff, saved
register/stack context, returned boundary and physical ROM mapping; a missing
caller hit remains unclassified. `apply_runtime_rom_properties_v1.py` installs
the required bridge sources and compile define into the diagnostic checkout.
`tests/graphics_transaction_bridge_test.c` validates deterministic hashes,
partial transfer linkage and byte-specific invalidation.

`src/tools/thor_evidence/graphics_resource_transaction.py` validates and
deterministically assembles one bounded 0x3820 -> RAM -> VDP -> SAT -> visible
sprite receipt. It fails closed on identity, ordering, range, transfer-content,
frame, publication-coverage, and visibility gaps; it does not mutate any map or
property authority. `tests/graphics_resource_transaction_test.py` covers
successful replay and core rejection conditions. Runtime evidence collection
has a host-tested collector. `runtime-build-inputs/graphics-transaction-lifecycle-v1/`
freezes the lifecycle collector sources and original overlay inputs.
`runtime-build-inputs/graphics-publication-path-diag-v1/` contains the bounded
publication-path diagnostic overlay and exact source snapshots. The current
collector retains completed decoder output across a bounded 120-frame
publication window, clearing origin on overwrite, run/reset/loadstate changes,
or timeout. Cross-frame matching is an intermediate diagnostic, not yet a
closed graphics resource receipt.

`tools/bizhawk-native-ring/rom_properties_z80_origin.c/.h` owns the bounded
Z80 RAM origin table, lifecycle invalidation, exact fetched-byte validation,
and completed-instruction origin batch. `rom_properties_runtime.c` applies
those validated offsets to the runtime property map; metrics remain in
`rom_properties_z80_metrics.c/.h`. The pinned GPGX patcher includes these
modules in the Waterbox build. The bounded live acceptance report and all
runtime-only checkpoints/contributions remain under ignored
`build/z80-origin-live-acceptance/`.

`tools/bizhawk-native-ring/live_forward_v3_abi.py` independently audits the
frozen Runtime V3 32-byte FLOW ABI against
`flow_schemas/runtime-v3-flow32.json`; historical 48-byte auditing remains in
`live_forward_scaling_audit.py`. `live_forward_v3_acceptance.lua` supplies the
V3 bounded runtime capture with ABI-correct API field indices.
`tests/live_forward_v3_abi_test.py` covers the known byte fixture, layout and
identity rejection, malformed lengths, capabilities and segment validation.
The format-audit report is `docs/reports/RUNTIME_V3_FLOW_FORMAT_AUDIT_20260929.md`.
The follow-up V3 runtime-gate receipt is
`docs/reports/RUNTIME_V3_ACCEPTANCE_CLOSE_20260929.md`; it records replay,
checkpoint, removable contribution, throughput and same-run audio blockers.

# M12 S8 proven logical VDP frame

`src/tools/re_bizhawk_m12_vdp_frame.lua` is a read-only BizHawk capture for
one coherent frame's ordered VDP register writes and complete VRAM/CRAM/VSRAM.
`src/tools/m12_vdp_frame_artifact.py` reuses the S1 tile/attribute decoder,
renders Plane A, Plane B, Window and backdrop, and composes the S7 sprite
raster with per-pixel provenance. `tests/m12_vdp_frame_artifact_test.py`
covers the A–U S8 contract. Generated capture and frame JSON remain ignored
runtime artifacts under `build/m12-gfx-runtime`.

# M12 S7 proven sprite scanline raster

`src/tools/m12_sprite_scanline_raster.py` reads the unchanged S6 frame and
observed VDP mode evidence, derives H32/H40 limits, evaluates each scanline
in SAT-link order, and emits `hardware-sprite-raster-v1` with logical and
hardware-filtered matrices plus per-pixel suppression provenance.
`tests/m12_sprite_scanline_raster_test.py` covers synthetic count, dot-budget,
transparency, offscreen, masking, flip, mode, determinism, and fail-closed
boundaries. The generated raster and mode-derived evidence remain ignored
runtime artifacts under `build/m12-gfx-runtime`.

# M12 S6 proven hardware sprite frame

`src/tools/m12_sprite_frame_artifact.py` consumes the S5 piece catalog,
validates a complete SAT-link traversal, and emits the deterministic
`hardware-sprite-frame-v1` JSON proof with exact tile/CRAM records, sprite-only
composition matrices, and pixel-level provenance. `tests/m12_sprite_frame_artifact_test.py`
covers the A–N fail-closed frame contract. The generated frame artifact is an
ignored runtime file under `build/m12-gfx-runtime`.

# M12 S5 SAT persistence and causality

`src/tools/m12_sprite_piece_catalog.py` now distinguishes
`DIRECT_PUBLICATION` from `PERSISTED_FROM_PUBLICATION`, proves SAT and shadow
persistence only under complete supported mutation coverage, and retains
entry-specific causal status. `tests/m12_sprite_persistence_test.py` covers
the A–L persistence contract. S5 mutation receipts are emitted by the
developer-only BizHawk capture and remain under `build/m12-gfx-runtime`.

# M12 S4 sprite evidence gap closure

`src/tools/m12_sprite_evidence_gap.py` produces the exact reason histogram,
frame/SAT-entry and shape distributions, before/after classification counts,
and measured capture-cost report. `tests/m12_sprite_evidence_gap_test.py`
covers the A–L gap-closure contract. The expanded S3/S4 capture records
bounded SAT-window DMA segments with source offsets and event sequences.

# M12 S3 hardware sprite piece catalog

`src/tools/m12_sprite_piece_catalog.py` classifies each `(run_id, frame,
SAT-entry)` observation as PROVEN, INCOMPLETE, OBSERVED_LINKAGE_ONLY, or
CONFLICT, preserving exact multi-tile/CRAM evidence and deterministic semantic
fingerprints. `tests/m12_sprite_piece_catalog_test.py` covers the A–L
fail-closed catalog contract. The generated catalog and capture remain ignored
runtime artifacts under `build/m12-gfx-runtime`.

# M12 S2 hardware sprite piece

`src/tools/re_bizhawk_m12_sprite_piece.lua` is a developer-only, read-only
BizHawk witness capture for one same-frame producer → shadow SAT → VRAM SAT →
VRAM tile → CRAM chain. `src/tools/m12_sprite_piece_artifact.py` validates the
capture and emits the compact `hardware-sprite-piece-v1` JSON proof using the
S1 Genesis decoder. `tests/m12_sprite_piece_artifact_test.py` covers the A–K
fail-closed contract.

# THOR M12 AUTO64.1 invariant repair

# THOR M12 AUTO65 multi-chain campaign

`src/tools/thor_evidence/auto65_chain.py` owns normalized chain construction,
structural equivalence, novelty classes, prefix reuse, conflict retention and
shared-prefix clustering. `auto65_campaign.py` owns fixed-point scenario
selection, anti-repeat fingerprints, static-before-runtime scheduling,
investigation persistence and replay accounting. The regression contract is
`tests/thor_evidence_auto65_test.py`, registered in `cmake/m12_auto2.cmake`.
The real campaign receipt and ignored DAG are under
`build/thor-evidence/auto65`; the published result is
`docs/reports/THOR_M12_AUTO65_MULTI_CHAIN_CAMPAIGN.md`.

# THOR M12 AUTO66 multi-scenario campaign

`src/tools/thor_evidence/auto66_campaign.py` owns the repository scenario
planner, equivalent-scenario rejection, bounded BizHawk discovery dispatch,
multi-scenario receipts and cumulative campaign metrics while reusing AUTO65's
chain engine. Its regression is
`tests/thor_evidence_auto66_test.py`, registered in
`cmake/m12_auto2.cmake`; the result is recorded in
`docs/reports/THOR_M12_AUTO66_MULTI_SCENARIO_CAMPAIGN.md` and ignored runtime
evidence is under `build/thor-evidence/auto66`.

# THOR M12 AUTO67 live opportunistic RE

`src/tools/thor_evidence/auto67_live.py` launches the canonical ROM with the
developer-only live sampler, owns the atomic Dispatcher, bounded rolling-window
accounting, free worker pool, claims/merges, and snapshot-backed operator view.
`src/tools/thor_evidence/auto67_window.py` renders that replaceable snapshot in
the parallel native operator window; it has no raw-event backlog or Dispatcher
access. `auto67_status.py` retains file publication and explicit browser
compatibility mode.
`auto67_runner.py` contains launcher/process orchestration so the Dispatcher
remains below the source-file limit; `auto67_profile.py` keeps bounded T0-T8
dispatch and claim-lock timing samples. The live capsule Lua path records
bounded frame spikes and filtered-hook installation timing.
`capture/auto67_hook_metrics.lua` adds bounded hook registration, callback
self-time, per-frame, and callback-budget diagnostics for AUTO67.2.
`src/tools/thor_evidence/capture/live_opportunistic.lua` performs only minimal
frame/bus sampling and fixed-ring updates inside emulator callbacks. The view
is served from the same launcher and reads `*.view.json`; it has no raw event
backlog and cannot claim or process work. `tests/thor_evidence_auto67_test.py`
covers claims, known/active collision handling, worker return, bounded overwrite,
and 1/8/16 worker configuration. The validation and evidence are recorded in
`docs/reports/THOR_M12_AUTO67_LIVE_OPPORTUNISTIC_RE.md`.
The dispatch-only timing proof is recorded in
`docs/reports/THOR_M12_AUTO67_DISPATCH_PROFILE.md` and its bounded JSON
counterpart; raw runtime receipts remain under `build/`.
AUTO67.2 hook scaling and hunt evidence are recorded in
`docs/reports/THOR_M12_AUTO67_2_HOOK_DIAGNOSIS.md` and its JSON proof.
`auto67_yield.py` is retained as historical AUTO67.2 code but is no longer
imported by the AUTO67 acceptance path. Its session-only yield classifications
and unresolved-context rejection are intentionally disabled. The native
operator window does not show those classifications; regression coverage for
the chain-store-first boundary remains in `tests/thor_evidence_auto67_1_test.py`.
The measured AUTO67.2 yield proof is
`docs/reports/THOR_M12_AUTO67_2_KNOWLEDGE_YIELD.md` and its bounded JSON
counterpart; the full ignored runtime artifact is under `build/`.

`auto67_persistence.py` contains `LiveMapSink`, the AUTO67-only single bounded
local-chain queue and RAM SQLite Cartographer writer. `map_merge.py` performs
the post-shutdown deterministic session-to-GLOBAL merge with an atomic replace.
The historical descriptor/Store
bridge is isolated in `auto67_legacy_persistence.py`; its consumers are
`tests/thor_evidence_auto67_3_test.py`,
`tests/thor_evidence_auto67_3_identity_audit.py` and
`tests/thor_evidence_auto67_4_test.py`. The live runner never imports that
module. The authoritative AUTO67.3 checkpoint is
  `docs/reports/THOR_M12_AUTO67_3_FINAL_CANONICAL_CHAIN_IDENTITY_CHECKPOINT.md`
  and its JSON counterpart; earlier AUTO67 iteration reports are historical.

`src/tools/thor_evidence/auto67_status.py` handles bounded, lossy status
publication and HTTP serving for the existing launcher. The AUTO67 regression
also exercises slow-disk isolation, nonblocking snapshot contention, and
bounded/detached history. `THOR_M12_AUTO67_LIVE_OPERATOR_TEST.md` and its JSON
under `docs/reports/` distinguish failed manual responsiveness from the
subsequent sparse-capture performance experiment and its coverage limitation.

# M12 AUTO67 live-forward RAM Cartographer 2A

`src/tools/thor_evidence/live_forward_cartographer.py` adapts only completed,
host-validated FLOW_V1 segments into a fresh RAM MAP-1 observed execution graph
and saves a closed SQLite session by backup. `live_forward_archivist.py`
validates and atomically seeds or merges those saved sessions using
`map_merge.py`. `tools/bizhawk-native-ring/live_forward_cartographer_runtime.py`
coordinates two isolated EmuHawk sessions and invokes the Archivist only after
each process exits; the optional callback in `live_forward_scaling_runtime.py`
is downstream of full segment validation and immediately before ACK.
`tests/live_forward_cartographer_test.py` covers observed edges, overlap,
branches, lineage, session round-trip, status preservation, merge idempotency
and fail-closed archive behavior. `tools/bizhawk-native-ring/live_forward_cartographer_audit.py`
independently reconciles every segment receipt against both session databases
and the merged master. Evidence and real BizHawk receipts belong in the ignored
`build/thor-evidence/live-forward-worker-2a/` campaign directory.

# M12 FLOW_V1 exact ROM range linkage 2B

`src/tools/re_rom_range_decode.cpp` is a developer-only batch adapter over the
existing bounded M68K decoder and explicit memory-address classifier.
`src/tools/thor_evidence/live_forward_rom_link.py` stages already-audited
FLOW_V1 bytes and, after EmuHawk exits, adds stable `ROM_INSTRUCTION_RANGE`
nodes, occurrence-lineaged `EXECUTED_FROM_ROM` claims and terminal
`OBSERVED_NEXT_PC` address facts to the 2A MAP-1 session. The one-run driver is
`tools/bizhawk-native-ring/live_forward_rom_link_runtime.py`; its independent
saved-result verifier checks every instruction occurrence and terminal fact
against raw segments, a fresh decode and canonical ROM bytes. Deterministic
fixtures are in `tests/live_forward_rom_link_test.py`, registered by
`cmake/m12_auto2.cmake`. Interval output and raw FLOW_V1 evidence belong only
under the ignored `build/thor-evidence/live-forward-rom-link-2b/` directory.
Implementation, runtime evidence and final audit are recorded in
`docs/reports/THOR_M12_ROM_RANGE_LINKAGE_2B.md` and its compact JSON receipt.

`live_forward_rom_link_runtime.py` gates post-run analysis on a sealed
interactive stop outcome; `tests/live_forward_rom_link_runtime_test.py` keeps
preflight rejection from opening a misleading analyzer failure window.

`docs/reports/THOR_M12_RUNTIME_CONTROL_PROVENANCE_2C.md` and its compact JSON
receipt record the Phase 0 classification of the saved 2B FLOW_V1 evidence.
The bounded result is `STOP_NO_RUNTIME_PROVENANCE_GAP`; no native runtime
sensor or per-memory-access trace was added.

# M12 canonical ROM knowledge map 2D

`src/tools/thor_evidence/rom_knowledge_map.py` owns the deterministic SQLite
schema, stable ROM-range/object identities, queries, metrics, hashes and
compact export. `rom_knowledge_sources.py` imports the accepted AUTO61
manifest, reconciles its ownership history, preserves Carver candidates as
hypotheses and imports qualified 2B facts. `rom_knowledge_import.py` builds
the ignored `build/thor-evidence/canonical-rom-knowledge-2d/knowledge.sqlite`
and the tracked JSON/Markdown checkpoint reports. The independent read-only
auditor is split between `rom_knowledge_audit.py` and
`rom_knowledge_audit_records.py`; focused deterministic query/idempotence tests
are in `tests/rom_knowledge_map_test.py`, registered by
`cmake/m12_auto2.cmake`. Runtime sessions, raw FLOW and the generated SQLite
map remain local under ignored `build/`.

`src/tools/thor_evidence/auto67_capsule.py` owns the fixed sixteen-slot,
128 KiB capsule pool, bounded command publication, capture freeze/reuse
metrics, and capsule status snapshots. `src/tools/thor_evidence/capture/live_capsule.lua`
owns the discovery ring, targeted hooks, per-frame callback budget, and frozen
capsule binary output. `tests/thor_evidence_auto67_1_test.py` covers exact pool
shape, the four-capture limit, freeze/reuse, dispatcher release, and bounded
control state. The experiment report is
`docs/reports/THOR_M12_AUTO67_1_128K_CAPSULE.md` with its bounded JSON proof.

AUTO67.4 adds `auto67_capsule_codec.py`, a bounded O67V v2 decoder with
legacy O67C compatibility and lease/capsule validation, and
`auto67_materializer.py`, the worker-side conversion of decoded runtime
observations into explicitly unresolved evidence fields. `auto67_capsule.py`
exposes frozen capsule metadata and rejects stale status snapshots; new
materialized results are persisted as `MATERIALIZED_CHAIN` while historical
seed-only rows remain `SEED_ONLY`. Regression coverage is in
`tests/thor_evidence_auto67_4_test.py`; the real short proof is recorded in
  `docs/reports/THOR_M12_AUTO67_4_FROZEN_CAPSULE_REAL_WORKER_CHAIN.md` and its
  bounded JSON counterpart.

AUTO67.5 extends `auto67_materializer.py` with schema-2 observed/causal
separation and a bounded generic M68K MOVE memory-write decoder. Its integrity
tests and real proof are recorded in
`tests/thor_evidence_auto67_4_test.py` and
`docs/reports/THOR_M12_AUTO67_5_CAUSAL_INTEGRITY_GATE.md` with the JSON proof.

AUTO67.6R2 extends `auto67_predecessor.py` with the O67P v2 bounded global
prehistory decoder and fail-closed reaching-definition resolver.
`capture/predecessor_capture.lua` owns one continuously active 4096-entry
execution ring and exact BUS_WRITE joins; `capture/live_capsule.lua` exposes
frozen slice metadata through the existing bounded status path.
`tests/thor_evidence_auto67_6_test.py` covers exact occurrence identity, latest
reaching writes, ROM-PC opcode recovery, generic indexed/PC operands, gaps,
truncation, overwrites, epoch mismatch, unsupported writers and generic
register selection. The R2 checkpoint is
`docs/reports/THOR_M12_AUTO67_6R2_CONTINUOUS_PREHISTORY.md` with its JSON
counterpart. The proof is positive for register evidence but records the
measured global-hook frame-time limitation; it does not claim smooth gameplay.

AUTO67.6R2's `auto67_predecessor.py` also validates continuous, complete,
audited FLOW_V1 intervals for runtime control provenance. The bounded
post-run replay is in `auto67_control_provenance.py`; exact 68000 EA/register
semantics are isolated in `auto67_control_provenance_decode.py`. The
independent occurrence/source/target auditor and BizHawk campaign driver are
`tools/bizhawk-native-ring/control_provenance_audit.py` and
`control_provenance_campaign.py`. `tests/thor_evidence_control_provenance_test.py`
covers exact pointer, offset, copy, selected-entry, generic-register and
fail-closed cases; CTest runs it. The checkpoint distinguishes the emulator
capability proof from the canonical-game witness in
`docs/reports/THOR_M12_RUNTIME_CONTROL_PROVENANCE_2E.md` and its JSON receipt.
Raw captures and per-segment outputs remain under ignored `build/`.

`src/tools/re_full_split_run.py` owns the canonical generated-layout alias
filter and deterministic full-layout writer; `src/tools/re_auto_promote.py`
delegates to it. `tests/re_full_split_test.py` and
`tests/re_auto_promote_test.py` cover equivalent/conflicting aliases and
deterministic layout output. `tests/check_file_limits.cmake` checks the
governed Git inventory under `cmake/`, `src/`, `tests/`, and root
`CMakeLists.txt` without walking ignored runtime/build trees. Evidence and
validation are recorded in
`docs/reports/THOR_M12_AUTO64_1_INVARIANT_REPAIR.md`.

# THOR Evidence Engine design

# AUTO64 Knowledge Coverage and scheduler

`src/tools/thor_evidence/auto64_knowledge_coverage.py` imports existing AUTO62,
AUTO63, runtime and bounded static artifacts into multidimensional fail-closed
coverage without changing ownership. `auto64_provenance.py` persists the
dependency graph and deterministic request, `auto64_provenance_analyze.py`
updates it from the focused capture, and `auto64_static_queue.py` consumes the
remaining bounded static obligations. Their focused developer-only capture is
`capture/auto64_caller_provenance.lua` with its PowerShell runner. Tests are
`tests/thor_evidence_auto64*_test.py`; ignored evidence is under
`build/thor-evidence/auto64-provenance`. The result is recorded in
`docs/reports/THOR_M12_AUTO64_KNOWLEDGE_COVERAGE.md`.

`docs/reports/THOR_EVIDENCE_ENGINE_ARCHITECTURE.md` specifies the proposed
developer-only M12 capture/provenance sidecar, relational schema, temporal
identity, proof obligations and non-owning Carver integration.
`docs/reports/THOR_EVIDENCE_ENGINE_STAGES.md` defines bounded V0–V9 goals,
acceptance tests, performance budgets, STOP gates and transition evidence.
The listed future source modules are a plan; no engine implementation is
introduced by these reports. See ADR-0044.

# THOR Evidence Engine V0 foundation

`src/tools/thor_evidence/identity.py` defines full-hash environment/location/
temporal identities. `events.py` validates the sealed JSONL transport,
`normalize.py` converts one bounded local BizHawk observation into that format,
and `report.py` emits the evidence-backed capability matrix and source receipts.
`receipt.py` creates/finalizes launch identity; `capture/run_v01.ps1` binds the
existing launcher to that receipt for the bounded repeat. The V0.1 collector
raw envelope is rejected before normalization when completion or identity is
wrong.
`store.py` and `schema.sql` provide the non-owning SQLite receipt, epoch, event,
value-version and UNKNOWN relation store. The one bounded collector is
`src/tools/thor_evidence/capture/capabilities.lua`; its raw output remains local
under ignored `build/thor-evidence/v0`. `tests/thor_evidence_v0_test.py` covers
the storage and fail-closed transport contract; CTest registration is in
`cmake/m12_auto2.cmake`.

# THOR Evidence Engine V1-GATE local FF13CC closure

`src/tools/thor_evidence/v1_gate.py` validates the bounded A372 static form,
selected ordered runtime pairing, four-byte local versions, writer-coverage
certificates and interruption certificates. It intentionally stops before a
provenance graph or general last-writer engine. `tests/thor_evidence_v1_gate_test.py`
and `tests/fixtures/thor_evidence_v1_gate/negative_cases.json` cover the
adversarial fail-closed cases; `cmake/m12_auto2.cmake` registers the test.
The gate result is recorded in
`docs/reports/THOR_EVIDENCE_ENGINE_V1_GATE.md`.

# THOR Evidence Engine V1-GATE-COVERAGE dense local closure

`src/tools/thor_evidence/capture/dense.lua` and `run_dense.ps1` use the
existing BizHawk receipt/raw envelope to capture the minimal A370..A374
execution interval through `event.on_bus_exec_any`. `src/tools/thor_evidence/dense_gate.py`
validates ROM-bound static decoding, concrete effective write ranges,
interruption continuity and the machine-readable coverage certificate.
`tests/thor_evidence_dense_gate_test.py` and
`tests/fixtures/thor_evidence_v1_gate_coverage/negative_cases.json` cover
incomplete, reordered, forged, duplicate-PC, loop and interruption cases.
The result is recorded in
`docs/reports/THOR_EVIDENCE_ENGINE_V1_GATE_COVERAGE.md`.

# Independent BizHawk infrastructure diagnosis

`docs/reports/BIZHAWK_CONTROLLED_HARNESS_DIAGNOSIS.md` records the verified
BizHawk 2.11.1 CLI/Lua controlled-state workflow, input API correction,
process coexistence and lag-frame oracle limitation. Local diagnostic scripts
and evidence remain ignored under `build/bizhawk-controlled-harness`.
`docs/reports/THOR_M12_CONTROLLED_RUNTIME_ROOT_CAPTURE.md` records the bounded
Right-versus-neutral register/root control and the re-ranked M12 forks.

# M12-GFX-MAX closes the screen graphics root table

`src/tools/re_m12_gfx_max_closure.py` and
`tests/re_m12_gfx_max_closure_test.py` validate the complete known screen
graphics root: the 21-entry group-pointer table, 167 descriptor uses and 159
exact decompressor streams. The transaction output remains ignored/local.

`src/tools/re_m12_gfx_loader_census.py` and
`tests/re_m12_gfx_loader_census_test.py` build the exact direct-xref census for
the `0x00D406` graphics loader and split screen-descriptor call sites from
unmatched direct calls without promoting bytes. The census also records bounded
forward-adjacency candidates, verifies the 15 known small 68000 continuation
forms, and records descriptor-shaped unmatched-call candidates; the generated
census remains ignored/local.

`src/tools/re_m12_gfx_37d2_census.py` and
`tests/re_m12_gfx_37d2_census_test.py` census the seven direct `0x37D2`
graphics-wrapper calls. Five exact ROM-source setups are checked against their
Ancient boundaries and existing manifest ownership; two inherited/post-state
sites remain blocked. The generated census is ignored/local and never promotes
bytes by itself.

`src/tools/re_m12_gfx_d3b2_census.py` and
`tests/re_m12_gfx_d3b2_census_test.py` close the exact indexed `0x00D3B2`
graphics-loader family: seven direct callers, the 108-entry root table, and
all 107 finite child streams. The generated census is ignored/local and
records already-owned bytes without promoting them.

`src/tools/re_m12_gfx_36d4_census.py` and
`tests/re_m12_gfx_36d4_census_test.py` census the six direct callers of the
`0x0036D4` wrapper, verify its bounded `0x36D4..0x372A` body and
`0x3820 -> 0x2CBC` edges, and retain its inherited-A0 source as an explicit
non-owning blocker.

`src/tools/re_m12_gfx_sibling_census.py` and
`tests/re_m12_gfx_sibling_census_test.py` classify the complete direct-xref
sets for the proven `0x00D950`, `0x002CBC`, and `0x002E1E` sibling helpers.
They verify each bounded body fingerprint and record the non-owning VDP/RAM
helper classification; generated output is ignored/local.

`src/tools/re_m12_gfx_d406_residual_census.py` and
`tests/re_m12_gfx_d406_residual_census_test.py` classify the five remaining
non-screen direct `0x00D406` callers after screen continuations and descriptor
families are removed. They verify exact bounded caller fingerprints and retain
inherited/zero-source blockers without promoting bytes; output is ignored/local.

`src/tools/re_bizhawk_m12_gfx_provenance.lua` captures only the ten unresolved
`0x3820` caller PCs and decoder entry under an existing frozen natural scenario.
`src/tools/re_m12_gfx_runtime_provenance.py` validates canonical identity,
byte-identical replays, ROM `A0`, exact Ancient ends, and manifest ownership;
runtime JSON and the provenance report remain ignored/local.

# M12-AUTO56 adds the overlapping PC-relative word-table promoter

`src/tools/re_m12_pc_word_overlap_promote.py` and
`tests/re_m12_pc_word_overlap_test.py` record the exact 16-word table at
`0x062DA8`, promoting only its unknown 24-byte prefix because the final four
words overlap the existing AUTO28 table. Generated transaction output remains
ignored/local.

# M12-AUTO55 adds the bounded 0x03B8DE table promoter

`src/tools/re_m12_table_03b8de_promote.py` and
`tests/re_m12_table_03b8de_test.py` record the exact eight-record,
16-byte-stride descriptor-table contract. Generated transaction output remains
ignored/local.

# M12-AUTO52 adds the exact direct-loader graphics-stream promoter

`src/tools/re_m12_direct_loader_stream_promote.py` and
`tests/re_m12_direct_loader_stream_test.py` record three direct-loader pointer
contracts and their exact decompressor boundaries. Generated transaction output
remains ignored/local.

# M12-AUTO51 adds the exact static-stream-pointer promoter

`src/tools/re_m12_static_stream_pointer_promote.py` and
`tests/re_m12_static_stream_pointer_test.py` record nine closed static pointer
stream contracts. Generated transaction output remains ignored/local.

# M12-AUTO50 adds the exact multi-resource-family promoter

`src/tools/re_m12_multi_resource_family_promote.py` and
`tests/re_m12_multi_resource_family_test.py` record repeated descriptor/table
families and direct loader streams. Generated transaction output remains
ignored/local.

# M12-AUTO49 adds the exact descriptor-stream promoter

`src/tools/re_m12_descriptor_stream_promote.py` and
`tests/re_m12_descriptor_stream_promote_test.py` record the ten closed
descriptor-backed graphics streams. Generated transaction output remains
ignored/local.

# M12-AUTO48..46 add exact dispatch promoters

`src/tools/re_m12_pc_relative_dispatch_promote.py`,
`src/tools/re_m12_multi_dispatch_targets_promote.py`, and
`src/tools/re_m12_static_dispatch_targets_promote.py`, with their paired tests,
record the closed dispatch/table contracts. Generated transaction output
remains ignored/local.

# M12-AUTO45 adds the exact indexed offset-table promoter

`src/tools/re_m12_indexed_offset_table_promote.py` and its deterministic
`tests/re_m12_indexed_offset_table_promote_test.py` record the closed
`0x00AD56..0x00AD76` table contract. Generated transaction output remains
ignored/local.

# M12-AUTO44 adds the runtime record-region promoter

`src/tools/re_m12_runtime_record_regions_promote.py` and its deterministic
`tests/re_m12_runtime_record_regions_promote_test.py` record the new exact
count-bounded regions selected by readers `0x00AF16` and `0x00ABB4`.
Generated transaction output remains ignored/local; already-owned AUTO11
regions are explicitly excluded from the new byte count.

# M12-AUTO43 adds the small caller-backed runtime-code promoter

`src/tools/re_m12_runtime_code_small_caller_promote.py` and its deterministic
`tests/re_m12_runtime_code_small_caller_promote_test.py` record only the
exact routine `[0x0083F8,0x00846C)`. Generated evidence and transaction
output remain ignored/local.

# M12-AUTO42 adds the caller-backed runtime-code promoter

`src/tools/re_m12_runtime_code_caller_promote.py` and its deterministic
`tests/re_m12_runtime_code_caller_promote_test.py` record only the exact
caller-backed routine `[0x03B358,0x03B486)`. Generated evidence and
transaction output remain ignored/local.

# M12-AUTO41 adds the developer-only bounded runtime-code promoter

`src/tools/re_m12_runtime_code_range_promote.py` and its deterministic
`tests/re_m12_runtime_code_range_promote_test.py` record only the exact
runtime-observed routine `[0x03AE74,0x03B092)`. Generated evidence and
transaction output remain ignored/local.

# M12-AUTO40 adds the developer-only runtime-code promoter

`src/tools/re_m12_runtime_code_promote.py` and its deterministic
`tests/re_m12_runtime_code_promote_test.py` record only the runtime-observed,
byte-exact routine `[0x00B79A,0x00B852)`. Generated evidence and transaction
output remain ignored/local.

# M12-AUTO39 adds the developer-only relative-selector promoter

`src/tools/re_m12_relative_selector_table_promote.py` and its deterministic
`tests/re_m12_relative_selector_table_promote_test.py` record only the exact
`0x15A9A6..0x15A9B0` selector table. Generated transaction output remains
ignored; selected payloads remain outside ownership.

# M12-AUTO38 adds the developer-only menu record-stream promoter

`src/tools/re_m12_menu_record_stream_promote.py` and its deterministic
`tests/re_m12_menu_record_stream_promote_test.py` record only the exact
`0x15B9D4..0x15BAC2` menu offset table and count-bounded streams. Generated
transaction output remains ignored.

# M12-AUTO37 adds the developer-only enum-lookup promoter

`src/tools/re_m12_enum_lookup_promote.py` and its deterministic
`tests/re_m12_enum_lookup_promote_test.py` record only the exact
`0x05CE56..0x05CE96` table. Generated transaction output remains ignored.

# M12-AUTO36 adds the developer-only menu-graphics stream promoter
`src/tools/re_m12_menu_graphics_promote.py` and its deterministic regression
`tests/re_m12_menu_graphics_promote_test.py`. It records only the three exact
`0x15BAC2..0x15CEA0` stream intervals; the `0x15CA9B` separator and generated
transaction output remain outside ownership.

# M12-AUTO35 adds the developer-only bit-7 lookup promoter
`src/tools/re_m12_bit7_lookup_promote.py` and its deterministic regression
`tests/re_m12_bit7_lookup_promote_test.py`. It records only the exact
`0x05CE16..0x05CE56` table; generated transaction output remains ignored.

# M12-AUTO34 adds the developer-only resource-pointer promoter
`src/tools/re_m12_resource_pointer_table_promote.py` and its deterministic
regression `tests/re_m12_resource_pointer_table_promote_test.py`. It records
only the exact `0x05CE96..0x05D046` pointer table; generated transaction
output remains ignored.

# M12-AUTO33 adds the developer-only item-label promoter
`src/tools/re_m12_item_label_table_promote.py` and its deterministic
regression `tests/re_m12_item_label_table_promote_test.py`. It records only
the exact `0x05CC16..0x05CE16` 64-record table; generated transaction output
remains ignored.

# M12-AUTO32 adds the developer-only menu-label promoter
`src/tools/re_m12_label_table_promote.py` and its deterministic regression
`tests/re_m12_label_table_promote_test.py`. It records only the exact
`0x05CBA6..0x05CBD6` fixed-width label table; generated transaction output
remains ignored.

# M12-AUTO31 adds the developer-only threshold-table promoter
`src/tools/re_m12_threshold_table_promote.py` and its deterministic
regression `tests/re_m12_threshold_table_promote_test.py`. It records only
the exact `0x05D906..0x05D918` sentinel table; generated transaction output
remains ignored.

# M12-AUTO30 adds the developer-only dispatch pointer-table promoter
`src/tools/re_m12_dispatch_pointer_table_promote.py` and its deterministic
regression `tests/re_m12_dispatch_pointer_table_promote_test.py`. It records
only the exact `0x00DF54..0x00E0B8` pointer table; pointed-to handler code and
generated transaction output remain outside this source change.

# M12-AUTO29 adds the preserved candidate-map converter
`src/tools/re_candidate_map_to_ghidra.py` reconstructs the bounded
developer-only Ghidra map used to test the preserved code-census provenance
path. `tests/re_candidate_map_to_ghidra_test.py` covers deterministic
conversion; generated maps and transaction output remain ignored.

# M12-AUTO28 adds the developer-only nibble-lookup promoter
`src/tools/re_m12_nibble_lookup_promote.py` and its deterministic regression
`tests/re_m12_nibble_lookup_promote_test.py`. It records only the exact
`0x062DC0..0x062DE0` lookup; generated transaction output remains ignored.

# M12-AUTO27 adds the developer-only event dispatch promoter
`src/tools/re_m12_event_dispatch_promote.py` and its deterministic regression
`tests/re_m12_event_dispatch_promote_test.py`. It records only the exact
`0x00532C..0x005378` signed-relative event table; generated transaction output
remains ignored.

# M12-AUTO26 adds the developer-only field-86 callback promoter
`src/tools/re_m12_callback_promote.py` and its deterministic regression
`tests/re_m12_callback_promote_test.py`. It materializes only the exact
`0x10000` callback and `0x30000` RTS entrypoint; transaction output remains
ignored.

# M12-AUTO25 adds the developer-only PC-relative table promoter
`src/tools/re_m12_pc_tables_promote.py` and its deterministic regression
`tests/re_m12_pc_tables_promote_test.py`. It records only closed lookup,
fixed-record, copied-record, and VDP initialization tables; generated output
remains ignored.

# M12-AUTO24 adds the developer-only exact static-code promoter
`src/tools/re_m12_static_code_promote.py` and deterministic regression
`tests/re_m12_static_code_promote_test.py`. It records only the caller-backed
`[0x0167BE,0x01685A)` island; generated transaction output remains ignored.

# M12-AUTO23 adds the developer-only save-slot promoter
`src/tools/re_m12_save_slots_promote.py` and deterministic regression
`tests/re_m12_save_slots_promote_test.py`. It records only the exact primary
save slot and independently bounded secondary prefix; generated transaction
output remains ignored and no C++ migration or ROM asset is tracked.

# M12-AUTO20 extends the developer-only graphics promoter
`src/tools/re_m12_table_graphics_promote.py` and its deterministic regression
`tests/re_m12_table_graphics_promote_test.py` promote only wholly UNKNOWN
graphics streams selected by exact field1 pointers in the 99-row resource
table and independently bounded by the graphics census.

M12-AUTO19 extends the developer-only graphics promoter
`src/tools/re_m12_sound_data_promote.py` and its deterministic regression
`tests/re_m12_sound_data_promote_test.py` promote only the exact bounded
Ancient Music Driver sound-data archive after table and terminal-boundary
validation.

`src/tools/re_m12_runtime_graphics_promote.py` and its deterministic
regression `tests/re_m12_runtime_graphics_promote_test.py` with exact static
`LEA → D9A4 → 37D2 → 0x3820` consumers. It records two decoder-bound streams;
generated transaction output remains ignored.

# M12-AUTO17 adds the developer-only runtime-correlated graphics promoter
`src/tools/re_m12_runtime_graphics_promote.py` and its deterministic
regression `tests/re_m12_runtime_graphics_promote_test.py`. It records only
two exact decoder-bound streams with canonical runtime reader and static
pointer evidence; generated transaction output remains ignored.

# M12-AUTO9 adds the developer-only erased-alignment padding promoter
`src/tools/re_m12_padding_promote.py` and deterministic regression
`tests/re_m12_padding_promote_test.py`. It records only complete UNKNOWN
`0xFF` runs ending on 4 KiB ROM boundaries; generated transaction output
remains ignored and no C++ migration or ROM asset is tracked.

M12-AUTO8 adds the developer-only sequential direct-graphics-chain promoter
`src/tools/re_m12_direct_graphics_chain_promote.py` and its deterministic
regression `tests/re_m12_direct_graphics_chain_promote_test.py`. It records
five new decoder-verified continuation streams while revalidating three
already-owned anchors; generated transaction output remains ignored.

The separate structural closure is implemented by
`src/tools/thor_evidence/m12_auto8_direct_graphics_chain_closure.py`, tested in
`tests/m12_auto8_direct_graphics_chain_closure_test.py`, and documented in
`docs/reports/THOR_M12_AUTO8_DIRECT_GRAPHICS_CHAIN_20261002.md`. Its machine
census and removable contribution live under
`docs/reports/THOR_M12_AUTO8_STRUCTURAL_20261002/`; generated evidence remains
local and does not alter runtime or source-ownership authorities.
Its variable-range union/intersection and ledger validation are isolated in
`src/tools/thor_evidence/m12_auto8_structural_ledger.py`.

M12-AUTO52's structural closure is implemented by
`src/tools/thor_evidence/m12_auto52_direct_loader_closure.py`, with its
wrapper-specific contribution validation in
`src/tools/thor_evidence/m12_auto52_direct_loader_ledger.py` and focused
regression in `tests/m12_auto52_direct_loader_closure_test.py`. The closure
report is `docs/reports/THOR_M12_AUTO52_DIRECT_LOADER_20261003.md`; its
machine census, independently removable 37D2/36D4 ledgers, and derived views
are local under `docs/reports/THOR_M12_AUTO52_STRUCTURAL_20261003/`.

M12.1 adds the developer-only transactional promotion orchestrator
`src/tools/re_m12_1_promote.py`, its helper regression
`tests/re_m12_1_test.py`, exact status-register decoding coverage in
`src/tools/re_slice_exact.cpp`, and the M12.1 report
`docs/reports/ASM_PROMOTION_06042A_0611F4_M12_1.md`. The transaction promotes
only six exact source-owned slices; no production/core file changed.

M12.0 adds the roadmap rebase, exact ASM completion census report
docs/reports/ASM_COMPLETION_CENSUS_M12_0.md and the M12.0 state/task/ledger
updates. The report contains the machine-readable 136-range blob inventory.
No source or production/core file changed.

M11.64 adds the documentation-only bounded G0 ledger
`docs/RE_LEDGER.md`, reusable method index `docs/RE_METHOD_CATALOG.md`, and
closure report `docs/reports/G0_PORTABILITY_BOUNDARY_M11_64.md`. No source or
production/core file changed.

M11.63 adds `src/tools/hybrid/callee_623ac_observer.hpp/.cpp` and runtime
wiring for the bounded 0x0623AC natural callee contract. The deterministic
regression is `tests/hybrid_callee_623ac_test.cpp`; the report is
`docs/reports/CALLEE_0623AC_CONTRACT_M11_63.md`. No `oasis_core` file changed.

M11.62 adds `src/tools/hybrid/callee_61934_observer.hpp/.cpp` and runtime
wiring for the bounded 0x061934 natural callee contract. The deterministic
regression is `tests/hybrid_callee_61934_test.cpp`; the report is
`docs/reports/CALLEE_061934_CONTRACT_M11_62.md`. No oasis_core file changed.
# M12-AUTO6 adds the developer-only fixed-stride table promoter
`src/tools/re_m12_fixed_stride_table_promote.py` and its deterministic
regression `tests/re_m12_fixed_stride_table_promote_test.py`. It records only
the bounded 50-record table `[0x5D046,0x5D686)` selected by four exact 68000
consumers; generated transaction output remains ignored and no C++ migration
or ROM asset is tracked.

M12-AUTO7 adds the developer-only direct-graphics promoter
`src/tools/re_m12_direct_graphics_promote.py` and its deterministic regression
`tests/re_m12_direct_graphics_promote_test.py`. It records seven exact
decoder-consumed streams selected by direct `0x3820` consumers and sequential
`A0` continuation; generated transaction output remains ignored.

# M12-AUTO16 adds the developer-only exact runtime-correlated probe-slice
promoter src/tools/re_m12_exact_probe_slices_promote.py and deterministic
regression tests/re_m12_exact_probe_slices_promote_test.py. It records nine
bounded canonical-equal ASM slices totalling 1,430 bytes; generated transaction
output and local probe evidence remain ignored.

# M12-AUTO15 adds the developer-only constant-D0 CC-B0 selected-slot promoter
`src/tools/re_m12_ccb0_selected_slots_promote.py` and its deterministic
regression `tests/re_m12_ccb0_selected_slots_promote_test.py`. It records only
15 unique 16-bit relative-pointer slots selected by exact constant-D0 callers;
dynamic callers and nested table extents remain UNKNOWN.

# M12-AUTO14 adds the developer-only CC-B0 group pointer-table promoter
`src/tools/re_m12_ccb0_group_table_promote.py` and its deterministic regression
`tests/re_m12_ccb0_group_table_promote_test.py`. It records only the exact
32-entry longword table `[0x04371E,0x04379E)`; nested target subtables remain
UNKNOWN and generated transaction output remains ignored.

# File Map

M12-AUTO22 adds the developer-only exact small-table promoter
`src/tools/re_m12_exact_small_tables_promote.py` and deterministic regression
`tests/re_m12_exact_small_tables_promote_test.py`. It records only three
fixed consumer-bounded tables; generated transaction output remains ignored.

M12-AUTO21 adds the developer-only CC-B0 nested target-table promoter
`src/tools/re_m12_ccb0_target_tables_promote.py` and deterministic regression
`tests/re_m12_ccb0_target_tables_promote_test.py`. It records only exact
256-slot target windows; generated transaction output remains ignored.

M12-AUTO13 extends the developer-only record-stream promoter with the exact
BCEA field3 sentinel-list edge. It records only the five merged ranges derived
from 30 field3 bases and 100 terminated list views; generated transaction
output remains ignored and no ROM or C++ gameplay/runtime source is tracked.

M12-AUTO11 adds the developer-only count-bounded record-stream promoter
`src/tools/re_m12_record_stream_promote.py` and deterministic regression
`tests/re_m12_record_stream_promote_test.py`. It records the exact 99-record
table `[0x3F306,0x3FF66)` and the merged count-bounded six-byte stream family;
generated transaction output remains ignored and no ROM or C++ gameplay/runtime
source is tracked.

M12-AUTO5 adds the developer-only exact lookup-table promoter
`src/tools/re_m12_lookup_table_promote.py` and its deterministic regression
`tests/re_m12_lookup_table_promote_test.py`. It records the bounded
`0x5D686..0x5D706` word table and `0x5D706..0x5D906` byte table from their
68000 consumers; generated transaction output remains ignored.

M12-AUTO4 adds the developer-only nested level-table promoter
`src/tools/re_m12_level_table_promote.py` and its deterministic regression
`tests/re_m12_level_table_promote_test.py`. It records the exact
`0x5D918..0x5D958` outer table, contiguous count-bounded nested groups, and
only the 185 pointer-backed NUL-terminated record spans through `0x5E1A0`;
the generated transaction remains ignored build evidence and no C++ migration
or ROM asset is tracked.

M12-AUTO2 adds the developer-only consumer-backed graphics census
`src/tools/re_graphics_stream_census.cpp`, pointer consumer scanner
`src/tools/re_m12_pointer_resource_scan.cpp`, and transactional promoter
`src/tools/re_m12_consumer_promote.py` with helper regression
`tests/re_m12_consumer_promote_test.py`. The bounded transaction preserves
the canonical ROM while recording exact 68000 graphics consumers, the Z80
upload, screen descriptors, and fixed 1208-byte parser records. Census and
materialization outputs stay in ignored build evidence directories; the
isolated target/test definitions live in `cmake/m12_auto2.cmake`; no ROM or
extracted commercial asset is tracked.

M12-AUTO3 adds the developer-only indexed script-table promoter
`src/tools/re_m12_script_promote.py` and its deterministic regression
`tests/re_m12_script_promote_test.py`. It records the exact `0x51514` table,
the `0x00C2EC` index loop, and the NUL-terminated streams without beginning
production C++ migration; generated transaction output remains ignored.

`src/tools/thor_evidence/indexed_script_family_audit.py` independently audits
the 98-entry signed-relative script table and fail-closes structural admission
when an alternate dynamic selector bypasses the bulk-loop exclusions.
`tests/indexed_script_family_audit_test.py` covers independent offset/NUL
enumeration and the open-selector rejection for the full-table family. Its
machine census, no-op structural views, and closure report are under
`docs/reports/THOR_M68K_INDEXED_SCRIPT_FAMILY_20261004/`; the separately
scoped bulk-loop subfamily is recorded below.

`src/tools/thor_evidence/indexed_script_bulk_family.py` regenerates and accepts
the path-scoped 95-entry bulk-loop subfamily. Its versioned validator is
`src/tools/thor_evidence/static_family_indexed_strings.py`; focused ledger and
removal tests are in `tests/indexed_script_bulk_family_test.py`. The accepted
census, contribution ledger, before/after/removal structural views, and report
are in `docs/reports/THOR_M68K_INDEXED_SCRIPT_FAMILY_20261004/bulk-loop-95/`.

M12-AUTO adds the developer-only combined promoter
`src/tools/re_m12_auto_promote.py`, the compressed-resource boundary scanner
`src/tools/re_resource_boundary_scan.cpp`, their helper regression
`tests/re_m12_auto_test.py`, the M12.5 bounded report
`docs/reports/ASM_PROMOTION_003B3E_004A92_M12_5.md`, and the autonomous final
report `docs/reports/ASM_AUTONOMOUS_TO_90_PERCENT_M12_AUTO.md`. Generated
ROM/source/assets remain local ignored build outputs; no ROM or extracted
commercial asset is tracked.
M12.4 adds the developer-only ROM-start transaction
`src/tools/re_m12_4_promote.py`, its deterministic helper regression
`tests/re_m12_4_test.py`, and the report
`docs/reports/ASM_PROMOTION_000000_0007C4_M12_4.md`. The exact reassembler now
retains vasm-safe raw encoding for `MOVE USP`; full-split metrics and
materialization recognize explicit vector/header/structured ASM ownership.

M12.3 adds the developer-only transactional promoter
`src/tools/re_m12_3_promote.py`, its deterministic helper regression
`tests/re_m12_3_test.py`, the exact decoder/reassembler regression extensions,
and the bounded report
`docs/reports/ASM_PROMOTION_006516_0083D4_M12_3.md`. No production/core
runtime file or ROM/asset was added; M12.3 materialization remains under the
ignored build evidence directory.

M12.2 adds the developer-only transactional promoter
`src/tools/re_m12_2_promote.py`, its deterministic helper test
`tests/re_m12_2_test.py`, the bounded promotion report
`docs/reports/ASM_PROMOTION_00DE00_00E338_M12_2.md`, and exact decoder/
reassembler coverage for the target's ADDX, MULU, EOR/CMP and indirect-call
forms. No production/core runtime file or ROM/asset was added.

M11.61 adds `src/tools/hybrid/callee_62ae0_observer.hpp/.cpp` and the
developer-only runtime wiring for natural entry/return, A5 equality, paths,
and memory-effect evidence. `tests/hybrid_callee_62ae0_test.cpp` locks the
minimum deterministic contract. The report is
`docs/reports/CALLEE_062AE0_CONTRACT_M11_61.md`; no `oasis_core` file changed.

M11.60 adds `src/tools/hybrid/a5_lifetime_observer.hpp/.cpp` and the small
`a5_lifetime_runtime.*` bridge for opt-in natural G0 provenance, plus
`runner_support.*` to keep the developer runner below the source-size limit.
`tests/hybrid_a5_lifetime_test.cpp` locks deterministic materialization,
consumer and parent-restore evidence. The milestone report is
`docs/reports/A5_CONSUMER_LIFETIME_M11_60.md`. These are developer-only
artifacts; no `oasis_core` or production behavior changed.

M11.59 adds `docs/reports/RAW_DATA_OWNERSHIP_M11_59.md`, the all-ROM raw-data
ownership/alias/lifetime census and typed-gate result. It also adds
`tests/raw_data_provenance_test.cpp`, a decoder-only address-mode regression,
and its CTest registration in `CMakeLists.txt`. No production/core runtime
module or ROM/GPGX adapter changed.

M11.58 adds `docs/reports/PORTABLE_BEHAVIOR_CLUSTER_M11_58.md`, the raw
footprint/ownership census and cluster gate result. No source files or runtime
modules were added; the existing `src/core/parent_suffix.*` composition remains
the minimal behavior-cluster boundary and `src/tools/hybrid/candidate_parent_suffix.*`
continues to own ROM/GPGX adaptation and proof.


M11.57 additions: `src/core/parent_suffix.hpp/.cpp` owns the minimal portable
parent-owned suffix contract; `src/tools/hybrid/candidate_parent_suffix.hpp/.cpp`
owns ROM/GPGX adapter and shadow/native proof; `tests/parent_suffix_test.cpp`
covers adverse entries and every represented boundary; report:
`docs/reports/PARENT_SUFFIX_HANDOFF_M11_57.md`.

M11.56 additions: src/tools/hybrid/caller_continuation.hpp/.cpp owns the
bounded EMULATED-only parent/entry/exit and data-hook evidence observer;
src/tools/hybrid/validate_caller_continuation.py checks paired local traces,
canonical bytes, saved-frame ownership and the exact memory journal;
tests/hybrid_caller_continuation_test.cpp tests capture boundaries and negative
paths; docs/reports/RAMFLAG_CALLER_ROUTINE_M11_56.md records the exact internal
tail, shared epilogue and negative third-routine promotion result.

M11.55 additions: src/tools/hybrid/caller_attribution.hpp/.cpp owns the
developer-only natural RamFlag caller/return/register/cycle attribution schema;
src/tools/hybrid/external_library.hpp owns the shared dynamic-library wrapper;
tests/hybrid_caller_attribution_test.cpp covers deterministic attribution JSON;
docs/reports/RAMFLAG_CALLER_DATA_CLOSURE_M11_55.md records the caller CFG,
shared-data, hardware-boundary and STOP-gate evidence.
M11.54 additions: docs/reports/NATIVE_ROUTINE_CLUSTER_M11_54.md records the
bounded caller/data cluster, provenance, metrics and proposed M11.55.
M11.53 additions: src/core/ram_flag_routine.cpp/.hpp owns the portable
structured RAM flag/output routine contract; src/tools/hybrid/candidate_604bc
owns the ROM-specific per-instruction adapter; tests/ram_flag_routine_test.cpp
and tests/hybrid_candidate_604bc_test.cpp provide standalone and bridge
regressions; docs/reports/SECOND_PORTABLE_NATIVE_ROUTINE_M11_53.md records the
candidate inventory and dual promotion evidence.

This document is the canonical map of repository structure. Update it whenever structure or responsibilities change.

```text
/
├── AGENTS.md                  Mandatory rules for AI agents and contributors
├── AI_DEVELOPMENT_CONTRACT.md Operational contract for evidence/focus/completion
├── CMakeLists.txt             Top-level build configuration and CTest registration
├── PROJECT_STATE.md           Current milestone/task/status for context recovery
├── README.md                  Public project overview and build entry point
├── TASK.md                    Current task, evidence, acceptance criteria, next action
├── .github/
│   └── workflows/
│       ├── ci.yml                    Build + ordinary CTest verification
│       └── (ROM-backed probes run locally; the commercial ROM is never fetched by CI)
├── assets/gfx3820/manifest.tsv  Hash-pinned exact ResourceID 1..107 compressed/output manifest (no payloads)
├── assets/gfx3820_screen_roots/manifest.tsv  Hash-pinned root/descriptor/stream graph and unique-stream hashes (no payloads)
├── assets/fixed_count_records/manifest.tsv  Row-preserving 96-to-78 raw stream portability manifest (no payloads)
├── docs/
│   ├── ARCHITECTURE.md        Layering, dependencies, translation strategy
│   ├── DECISIONS.md           Architecture decision records (ADR-style)
│   ├── DEVELOPMENT_RULES.md   Coding, testing and documentation rules
│   ├── FILE_MAP.md            This canonical repository map
│   ├── PORTING.md             Portability boundary/target notes
│   ├── PROJECT_VISION.md      Goal, scope, non-goals and end-state
│   ├── RE_TOOLCHAIN_GUIDE.md  Historical Mega Drive SDK/toolchain evidence and fingerprinting rules
│   ├── REASSEMBLY_POC.md     M11.9/M11.10 bounded reassembly reproduction and measured results
│   ├── FULL_ROM_SPLIT_BASELINE.md M11.11 full-ROM split manifest and exact baseline report
│   ├── AUTO_BLOB_PROMOTION.md M11.12 transactional blob-to-source promotion report
│   ├── AUTO_PROMOTION_SCALE.md M11.13 100-attempt promotion scale report
│   ├── AUTO_PROMOTION_BATCH2.md M11.14 large batch II and saturation report
│   ├── EVIDENCE_INTEGRITY_AUDIT.md M11.15 exactness/trust classification audit
│   ├── TARGETED_DYNAMIC_CONFIRMATION.md M11.16 bounded natural evidence report
│   ├── STRUCTURED_DATA_CLASSIFICATION.md M11.17 bounded data classification report
│   ├── NATIVE_VERTICAL_SLICE.md M11.18 native controlled-screen boundary and status
│   ├── reports/
│   │   └── ASM_COMPLETION_CENSUS_M12_0.md M12.0 full-ROM ASM completion census
│   │   └── NATIVE_ROM_RESOURCE_ID3.md M11.25 verified native resource baseline
│   │   └── NATIVE_ROM_RESOURCE_ID3_VISUAL_ROLE.md M11.26 bounded visual-role report
│   │   └── HOT_PATH_MULTI_BLOCK_COVERAGE_M11_39.md M11.39 bounded hot-path coverage report
│   │   └── REMAINING_INTERPRETER_ATTRIBUTION_M11_40.md M11.40 baseline-blocked attribution report
│   │   └── CHECKPOINT_IDENTITY_M11_41.md M11.41 checkpoint provenance/repair report
│   │   └── REMAINING_INTERPRETER_ATTRIBUTION_M11_42.md M11.42 restart-gate blocker report
│   │   └── CHECKPOINT_CANONICALIZATION_M11_43.md M11.43 exact GPGX state-layout and identity proof
│   │   └── REMAINING_INTERPRETER_ATTRIBUTION_M11_44.md M11.44 exhaustive pre-promotion interpreter ledger
│   │   └── REMAINING_INTERPRETER_ATTRIBUTION_FINAL_M11_44.md M11.44 final post-promotion interpreter ledger/Pareto
│   │   └── SEMANTIC_CLOSURE_M11_45.md M11.45 candidate selection and shadow/native proof
│   │   └── REMAINING_INTERPRETER_ATTRIBUTION_M11_45.md M11.45 exhaustive final interpreter ledger
│   │   └── RUNTIME_ADDRESS_PROVENANCE_M11_46.md M11.46 bounded runtime address and memory-class evidence
│   │   └── SAFE_MEMORY_SEMANTIC_CLOSURE_M11_47.md M11.47 safe-memory semantic and primitive proof
│   │   └── REMAINING_INTERPRETER_ATTRIBUTION_M11_47.md M11.47 exhaustive post-promotion interpreter ledger
│   │   └── NATIVE_MECHANICAL_PRIMITIVE_M11_48.md M11.48 resumable native mechanical primitive proof
│   │   └── MECHANICAL_PRIMITIVE_FAMILY_M11_49.md M11.49 mechanical primitive family closure proof
│   │   └── FIRST_PORTABLE_NATIVE_ROUTINE_M11_51.md M11.51 routine extraction and replacement gate
│   │   └── NATIVE_ROUTINE_MISMATCH_M11_52.md M11.52 continuation mismatch closure and promotion proof
│   │   └── SECOND_PORTABLE_NATIVE_ROUTINE_M11_53.md M11.53 second routine contract and dual promotion proof
│   │   └── RAMFLAG_CALLER_DATA_CLOSURE_M11_55.md M11.55 caller/data/hardware closure evidence
│   │   └── PARENT_SUFFIX_HANDOFF_M11_57.md M11.57 parent-owned helper and handoff proof
│   ├── M11_19_TEST_A.md       M11.19 external Genesis-Plus-GX live-coverage report
│   ├── REVERSE_ENGINEERING.md Address/routine/ROM/data research ledger
│   ├── ROADMAP.md             Ordered milestones and current active milestone
│   ├── TASK_TEMPLATE.md       Mandatory task/session handoff template
│   ├── VDP_MODEL.md           Narrow portable video-state model and non-goals
│   └── WORKLOG.md             Chronological record of development actions
├── src/
│   ├── main.cpp               Canonical ROM gate and native runtime entry point
│   ├── core/
│   │   ├── rom.cpp            ROM file loading/basic title access
│   │   ├── rom.hpp            ROM byte-container API
│   │   ├── rom_identity.cpp   Header/checksum/hash/known-revision identification
│   │   ├── rom_identity.hpp   ROM identity models and public API
│   │   ├── mechanical_primitive.cpp Portable proven copy/clear/DBF executor
│   │   ├── mechanical_primitive.hpp Portable primitive contract and continuation API
│   │   ├── table_copy_routine.cpp Portable structured table-copy routine executor
│   │   ├── table_copy_routine.hpp Routine contract, machine interface and continuation API
│   │   ├── ram_flag_routine.cpp Portable structured RAM flag/output routine executor
│   │   ├── ram_flag_routine.hpp RAM flag/output contract and continuation API
│   │   ├── runtime.cpp        Explicit deterministic frame stepping
│   │   └── runtime.hpp        Portable controller/input/frame runtime API
│   ├── genesis/
│   │   ├── memory_bus.cpp     Minimal Mega Drive address-space compatibility
│   │   ├── memory_bus.hpp     Memory bus interface
│   │   ├── vdp.cpp            Bounded VRAM/CRAM/VSRAM storage/access
│   │   ├── vdp.hpp            Narrow VDP state API
│   │   └── vdp_types.hpp      Tile/plane/sprite raw attribute decoding
│   ├── game/
│   │   ├── controlled_screen.cpp Native screen controller supporting synthetic fixture, ROM screens, transitions, combat collision, loot drop spawning, pickup collection, and spirit summoning
│   │   ├── controlled_screen.hpp Controlled-screen state, ROM loader, transition API, and spirit slot integration
│   │   ├── screen_instance.cpp ScreenInstance loader, decompressor, and view rebind implementation
│   │   ├── screen_instance.hpp ScreenInstance runtime model for ROM-backed screens
│   │   ├── render/
│   │   │   ├── debug_overlay.cpp Bitmap font diagnostic overlay text rasterizer
│   │   │   ├── debug_overlay.hpp Diagnostic text overlay API
│   │   │   ├── framebuffer.cpp Software framebuffer storage
│   │   │   └── framebuffer.hpp Packed software framebuffer API
│   │   ├── graphics_decompress.cpp Native translation of original 0x3820 routine
│   │   ├── graphics_decompress.hpp Decompressor result/API
│   │   ├── audio_format_a.cpp Native Format-A selector parser and bounded Mode 0/1 PCM decoder candidate
│   │   ├── audio_format_a.hpp Format-A descriptor, selector ID and decode result API
│   │   ├── genesis_graphics.cpp Pure 4bpp tile + CRAM palette decoding
│   │   ├── genesis_graphics.hpp Graphics decoder data types/API
│   │   ├── resource_loader.cpp Hash-verified ResourceID 1..107 ROM/file graphics loader
│   │   ├── resource_loader.hpp GFX3820 ResourceID loader and migration status API
│   │   ├── screen_resource_loader.cpp Root-path to stable stream-ID loader with exact file override and ROM fallback
│   │   ├── screen_resource_loader.hpp Screen-root manifest/alias and backend result API
│   │   ├── fixed_count_resource_loader.cpp Row-to-stream resolver, exact raw override validation and ROM fallback
│   │   ├── fixed_count_resource_loader.hpp Neutral fixed-count record API and backend result
│   │   ├── resource_diagnostic.cpp Native VRAM transfer and tile-atlas renderer
│   │   ├── resource_diagnostic.hpp Diagnostic resource visualization API
│   │   ├── translated_routines.cpp Initial translated compatibility routines
│   │   ├── translated_routines.hpp Their public declarations
│   │   ├── player/
│   │   │   ├── player.cpp     Portable input, state, terrain-gated movement, hurt/knockback physics, melee attack state machine, damage pipeline, and item pickup healing
│   │   │   └── player.hpp     Player movement API, damage state, melee attack state/hitbox API, item pickup API, combat routines, and ROM evidence constants
│   │   ├── entities/
│   │   │   ├── entity_pool.cpp  Raw entity-pool bounds and active-record view
│   │   │   ├── entity_pool.hpp  Entity-pool descriptors and raw field offsets
│   │   │   ├── screen_entity.cpp ROM-backed entity descriptors, frame piece decoding, animation player, interaction logic, moving actor state machine, attack hitboxes, damage reception, death lifecycle, and loot drop entity spawning
│   │   │   └── screen_entity.hpp ScreenEntity, EntityDescriptor, AnimationState, ActorRuntime (with HP/hurt/death), EntityHitbox, directional attack hitbox, loot drop spawning, EntityInteractionState, and SpritePiece public API
│   │   ├── scripts/
│   │   │   ├── event_router.cpp  Raw event producer and handler-range mapping
│   │   │   └── event_router.hpp  Event RAM addresses and raw router API
│   │   ├── spirits/
│   │   │   ├── spirit_slots.cpp  Evidence-backed slot, active spirit state machine, Dytto summon, follow tracking, active command decoding, and water bubble projectile pipeline
│   │   │   └── spirit_slots.hpp  Spirit slot/target/dispatch constants, ActiveSpirit model, active command API, and public declarations
│   │   └── world/
│   │       ├── byte_grid.cpp          8-pixel world grid and footprint aggregation
│   │       ├── byte_grid.hpp          Bounded world-grid view API
│   │       ├── screen_descriptor.cpp Screen descriptor table reader
│   │       ├── screen_descriptor.hpp Screen descriptor data types/API
│   │       ├── terrain_collision.cpp  Terrain-state and movement gate semantics
│   │       └── terrain_collision.hpp  Terrain gate API
│   ├── platform/
│   │   ├── window.cpp Minimal Win32 window/input/presentation adapter; non-Windows stub
│   │   └── window.hpp Platform boundary for controller polling and framebuffer presentation
│   └── tools/
│       ├── hybrid/
│       │   ├── caller_attribution.hpp/.cpp Developer-only natural RamFlag caller attribution and JSON evidence
│       │   ├── external_library.hpp Developer-only cross-platform external-library loading wrapper
│       │   ├── generated_block_runtime.cpp Helpers used by generated blocks
│       │   ├── generated_block_runtime.hpp Generated-block helper boundary
│       │   ├── basic_block_reference.cpp Generic prediction adapter for generated blocks
│       │   ├── basic_block_reference.hpp Generic generated-block prediction API
│       │   ├── generated_blocks.cpp Generated historical M11.33 bodies
│       │   ├── generated_blocks_m1137.cpp Generated M11.37 single-instruction bodies
│       │   ├── generated_blocks_m1138.cpp Generated M11.38 multi-instruction bodies
│       │   ├── generated_blocks_m1139.cpp Generated M11.39 hot-path bodies
│       │   ├── generated_blocks_m1144.cpp Generated M11.44 0x03A7AE block body
│       │   ├── generated_blocks_m1145_*.cpp Generated M11.45 semantic candidate bodies and registry fragments
│       │   ├── generated_blocks_m1147.cpp Generated M11.47 safe-memory candidate bodies
│       │   ├── generated_blocks_m1147_registry.cpp Generated M11.47 registry fragment
│       │   ├── mechanical_primitive.hpp/.cpp Hybrid ROM registry, canonical adapter and evidence glue
│       │   ├── runner_report.hpp/.cpp Developer-only hybrid summary detail serializer
│       │   ├── generated_block_registry.cpp Generated registry metadata/glue
│       │   ├── generated_blocks.hpp Generated block declarations and registry API
│       │   ├── basic_block.cpp Handwritten generic shadow/native boundary glue
│       │   ├── basic_block.hpp BlockExit contract, registry state and metrics
│       │   ├── interpreter_profile.cpp Ranked interpreter fallback profile writer
│       │   ├── interpreter_profile.hpp Interpreter profile writer API
│       │   ├── interpreter_ledger.cpp Exhaustive decoder-backed interpreter attribution CLI
│       │   ├── address_provenance.hpp/.cpp Developer-only runtime address observer and memory classification
│       │   ├── recomp_generator.cpp Decoder-to-C++ generator with fail-closed forms
│       │   ├── recomp_generator.hpp Generator model/emitter API
│       │   ├── checkpoint_evidence.cpp Raw checkpoint evidence and canonical identity adapter
│       │   ├── checkpoint_evidence.hpp Checkpoint identity/evidence API
│       │   ├── gpgx_checkpoint_layout.cpp Pinned GPGX ABI layout and representation span table
│       │   ├── gpgx_checkpoint_layout.hpp Checkpoint layout contract API
│       │   ├── runner.cpp Developer-only GPGX hybrid scenario runner
│       │   └── recomp_generator_report.cpp Generator CLI
│       ├── asset_inspector.cpp Local-only ROM graphics inspection CLI
│       ├── w5_audio_native_reference.cpp Canonical-ROM native Format-A selector/decode hash reporter
│       ├── generate_gfx3820_resource_manifest.py Regenerates the pinned externalization manifest from accepted census/closure data
│       ├── fixed_count_file_manifest.py Generates the row-preserving fixed-count portability catalog from accepted enumeration plus canonical ROM
│       ├── m12_gfx_ancient.py Independent Ancient graphics parser/classifier
│       ├── m12_gfx_sweep.py Developer-only whole-ROM M12-GFX-1 campaign/report
│       ├── m12_gfx_caller_closure.py Developer-only M12-GFX-2 0x3820 caller/source closure and promoter
│       ├── re_m12_gfx_loader_census.py Developer-only exact 0x00D406 direct-xref census
│       ├── re_m12_gfx_37d2_census.py Developer-only exact 0x37D2 wrapper-family census
│       ├── re_m12_gfx_d3b2_census.py Developer-only exact 0xD3B2 indexed-loader census
│       ├── re_m12_gfx_36d4_census.py Developer-only exact 0x36D4 wrapper census
│       ├── re_m12_gfx_sibling_census.py Developer-only proven sibling-helper census
│       ├── re_m12_gfx_d406_residual_census.py Developer-only residual 0x00D406 census
│       ├── re_m12_gfx_runtime_provenance.py Developer-only runtime source-provenance validator
│       ├── re_resource_boundary_scan.cpp Developer-only pointer-table resource boundary scanner
│       ├── ghidra/OasisGhidraMap.java Developer-only Ghidra map exporter; never production-linked
│       ├── re_slice_decoder.cpp Developer-only bounded 68000 evidence decoder/reporter
│       ├── re_slice_decoder.hpp Decoder data types and report API
│       ├── re_slice_exact.cpp Bounded exact instruction normalization over decoder-owned operands
│       ├── re_assemble.hpp/.cpp Developer-only ASM emission and first-byte comparator
│       ├── re_assemble_format.cpp Raw-word and typed-operand JSON output
│       ├── re_assemble_report.cpp Canonical-ROM 25-slice emit/verify CLI and bounded split manifest
│       ├── re_assemble_run.py Local vasm orchestration, gap extraction and exact comparison
│       ├── re_full_split_run.py Full-ROM local-blob split, manifest metrics and exact comparison
│       ├── re_m12_auto_promote.py Combined exact-island/resource transactional promoter
│       ├── re_assemble_range_report.cpp Generic bounded decoder/ASM emitter for candidate slices
│       ├── re_auto_promote.py Evidence-ranked transactional UNKNOWN→ASM promotion runner
│       ├── re_auto_promote_helpers.py Promotion failure metadata, clustering and trend helpers
│       ├── re_dynamic_confirm.py Fail-closed bounded natural evidence confirmation
│       ├── re_structured_data.py Exact bounded ROM data structure classifier
│       ├── re_slice_format.cpp Deterministic JSON/human report formatting
│       ├── re_slice_report.cpp Local-only bounded ROM slice report CLI
│       ├── gpgx_coverage_report.cpp Developer-only NEW executed-PC bitmap decoder/report
│       ├── gpgx_import_gpgx_coverage.cpp Developer-only address-level GPGX trust evidence importer
│       ├── gpgx_import_io.cpp Structural JSON range/evidence import helpers
│       ├── gpgx_import_io.hpp GPGX importer model and provenance helper API
│       ├── gpgx_json.cpp Minimal dependency-free structural JSON parser
│       ├── gpgx_json.hpp Structural JSON value/parser declarations
│       ├── gpgx_unknown_priority.py Deterministic ranking and bounded-slice report for runtime-unknown PCs
│       ├── oasis_gpgx_rom_reader_correlation.py Deterministic analysis ROM-read to reader-PC correlation
│       ├── re_program.hpp Multi-function bounded RE aggregation types/API
│       ├── re_program.cpp Conservative function boundaries/call and memory bindings
│       ├── re_program_format.cpp Deterministic multi-slice JSON/human formatting
│       ├── re_program_report.cpp Local-only representative multi-slice CLI
│       ├── re_trace.hpp Bounded dynamic trace data model/API
│       ├── re_trace.cpp Isolated scenario interpreter and static/dynamic comparison
│       ├── re_trace_format.cpp Deterministic trace JSON/human formatting
│       ├── re_trace_report.cpp Local-only bounded dynamic trace CLI
│       ├── re_diff.hpp Retail/beta bounded correspondence types/API
│       ├── re_diff.cpp Conservative normalized signature and analogue matching
│       ├── re_diff_detail.cpp Bounded changed-block CFG/instruction detail
│       ├── re_diff_format.cpp Deterministic revision-diff JSON/human formatting
│       ├── re_diff_report.cpp Local-only retail/beta differential CLI
│       ├── re_atlas.hpp Typed bounded ROM Atlas model/query API
│       ├── re_atlas.cpp Atlas manifest, evidence aggregation and conflict detection
│       ├── re_atlas_format.cpp Deterministic Atlas JSON/human formatting
│       ├── re_atlas_report.cpp Local-only bounded Atlas CLI
│       ├── re_atlas_ranking.hpp Atlas unresolved-evidence ranking API
│       ├── re_atlas_ranking.cpp Deterministic ranking aggregation and priority groups
│       ├── re_atlas_ranking_report.cpp Local-only Atlas ranking CLI
│       ├── re_resolution.hpp Bounded address-displacement resolution API
│       ├── re_resolution.cpp Conservative register propagation and CFG merge handling
│       ├── re_resolution_format.cpp Deterministic resolution JSON/text formatting
│       ├── re_resolution_report.cpp Local-only bounded resolution CLI
│       ├── re_cfg_audit.hpp Bounded unreachable-CFG audit data model/API
│       ├── re_cfg_audit.cpp Audit classifications, islands and bounded CFG context
│       ├── re_cfg_audit_format.cpp Deterministic CFG-audit JSON/text formatting
│       ├── re_cfg_audit_report.cpp Local-only USA CFG-audit CLI
│       ├── re_reachable_closure.hpp Bounded reachable unresolved closure API
│       ├── re_reachable_closure.cpp Backward register provenance and reason audit
│       ├── re_reachable_closure_format.cpp Deterministic closure JSON/text formatting
│       ├── re_reachable_stack.hpp Narrow MOVEA postincrement stack API
│       ├── re_reachable_stack.cpp Bounded push/pop value provenance only
│       ├── re_reachable_closure_report.cpp Local-only USA closure CLI
│       ├── re_callee_effect.hpp Bounded direct-callee register/stack effect API
│       ├── re_callee_effect.cpp Conservative callee return-effect audit
│       ├── re_callee_effect_format.cpp Deterministic callee-effect JSON/text formatting
│       ├── re_callee_effect_report.cpp Local-only USA callee-effect CLI
│       ├── re_caller_stack.hpp Bounded caller pre-BSR stack provenance API
│       ├── re_caller_stack.cpp Symbolic A7 paths and conservative stack effects
│       ├── re_caller_stack_format.cpp Deterministic caller-stack JSON/text formatting
│       ├── re_caller_stack_report.cpp Local-only USA caller-stack CLI
│       ├── re_emulator_trace.hpp Neutral external emulator capture model/API
│       ├── re_emulator_trace.cpp Capture parser, normalization and Atlas comparison
│       ├── re_emulator_trace_format.cpp Deterministic emulator-trace JSON/text formatting
│       ├── re_emulator_trace_report.cpp External capture import CLI; no emulator backend
│       ├── re_scenario.hpp              Frozen natural emulator-scenario data/API
│       ├── re_scenario.cpp              Frozen natural emulator-scenario parser/JSON
│       ├── re_candidate_map.hpp         Ghidra/Atlas normalized candidate model/API
│       ├── re_candidate_map.cpp         Conservative evidence merge/classification/ranking
│       ├── re_candidate_map_parse.cpp   Strict parser for the external Ghidra JSON schema
│       ├── re_candidate_map_format.cpp  Deterministic candidate JSON/top-report formatting
│       ├── re_candidate_map_report.cpp  Local-only Ghidra-to-Atlas candidate CLI
│       ├── re_mass_verify.hpp            Batch structural-verification data/API
│       ├── re_mass_verify.cpp            Bounded decode, overlap, classification and clustering
│       ├── re_mass_verify_format.cpp     Deterministic mass JSON/text report formatting
│       ├── re_mass_verify_report.cpp     Local-only mass verification CLI
│       ├── re_explore.hpp                Recursive explorer model, states, edges and frontier API
│       ├── re_explore.cpp                Tiered deterministic guarded structural worklist
│       ├── re_explore_format.cpp         Deterministic explorer JSON/human formatting
│       ├── re_explore_report.cpp         Local-only bounded/ROM-wide explorer CLI
│       ├── re_explore_dynamic.cpp        Dynamic-edge validation for natural ant evidence
│       ├── re_ant.hpp                     Single-ant job/result/merge model and API
│       ├── re_ant.cpp                     Deterministic ant identity, parsing and merge rules
│       ├── re_ant_format.cpp              Deterministic ant JSON/text formatting
│       ├── re_ant_report.cpp              Local-only single-ant job/merge CLI
│       ├── re_ant_queue.hpp               Frozen sequential ant queue model and lifecycle API
│       ├── re_ant_queue.cpp                Deterministic queue selection, lifecycle and parsing
│       ├── re_ant_queue_format.cpp         Deterministic queue JSON/text formatting
│       ├── re_ant_queue_report.cpp         Local-only queue make/claim/finalize/merge CLI
│       ├── re_static_translation.hpp        Minimal PoC state/memory/comparison API
│       ├── re_static_translation.cpp        Developer-only bounded static C++ PoC
│       ├── re_static_translation_report.cpp Local ROM-backed PoC report CLI
│       ├── re_bizhawk_boot_trace.lua Developer-only BizHawk boot trace and bus-write probe
│       ├── re_bizhawk_natural_reach.lua Developer-only bounded natural-input target/caller/state probe
│       ├── re_bizhawk_m12_gfx_provenance.lua Developer-only targeted runtime source-register probe for unresolved 0x3820 callers
│       ├── re_bizhawk_m12_capabilities.lua Developer-only one-frame BizHawk observable/API capability probe
│       ├── re_bizhawk_m12_sat_provenance.lua Developer-only live VDP/SAT/DMA/source-RAM provenance probe
│       ├── re_bizhawk_m12_sprite_context.lua Developer-only bounded sprite-table caller/producer context probe
│       ├── re_bizhawk_m12_ali_transition.lua Developer-only one-shot F1 Ali transition provenance probe
│       ├── re_bizhawk_m12_targeted_reads.lua Developer-only exact-address ROM-read provenance probe
│       ├── re_bizhawk_m12_b730_calls.lua Developer-only bounded table-to-B730 register-context probe
│       ├── m12_sprite_context_catalog.py Developer-only payload-free table/live provenance catalog validator
│       ├── m12_static_sprite_dispatch_catalog.py Developer-only bounded static dispatch-table consumer catalog
│       ├── m12_sprite_reconstruction.py Developer-only SAT/tile/CRAM/piece reconstruction and provenance validator
│       ├── re_bizhawk_ant.lua            Developer-only one-frontier natural ant worker
│       ├── re_bizhawk_stack_provenance.lua Developer-only bounded runtime stack-value/writer probe
│       ├── re_bizhawk_natural_scenario.txt Frozen neutral-input reachability scenario
│       ├── re_bizhawk_m11_8_natural_scenario.txt M11.8 natural input/target/RAM scenario
│       ├── re_mame_boot_trace.cmd Developer-only MAME fixed-instruction debugger trace
│       ├── re_mame_writer_probe.cmd Developer-only MAME RAM writer watchpoint probe
│       └── re_mame_trace_normalize.ps1 Normalize MAME/BizHawk raw events to neutral trace
└── tests/
    ├── check_file_limits.cmake           Enforces <=500-line rule through CTest
    ├── check_core_boundary.cmake         Enforces core source/link dependency boundary
    ├── mechanical_primitive_test.cpp     Standalone oasis_core primitive semantics and continuation tests
    ├── table_copy_routine_test.cpp       Standalone portable routine oracle and yield/resume tests
    ├── table_copy_routine_contract_test.cpp Synthetic 0x2D66 CFG/entry/exit contract test
    ├── byte_grid_test.cpp                Synthetic world-grid/footprint tests
    ├── graphics_decompress_test.cpp      Synthetic decompressor behavior tests
    ├── m12_gfx_sweep_test.py             Independent Ancient parser safety/classifier tests
    ├── m12_sprite_reconstruction_test.py Synthetic hardware-sprite reconstruction and provenance tests
    ├── m12_sprite_context_catalog_test.py Ordered live selector/producer catalog validation tests
    ├── m12_static_sprite_dispatch_catalog_test.py Static M12 dispatch-table overlap and bounds tests
    ├── m12_gfx_caller_closure_test.py    M12-GFX-2 caller/source closure regression tests
    ├── re_m12_gfx_loader_census_test.py  Exact 0x00D406 direct-xref census regression tests
    ├── re_m12_gfx_37d2_census_test.py    Exact 0x37D2 wrapper-family census regression tests
    ├── re_m12_gfx_d3b2_census_test.py    Exact 0xD3B2 indexed-loader census regression tests
    ├── re_m12_gfx_36d4_census_test.py    Exact 0x36D4 wrapper census regression tests
    ├── re_m12_gfx_sibling_census_test.py Exact sibling-helper census regression tests
    ├── re_m12_gfx_d406_residual_census_test.py Exact residual 0x00D406 census regression tests
    ├── re_m12_gfx_runtime_provenance_test.py Static runtime-probe contract regression test
    ├── re_m12_gfx_runtime_provenance_validator_test.py Fail-closed runtime-source validator test
    ├── graphics_decompress_reference.cpp ROM-backed differential oracle verifier
    ├── resource_id3_reference.cpp Local USA-ROM resource/VRAM/atlas oracle verifier
    ├── gfx3820_file_override_reference.cpp Manual all-ID ROM fallback and exact-file override verifier
    ├── audio_format_a_test.cpp Synthetic Mode 0/1 accumulator and repeat-code decoder tests
    ├── w5_audio_native_differential.py Whole-catalog C++ vs W5 Python differential and evidence receipt
    ├── genesis_graphics_test.cpp         Synthetic tile/palette conversion tests
    ├── rom_identity_test.cpp             Synthetic ROM/hash/header tests
    ├── player_test.cpp                   Deterministic input and movement tests
    ├── spirit_slots_test.cpp             Deterministic spirit summoning, slot allocation, SP drain, follow movement, and transition persistence tests
    ├── environment_interaction_test.cpp  Deterministic Dytto Water Bubble -> Flame environment interaction and persistence tests
    ├── native_vertical_slice_test.cpp    M11.18 fixture, replay and framebuffer oracle
    ├── player_reference.cpp              Local USA-ROM oracle for player vectors
    ├── entity_pool_test.cpp              Synthetic raw entity-pool/active-record tests
    ├── entity_pool_reference.cpp         Local USA-ROM oracle for entity pool loops
    ├── event_router_test.cpp              Synthetic raw event producer/router tests
    ├── event_router_reference.cpp         Local USA-ROM oracle for event boundaries
    ├── re_slice_decoder_test.cpp           Synthetic bounded decoder/report tests
    ├── re_assemble_test.cpp                Exact operands, golden ASM, encoding widths and byte-difference tests
    ├── re_full_split_test.py               Synthetic deterministic full-split manifest/helper tests
    ├── re_auto_promote_test.py             Synthetic ranking/promotion/rollback helper tests
    ├── re_dynamic_confirm_test.py           Synthetic range-linkage/coverage/trust tests
    ├── re_structured_data_test.py            Synthetic conservative data classifier tests
    ├── re_slice_reference.cpp              Local USA-ROM oracle for the 0x60004 slice
    ├── re_program_test.cpp                 Synthetic multi-function RE aggregation tests
    ├── re_program_reference.cpp            Local USA-ROM oracle for representative RE targets
    ├── re_trace_test.cpp                   Synthetic dynamic trace/report tests
    ├── re_trace_reference.cpp              Local USA-ROM oracle for bounded dynamic scenario
    ├── re_diff_test.cpp                    Synthetic exact/structural/changed matching tests
    ├── re_diff_reference.cpp               Local retail/beta fingerprint/correspondence oracle
    ├── re_atlas_test.cpp                   Synthetic Atlas model/query/conflict tests
    ├── re_atlas_reference.cpp              Local USA/Beta Atlas oracle
    ├── re_atlas_ranking_test.cpp            Synthetic ranking/grouping tests
    ├── re_resolution_test.cpp               Synthetic bounded propagation/merge tests
    ├── re_resolution_reference.cpp          Local USA-ROM resolution oracle
    ├── re_cfg_audit_test.cpp                Synthetic CFG-audit classification/island tests
    ├── re_cfg_audit_reference.cpp           Local USA-ROM CFG-audit oracle
    ├── re_reachable_closure_test.cpp        Synthetic backward closure/stack/merge tests
    ├── re_reachable_closure_reference.cpp   Local USA-ROM closure oracle
    ├── re_callee_effect_test.cpp             Synthetic bounded callee-effect tests
    ├── re_callee_effect_reference.cpp        Local USA-ROM callee-effect oracle
    ├── re_caller_stack_test.cpp              Synthetic bounded caller-stack tests
    ├── re_caller_stack_reference.cpp         Local USA-ROM caller-stack oracle
    ├── re_emulator_trace_test.cpp            Synthetic external capture/import tests
    ├── re_scenario_test.cpp                  Synthetic scenario parser/serialization tests
    ├── re_candidate_map_test.cpp              Synthetic Ghidra/Atlas merge/ranking tests
    ├── re_mass_verify_test.cpp                Synthetic batch classification/clustering tests
    ├── re_explore_test.cpp                    Synthetic recursive explorer/control/frontier tests
    ├── re_ant_test.cpp                         Synthetic single-ant contract/merge tests
    ├── re_ant_queue_test.cpp                    Synthetic queue selection/lifecycle tests
    ├── re_static_translation_test.cpp            Synthetic static translation differential tests
    ├── gpgx_persistent_coverage_test.py           Synthetic persistent bitmap merge contract test
    ├── gpgx_rom_reader_correlation_test.py        Synthetic reader correlation/provenance tests
    ├── gpgx_bounded_classification_test.py        Bounded decode/runtime completeness tests
    ├── re_natural_reference.cpp              Local USA-ROM natural reachability oracle
    ├── runtime_test.cpp                  Deterministic frame/input sequence tests
    ├── screen_descriptor_test.cpp        Synthetic screen descriptor tests
    ├── terrain_collision_test.cpp        Synthetic terrain-gate tests
    ├── smoke.cpp                         Minimal build/runtime smoke test
    └── vdp_test.cpp                      VDP storage/bounds/attribute tests
```

`docs/reports/GPGX_RUNTIME_EXECUTION_TRUST.md` is the retained human-readable
report for the canonical manual-realtime evidence import. The bounded M11.21
region investigation is retained in
`docs/reports/RUNTIME_REGION_060BB6.md`; its M11.22 bounded classification is
retained in `docs/reports/RUNTIME_REGION_060BB6_CLASSIFICATION.md`.
The analysis ROM-read to reader-PC correlation report is retained in
`docs/reports/GPGX_ROM_READER_CORRELATION.md`.
The bounded M11.24 resource contract is retained in
`docs/reports/RESOURCE_CONTRACT_ID3.md`.

## M11.28 hybrid experiment files
M11.28 adds the developer-only `src/tools/hybrid/` directory:

- `CMakeLists.txt`: separate contract/tests and opt-in external GPGX frontend.
- `contract.hpp/.cpp`: 0x3820 bounded effect prediction/comparison and explicit
  unsupported override gate, reusing native/mechanical decompressors.
- `dispatch.hpp/.cpp`: natural entry/return capture, footprint checks and ISR
  isolation through the existing emulator hook.
- `candidate_2d66.hpp/.cpp`: the M11.29/M11.52 target adapter for the bounded
  `0x2D66` leaf, including exact GPGX instruction-continuation reconstruction.
- `runner.cpp`: deterministic neutral libretro frontend, local hash evidence.
- `grid_query_capture.cpp`: opt-in direct-register PRE/POST capture around the
  M68K `0x9C40` entry and its stack-derived RTS continuation.
- `grid_query_micro_oracle.cpp/.hpp`: independent MOVEQ/MOVE/RTS/sentinel test
  for HOOK_CPU timing, stack byte order, register writes, and read-value
  propagation.
- `gpgx_capture_bridge.c`: minimal HOOK_CPU register, memory-peek, cycle, and
  callback exports for that capture runner; excluded from production targets.
- `gpgx_bridge.c`: read-only bridge compiled only inside the external emulator.
- `prepare_gpgx.py`: reproducible bridge installation, coverage opt-out guard
  and M11.36 post-instruction comparison hook.
- `tests/hybrid_contract_test.cpp`, `tests/hybrid_dispatch_test.cpp`: independent
  synthetic state/output/stack, corruption, footprint and override-gate tests.
- `tests/hybrid_candidate_2d66_test.cpp`: synthetic exact MOVEM-stack shadow,
  instruction bridge and M11.51 refresh-blind-spot regression for the selected
  candidate.
- `docs/reports/HYBRID_NATIVE_MIGRATION_POC.md`: evidence and exact blockers.
- `docs/reports/HYBRID_NATIVE_OVERRIDE_MINIMAL_POC.md`: M11.29 target selection,
  identity and shadow/override equivalence evidence.
- `src/tools/hybrid/replacement.hpp/.cpp`: developer-only explicit routine registry and metrics.
- `src/tools/hybrid/candidate_604bc.hpp/.cpp`: M11.30 bounded RAM flag routine contract.
- `src/tools/hybrid/candidate_61032.hpp/.cpp`: M11.30 bounded table/RAM routine contract.
- `tests/hybrid_replacement_test.cpp`: synthetic registry routing regression.
- `docs/reports/HYBRID_NATIVE_OVERRIDE_BATCH_POC.md`: M11.30 shadow evidence and override blocker.
- `basic_block.hpp/.cpp`: M11.32 developer-only bounded block registry and
  generic M11.38 `BlockExit`/yield continuation glue; generated execution
  remains separate from this handwritten registry.
- `tests/hybrid_semantic_core_test.cpp`: M11.34 independent reference vectors,
  M11.35 newly required semantic vectors, exact decode/length checks and
  exact-IR assertions.
- `tests/hybrid_m1145_semantic_test.cpp`: M11.45 independent exact vectors for
  the bounded generated semantic helpers.
- `tests/hybrid_m1147_semantic_test.cpp`: M11.47 exact safe-memory vectors,
  overlap/address-wrap checks and generator provenance regression.
- `tests/hybrid_generated_provenance_test.cpp`: generated M11.33/M11.35 block
  extension-word consumption and direct-successor PC/provenance regression.
- `tests/hybrid_basic_block_test.cpp`: post-instruction boundary regression
  proving shadow closes before a scheduler/frame rebase.
- `tests/hybrid_generated_block_test.cpp`: generated semantics and
  instruction-boundary yield/resume regression for a frozen M11.38 range.
- `tests/hybrid_checkpoint_layout_test.cpp`: GPGX layout, representation-span,
  version-guard and cross-process canonicalization regression.
- `tests/hybrid_address_provenance_test.cpp`: address-class and observer
  aggregation regression for the developer-only runtime provenance path.
- `src/core/mechanical_primitive.*`: M11.50 portable primitive contract,
  executor and continuation state; contains no ROM candidate/opcode metadata.
- `src/core/table_copy_routine.*`: M11.51 tokenized structured table-copy
  routine contract and resumable executor; contains no ROM PC constants.
- `tests/table_copy_routine_test.cpp` and
  `tests/table_copy_routine_contract_test.cpp`: standalone semantic,
  continuation and synthetic CFG proof.
- `tests/hybrid_mechanical_primitive_test.cpp`: hybrid registry metadata and
  portable-contract conversion regression.
- `tests/check_core_boundary.cmake`: source scan for hybrid/GPGX/libretro and
  candidate-specific constants, paired with the CMake no-link assertion.
- `docs/reports/PORTABLE_MECHANICAL_PRIMITIVE_LAYER_M11_50.md`: exact before /
  after ownership, dependency audit and extraction proof.
- `docs/reports/BASIC_BLOCK_RECOMPILATION_TIMING_M11_32.md`: M11.32 identity,
  shadow, native override and state/video equivalence evidence.
- `docs/reports/DEMAND_DRIVEN_BLOCK_PROMOTION_M11_35.md`: bounded discovery,
  semantic/generation gate and first runtime shadow blocker; no promotion claim.
- `docs/reports/GPGX_TIMING_REFRESH_BRIDGE_M11_36.md`: counter ownership,
  frame epoch, bounded A–G boundary evidence and final three-candidate gate.
- `docs/reports/INTERRUPT_SAFE_MULTI_INSTRUCTION_BLOCKS_M11_38.md`: frozen
  candidate ledger, generic yield contract, per-boundary shadow and interrupted
  continuation evidence.
- `docs/reports/REMAINING_INTERPRETER_ATTRIBUTION_M11_40.md`: M11.40 baseline
  reproduction and checkpoint identity mismatch; no attribution or promotion.
- `docs/reports/CHECKPOINT_CANONICALIZATION_M11_43.md`: exact pinned GPGX
  wholesale-state layout, canonicalization contract and cross-process proof.
- `docs/reports/NATIVE_ROUTINE_MISMATCH_M11_52.md`: exact four-byte mismatch,
  temporal root cause, continuation repair and paired authoritative proof.

## M11.31 comparative method transfer
- `docs/reports/COMPARATIVE_DISASSEMBLY_METHOD_TRANSFER_M11_31.md`: pinned
  public Streets of Rage 2/3 project inventory, bounded similarity matrix,
  transferable workflow, Beyond Oasis dry-run and separated conclusions.

## Planned directories
Create these only when their milestone begins and evidence justifies the structure:

```text
src/game/entities/  Enemy/NPC/entity systems
src/game/scripts/   Event/script interpreter or translated semantics
src/audio/          Audio runtime/compatibility layer
src/platform/       Window/input/render/audio platform integration
```

## Rules
- Do not create a generic `utils` dumping ground. Name modules by responsibility.
- Do not create planned directories early merely to make the tree look complete.
- New top-level directories require an architecture decision.
- Keep executable/source/build-code files at or below 500 lines; prose/reference documentation is exempt from the numeric limit.
- When a source/build file approaches 400 lines, consider splitting before adding major functionality.
# M12-AUTO54 adds the PC-island table promoter

`src/tools/re_m12_pc_island_tables_promote.py` and
`tests/re_m12_pc_island_tables_test.py` record 15 exact PC-relative table and
text-literal contracts. Generated transaction output remains ignored/local.

# M12-AUTO53 adds the PC-relative lookup-family promoter

`src/tools/re_m12_pc_lookup_family_promote.py` and
`tests/re_m12_pc_lookup_family_test.py` record five fixed lookup-table
contracts. Generated transaction output remains ignored/local.
# M12-AUTO57 adds the bounded word-transform promoter

`src/tools/re_m12_bounded_word_transform_promote.py` materializes the exact
128-byte `0x03BD86` word table closed by routine `0x03B7C0` and its four
callers. `tests/re_m12_bounded_word_transform_test.py` checks the contract,
closed word count and manifest split; CTest registration is in
`cmake/m12_auto2.cmake`.
# M12-AUTO59 adds bounded word-copy provenance tooling

`src/tools/re_m12_bounded_word_copy_promote.py` materializes the two exact
word-copy tables closed by `0x002D66`; its regression test is
`tests/re_m12_bounded_word_copy_test.py`.

# M12-AUTO58 adds an exact static ASM island

`src/tools/re_m12_static_003260.asm` is the human-maintained exact source for
`0x003260..0x0032E8`; `re_m12_static_003260_roundtrip.asm` supplies isolated
external labels for vasm verification, and
`re_m12_static_003260_promote.py` performs the transaction. The regression test
is `tests/re_m12_static_003260_test.py`.
# M12-AUTO60 adds the contiguous static-island promoter

`src/tools/re_m12_contiguous_islands_promote.py` consumes the positive results
of the generated direct-caller-to-RTS audit in
`build/m12-auto60-island-audit/`; its regression test is
`tests/re_m12_contiguous_islands_test.py`. CTest registration is in
`cmake/m12_auto2.cmake`.

# M12-CARVER-1 adds Stage 1 evidence/gap orchestration

`src/tools/m12_carver.py` owns the deterministic half-open IntervalDB,
typed evidence, provenance graph, conflicts, fixed-point reconciliation, gap
analysis and campaign ranking. `src/tools/m12_carver_adapters.py` adapts only
the existing deterministic M12 evidence producer schemas. The command-line
entry point is `src/tools/re_m12_carver.py`; its generated DB/report stay in
ignored build output because they may reference an external user ROM and local
evidence. `tests/re_m12_carver_test.py` covers manifest coverage, ownership
preservation, conflict blocking, provenance cycles and deterministic output.

# M12-CARVER-2 adds graph-guided evidence expansion

`src/tools/m12_carver_expansion.py` and
`src/tools/re_m12_carver_expand.py` add a deterministic, analysis-only pass
over the confirmed 99-row field1 table and existing graphics census. It emits
typed pointer/resource graph records and non-owning raw-size hypotheses while
leaving existing promoters and the manifest unchanged. The expansion report
is documented in `docs/reports/THOR_ROM_CARVER_M12_STAGE2.md`; regression
coverage remains in `tests/re_m12_carver_test.py`.
Expansion helper contracts are covered by
`tests/re_m12_carver_expansion_test.py`.

# M12-CARVER-3 adds global runtime-evidence sweep orchestration

`src/tools/m12_carver_global_sweep.py` and
`src/tools/re_m12_carver_global_sweep.py` discover canonical-ROM-compatible
runtime, checkpoint, census and existing M12 promotion artifacts under an
input root, normalize global ROM spans, and emit a whole-ROM blocker census
without changing manifest ownership. `tests/re_m12_carver_global_sweep_test.py`
covers deterministic span merging and exhaustive unobserved-gap classification.

# M12-CARVER-4 adds global static consumer recovery

`src/tools/m12_carver_static_recovery.py` and
`src/tools/re_m12_carver_static_recovery.py` consume existing static reverse-
engineering artifacts globally. They emit typed ROM-reference census edges,
parser-contract observations, consumer families, container candidates and a
complete blocker map without writing manifest ownership or repeating runtime
capture. Regression coverage is in
`tests/re_m12_carver_static_recovery_test.py`; the result is documented in
`docs/reports/THOR_ROM_CARVER_M12_STATIC_CONSUMER_RECOVERY.md`.

# M12-CARVER-5 adds whole-ROM format/container reconstruction

`src/tools/m12_carver_format_reconstruction.py` and
`src/tools/re_m12_carver_format_reconstruction.py` fingerprint every UNKNOWN
range, solve bounded structural candidates, scan the existing BO grammar, and
attach non-owning typed evidence to the Carver IntervalDB. The regression test
is `tests/re_m12_carver_format_reconstruction_test.py`. Machine-readable
outputs remain ignored under `build/m12-carver-m12c5-format-reconstruction-c/`;
the result is documented in
`docs/reports/THOR_ROM_CARVER_M12_FORMAT_CONTAINER_RECONSTRUCTION.md`.

`src/tools/m12_selector_descriptor_grammar.py` emits the payload-free M12
selector/descriptor/dispatch/child-table grammar catalog and validates the
canonical ROM contracts without changing ownership. Its regression coverage is
`tests/m12_selector_descriptor_grammar_test.py`; CTest registration is in
`cmake/m12_auto2.cmake`. The generated JSON under `build/m12-gfx-runtime/` is
local ignored evidence and is not repository data.

# M12-AUTO62 adds bounded live-discovery comparison

`src/tools/thor_evidence/live_discovery_plan.py` creates a ROM-bound watch plan
from the current ownership manifest, Evidence Engine SQLite sidecars and prior
GPGX execution evidence. The developer-only Lua collector is
`src/tools/thor_evidence/capture/live_discovery.lua`; its launcher is
`src/tools/thor_evidence/capture/run_live_discovery.ps1`. The analyzer
`src/tools/thor_evidence/live_discovery_analyze.py` persists novelty classes,
deduplicated edges and fail-closed investigation records without changing
ownership. Regression coverage is in
`tests/thor_evidence_live_discovery_test.py`; the campaign result is documented
in `docs/reports/THOR_M12_AUTO62_LIVE_DISCOVERY.md`. Raw captures and reports
remain ignored under `build/thor-evidence/live-discovery/`.

# M12-AUTO63 adds focused register-slice investigation

`src/tools/thor_evidence/live_discovery_followup.py` ranks persisted AUTO62
novelties and emits one narrow static/request plan. The developer-only
`src/tools/thor_evidence/capture/focused_register_slice.lua` and
`run_focused_register_slice.ps1` capture only the selected register dependency
chain and target ROM reads. `src/tools/thor_evidence/live_discovery_followup_analyze.py`
requires source/consumer ordering, active-count and exact stride evidence; it
fails closed when any frontier is missing. `src/tools/re_m12_auto63_promote.py`
reuses the existing record-stream materializer and cannot run on a blocked
report. Regression coverage is in `tests/thor_evidence_followup_test.py` and
the result is documented in `docs/reports/THOR_M12_AUTO63_INVESTIGATION.md`.
Generated capture, static and promotion outputs remain ignored under
`build/thor-evidence/auto63-followup/`.

`src/tools/re_m12_child_tables_promote.py` applies the exact table promotion
contract to the finite selector child-table streams at `0x03B95E..0x03BA46`,
preserving the two-byte descriptor alias prefix. Its regression test is
`tests/re_m12_child_tables_promote_test.py`; generated transaction output
remains ignored/local.

`src/tools/m12_selector_control_analysis.py` proves the reset/main-dispatch
source of the `0x03A748` selector handler. `src/tools/m12_relative_table_analysis.py`
proves the bounded `0x03BDA6` relative-pair consumer grammar and fails closed
at its unresolved extent edge. Regression coverage is in
`tests/m12_selector_control_analysis_test.py` and
`tests/m12_relative_table_analysis_test.py`; local JSON outputs remain ignored
ROM-derived evidence. The combined result is documented in
`docs/reports/THOR_M12_SELECTOR_RELATIVE_CLOSURE.md`.

`src/tools/m12_b092_tail_dispatch.py` validates the finite second-level
dispatch rooted at `0x03B092`/`0x03B0A8`, its four-entry table at `0x03B0AA`,
all four RTS-closed targets, static I/O, and interpreter negative evidence.
Its regression test is `tests/m12_b092_tail_dispatch_test.py`; CTest
registration is in `cmake/m12_auto2.cmake`. Local JSON output is ignored
ROM-derived evidence. The result is documented in
`docs/reports/THOR_M12_SELECTOR_RELATIVE_CLOSURE.md`.

`src/tools/m12_indirect_body_dispatch.py` validates the two exact
selector-masked indirect body calls at `0x03AA28` and `0x03AAA8`, enumerates
their 15 unique targets, records static target I/O/call provenance, and
closes 14 RTS routine boundaries while classifying `0x03BA46` as a data alias.
Its regression test is `tests/m12_indirect_body_dispatch_test.py`; the CTest
registration is in `cmake/m12_auto2.cmake`. Local JSON output is ignored
ROM-derived evidence. The bounded result is documented in
`docs/reports/THOR_M12_SELECTOR_RELATIVE_CLOSURE.md`.

`src/tools/m12_a372_shadow_sat_producer.py` validates the bounded static
`0xA342..0xA438` producer, its two 9x8 root tables, D2/A5 dataflow, and the
`0xFF1858` root selector without emitting payload. Its regression test is
`tests/m12_a372_shadow_sat_producer_test.py`; CTest registration is in
`cmake/m12_auto2.cmake`. The result is documented in
`docs/reports/THOR_M12_A372_SHADOW_SAT_PRODUCER.md`.

`src/tools/m12_a372_caller_join.py` validates the static scheduler-to-A372
caller join, the `FF1858` writer context, and the bounded negative neighbor
result without emitting payload. Its regression test is
`tests/m12_a372_caller_join_test.py`; CTest registration is in
`cmake/m12_auto2.cmake`. The result is documented in
`docs/reports/THOR_M12_A372_CALLER_JOIN.md`.

`src/tools/re_bizhawk_m12_a372_runtime.lua` is a developer-only bounded
BizHawk register/root probe for the `A196 -> A342 -> A372` path. Its
payload-free validator is `src/tools/m12_a372_runtime_report.py`, tested by
`tests/m12_a372_runtime_report_test.py`; CTest registration is in
`cmake/m12_auto2.cmake`. The bounded negative result is documented in
`docs/reports/THOR_M12_A372_RUNTIME_CAPTURE.md`.

The independent non-UI BizHawk launcher and Lua baseline live in the ignored
`build/bizhawk-controlled-harness` directory. Its verified policy and exact
CLI/Lua sequence are documented in
`docs/reports/BIZHAWK_CONTROLLED_HARNESS_DIAGNOSIS.md`; the controlled
Right-versus-neutral register/root result and M12 fork ranking are documented
in `docs/reports/THOR_M12_CONTROLLED_RUNTIME_ROOT_CAPTURE.md`.

`src/tools/m12_shadow_sat_access_graph.py` is a payload-free direct-access
census for `FF1858`, `FF188A` and `FF188C`; its regression test is
`tests/m12_shadow_sat_access_graph_test.py`. The two-pass closure and writer
accounting are documented in
`docs/reports/THOR_M12_SHADOW_SAT_ACCESS_CLOSURE.md`.
# THOR Evidence Engine V1 FF13CC canary

`src/tools/thor_evidence/canary_engine.py` derives and validates the bounded
engine-derived provenance certificate from checked static JSON and sealed
dynamic events. `src/tools/thor_evidence/capture/canary.lua` and
`run_canary.ps1` are developer-only capture helpers. The existing
`schema.sql`/`store.py` sidecar now has separate operation-instance and
provenance-dependency tables. Regression and adversarial coverage is in
`tests/thor_evidence_v1_canary_test.py` and
`tests/fixtures/thor_evidence_v1_canary/negative_cases.json`; CTest registration
is in `cmake/m12_auto2.cmake`. The bounded result is documented in
`docs/reports/THOR_EVIDENCE_ENGINE_V1_FF13CC_CANARY.md` and governed by
ADR-0048.
# THOR Evidence Engine V2 RAM provenance

`src/tools/thor_evidence/ram_versions.py` is the reusable byte-version,
overlap, epoch and coverage-aware last-writer primitive. The existing
`schema.sql`/`store.py` sidecar persists RAM versions, write operations,
operation outputs and coverage certificates. `tests/thor_evidence_v2_ram_test.py`
and `tests/fixtures/thor_evidence_v2_ram/negative_cases.json` cover the
fail-closed V2 contract; CTest registration is in `cmake/m12_auto2.cmake`.
The bounded result is documented in
`docs/reports/THOR_EVIDENCE_ENGINE_V2_RAM_PROVENANCE.md` and governed by
ADR-0049.
`docs/reports/THOR_M12_AUTO67_6_REGISTER_PROVENANCE_GATE.md` and its JSON
counterpart record the negative register-version evidence gate: the current
O67V v2 capsules lack register values and bounded predecessor execution needed
to prove an A4/A5 reaching definition. No runtime code change was made.

`docs/reports/THOR_M12_AUTO67_6R3_LOW_OVERHEAD_PREHISTORY.md` and its JSON
counterpart record the negative low-overhead source audit: BizHawk TraceLogger
has no bounded machine-readable stream, the global Lua hook fails the real
performance gate, and the existing external GPGX block hook is not connected
to authoritative BizHawk QuickSave1 execution.

`src/tools/thor_evidence/capture/predecessor_burst.lua` is the developer-only
AUTO67.6R3B targeted producer-triggered, 64-record execution burst source;
`predecessor_disabled.lua` is its no-hook matrix control. The existing
`live_capsule.lua` selector, `live_capsule_status.lua` bounded status output,
and `auto67_runner.py` launcher expose the mode without changing the
authoritative runtime architecture. Regression coverage is in
`tests/thor_evidence_auto67_6r3b_test.py`; the real gate is documented in
`docs/reports/THOR_M12_AUTO67_6R3B_TARGETED_BURST_FEASIBILITY.md` and JSON.

`auto67_predecessor.py` now also exposes the reusable static
`register_writer_candidate_report()` used to generate generic A4/A5 writer
targets. `tests/thor_evidence_auto67_6r3c_test.py` covers its candidate and
runtime-configuration gate. The negative generic result is documented in
`docs/reports/THOR_M12_AUTO67_6R3C_GENERIC_REGISTER_WRITER_TARGETING.md` and
JSON.

`src/tools/thor_evidence/cartographer.py` is the MAP-1 persistent generic
provenance graph; `map_sources.py` imports bounded existing JSON evidence and
`m12_map1.py` runs the deterministic proof gate. Focused regression coverage is
`tests/thor_evidence_map1_test.py`. The authoritative checkpoint report is
`docs/reports/THOR_M12_MAP_1_GLOBAL_PROVENANCE_CARTOGRAPHER.md` with its JSON
counterpart; SQLite proof artifacts remain under ignored `build/`.

`src/tools/thor_evidence/capture/walker_two_window.lua` is the developer-only
WALKER-1 transport experiment: one bounded W1 window, a guarded global-hook-off
straight-line body, and one bounded W2 window. `src/tools/thor_evidence/walker1.py`
validates runtime identity, fail-closed joins, and stable Cartographer replay;
`tests/thor_evidence_walker1_test.py` covers the focused gate. The result is
documented in `docs/reports/THOR_M12_WALKER_1_TWO_WINDOW_SPARSE_ADVANCEMENT.md`
and its JSON counterpart; runtime SQLite and emulator artifacts remain under
ignored `build/`.

`src/tools/thor_evidence/auto67_live.py` owns occurrence-only Dispatcher
leasing: branch fingerprints are diagnostic, exact window items use
`dispatch_state`, and runtime occurrence identity is carried into worker
investigations and leases. `src/tools/thor_evidence/capture/live_opportunistic.lua`
publishes the source `epoch + seq` identity; `auto67_persistence.py` retains it
in worker provenance. Regression coverage is in
`tests/thor_evidence_dispatcher1_test.py` plus the updated
`tests/thor_evidence_auto67_test.py`. The checkpoint is documented in
`docs/reports/THOR_M12_DISPATCHER_1_OCCURRENCE_ONLY.md` and its JSON
counterpart.

`src/tools/thor_evidence/auto67_snapshot_admission.py` is a developer-only
1:1 snapshot admission probe. It freezes immutable occurrence-scoped snapshot
metadata into a fixed 16-slot pool, observes the existing Dispatcher/Worker
flow, and exposes occupancy, ownership and fail-visible pool-full metrics
without switching production AUTO67 or adding a queue. Its contract is tested
by `tests/thor_evidence_auto67_snapshot_admission_test.py` and documented in
`docs/reports/THOR_M12_AUTO67_NATIVE_RING_BRIDGE_1A_ADMISSION_1X.md`.

`src/tools/thor_evidence/auto67_native_snapshot.py` freezes and validates
actual experimental GPGX native-ring records at the existing occurrence
boundary, leases the exact immutable tuple through the fixed snapshot pool,
adapts it to the existing predecessor resolver input, and releases its slot
after the matching Worker returns. `capture/native_trace_snapshot.lua` invokes
the test-only BizHawk API synchronously; `tools/bizhawk-native-ring/bizhawk-native-snapshot-freeze.patch`
adds that API to the isolated managed host build. The experiment is opt-in in
`auto67_runner.py` and does not change production scheduling. Its native record,
identity, resolver and slot-lifecycle checks are in
`tests/thor_evidence_auto67_native_snapshot_test.py`; the runtime receipt is in
`docs/reports/THOR_M12_AUTO67_BIZHAWK_NATIVE_RING_BRIDGE_1B_FREEZE_WORKER_1.md`.

`src/tools/thor_evidence/auto67_worker_input_trace.py` provides a bounded,
opt-in equality trace for native-snapshot `BUS_WRITE_PC` events at Dispatcher
mailbox write, Worker mailbox read, and immediately before materialization. It
hashes the complete normalized event and recomputes the immutable frozen-record
SHA-256 at every point. The 1C regression in
`tests/thor_evidence_auto67_native_snapshot_test.py` exercises the real
Dispatcher/Worker mailbox path and reports per-field equality. The runtime
receipt is in
`docs/reports/THOR_M12_AUTO67_BIZHAWK_NATIVE_RING_BRIDGE_1C_WORKER_INPUT_TRACE.md`.

`tools/bizhawk-native-ring/live_forward_trace.[ch]` records one shared native
execution stream; its W1 sideband API preserves the fixed record ABI for bus
and frame events. The isolated GPGX checkout adds
`core/debug/live_forward_bus.c` for memory-map domain resolution and hooks
top-level 68K data accesses in `core/m68k/m68kcpu.h`; `cinterface.c` marks
authoritative frame advancement. `tests/live_forward_sideband_test.cpp`
covers ordered native event encoding. `live_forward_pool.c` owns checked dynamic pool allocation,
identity tables and memory plans; `live_forward_metrics.c` exports aggregate
and per-slot transition counters; `live_forward_worker.c` owns the Worker lifecycle,
bounded memory/retention termination, exact-range sealing, and immutable ACK
contract; `live_forward_flow.c` classifies M68K control flow. `live_forward.cmake`
builds developer-only native tests, including a 100-cycle-per-slot regression;
`live_forward_scaling.lua` and `live_forward_scaling_runtime.py` run progressive
scaling rounds and require two identical binary audits, per-slot transition
counters and exact ACK before slot reuse. `live_forward_scaling_stream.py`
tails the replaceable status stream without retaining cycle history, while
`live_forward_scaling_campaign.py` owns progressive count selection and its
CLI. `live_forward_scaling_audit.py`
checks IDs, stream sequence, depth, bounds and immutable copies.
`live_forward_runtime.lua` and `live_forward_runtime.py` run and verify the
single-Worker 1A proof. The
exact BizHawk/GPGX source patches and final runtime/boundary evidence are in
`tools/bizhawk-native-ring/*live-forward-worker-1a.patch` and
`docs/reports/THOR_M12_AUTO67_LIVE_FORWARD_WORKER_1A.md` plus JSON. Incremental
BizHawk and GPGX patches are in
`tools/bizhawk-native-ring/*live-forward-worker-1b.patch`; the 1B runtime gate
and runtime stop are recorded in
`docs/reports/THOR_M12_AUTO67_LIVE_FORWARD_WORKER_1B_SCALING.md` and its JSON
receipt. The BizHawk runtime campaign proves natural counts 1-64 and forced
counts 2-128 with 100 cycles and N*100 audited segments at each PASS count;
natural 128 and forced 256 stop before immutable reread when CPU execution
stalls. Per-count logs and audit JSONL files remain under the ignored
`build/thor-evidence/live-forward-worker-1b/` directory. This path does not
alter the live AUTO67 runner or Cartographer.

W1 native bus-event acceptance extends that developer-only path with the
canonical `*native-bus-event-sideband-v1.patch` pair and the
`*native-bus-event-sideband-v1-observability.patch` pair. The isolated GPGX
source owns bus hooks, map-domain resolution, event schema and frame emission;
the managed patch exposes only the frame-boundary witness API. The
`live_forward_sideband_probe.lua` and `run_sideband_probe.py` files run the
real micro probe; `run_counted_w1_acceptance.py` owns the exact final 128-Worker
record-count receipt. Exact source, build, 128-Worker, control-flow A/B,
witness and measured-cost evidence is recorded in
`docs/reports/THOR_M12_NATIVE_BUS_EVENT_SIDEBAND_V1.md`; raw receipts remain
under the ignored `build/m12-w1-acceptance/` tree.

W2 active-resource classification is worker-side only. `w2_resource_classification.py`
preserves copied W1 event fields, classifies only mapped primitive bus facts,
groups events by exact instruction sequence, emits bounded deterministic
candidates and conservative shadow-SAT/Z80 reports. `w2_vdp_decoder.py`
decodes observed VDP register/control writes and exact CPU DMA programming
fields without claiming internal DMA bytes. `run_w2_active_resource_classification.py`
reuses the audited scaling callback for a fresh short real-runtime receipt;
`tests/w2_active_resource_classification_test.py` covers the A–Q W2 contract.
The report is `docs/reports/THOR_M12_WORKER_ACTIVE_RESOURCE_CLASSIFICATION_V1.md`
with ignored raw output under `build/m12-w2-acceptance/`. This path does not
modify W1 capture, Worker scheduling, S1–S8 artifacts, or `SOURCE_OWNED`.

W2.1 frame-coherent evidence adds only segment-level `entry_frame` and
`exit_frame` snapshots to the accepted GPGX result metadata. The incremental
source-of-truth patches are
`tools/bizhawk-native-ring/*worker-frame-coherent-v1.patch`; the authoritative
counter remains W1's `frame_number`. `w2_frame_coherence.py` partitions only
when contained FRAME_BOUNDARY markers prove the transition, otherwise it emits
`MULTI_FRAME_UNRESOLVED`. `run_w2_frame_coherent_evidence.py` records exact
`(run_id, epoch, frame)` identity in W2 facts and keeps S1–S8 bridging
read-only. Its focused tests are in
`tests/w2_frame_coherent_evidence_test.py`; the acceptance report is
`docs/reports/THOR_M12_WORKER_FRAME_COHERENT_EVIDENCE_V1.md` with ignored raw
output under `build/m12-w2-1-acceptance/`. This adds no graphics semantics or
Z80 implementation and keeps `SOURCE_OWNED` unchanged.

# M12 AUTO67 Worker-clean boundary (2026-09-15)

`src/tools/thor_evidence/auto67_live.py` owns occurrence-only leasing and a
bounded factual Worker history (`recent_investigations`, 16 entries). It has no
unbounded investigation map or Worker novelty classifier. `auto67_capsule.py`
owns temporary 16-slot evidence resources and id-based release without semantic
flags. `auto67_materializer.py` keeps static causal facts and bounded
register/capture diagnostics; live Worker calls omit prescriptive frontiers.
`auto67_persistence.py` remains a downstream optional writer and preserves
occurrence provenance. `tests/thor_evidence_auto67_worker_clean_test.py`
covers the A-G Worker-clean and Dispatcher regression contract. The published
checkpoint is `docs/reports/THOR_M12_AUTO67_WORKER_CLEAN_1.md` plus JSON.
# M12 AUTO67 current-event ring boundary (2026-09-15)

`src/tools/thor_evidence/auto67_live.py::RollingWindow` is the bounded Python
current-event window for Dispatcher leasing. It owns only `capacity`,
`items` and `overwrites`; the exact item still carries `dispatch_state=LEASED`.
`src/tools/thor_evidence/capture/live_opportunistic.lua` remains the separate
bounded transport ring across the status poll boundary and preserves event
order and occurrence identity. The two rings are not merged and neither is a
backlog. `tests/thor_evidence_auto67_ring_clean_test.py` is the deterministic
Python/Lua-contract fixture for this checkpoint.

`src/tools/thor_evidence/auto67_transport.py` owns the bounded pre-dispatch
status cursor. It deduplicates replaceable Lua snapshots by epoch/sequence/
occurrence identity and passes current records directly to the Dispatcher;
`tests/thor_evidence_auto67_predispatch_transport_clean_test.py` covers epoch
rollover, duplicate/stale rejection, legacy key compatibility, and the
no-backlog boundary.

R1 keeps final-snapshot consumption on the same cursor: the launcher consumes
`lua_final` after the emulator exits, ingests those records, then stops the
Dispatcher. The live-opportunistic runner has no state option or
`OASIS_LIVE_STATE` export; savestate-aware capture scripts are separate.

# M12 AUTO67 minimal mailbox boundary (2026-09-15)

`auto67_live.py::Dispatcher` sends one detached event message per worker. The
message carries one occurrence-derived `investigation_id` and one `lease_id`
through CapsulePool and persistence; branch/context, seed, duplicate
occurrence, worker and capsule-lease mailbox fields are removed. Worker
history remains bounded and factual. The contract is covered by
`tests/thor_evidence_auto67_mailbox_clean_test.py` and the checkpoint report
`docs/reports/THOR_M12_AUTO67_MAILBOX_CLEAN_1.md` plus JSON.

# M12 AUTO67 Cartographer seam (2026-09-15)

`auto67_cartographer.py` converts Worker local-chain results into a stable
MAP-1 bundle only for the predecessor-produced
`REGISTER_REACHING_DEFINITION` proof contract. The existing Cartographer owns
PROVEN, duplicate, conflict and component decisions. `LivePersistenceSink`
feeds the adapter through its single bounded queue and retains the temporary
`live_chain` compatibility write. `--map-db` supports map-only mode; combining
it with `--chain-db` supports compatibility mode. Regression coverage is in
the writer-thread ownership, all-step canonicalization and drop metrics in
`tests/thor_evidence_auto67_cartographer_seam_test.py` and the report
`docs/reports/THOR_M12_AUTO67_CARTOGRAPHER_SEAM_1.md` plus JSON.
The follow-up removes live-chain output and is recorded in
`docs/reports/THOR_M12_AUTO67_LEGACY_CHAIN_CLEAN_1.md` plus JSON.

# M12 Archivist → Canonical Knowledge Pipeline 2G

`src/tools/thor_evidence/live_forward_archivist.py` emits a compact
content-addressed merge receipt. `rom_knowledge_live_import.py` validates the
MAP-1/Archivist chain and relation vocabulary; `rom_knowledge_live_delta.py`
translates one accepted session into stable 2D objects, claims, relations and
evidence. `rom_knowledge_pipeline.py` builds an ignored versioned pair of
master/knowledge SQLite databases, independently audits it, then atomically
selects the pair through `current.json`. `rom_knowledge_pipeline_audit.py`
recomputes graph and canonical evidence without using importer calculations.
The A–O contract plus unsupported-fact stop are in
`tests/rom_knowledge_pipeline_test.py`, registered in
`cmake/m12_auto2.cmake`; the canonical result is in
`docs/reports/THOR_M12_ARCHIVIST_KNOWLEDGE_PIPELINE_2G.md` and its compact JSON
receipt. Runtime databases and raw evidence remain ignored under `build/`.

# M12 Live Worker Control Window 2H

`tools/bizhawk-native-ring/live_forward_worker_control_model.py` validates
unbounded positive next-run values, shares the launcher's resource-budget
arithmetic, and decodes bounded Worker/status records. The host-only
`live_forward_worker_control.py` publishes a replaceable 4 Hz snapshot and
measured evidence growth; `live_forward_worker_control_window.py` renders it
in a separate Tk process and writes only next-run configuration. The Lua
status read uses the native read-only `oasis_lf_worker_status_get` API declared
in `live_forward_trace.h` and implemented in `live_forward_metrics.c`.
Regression coverage is in `tests/live_worker_control_test.py` and the native
status assertions in `tests/live_forward_scaling_test.cpp`. Incremental
BizHawk/GPGX integration patches are
`tools/bizhawk-native-ring/bizhawk-2.11.1-live-worker-control-2h.patch` and
`tools/bizhawk-native-ring/genesis-plus-gx-live-forward-worker-control-2h.patch`.
The runtime proof and compact receipt are
`docs/reports/THOR_M12_LIVE_WORKER_CONTROL_WINDOW_2H.md` and its JSON
counterpart. Runtime/session databases and segment evidence remain ignored
under `build/thor-evidence/`. The optional desktop `--until-closed` mode keeps
the same Worker/1B lifecycle and bounded 2H window active until EmuHawk exits;
`live_forward_segment_spool.py` appends individually host-audited FLOW_V1 bytes
and segment indexes with constant-memory bookkeeping. It stops cleanly at a
1 GiB free-disk reserve, labeled separately from RAM in the status window, and
does not run Cartographer/Archivist or claim a bounded campaign PASS. The
normal bounded 2B proof path remains unchanged.
`live_forward_scaling_stream.py` tails the append-only Lua status stream and
preserves SEG, LIFECYCLE, and round-ACK ordering during host validation so a
single read spanning adjacent rounds cannot validate an older lifecycle receipt
against already-advanced counters.
The 2H Worker rows show per-Worker start, completion, ACK, host-audited capture,
and latest/cumulative FLOW_V1 record counts; the host publisher copies these
summaries only for the currently visible status page.
Its operating limits and verification record are in
`docs/reports/THOR_M12_LIVE_WORKER_INTERACTIVE_MODE.md`.
# M12 2I post-run orchestration

The developer-only live-forward tools now include `live_forward_rolling_master.py`
for compact, hash-bound FLOW_V1 rolling generations and
`live_forward_postrun_window.py` for the post-run stage/result window. The
existing Worker Control window publishes a single idempotent `END GAME`
request; Lua consumes it only between complete rounds and closes EmuHawk
through the supported exit path.

`live_forward_progress.py` owns the atomic post-run heartbeat/status contract;
`live_forward_postrun_window.py` renders determinate or indeterminate stage
progress, heartbeat age, partial/full terminal state, and a separate DETAILS
view. `live_forward_rolling_master.py` publishes snapshots while auditing and
compacting the sealed FLOW run.
# M12 end-game post-run stages 5–9 (2I.2)

`tools/bizhawk-native-ring/live_forward_complete_pipeline.py` is the single
post-run coordinator after rolling-master compaction. It builds a closed
session only from sealed ordered FLOW, performs an atomic canonical generation
refresh when accepted inputs exist, checks ordered control provenance, and
stops before unsupported reconstruction or destructive cleanup. It reuses the
2I.1 `live_forward_progress.py` status contract. Regression coverage is in
`tests/live_forward_complete_pipeline_test.py`.
# M12 2I.2a terminal post-run state

`tools/bizhawk-native-ring/live_forward_progress.py` owns the single terminal
finalizer and status invariants. It distinguishes RUNNING/UNRESPONSIVE from
terminal COMPLETE, PARTIAL_COMPLETE, STOPPED and FAILED states, and records
BLOCKED stages after a mandatory STOP/ERROR. The post-run window maps terminal
states to backend-complete or backend-stopped text without a false heartbeat
warning. Deterministic coverage is in
`tests/live_forward_postrun_progress_test.py`.
### M12 Stage 5 refresh additions

`tools/bizhawk-native-ring/live_forward_complete_pipeline.py` owns the
run-scoped compact FLOW session, exact ROM-range linkage, canonical ownership
audit, and atomic post-run generation publication. The Stage 5 regression
coverage is in `tests/live_forward_stage5_test.py`.
* `src/tools/thor_evidence/map_driven_asm_closure.py` — pure 2F/2I.3 closure
  gates and deterministic ownership partition rules.
* `src/tools/thor_evidence/map_driven_asm_stage7.py` — current canonical
  generation Stage 7 coordinator; Stage 8 is invoked by the post-run
  coordinator after the atomic Stage 7 terminal receipt.
* `src/tools/thor_evidence/stage7_subprocess.py` — bounded Stage 7/8 external
  tool runner with heartbeat-safe output tails and separate fail-closed
  Stage 7/Stage 8 timeout codes.
* `tests/map_driven_executed_asm_closure_test.py` — historical synthetic 2F
  proof gates; `tests/map_driven_stage7_test.py` — Stage 7 artifact split
  regression; `tests/stage7_subprocess_test.py` — event-driven wait, pipe
  draining, timeout, crash and kill-fallback regressions.

* `tools/bizhawk-native-ring/live_forward_control_provenance_stage.py` — receipt-bound Stage 6 ordered-FLOW scan and structured lifecycle result.
* `tests/live_forward_control_provenance_stage_test.py` — Stage 6 NO_DELTA and fail-closed consumer contract tests.

* `src/tools/thor_evidence/map_driven_stage7_preflight.py` — lineage-bound
  Stage 7 canonical map versus emission-manifest authority resolution and
  diagnostic receipt generation.
* `tests/map_driven_stage7_preflight_test.py` — matching and fail-closed
  emission-partition preflight regressions.
* `tests/live_stage7_stage8_stage9_contract_test.py` — structural handoff
  contract ensuring Stage 7 returns before Stage 8 and Stage 9 cleanup.

* `tools/bizhawk-native-ring/live_forward_control_provenance_stage.py` —
  Stage 6 diagnostic receipt, integrity-stop classification, and unresolved
  consumer NO_DELTA contract.
* `tools/bizhawk-native-ring/live_forward_postrun_window.py` — exact STOP/ERROR
  code rendering in the current-stage and stage-list UI.
* `tests/live_forward_control_provenance_stage_test.py` — unresolved-consumer
  NO_DELTA and FLOW-corruption STOP regressions.

# M12 Stage 9 absorbed raw permanent reclaim 2I.5

`tools/bizhawk-native-ring/live_forward_absorption_cleanup.py` owns the
fail-closed absorption proof, semantic-equivalence audit, atomic delete
manifest/receipt publication, bounded WinError 32 retry and direct permanent
deletion of only run-scoped redundant artifacts. The accepted rolling master,
canonical map, Stage 5–8 receipts, final report/status and ROM remain protected.
`tests/live_forward_absorption_cleanup_test.py` covers lineage/stage gates,
semantic failure, permanent deletion, lock retry/stop, replay idempotence,
protected paths and post-delete integrity. Cleanup UI details are rendered by
`tools/bizhawk-native-ring/live_forward_postrun_window.py`.

## M12 RAM migration R1 shadow representation

`tools/bizhawk-native-ring/master_v2_shadow.py` is a read-only shadow encoder
for the accepted rolling master and canonical map. It produces a deterministic
self-contained container with streamed sections for rolling facts, canonical
map, canonical knowledge/ownership/emission and normalized Stage 5–9 outcomes.
The footer verifies section hashes and the overall logical content hash. It
never writes `current.json`, deletes legacy artifacts, or changes runtime
capture and cleanup authority. `tests/master_v2_shadow_test.py` covers
round-trip equality, deterministic bytes, corruption/missing-section rejection
and legacy pointer preservation.

## M12 RAM migration R2 — MASTER V2 canonical read authority

- `tools/bizhawk-native-ring/master_canonical_view.py` — single canonical
  access layer backed by verified MASTER V2 sections; provides map objects,
  ranges, instruction facts, runtime relations, ownership/emission partition,
  lineage and legacy-pointer shadow verification, plus stage-local SQLite
  materialization.
- `tests/master_canonical_view_test.py` — API, relation, ownership,
  materialization and legacy-shadow contract tests.
- `build/thor-evidence/master-v2-shadow-r2/` — ignored R2 shadow and current
  pointer evidence; legacy canonical files remain the comparison authority.

## M12 RAM migration R3 — provenance and outcomes authority

- `tools/bizhawk-native-ring/master_outcome_view.py` — verified read API for
  MASTER V2 provenance, Stage 5–9 outcomes and absorbed-run history; missing or
  corrupt sections fail closed without a legacy fallback.
- `tests/master_outcome_view_test.py` — projection, corruption and authority
  contract tests.
- `build/thor-evidence/master-v2-shadow-r3/` — ignored R3 shadow container,
  deterministic acceptance report and authority pointer; legacy receipts stay
  present for shadow comparison.

## M12 RAM migration R4 — MASTER V2 startup authority

- `tools/bizhawk-native-ring/master_startup_authority.py` — fail-closed
  MASTER V2 plus canonical ROM startup validator, explicit legacy shadow mode,
  startup IO metrics and deterministic N+1 candidate boundary.
- `tools/bizhawk-native-ring/live_forward_rom_link_runtime.py` — optional
  `--master-startup` preflight used by the desktop launcher before BizHawk.
- `tests/master_startup_authority_test.py` — MASTER-only startup, integrity,
  no-legacy-read and continuation regressions.
- `build/thor-evidence/master-v2/current.json` — ignored atomic startup pointer;
  it references the accepted R3 MASTER without replacing legacy artifacts.

## M12 R5 in-process post-run

- `tools/bizhawk-native-ring/live_forward_postrun_coordinator.py` — explicit
  Stage 1–9 in-process coordinator, context/result objects, and event sink.
- `tests/live_forward_postrun_coordinator_test.py` — event, background lifetime,
  STOP/ERROR, and diagnostic snapshot contracts.
- `tools/bizhawk-native-ring/live_forward_rom_link_runtime.py` — selects the
  in-process coordinator for MASTER-authorized interactive runs while retaining
  the old subprocess path.

## M12 V2 run contribution reconciliation

- `tools/bizhawk-native-ring/master_v2_contribution_boundary.py` — run-scoped
  contribution hashes, apply-once/NO_DELTA/conflict handling, VERIFY_ONLY,
  forensic no-write replay, and streaming publication of the hashed
  `run_contributions` MASTER V2 section.
- `tests/master_v2_contribution_boundary_test.py` — occurrence-safe idempotent
  replay, conflict, forensic, absorption-gate and SOURCE_OWNED regressions.
- `build/thor-evidence/historical-run-contribution-ledger.json` — ignored
  forensic ledger for all historical entries; raw evidence remains external.

## M12 V2 runtime authority

- `tools/bizhawk-native-ring/master_v2_runtime_bridge.py` — materializes a
  campaign-local rolling scratch from V2 and atomically promotes a sealed
  post-run contribution back into the current MASTER V2.
- `build/thor-evidence/master-v2/current.json` and its resolved binary — the
  single persistent canonical/provenance/outcome read and write authority.

## M12 RAM migration R6 — Stage 5 in-memory Cartographer

- `tools/bizhawk-native-ring/stage5_session_memory.py` — production Stage 5
  RAM session, exact FLOW aggregation, semantic witnesses, and ephemeral exact
  decoder adapter.
- `tools/bizhawk-native-ring/stage5_runtime_occurrences.py` — raw-record-bound
  occurrence witnesses persisted into the in-memory Stage 5 MAP-1 session.
- `tools/bizhawk-native-ring/stage5_disk_legacy.py` — retained disk-backed
  Stage 5 implementation for explicit shadow/regression comparison only.
- `tools/bizhawk-native-ring/live_forward_complete_pipeline.py` — selects the
  RAM Stage 5 path while keeping canonical candidate/master publication on
  disk and releasing the session before Stage 6.

## M12 RAM migration R7 — in-memory FLOW handoff

- `tools/bizhawk-native-ring/flow_stream.py` — bounded ordered FLOW chunks,
  backpressure, hashes and zero-disk accounting.
- `tools/bizhawk-native-ring/stage5_stream.py` — incremental Stage 5 RAM
  consumer that builds the existing `Stage5SessionMemory` contract.
- `tools/bizhawk-native-ring/flow_handoff_runtime.py` — runtime owner and
  consumer thread that fans chunks to Stage 5 and control provenance.
- `tools/bizhawk-native-ring/live_forward_control_provenance_stage.py` —
  incremental control-provenance consumer with the legacy disk comparator
  retained.
- `tests/flow_stream_test.py` — queue ordering, backpressure, hash and failure
  propagation regressions.

## M12 RAM migration R8 — analysis performance

- `src/tools/thor_evidence/stage7_decode.py` — bounded parallel external
  candidate decoder with stable ordinal results and fail-closed error capture.
- `src/tools/thor_evidence/map_driven_asm_stage7.py` — uses the bounded decode
  batch while retaining ordered proof, promotion and merge.
- `tools/bizhawk-native-ring/live_forward_postrun_coordinator.py` — publishes
  post-run timing, throughput, queue/RAM peaks and R7 invariant counters.
- `tests/stage7_decode_test.py` — stable ordering and decode-error regressions.
- `tests/stage7_decoder_contract_test.py` — Stage 7 range-tool capability,
  fail-closed classification, and canonical-ROM decode fixtures.
- `tests/live_forward_postrun_coordinator_test.py` — performance report and
  invariant accounting regression.

## M12 R9 — measured decode deduplication and Stage 7 snapshot reuse

- `src/tools/thor_evidence/stage7_decode.py` — complete decode request-key telemetry, duplicate accounting, bounded pool metrics and cross-iteration metric summarization; no persistent cache.
- `src/tools/thor_evidence/map_driven_asm_stage7.py` — reuses unchanged SQLite snapshots while preserving fresh reads after promotion.
- `tests/stage7_decode_test.py` — key distinction, duplicate measurement, cross-iteration summary and stable failure/order regressions.

## M12 Worker W6 — sustained live game discovery run and analysis

- `src/tools/thor_evidence/w6_live_discovery.py` — long-run live discovery session orchestrator and wave chunk manager for 128 Workers at depth 512.
- `src/tools/thor_evidence/w6_discovery_analysis.py` — post-run discovery and novelty analysis engine classifying execution, memory, I/O, VDP, and audio activity against accepted Thor Brain with zero SOURCE_OWNED promotion.
- `tests/w6_live_discovery_test.py` — unit and integration test suite covering canonical constants, ROM verification, chunk scanning, interval merging, novelty classification, schema verification, and SOURCE_OWNED conservation.
- `build/m12-w6-live-discovery/` — directory containing 106 raw wave chunks and all 13 canonical discovery artifacts.

## Desktop Worker Control launcher update (W6/W3 coherent runtime)

- `tools/bizhawk-native-ring/live_forward_worker_control_launcher.py` — dedicated desktop launcher script running with zero mandatory arguments, defaulting to 128 workers, depth 512, 512 KiB memory, verified BizHawk install, canonical ROM, and scaling Lua script.
- `tools/bizhawk-native-ring/live_forward_worker_control.py` — host-side status publisher with updated 512 KiB memory configuration and direct CLI launcher entry point.
- `tools/bizhawk-native-ring/live_forward_worker_control_model.py` — resource budget and next-run configuration model with defaults updated to 128 workers and depth 512.
- `tools/bizhawk-native-ring/live_forward_rom_link_runtime.py` — runtime coordinator with modular `build_parser()`, legacy disassembler prerequisites removed/optional, and 48-byte segment spooling for interactive sessions.
- `tools/bizhawk-native-ring/thor_worker_control_desktop.ps1` — repository PowerShell launcher script backing the Windows desktop shortcut for W6/W3 coherent runtime.
- `tests/live_forward_worker_control_launcher_test.py` — regression tests for parser defaults, custom parameter overrides, fallback configuration, and launcher callability.

## M12 VDP / DMA Analysis and Durable Real VDP Oracle V2

- `tools/bizhawk-native-ring/live_forward_vdp_stage.py` — fail-closed VDP and DMA analysis stage for M12 post-run pipeline, decoding control commands, registers, DMA operations, and verifying against Durable Real VDP Oracle V2.
- `tools/bizhawk-native-ring/capture_real_vdp_oracle.py` — capture script using real BizHawk runtime to freeze Durable Real VDP Oracle V2 (`vdp_oracle_v2_f200`) with raw event capture, logical frame artifact, manifest, and receipt.
- `docs/reports/THOR_M12_VDP_ORACLE_V2_RECEIPT.json` — non-copyrighted checked-in receipt for Durable Real VDP Oracle V2 with SHA256 hashes, metadata, and zero SOURCE_OWNED delta.
- `tests/live_forward_vdp_stage_test.py` — unit and regression test suite verifying VDP/DMA decoding, register matching, destination domains, DMA classification, and Durable Real VDP Oracle V2 validation.
# M12 targeted dynamic sprite capture

- `src/tools/re_bizhawk_m12_targeted_dynamic_sprite.lua` — developer-only
  natural-input BizHawk collector; snapshots the SAT window and accepted
  `0xFF13CC` shadow range every frame and records shadow write PCs.
- `src/tools/m12_targeted_dynamic_sat_capture.py` — fail-closed entry-level
  SAT/shadow diff and writer-window analyzer; does not promote ownership or
  gameplay semantics.
- `tests/m12_targeted_dynamic_sat_capture_test.py` — focused analyzer
  regression tests.
# M12 dynamic SAT producer backtrace

- `src/tools/re_bizhawk_m12_dynamic_sat_backtrace.lua` — developer-only
  runtime register/source capture for the bounded `0xB74E..0xB78C` producer.
- `src/tools/m12_dynamic_sat_producer_backtrace.py` — static decode, exact
  ROM-source transform, multi-frame validation, and four-artifact receipt.
- `tests/m12_dynamic_sat_producer_backtrace_test.py` — writer mapping and
  transform regression tests.
- `docs/reports/THOR_M12_DYNAMIC_SAT_PRODUCER_BACKTRACE_V1.md` — checkpoint
  report and evidence boundary.

# M12 dynamic SAT ROM index / A0 provenance

- `src/tools/m12_dynamic_sat_rom_index_provenance.py` — fail-closed analyzer
  for A0 reaching definitions, ROM-index decomposition, caller candidates,
  and multi-frame provenance receipt.
- `docs/reports/THOR_M12_DYNAMIC_SAT_ROM_INDEX_A0_PROVENANCE_V1.md` — STOP
  report preserving the unresolved B730-entry caller boundary.

# M12 dynamic SAT caller-side A0 provenance

- `src/tools/m12_dynamic_sat_caller_a0_provenance.py` — caller-side A0
  backtrace analyzer and multi-frame receipt.
- `docs/reports/THOR_M12_DYNAMIC_SAT_CALLER_A0_PROVENANCE_V1.md` — accepted
  caller/control-transfer and A0 reaching-definition report.

# M12 dynamic SAT selector / A6 provenance

- `src/tools/m12_dynamic_sat_selector_a6_provenance.py` — A6 field writer,
  selector-to-ROM mapping, and multi-frame provenance analyzer.
- `docs/reports/THOR_M12_DYNAMIC_SAT_SELECTOR_A6_PROVENANCE_V1.md` — accepted
  selector/A6 provenance report.

# M14.7B normalized-generic v2 adapter

- `src/tools/thor_evidence/normalized_v2_canonical_adapter.py` streams only
  producer-defined primary `records[]`, maps exact existing M68K instructions,
  preserves per-ordinal evidence provenance, and accounts unresolved/rejected
  records without skipping.
- `src/tools/thor_evidence/rom_knowledge_hash_stream.py` computes existing
  canonical logical hashes directly from ordered SQLite rows with bounded
  memory.
- `src/tools/thor_evidence/rom_knowledge_fusion_stream.py` provides the shared
  large-JSON reader and bounded structural skipping used by the normalized-v2
  adapter.
- `tests/normalized_v2_canonical_adapter_test.py` covers primary-array
  accounting, exact instruction linking, unresolved/rejected preservation,
  replay idempotence, hash compatibility, and staged canonical publisher
  integration.
# Runtime ROM property map v1

- `src/tools/thor_evidence/runtime_rom_properties.hpp/.cpp` implement the
  dense observed-property map, dirty pages, range conversion, identity checks,
  versioned checkpoint codec, atomic checkpoint persistence, and rebuildable
  compatible-run unions.
- `src/tools/thor_evidence/runtime_rom_observation.hpp/.cpp` validate supported
  M68K decoder spans, exact physical-offset data reads, captured Z80 byte
  spans, exact-copy RAM origins, and consumed VDP source bytes.
- `src/tools/thor_evidence/runtime_rom_properties_contract.json` is the pinned
  bit-level proof and rejection contract. Its exact SHA-256 is embedded in the
  checkpoint schema.
- `src/tools/thor_evidence/runtime_rom_properties_tool.cpp` exports complete
  `[start,end)` property ranges and merges per-run checkpoint contributions
  with identity checks and a source-file checksum manifest.
- `src/tools/thor_evidence/runtime_rom_map_integration.py` validates range
  exports and overlays their property masks on canonical boundaries while
  preserving canonical ownership.
- `tools/bizhawk-native-ring/rom_properties_core.c/.h` provide the native
  property bitmap, counters, dirty pages, and exact-span collector used by
  instrumented GPGX.
- `tools/bizhawk-native-ring/rom_properties_runtime.c/.h` adapt pinned M68K,
  Z80, direct VDP DMA, completed same-instruction ROM `MOVE.W` data-port
  writes, and exact Format-A decoder reads to fail-closed physical ROM offsets.
- `tools/bizhawk-native-ring/apply_runtime_rom_properties_v1.py` applies the
  pinned BizHawk/GPGX integration; `GPGX.LiveRomProperties.cs` and
  `runtime_rom_properties_capture.lua` expose bounded post-run map export.
  `runtime_rom_properties_parity.lua` records per-frame M68K/Z80 registers,
  deterministic RAM markers, and RAM hashes; `runtime_rom_properties_acceptance.py`
  runs matched parity and performance samples while restoring the corrected
  core afterward. `runtime_rom_properties_scenario_capture.lua` runs a bounded,
  deterministic input schedule and exports an independently identified map.
  `runtime_rom_properties_parity.lua` records bounded per-frame M68K/Z80
  registers, deterministic RAM markers, and RAM hashes for parity runs.
- `cmake/runtime_rom_properties_core.cmake` and
  `tests/runtime_rom_properties_core_test.c` build and test the native C map.
- `cmake/project_tests.cmake` holds test and test-tool target declarations
  separated from the root build file to keep each CMake source below 500 lines.
- `tests/runtime_rom_properties_tool_test.py` also tests strict live hex import
  into a ROM/core-identified checkpoint.
- `tests/runtime_rom_properties_z80_arg16_test.py` guards two operand-byte
  reads, exact Z80 PC advancement by two, and following-opcode alignment.
- `tests/runtime_rom_properties_vdp_hook_test.py` guards the Mode 5 CPU data
  port hook and its same-instruction source gate.
- `tests/runtime_rom_properties_audio_hook_test.py` guards the existing Z80
  banked-read hook, exact decoder PCs, and Format-A source bounds.
- `tests/runtime_rom_properties_graphics_hook_test.py` guards the completed
  `0x3820` input-span contract, exact ROM continuity checks, and compressed
  graphics property declaration.
- `tests/runtime_rom_properties_z80_arg16_test.py` guards the patched Z80
  two-byte operand read, exact PC advance, and following-opcode alignment.
- `tests/runtime_rom_properties_test.cpp` and
  `tests/runtime_rom_map_integration_test.py` cover maps, checkpoint integrity,
  merge algebra, supported instruction spans, origins, range coverage, and
  canonical ownership invariance.
- `tests/runtime_rom_properties_fixture.cpp` and
  `tests/runtime_rom_properties_tool_test.py` exercise checkpoint merge,
  source manifest checksums, CLI range export, duplicate-run rejection, and
  incompatibility rejection end-to-end.
- `docs/ROM_PROPERTY_MAP.md` defines proof contracts and implementation
  boundaries.
- `docs/reports/THOR_RUNTIME_ROM_PROPERTIES_RESEAL_20260927.md` records the
  corrected runtime manifest/checkpoint identity, exact replay results,
  bounded parity, repeated A/B/C performance measurements, and M2–M9 status.
- `docs/reports/THOR_RUNTIME_ROM_PROPERTIES_EXPANSION_20260927.md` records the
  longer corrected-runtime capture, exact range replay and read-only canonical
  overlay, expanded direct-class coverage, and remaining proof gaps.
- `docs/reports/THOR_ROM_PROPERTY_CONSUMER_CLASSIFICATION_20260927.md` records
  the bounded direct-VDP and audio consumer iterations, fresh audio checkpoint
  replay, classified-coverage delta, parity, and benchmark limitations.
- `docs/reports/THOR_ROM_PROPERTY_GRAPHICS_CONSUMER_20260927.md` records the
  live `0x3820` compressed-source property, sealed checkpoint, exact replay,
  bounded parity, and map-off/map-on timing.
- `tools/bizhawk-native-ring/rom_coverage_gui.py` displays sealed property
  checkpoints in a read-only address-ordered Defraggler-style grid, with
  layer switcher ("PROPERTY VIEW" vs "EVIDENCE / RECOMPILATION VIEW"),
  evidence category mapping, runtime heat overlay, and deep inspection;
  `rom_coverage_model.py` validates checkpoint/overlay identities and computes
  byte-accurate cell states, coverage summaries, and filters.
- `tools/bizhawk-native-ring/evidence_overlay_model.py` provides read-only visual
  categorization, runtime heat intensity calculation, RAW_HOT_RANK, and RECOMP_CANDIDATE_RANK
  filtering out active native, verified, and non-code routines.
- `tools/bizhawk-native-ring/evidence_inspector_panel.py` provides the read-only
  Evidence Inspector panel formatting 18 verified fields, cell drawing with heat overlay,
  and dynamic legend generation.
- `tools/bizhawk-native-ring/evidence_knowledge_bridge.py` provides the read-only bridge
  joining knowledge.sqlite, machine receipts, canonical census, and C++ steppers.
- `tools/bizhawk-native-ring/generate_verification_receipts.py` generates the machine-readable
  verification receipt index with canonical timestamps and stream hashes.
- `tests/rom_coverage_model_test.py` covers checkpoint integrity, overlay
  pairing, exact cell partitioning, overlapping properties, coverage deltas,
  and property filters.
- `tests/evidence_knowledge_bridge_test.py`, `tests/evidence_overlay_model_test.py`,
  and `tests/rom_coverage_gui_evidence_test.py` cover machine receipt verification,
  activation provenance, candidate ranking, layer switching, heat overlay, and inspector UI.
- `tools/bizhawk-native-ring/rolling_master_map_gui.py` opens run-local rolling
  MASTER SQLite snapshots in a read-only ROM address grid, with occurrence heat
  and click-through 1 KiB, 32-byte, and byte detail. It explicitly separates
  runtime PCs from byte ownership and asset proof; `rolling_master_map_model.py`
  validates schema/ROM identity and performs bounded cell and instruction reads.
- `tools/bizhawk-native-ring/live_forward_postrun_map_panel.py` asynchronously
  follows a run-local MASTER pointer and renders the verified runtime-address
  overview in the post-run progress window; it never writes evidence.
- `tools/bizhawk-native-ring/live_forward_postrun_window.py` keeps the stage
  monitor responsive and provides the post-run runtime-map tab after live worker
  capture ends.
- `tests/rolling_master_map_model_test.py` covers read-only access, ROM/schema
  identity, range validation, cell aggregation, and instruction lookup for the
  rolling MASTER viewer.
- `src/tools/thor_evidence/consumer_coverage_backlog.py` validates the current
  property checkpoint against the local ROM and contract hash, verifies
  accepted W4/W5 receipt links, and writes a read-only ranked backlog in JSON
  and Markdown. Candidate overlap is an upper bound; it never modifies the
  checkpoint, property masks, or canonical ownership.
- `tests/consumer_coverage_backlog_test.py` covers interval union,
  observed-only intersection, fail-closed bounds, accepted evidence identity,
  and current backlog metrics. The generated report is
  `docs/reports/CONSUMER_COVERAGE_BACKLOG_20260928.md` with a JSON companion.
- `src/tools/thor_evidence/m14_consumer_evidence_import.py` validates accepted
  W5/W4 evidence and emits a separate sparse `ACCEPTED_ARTIFACT` contribution
  plus invariant report; it never changes the runtime checkpoint or canonical
  SQLite. `tests/m14_consumer_evidence_import_test.py` covers W5 secondary
  resource acceptance, identity/schema/proof/boundary rejection, exact replay,
  deterministic replay, removal, and authority invariance. Outputs are written
  under ignored `build/m14-consumer-evidence-import/`.
- `src/tools/thor_evidence/m14_gfx_accepted_artifact_import.py` reconstructs
  the accepted M12-GFX-2 closure for `[0x18A4BE,0x18CC6E)` from pinned Git
  evidence, replays the exact independent decoder boundary, and emits a
  removable `COMPRESSED_GRAPHICS_SOURCE` artifact contribution. Its focused
  tests are in `tests/m14_gfx_accepted_artifact_import_test.py`; generated
  artifact, contribution, and machine report remain under ignored
  `build/m14-consumer-evidence-import/`. The checked human report is
  `docs/reports/THOR_M14_GFX_ARTIFACT_IMPORT_18A4BE_18CC6E_20260928.md`.
- `src/tools/thor_evidence/m14_gfx_accepted_artifact_batch.py` reconstructs
  and imports the accepted TOP-20 exact M12-GFX source ranges through the same
  proof contract, with a separate removable ledger per range. Its tests are in
  `tests/m14_gfx_accepted_artifact_batch_test.py`; batch outputs remain under
  ignored `build/m14-consumer-evidence-import/`. The dry-run inventory and
  final acceptance report are under `docs/reports/`.
- `src/tools/thor_evidence/contracts/m14_z80_driver_image_upload_v1.json` and
  `m14_z80_driver_image_contract.py` bind accepted AUTO2 upload, W5 destination,
  and W4 execution/sink sources to one exact Z80 driver image range. The
  verifier reports overlap and potential gain without importing a property;
  its focused tests are `tests/m14_z80_driver_image_contract_test.py`. The
  ranked five-family inventory is recorded in
  `docs/reports/THOR_CONSUMER_FAMILY_BACKLOG_20260928.md` and `.json`; the
  machine proof receipt stays under ignored `build/m14-consumer-evidence-import/`.
- `src/tools/thor_evidence/m14_z80_driver_image_import.py` imports the exact
  accepted Z80 image as `Z80_DRIVER_IMAGE_SOURCE` in a typed artifact-only
  contribution schema, separate from runtime property masks. It revalidates
  the source contract and existing W5/GFX ledgers, measures the union coverage,
  and proves removal/rebuild and deterministic replay. Its regression test is
  `tests/m14_z80_driver_image_import_test.py`; the resulting removable ledger
  and report are generated under ignored `build/m14-consumer-evidence-import/`.
- `docs/reports/THOR_AUDIO_MODE0_CONSUMER_BACKLOG_20260930.md` and `.json`
  rank the next audio payload consumer family after low-yield graphics linkage.
  The report keeps all 65 unaccepted Mode-0 resources candidate-only and names
  the 9,698-byte top range; it does not extend the runtime property contract or
  import a contribution.
- `docs/reports/THOR_AUDIO_FORMAT_A_MODE0_ENTRY4_CLOSURE_20260930.md` audits
  accepted W4/W5/W6 consumer evidence for bank `0x17` entry `4`. It remains
  `NEEDS_LIVE_USE` because no accepted exact consumer-to-DAC chain covers it.
- `docs/reports/THOR_AUDIO_FORMAT_A_MODE0_ENTRY4_DISCOVERY_20260930.json`
  hash-binds 12 historical audio-scenario FLOW replays used only to locate a
  productive future trigger; they are not accepted contribution evidence.
- `docs/reports/RUNTIME_ORIGIN_V2_SOURCE_BUNDLE_PREFLIGHT_20260929.md` and
  `.json` record the frozen runtime source-bundle audit, expected and observed
  source hashes, missing baseline patch inputs, and the stop gate before clean
  dual builds. No incomplete bundle is presented as accepted runtime input.


## Runtime V3 build inputs and preflight

- `runtime-build-inputs/runtime-v3/` — candidate frozen runtime source/patch/generator inputs plus per-file SHA-256 manifest; not accepted until clean A/B builds and runtime gates pass.
- `docs/reports/RUNTIME_V3_BUILD_PREFLIGHT_20260929.*` — machine-readable and human-readable status of the V3 integration preflight.

- `docs/reports/RUNTIME_V3_GRAPHICS_BRIDGE_LIVE_20260929.md` and `.json` record capability-scoped V3 build identity, the bounded graphics transaction live result, isolated collector benchmark, and the frozen runtime lifecycle blocker. No core rebuild or evidence-map promotion was made.
- `runtime-build-inputs/runtime-v3-patch-stack-v2/` — exact ordered BizHawk/GPGX V3 patches, frozen lifecycle generator/source inputs, and SHA-256 manifest; clean-applied and built reproducibly, but graphics live acceptance remains blocked on VDP transfer observation.
- `docs/reports/RUNTIME_V3_PATCH_STACK_V2_20260929.md` and `.json` — exact pinned identities, file/patch/build hashes, clean apply, smoke, and bounded graphics live result.
# V3 Z80 audio consumer discovery

- `src/tools/thor_evidence/z80_audio_discovery.py` — bounded, identity-checked
  reducer for Mode-0 resource reads, Format-A decoder hits, and explicitly
  proven strict DAC causal-chain events. It is discovery-only and emits no
  ROM property.
- `src/tools/thor_evidence/z80_audio_discovery_cli.py` — JSONL-to-compact-JSON
  receipt command for the reducer.
- `tests/z80_audio_discovery_test.py` — catalog size, unique-read accounting,
  run identity, and causal-vs-temporal regression coverage.
- `docs/reports/THOR_V3_Z80_AUDIO_CONSUMER_DISCOVERY_20260930.md` and `.json` —
  current V3 instrumentation boundary, reducer status, and live-capture blocker.
- `runtime-build-inputs/runtime-v3-z80-audio-diag-v1/` — frozen, bounded
  V3 Z80 diagnostic overlay sources and manifest; separate from the accepted
  V3 runtime base and does not change property semantics.
- `tools/bizhawk-native-ring/z80_audio_diag_capture.lua` and
  `run_z80_audio_diag_capture.py` — bounded natural-run capture and receipt
  generation; `z80_audio_diag_perf.lua` and `run_z80_audio_diag_perf.py`
  measure collector-disabled/enabled throughput.
- `tests/z80_audio_diag_core_test.c` — bounded-ring, exact-ROM-resolution,
  and event-accounting synthetic tests.
- `docs/reports/THOR_V3_Z80_AUDIO_DIAGNOSTIC_OVERLAY_20260930.md` and `.json` —
  live discovery and isolated three-run performance results. The old consumer
  discovery report above remains the pre-overlay historical status.


## V3 Z80 audio strict-dataflow prototype (2026-09-30)

- `runtime-build-inputs/runtime-v3-z80-audio-diag-v2/`: frozen bounded observer/provenance source bundle and manifest for the experimental W5 strict dataflow overlay.
- `tools/bizhawk-native-ring/run_z80_audio_diag_capture.py`: bounded run launcher and host reducer entry point for this overlay.
- `tests/z80_audio_diag_core_test.c`, `tests/z80_audio_discovery_test.py`: synthetic provenance and strict-reducer tests.
- `docs/reports/THOR_V3_Z80_AUDIO_STRICT_DATAFLOW_20260930.*`: machine receipt and human-readable outcome.


## V3 bounded Z80 audio reject diagnostics (2026-09-30)

- `runtime-build-inputs/runtime-v3-z80-audio-diag-v2/sources/z80_audio_provenance.[ch]` — one-resource Format-A state shadow, strict sample/DAC linkage, up to 64 exact merged root ranges per pending sample, and first-eight reject diagnostics.
- `runtime-build-inputs/runtime-v3-z80-audio-diag-v2/sources/z80_audio_diag.c` — event serialization for bounded reject details, strict range roots (schema v3), and per-subreason totals.
- `src/tools/thor_evidence/z80_audio_discovery.py` — fail-closed v2 offset-list and v3 root-range validation, strict-root coverage accounting, and rejection-reason counters.
- `src/tools/thor_evidence/m14_audio_strict_import.py` — validates and imports exact sparse strict ROM→Format-A→DAC intervals as independently removable `AUDIO_STRICT_CAUSAL_SOURCE` artifact contributions, separate from the runtime property contract.
- `src/tools/thor_evidence/m14_audio_strict_baseline.py` — reconstructs total-proven coverage from the validated runtime checkpoint and accepted artifact ledgers, preserving zero-mask partition spans as unknown.
- `src/tools/thor_evidence/m14_audio_strict_sources.py` — verifies frozen collector inputs, raw capture hashes and receipts, and matching GPGX WBX/ZST identities for strict audio contributions.
- `tests/m14_audio_strict_import_test.py` — checks Entry 7 sparse complement, Entry 4/batch controlled identity, and the separation from runtime property masks.
- `tools/bizhawk-native-ring/run_z80_audio_diag_capture.py` — bounded capture, first-N reject extraction, strict coverage and rejection aggregates.
- `docs/reports/THOR_V3_Z80_AUDIO_REJECT4_RESOLUTION_20260930.*` — root cause, deterministic replay, and machine receipt.
- `docs/reports/THOR_V3_Z80_AUDIO_ENTRY7_PROVENANCE_SCALING_20260930.*` — exact Entry 7 range coverage, second-segment witness, deterministic replay hashes, and the fail-closed root-cap frontier.
- `docs/reports/THOR_V3_Z80_AUDIO_ENTRY7_GAP_CLOSURE_20260930.*` — byte-exact inventory for Entry 7's remaining strict-root gaps, frame-identity witness rejects, range-capacity analysis, and deterministic V3 replay hashes.
- `runtime-build-inputs/runtime-v3-z80-audio-diag-v3/` — catalog-scoped strict audio diagnostic overlay inputs for the second resource; its manifest pins every source and generator hash.
- `docs/reports/THOR_V3_Z80_AUDIO_SECOND_RESOURCE_20260930.*` — controlled command-to-entry-4 strict consumer result, replay comparison, and unproven payload remainder.

## V3 Mode-0 batch consumer harvest (2026-09-30)

- `runtime-build-inputs/runtime-v3-z80-audio-diag-v3/` — hash-verified catalog-scoped diagnostic source/generator bundle used for the controlled second-resource and batch runs.
- `docs/reports/THOR_V3_Z80_AUDIO_SECOND_RESOURCE_20260930.*` — strict partial consumer proof and deterministic replay for bank 0x17 entry 4.
- `docs/reports/THOR_V3_Z80_AUDIO_MODE0_BATCH_HARVEST_20260930.*` — two-run exact-range receipts and aggregate partial strict-root coverage for the remaining 64 Mode-0 candidates.
- `docs/reports/THOR_AUDIO_MODE0_STRICT_RESIDUAL_CLOSURE_20261001.md` and `.json` — byte-exact residual ranges across all 65 resources, read/decoder observations, strict reducer replay, fail-closed cause split and unchanged authority totals.
- `docs/reports/THOR_WHOLE_ROM_FAMILY_CLOSURE_20260930.md` — joins current Mode-0 residual replay, seven blocked `0x3820` caller audits, reusable known graphics/audio contracts, Carver structural UNKNOWN families, and bounded family stop/rank decisions; non-classifying.
- `docs/reports/THOR_CALLER_SCOPED_03C07C_OBSERVER_20260930.md` — records the narrow runtime observer contract and the no-hit bounded runs; no range classification or ownership promotion.
# `src/tools/thor_evidence/m14_graphics_vram_causal_import.py`

- Validates deterministic stage-1 graphics publication receipts and emits
  independently removable `GRAPHICS_VRAM_CAUSAL_SOURCE` artifact ledgers.
- Keeps graphics publication causality separate from runtime property masks,
  `COMPRESSED_GRAPHICS_SOURCE`, and canonical ROM ownership.

# `tests/m14_graphics_vram_causal_import_test.py`

- Exercises exact graphics transaction replay, sparse range accounting,
  contribution removal, and property namespace separation.
# `tools/bizhawk-native-ring/graphics_transaction_scenario_capture.lua`

- Runs reproducible bounded graphics bridge captures with scenario-specific
  controller input profiles while preserving the pinned runtime collector.

# `src/tools/thor_evidence/m14_graphics_vram_scenario_harvest.py`

- Replays bounded A/B scenario receipts, ranks exact graphics causal ranges by
  unique total-proven gain, and stores proof-depth receipts separately when no
  new coverage is available.

# `tests/m14_graphics_vram_scenario_harvest_test.py`

- Checks exact sparse overlap accounting, half-open range rejection, and
  replay identity matching for graphics publication candidates.

# `src/tools/thor_evidence/static_family_sentinel_list.py`

- Validates a separately versioned structural contribution for exact
  sentinel-terminated list members, their physical union, source hashes, and
  structural-only claim boundaries.

# `src/tools/thor_evidence/auto13_bcea_structural_closure.py`

- Independently regenerates the AUTO13 field-3 relative-pointer census,
  validates the `0x00BCEA` 44-byte/negative-key parser contract, computes exact
  overlap, and writes the removable evidence ledger and before/after/removal
  structural views.

# `tests/auto13_bcea_structural_closure_test.py`

- Covers independent census agreement, exact sentinel boundary behavior,
  machine-contract tamper rejection, structural removal, and evidence-plane
  separation.

# `docs/reports/THOR_AUTO13_BCEA_STRUCTURAL_20261004/`

- Stores A/B census JSON, full selector-slot audit, accepted contribution,
  current structural views, and closure receipt for the AUTO13 sentinel-list
  family.

# `docs/reports/THOR_AUTO13_BCEA_STRUCTURAL_CLOSURE_20261004.md`

- Records the exact signed-relative source chain, sentinel parser contract,
  family accounting, and remaining evidence limitations.

# Native caller-census receipt export

- `tools/bizhawk-native-ring/graphics_caller_census.c/.h` — bounded discovery
  event buffer, decoder completion fields, and JSON serializer.
- `tools/bizhawk-native-ring/graphics_caller_census_api.c/.h` — combines the
  existing native graphics transaction JSON with census rows and export
  diagnostics for the existing C ABI.
- `tests/graphics_caller_census_host/` — synthetic native→managed→Lua-visible
  receipt contract test with two callers and three ranges.
- `docs/reports/THOR_3820_DYNAMIC_CALLER_CENSUS_20260930.md` — export-path
  verification and the outstanding live-run timeout.

## Dynamic `0x3820` caller identity proof (2026-10-01)

- `tools/bizhawk-native-ring/graphics_call_decode.[ch]` — bounded 68000 JSR/BSR decode, exact predecessor handoff, target/return validation and `LSB_FIRST` stack lane normalization.
- `tools/bizhawk-native-ring/graphics_caller_census.[ch]` — v2 diagnostic receipt fields for proven caller/callsite, predecessor encoding and return/transaction status.
- `tools/bizhawk-native-ring/rom_properties_runtime_abort.c` — small M68K abort bridge split from the 500-line runtime module.
- `tests/graphics_caller_census_host/` — host-level census receipt and supported call-form contract tests.
- `docs/reports/THOR_3820_CALLER_IDENTITY_20261001.md` — live caller identities, exact ranges, current-union overlap, return mismatch root cause and remaining deterministic replay gap.
- `src/tools/thor_evidence/audio_mode1_structural_closure.py` — ROM-bound
  Mode-1 descriptor census, Z80 opcode contract checks, deterministic A/B
  enumeration, exact overlap, removable structural contribution, and removal
  replay.
- `tests/audio_mode1_structural_closure_test.py` — Mode-1 descriptor range,
  ROM opcode contract, A/B, authority-separation, and artifact-hash regressions.
- `docs/reports/THOR_AUDIO_MODE1_STRUCTURAL_20261002/` — dual census, 29-row
  enumeration, accepted contribution, before/after/removal views, and closure
  receipt.
- `docs/reports/THOR_AUDIO_MODE1_STRUCTURAL_20261002/THOR_AUDIO_MODE1_STRUCTURAL_CLOSURE_20261002.md` — exact Mode-1 loop contract, family accounting, and evidence limits.

# `docs/reports/THOR_M68K_SCRIPT_CONSUMER_XREFS_20261004.*`

- Records a whole-ROM discovery census of 47 decoder-recognized absolute-long
  calls to the indexed-script parser, 44 immediate D0 candidates, one finite
  local selector, two unresolved dynamic contexts, and explicit limits. This
  is not a structural or runtime contribution.
- `docs/reports/THOR_8DA4_MAIN_POOL_RESOURCE_BINDING_GRAPH_20261004.md` — direct-caller census, conditional selector-to-stream/GFX dispatch graph, field-lifecycle limits, and authority-neutral checkpoint.
- `docs/reports/THOR_MAIN_POOL_FIELD26_INDEXED_LOOKUP_20261004.md` — main-pool field census, selector-row `+0x26` pointer to indexed update lookup, and explicit unresolved extent/semantics.
- `docs/reports/THOR_MAIN_POOL_FIELD26_BOUNDARY_ATTEMPT_20261004.md` — bounded index/address audit, exact row-pointer alias scan, rejected table extent, and pivot to the adjacent `+0x22` field.

- `docs/reports/THOR_MAIN_POOL_FIELD22_CALLBACK_DISPATCH_20261004.md` — static `A6+0x22` main-pool callback sink and unresolved selector-2 instance/target join.

- `docs/reports/THOR_MAIN_POOL_FIELD1E_BOUNDED_LOOKUP_20261004.md` — bounded 16-entry secondary word lookup from descriptor row `+0x10` and its same-path relation to the fixed-count stream.

- `docs/reports/THOR_MAIN_POOL_CALLBACK_TARGET_CENSUS_20261005.md` — 99-row descriptor callback-value census and bounded target-decode results; code family/ABI unresolved.
- `docs/reports/THOR_MAIN_POOL_CALLBACK_TARGET_REGION_CLASSIFICATION_20261005.md` — runtime/direct-transfer/exception discriminator for the 30 unique `+0x18` targets; slot retained, whole-region classification unresolved.
- `docs/reports/THOR_ROOM_MAP_FRONTIER_MAIN_POOL_STREAM_LINK_20261005.md` — room/map frontier assessment and exact main-pool selector `0x00A6` to accepted `STREAM_2ECC04` edge; no RoomID semantics inferred.
- `docs/reports/THOR_D406_WORLD_GRID_CONSUMER_CLOSURE_20261005.md` — reuses accepted M7 evidence to connect the four D406 RAM structures to bounded world-grid and collision consumers; no RoomID/source-payload boundary inferred.
- `docs/reports/THOR_SCREEN_SETUP_0009_GRID_GRAPH_20261005.md` — joins ScreenID `0x0009` to D406 grid initialization and exact GFX ResourceIDs 3/4, while preserving the unresolved grid-backing source edge.
- `docs/reports/THOR_SCREEN_SETUP_DESCRIPTOR_FAMILY_20261005.md` — bounded census of three screen setup IDs sharing the 26-byte descriptor grammar and D406 setup mechanism; resource/grid limits remain explicit.
- `docs/reports/THOR_SCREEN_SETUP_FIELD_ROLE_CENSUS_20261005.md` — field-to-consumer census for the 26-byte setup descriptor, including the reusable four-byte D406 resource-selector vector.

# `docs/reports/THOR_SCREEN_SETUP_SELECTOR_VECTORS_20261005.md`

- Records exact selector bytes reconstructed from ROM-hash-verified descriptor segments for setup IDs 0x0009, 0x000C, and 0x0704; joins all twelve D406 slots to the accepted GFX ResourceID/stream manifest while retaining grid, main-pool, transition, and runtime unknowns. Machine receipt: `build/room-map-screen-setup-selector-vectors.json`.

# `docs/reports/THOR_SCREEN_SETUP_GROUP07_RESOURCE_REUSE_20261005.md`

- Records six exact group-7 screen setup mappings, two repeated four-ID vectors, and six exact descriptor `+0x04` joins to file-backed-capable screen-root streams, preserving the unresolved global selector domain and gameplay links. Machine receipt: `build/room-map-screen-setup-group07-reuse.json`.


## Screen setup group-1 family checkpoint

- `docs/reports/THOR_SCREEN_SETUP_GROUP01_FAMILY_20261005.md` — nine exact group-1 descriptor/resource mappings confirming the same path-specific screen-root plus four-slot indexed GFX vector and D406 handoff observed in group 7. Receipt: `build/room-map-screen-setup-group01-family.json`.


## Screen setup 0x0100 world-grid join

- `docs/reports/THOR_SCREEN_SETUP_0100_D406_GRID_STATE_20261005.md` — path-specific setup/resource → D406 selector → byte-grid consumer and movement-gate graph, with exact continuation stores and unresolved grid source. Receipt: `build/room-map-screen-setup-0100-grid-state.json`.

# Screen setup 0x0100 byte-grid asset

- `docs/reports/THOR_SCREEN_SETUP_0100_BYTE_GRID_ASSET_20261006.md` — exact decoded grid subrange within `STREAM_211F7A`, D406 pointer/dataflow join, 64×64 byte extent, consumer chain, externalization limits, and unchanged authorities. Machine receipt: `build/room-map-screen-setup-0100-byte-grid-asset.json`.

# Screen-root decoded layout

- `docs/reports/THOR_SCREEN_ROOT_DECODED_LAYOUT_20261006.md` — compares two exact setup-root outputs, establishes the repeated 64×64 byte-grid decoded subview, inventories proven versus unknown decoded ranges, and records the decoder-level exact-copy probe and production-loader limitation. Machine receipt: `build/room-map-screen-root-decoded-layout.json`.

## ByteGridView typed decoded subviews

- `src/game/world/byte_grid.*` — non-owning bounded grid queries, explicit width/height/stride/row-shift metadata, and a checked zero-copy factory for subranges of decoded parent buffers. The class remains a bounded native view; it does not claim ROM routine equivalence by itself.
- `tests/byte_grid_test.cpp` — synthetic boundary, dimension, pointer-identity, and distinct-parent-payload cases.

# ByteGridView and 0x9C40 candidate

- `docs/reports/THOR_BYTE_GRID_VIEW_9C40_CANDIDATE_20261006.md` — explicit zero-copy view shape and bounds, tests on both accepted decoded screen-root outputs, bounded M68K input/result contract, and why the missing stateful differential oracle keeps `0x9C40` at candidate status. Receipt: `build/byte-grid-9c40-native-candidate.json`.

# 0x9C40 natural caller capture attempt

- `src/tools/hybrid/grid_query_natural_capture.cpp` — developer-only HOOK_CPU runner that executes ordinary libretro frames, captures only an adjacent `0xE376` → `0x9C40` transfer, and records state/events through caller continuation. It never seeds guest CPU or RAM.
- `docs/reports/THOR_9C40_NATURAL_E376_CAPTURE_ATTEMPT_20261011.md` — bounded reachability result, caller liveness evidence, remaining stack/oracle blocker, and unchanged replacement status. Receipts are under ignored `build/9c40-state-capture/`.

# M68K grid footprint capture tool

- `src/tools/hybrid/grid_footprint_capture.cpp` — developer-only HOOK_CPU runner for the bounded `0x9BF2` footprint query. Validates canonical ROM identity and two exact screen-root/grid hashes, then records synthetic PRE/RTS/sentinel states, grid reads, stack writes, and ByteGridView D4/D5 aggregate comparisons. It does not alter production dispatch or evidence authorities.
- Receipt/report: `build/9bf2-contract-scout.json` and `docs/reports/THOR_9BF2_FOOTPRINT_ORACLE_20261007.md`.

# M68K grid column helper oracle

- `src/game/world/byte_grid.*` — adds a bounded vertical-column aggregation view operation (`aggregate_cell_column_world_y`) for the exact D4/D5 outputs of ROM helper `0x94A2`.
- `src/tools/hybrid/grid_leaf_capture.cpp` — developer-only HOOK_CPU runner; checks canonical identity, captures exact `0x94A2` PRE/RTS/sentinel state and cell reads, then compares the native column aggregate.
- `tests/byte_grid_test.cpp` — column aggregation and bounds regression cases.
- Report: `docs/reports/THOR_M68K_94A2_GRID_COLUMN_ORACLE_20261008.md`; machine census/receipts are under ignored `build/` artifacts.

- `src/game/entities/raw_record_initializer.*` — neutral bounded raw-record window translation for ROM helper `0x8EF8`; no gameplay semantics assigned to offsets.
- `tests/raw_record_initializer_test.cpp` — span-size and byte-layout checks for the translated initializer.
- `src/tools/hybrid/raw_record_capture.cpp` — developer HOOK_CPU PRE/RTS/sentinel runner comparing canonical ROM `0x8EF8` writes with the native helper.
- Report: `docs/reports/THOR_M68K_GAMEPLAY_HELPER_COHORT_20261008.md`.
- Census: `build/gameplay-helper-candidate-census.json`; acceptance: `build/raw-record-8ef8-acceptance.json`.
- `docs/reports/THOR_M68K_HYBRID_DISPATCH_DESIGN_20261008.md` — design-only proposal for reversible, caller-gated dispatch of the three verified path-specific helpers; no runtime implementation.
- `src/game/entities/sparse_word_store.*` — bounds-checked neutral four-word sparse store equivalent for ROM helper `0x16A46`, retaining exact register/CCR output data.
- `tests/sparse_word_store_test.cpp` — write offsets, big-endian payloads, result state, and bounds rejection checks for the sparse word-store helper.
- `src/tools/hybrid/sparse_word_store_capture.cpp` — developer-only HOOK_CPU canonical-ROM PRE/RTS/sentinel differential runner for helper `0x16A46`.
- Report: `docs/reports/THOR_M68K_16A46_SPARSE_WORD_STORE_20261008.md`; instruction census and receipts are under ignored `build/` artifacts.

# M68K transfer-source provenance

- `src/tools/hybrid/transfer_provenance_capture.cpp` — developer-only HOOK_CPU runner testing exact source-PC provenance for BSR/JSR edges, rejecting non-call transfers, and validating six known direct ROM call edges.
- `docs/reports/THOR_M68K_TRANSFER_SOURCE_PROVENANCE_20261008.md` — hook timing, provenance method, micro-test and real-call results, interrupt-safety limitation, and unchanged authority/status scope.
- Machine receipt: ignored `build/m68k-transfer-provenance.json`.

# M68K `0x8EF8` opt-in live route

- `src/tools/hybrid/raw_record_live_dispatch.cpp` — developer-only opt-in HOOK_CPU runner for a single path-specific native route, paired ROM/native direct-call probes, and unlisted-caller ROM fallback.
- Report: `docs/reports/THOR_M68K_8EF8_OPT_IN_LIVE_ROUTE_20261009.md`.
- Machine receipts are ignored under `build/raw-record-live-dispatch-*.json`.

# W6 M68K-to-Z80 ready handoff

- `docs/reports/THOR_M12_Z80_READY_HANDOFF_20261009.md` — bounded static/runtime join for the `0x60310` copy-and-ready path and Z80 poll at `0x04D0`; unresolved payload source and conditional continuation remain explicit.
- Machine metadata receipt is ignored at `build/m12-w6-z80-ready-handoff.json`.
- `docs/reports/THOR_M12_Z80_HANDOFF_SENTINEL_PATH_20261009.md` — one hash-bound W6 path closes RAM-staged `0xFF` bytes through M68K copy, ready bit, Z80 branch, and parser terminator; Mode 0/1 and other payload variants stay unresolved.
- Its payload-free machine receipt is ignored at `build/m12-w6-z80-sentinel-path.json`.
# W6 audio pointer/resource join

`docs/reports/THOR_M12_W6_91740_FORMAT_A_RESOURCE_JOIN_20261010.md` records a
hash-bound W6 Z80 sample-read join to the existing Format-A bank-18 descriptor
resource, including the Mode-1 descriptor alias and unresolved live selector
provenance. Its metadata-only machine receipt is emitted under ignored `build/`.

# Word-grid upload consumer scout

- `docs/reports/THOR_WORD_GRID_UPLOAD_SCOUT_20261010.md` — bounded static consumer shape for `0xDB84` / `0xDC26`; exact backing extent remains unresolved.
- Machine receipt: ignored `build/word-grid-upload-scout-20261010.json`.


# Controller-selected setup graph

- `docs/reports/THOR_MAIN_POOL_SCREEN_SETUP_001B_GRAPH_20261010.md` — static controller → setup descriptor → GFX/main-pool binding graph, with unresolved same-invocation edge explicit.
- `build/main-pool-screen-setup-001b-graph.json` — ignored machine receipt with ROM, manifest, and slice hashes.

- `docs/reports/THOR_MAIN_POOL_SCREEN_SETUP_001B_SAME_INVOCATION_AUDIT_20261010.md` — bounded same-invocation closure attempt; dynamic indirect target, FF0DC4 value, and row-20 consumer provenance remain unresolved.
- `build/main-pool-screen-setup-001b-same-invocation-audit.json` — ignored machine receipt pinning the closure attempt.

- `docs/reports/THOR_SETUP_MAIN_POOL_CANDIDATE_SCOUT_20261010.md` — alternate setup 0x000C candidate, conditional main-pool row-20 binding, and indirect-dispatch blocker/pivot.
- `build/setup-main-pool-candidate-scout-20261010.json` — ignored machine receipt pinning manifests, ROM, slices, and runtime novelty artifact.

- `docs/reports/THOR_RUNTIME_ANIMATION_SELECTOR_SCOUT_20261010.md` — bounded
  ranking and evidence limits for the runtime-covered selector-to-graphics/
  SAT path; its machine receipt is `build/m12-animation-selector-scout-20261010.json`.

- `docs/reports/THOR_SELECTOR_CAUSAL_CAPTURE_ATTEMPT_20261011.md` — bounded
  audit of available runtime evidence for the selector→descriptor→decoder→SAT
  chain and the resulting renderer-side pivot; machine receipt is under `build/`.

- `docs/reports/THOR_A342_SAT_SHADOW_PRODUCER_ANALYSIS_20261011.md` — bounded neutral producer contract for M68K `0xA342/0xA372`, with caller and runtime-capture limits; receipt is under ignored `build/`.
- `docs/reports/THOR_HELPER5_RUNTIME_CANDIDATE_SCOUT_20261011.md` — ranked runtime-covered helper scout; A342 full-call oracle gap and A196 stateful/indirect blocker recorded.
- `src/tools/thor_evidence/helper_candidate_index.py` — builds a SHA-256-bound callable-helper index from retained receipts/censuses, preserving UNKNOWN and distinguishing runtime PC-set membership from hit counts; records direct-caller PC-set intersections separately.
- `tests/helper_candidate_index_test.py` — schema, provenance-hash, mission-exclusion, and fail-closed strong-intersection checks for the helper index.
- `docs/reports/THOR_HELPER_CANDIDATE_INDEX_20261011.md` — top candidates, evidence intersection, blockers, and instrumentation recommendation.

- `src/tools/re_bizhawk_m12_b730_full_call.lua` — targeted full-call capture for the direct `0x003B448 -> 0xB730` path, with complete register snapshots and bounded operand/write observations.
- `src/game/render/sat_shadow_records.hpp/.cpp` — bounded native SAT shadow record effects translated from M68K `0xB730`; no runtime dispatch.
- `src/tools/m12_b730_native_differential.cpp` — ROM-bound differential runner for saved natural `0xB730` captures.
- `src/tools/hybrid/b730_branch_capture.cpp` — developer-only HOOK_CPU synthetic branch oracle for `0xB730` coordinate-cull and D2-limit paths.
- `tests/sat_shadow_records_test.cpp` — bounded validation and synthetic record-output checks.
- `docs/reports/THOR_B730_FULL_CALL_CAPTURE_20261011.md` — exact `0xB730` static and runtime call contract, native comparison, limits, and path-scoped status; machine artifacts are under ignored `build/`.

## Deterministic gameplay session capture/replay

- `src/tools/hybrid/gameplay_session.hpp/.cpp` — versioned effective input poll
  and ordered controller-query journal with strict capture/replay sequencing.
- `src/tools/hybrid/gameplay_session_identity.hpp/.cpp` — SHA-256 identities,
  deterministic configuration identity, and session manifest IO.
- `src/tools/hybrid/gameplay_session_receipts.hpp/.cpp` — stable replay-result
  receipt serialization.
- `src/tools/hybrid/grid_query_native_model.hpp/.cpp` — bounded, non-mutating
  `ByteGridView` model for M68K `0x9C40`, used by the developer replay oracle.
- `src/tools/hybrid/gameplay_session_runner.cpp` — developer-only libretro
  capture/replay executable, M68K PC/frame landmarks, and exact `0xE376 →
  0x9C40` full-call watch.
- `tests/gameplay_session_test.cpp` — poll/query roundtrip, fail-closed journal,
  manifest, identity, transfer census, and bounded full-call capture-limit tests.
- `src/tools/hybrid/m68k_call_census.hpp/.cpp` — replay-time exact transfer
  attribution for direct BSR/JSR forms and bounded natural full-call receipts.
-src/tools/hybrid/m68k_code_map.hpp/.cpp — execution-driven accumulative M68000
  instruction code map, coverage tracker, and ASM/JSON exporter.
- `src/tools/hybrid/m68k_code_formatter.hpp/.cpp` — complete, modular Motorola 68000
  instruction formatter with operand completeness, standard mnemonics, and assembler roundtrip.
- `tests/m68k_code_map_test.cpp` — unit verification for instruction observation,
  coverage calculation, and artifact emission.
- `tests/m68k_code_map_asm_test.cpp` — C++ regression verification for M68000 assembly
  operand formatting and branch target resolution.
- `tools/smoke_code_map_ab.py` — single-variable A/B smoke verification verifying
  zero behavioral divergence and bit-for-bit equivalence with trace enabled.
- `tools/run_attract_code_map.py` — full attract_3600 replay execution for ROM code
  reconstruction and performance profiling.
- `tools/merge_code_maps.py` — idempotent multi-scenario accumulative merge engine,
  bytecode consistency validator, and global coverage generator.
- `tools/run_game_coverage_campaign.py` — multi-scenario replay campaign orchestrator
  covering attract_3600, canonical_3600, beach_6000, menu_3600, fileselect_3600.
- `tools/verify_asm_roundtrip_classes.py` — independent M68000 reverse assembly verification
  across 15 instruction classes using GNU as, distinguishing exact matches from unverified.
- `tools/map_native_coverage.py` — native helper coverage mapper correlating global executed
  code maps with ownership indices and verification receipts.
- `tools/analyze_hot_ranges.py` — gameplay subsystem hot code range auditor analyzing
  callers, children, RAM accesses, and hardware dependencies.
- `tools/test_roundtrip_samples.py` — GNU as roundtrip assembler verification across
  distinct M68000 instruction mnemonics.
- `tools/verify_all_3708_quality.py` — automated batch ASM quality verification across
  executed instruction sets.
- `tools/audit_steppers.py` — systematic instruction stepper audit tool and semantic validation gate validating opcode consumption, operand fetch parity, and contract invariants across all 185 native helpers.
- `docs/reports/00638c-m15-187g-stepper-repair-20261103.json` — machine-readable verification receipt, 0x00638C repair data, 0 proven defects remaining across 185 helpers, 1,592-frame three-way continuation parity, and M15.188 readiness for M15.187G.
- `docs/reports/00e398-m15-187f-stepper-repair-20261103.json` — machine-readable verification receipt, 0x00E398 repair data, 1,592-frame checkpoint parity, and full 185-baseline 0x00663E reachability for M15.187F.
- `docs/reports/01ad14-m15-187e-stepper-correctness-audit-20261103.json` — machine-readable stepper correctness audit, 0x01AD14 repair receipt, 185-helper flaw census, and 668-frame continuation parity data for M15.187E.
- `src/tools/hybrid/e268_state_transform.hpp/.cpp` — native state-word effect
  model for the path-specific M68K routine at `0xE268`.
- `tests/hybrid_e268_state_transform_test.cpp` — 24 captured ROM input/output
  vectors and native effect checks for caller `0x00359A`.
- `docs/reports/THOR_REPLAY_HELPER6_E268_20261011.md` — replay census ranking,
  selected caller contract, differential result, and evidence scope.
- `docs/reports/THOR_DETERMINISTIC_GAMEPLAY_SESSION_CAPTURE_20261011.md` —
  validated capture/replay receipt and evidence limits; machine data is under
  ignored `build/gameplay-session-validation-20261011-v2/`.
- `docs/reports/THOR_9C40_E376_PATH_SPECIFIC_VERIFICATION_20261011.md` —
  24-call natural differential, caller liveness, stack-scratch scope, and
  synthetic edge-case oracle; machine receipts remain under ignored `build/`.
- `docs/reports/THOR_REPLAY_HELPER7_SCOUT_8B8E_8DD4_20261011.md` — bounded analysis of the two top replay candidates; freezes the zero-only observed paths and records unresolved state branches.
- `src/tools/hybrid/coordinate_bound_model.hpp/.cpp` — typed 16-bit model for
  the path-specific M68K bound helper at `0x9D4E`.
- `src/tools/hybrid/coordinate_bound_oracle.cpp` — developer-only canonical
  ROM HOOK_CPU oracle for bounded `0x9D4E` cases.
- `tests/coordinate_bound_model_test.cpp` — natural-capture vectors and
  arithmetic/branch tests for the coordinate-bound model.
- `docs/reports/THOR_REPLAY_HELPER7_9D4E_20261011.md` — exact boundary, caller
  ABI, natural read patterns, native differential, and limits.

- `src/tools/hybrid/gameplay_session_input_sources.{hpp,cpp}` — isolated
  fallback/scripted/physical input selection used by the session runner.
- `src/tools/hybrid/gameplay_session_landmarks.{hpp,cpp}` — deterministic
  per-frame landmark serialization and strict replay comparison.
- `src/tools/hybrid/gameplay_session_replay_diagnostic.{hpp,cpp}` — opt-in
  continued replay landmark capture with a machine receipt for the first
  mismatch; the one-shot native diagnostic falls back to ROM afterward.
- `tests/gameplay_session_replay_diagnostic_test.cpp` — matching/mismatching
  landmark capture and first-difference receipt regression test.
- `src/tools/hybrid/native_island_runtime.{hpp,cpp}` — replay-only, default-off
  route gate and typed staged adapter for `0x9D70` caller `0x009144`; no
  production dispatch.
- `tests/native_island_runtime_test.cpp` and
  `tests/gameplay_session_input_sources_test.cpp` — gate/fallback/effect and
  input-source checks.
- `docs/reports/THOR_REPLAY_NATIVE_ISLAND_ATTEMPT_20261012.{md,json}` —
  ROM-only replay pass, one-helper divergence, metrics, and unchanged statuses.

## Six-helper instruction-step replay island

- `src/tools/hybrid/m68k_9c40_instruction_step.{hpp,cpp}` — replay-only,
  caller-gated 0x9C40 instruction stepper that preserves MOVEM stack bus effects.
- `docs/reports/THOR_SIX_HELPER_INSTRUCTION_STEP_ISLAND_20261013.{md,json}` —
  exact 0x9C40 path timing/stack receipt and six-helper replay equivalence.
- Per-call replay evidence remains in ignored
  `build/multi-helper-step-9c40-20261013/` directories.
- `docs/reports/THOR_REPLAY_NATIVE_9D70_DIVERGENCE_DIAGNOSTIC_20261012.{md,json}` —
  matched one-call state comparison and strict PC-landmark divergence diagnosis.
- `src/tools/hybrid/gameplay_session_cpu.{hpp,cpp}` — shared HOOK_CPU register,
  memory, and serialized-state snapshot helpers used by replay captures.
- `src/tools/hybrid/replay_equivalence_trace.{hpp,cpp}` and
  `src/tools/hybrid/compare_normalized_replay.py` — normalized helper-event,
  guest-state, scheduler, and external-event comparison for the single
  `0x9D70` replay route.
- `docs/reports/THOR_REPLAY_NATIVE_9D70_TIMING_STATE_DIAGNOSTIC_20261012.{md,json}` —
  exact entry/return cycle and Z80 RAM mismatch receipt; route equivalence remains NO.

- `src/tools/hybrid/negative_bound_model.hpp/.cpp` — bounded native scalar model for the path-specific M68K helper at `0x9DB4`.
- `tests/negative_bound_model_test.cpp` — 24 captured replay vectors and branch/CCR-X arithmetic checks for `0x9DB4`.
- `docs/reports/THOR_REPLAY_HELPER8_9DB4_20261011.md` — candidate ranking, path-specific ABI/liveness, oracle results, and evidence limits.
- `docs/reports/THOR_REPLAY_HELPER8_9DB4_ACCEPTANCE_20261011.json` — machine-readable acceptance receipt.
- `docs/reports/THOR_REPLAY_HELPER_CANDIDATE_INDEX_20261011.json` — updated replay helper index and remaining candidate ranking.

- `src/tools/hybrid/positive_bound_model.hpp/.cpp` — typed 16-bit bound model for the path-specific M68K helper at `0x9D92`.
- `tests/positive_bound_model_test.cpp` — 24 replay vectors and clamp/flag/wrap checks for `0x9D92`.
- `docs/reports/THOR_REPLAY_HELPER9_9D92_20261011.md` — `0xA7E6` bounded rejection and `0x9D92` verification evidence.
- `docs/reports/THOR_REPLAY_HELPER9_9D92_ACCEPTANCE_20261011.json` — machine-readable path-specific acceptance receipt.
- `docs/reports/THOR_NATIVE_ISLAND_DESIGN_8_HELPERS_20261011.md/.json` — design-only registry, adapter, gate, fallback, replay-validation and metrics contract for eight verified helpers.


- `docs/reports/THOR_REPLAY_HELPER10_99D6_BOUNDARY_20261012.md/.json` — exact `0x99D6` read arithmetic, caller evidence, and unresolved extent disposition.
- `docs/reports/THOR_NATIVE_ISLAND_REGISTRY_PREP_9_HELPERS_20261012.md/.json` — nine verified path-scoped helper contracts grouped for future dispatch design; no runtime wiring.

- `src/tools/hybrid/grid_query_micro_oracle.{hpp,cpp}` — deterministic HOOK_CPU RTS harness fixture used by developer oracles; `src/tools/hybrid/subtraction_bound_model.{hpp,cpp}` — typed native model for M68K `0x9D70` word subtraction/clamp and exact flag/write effects; `tests/subtraction_bound_model_test.cpp` — 22 replay vectors plus branch vectors; `src/tools/hybrid/subtraction_bound_oracle.cpp` — developer-only canonical-ROM HOOK_CPU differential oracle. Evidence: `docs/reports/THOR_REPLAY_HELPER10_9D70_20261012.{md,json}`.

\n
# 2026-10-13 scheduler fixture additions

- `src/tools/hybrid/prepare_scheduler_fixture.py` — creates a dedicated,
  isolated external GPGX copy with scheduler/IRQ instrumentation and
  `HOOK_CPU=1`; does not modify the normal capture core.
- `src/tools/hybrid/scheduler_fixture_runner.cpp` — developer-only paired
  ROM/instruction-step micro-case runner using GPGX's frame scheduler.
- `src/tools/hybrid/scheduler_fixture_acceptance.py` — validates canonical ROM
  and hook configuration, compares ordered traces and final states, and emits
  a machine receipt.
- `docs/reports/THOR_SCHEDULER_FIXTURE_20261013.md/.json` — scope,
  instrumentation, limitations, build identities, and acceptance result.
- Full paired traces and JSON receipts are generated under ignored
  `build/scheduler-fixture-20261013/`.
- `src/tools/hybrid/m68k_9d70_instruction_step.{hpp,cpp}` — developer-only,
  caller-gated, resumable instruction-step substitute for the exact `0x9D70`
  body; production dispatch is not connected.
- `src/tools/hybrid/gameplay_session_call_capture.{hpp,cpp}` — isolated full
  call-capture serialization used by the replay runner.
- `src/tools/hybrid/replay_equivalence_trace.{hpp,cpp}` — per-call and
  per-frame CPU/scheduler/observable-memory trace for the selected `0x9D70`
  replay experiment.
- `docs/reports/THOR_9D70_INSTRUCTION_STEP_REPLAY_20261013.{md,json}` —
  natural-call timing, full-replay comparison, and disclosed serializer-hash
  instability.
- Paired replay traces are generated under ignored
  `build/9d70-step-diagnostic-20261013/post-marker3/`.

# 2026-10-13 three-helper instruction-stepped replay island

- `src/tools/hybrid/m68k_gameplay_step_set.{hpp,cpp}` — caller-gated,
  scheduler-backed instruction-step adapters for the accepted paths at
  `0x9D70`, `0x9D4E`, `0x9DB4`, and `0x9D92`; developer/replay-only.
- `src/tools/hybrid/gameplay_session_replay_options.{hpp,cpp}` — replay option
  parsing for selecting a step-helper set.
- `src/tools/hybrid/gameplay_session_step_trace.{hpp,cpp}` — filtered
  instruction-step trace collection and route markers.
- `src/tools/hybrid/normalize_step_helper_trace.py` — creates normalized helper
  events, path-specific cycle census including opcode decoding for `0x9D92`,
  and native/fallback route metrics.
- `docs/reports/THOR_MULTI_HELPER_INSTRUCTION_STEP_ISLAND_20261013.{md,json}`
  — baseline, single, pair, and triple replay acceptance for the pinned
  development replay; production dispatch remains inactive.
- `docs/reports/THOR_FOUR_HELPER_INSTRUCTION_STEP_ISLAND_20261013.{md,json}`
  — `0x9D92` single-helper and four-helper replay acceptance for the same
  pinned development session; production dispatch remains inactive.
- Raw replay runs and intermediate captures are under ignored
  `build/multi-helper-step-20261013/`.

# 2026-10-13 five-helper instruction-stepped replay island

- `src/tools/hybrid/m68k_94a2_instruction_step.{hpp,cpp}` — bounded, replay-only
  instruction-step adapter for 0x94A2, gated to direct caller 0x9468.
- `src/tools/hybrid/m68k_gameplay_step_flags.hpp` — shared condition-code
  helpers factored from the existing stepper module to keep it below 500 lines.
- `docs/reports/THOR_FIVE_HELPER_INSTRUCTION_STEP_ISLAND_20261013.{md,json}` —
  0x94A2 timing, read-bound, single-helper and five-helper replay receipt.
- Raw replay captures and reconstructed read/timing census are under ignored
  `build/multi-helper-step-94a2-20261013/`.

# 2026-10-13 six-helper instruction-stepped replay island

- `src/tools/hybrid/m68k_9c40_instruction_step.{hpp,cpp}` — replay-only,
  caller-gated 0x9C40 stepper preserving MOVEM stack bus effects.
- `docs/reports/THOR_SIX_HELPER_INSTRUCTION_STEP_ISLAND_20261013.{md,json}` —
  exact 0x9C40 path timing/stack receipt and six-helper replay equivalence.
- Per-call replay evidence is under ignored
  `build/multi-helper-step-9c40-20261013/`.

# 2026-10-13 seven-helper instruction-step replay island

- `src/tools/hybrid/m68k_8ef8_instruction_step.{hpp,cpp}` — exact ROM-checked, caller-gated stepper for the previously accepted 0x8EF8 BSR.W callers.
- `src/tools/hybrid/gameplay_session_replay_diagnostic.{hpp,cpp}` and `tests/gameplay_session_replay_diagnostic_test.cpp` — report actual replay landmark divergence independently from diagnostic-only status.
- `src/tools/hybrid/normalize_step_helper_trace.py` — 0x8EF8 opcode/cycle census and per-helper native/fallback counts for seven-helper runs.
- `docs/reports/THOR_SEVEN_HELPER_INSTRUCTION_STEP_ISLAND_20261013.{md,json}` — candidate comparison, single-helper and seven-helper acceptance receipt.
- Final validated replay artifacts are under ignored `build/multi-helper-8ef8-20261013/final-validated/`.

# 2026-10-13 eight-helper instruction-stepped replay island

- `src/tools/hybrid/m68k_e268_instruction_step.{hpp,cpp}` — ROM-checked,
  caller-gated instruction stepper for `0x00359A → 0xE268` only.
- `src/tools/hybrid/gameplay_session_metrics.{hpp,cpp}` — focused native-island
  and E268 counters shared by the developer replay runner.
- `docs/reports/THOR_EIGHT_HELPER_INSTRUCTION_STEP_ISLAND_20261013.{md,json}` —
  path contract, timing/stack evidence, replay comparisons, and machine receipt.
- Final v5 replay artifacts are under ignored
  `build/multi-helper-e268-20261013/`.


# 2026-10-13 frozen eight-helper replay baseline

- `docs/reports/THOR_FROZEN_EIGHT_HELPER_REPLAY_BASELINE_20261013.md` — human-readable freeze scope, repeat-run result, gates, limits, performance, and next-direction recommendation.
- `docs/reports/THOR_FROZEN_EIGHT_HELPER_REPLAY_BASELINE_20261013.json` — machine-readable pinned identities, eight caller contracts, repeated replay metrics, build/test status, counter coverage limits, and authority status.
- Repeated replay artifacts and comparison summary remain under ignored `build/multi-helper-e268-20261013/frozen-repro/`.

# 2026-10-13 frozen island timing telemetry V2

- `src/tools/hybrid/build_island_telemetry_v2.py` — fail-closed offline join of
  frozen helper step traces, E268 runtime counters, replay identities, and the
  clean-build dependency closure. Missing counters remain `UNINSTRUMENTED`.
- `tests/build_island_telemetry_v2_test.py` — unit checks for unknown-value
  preservation and path/cycle aggregation.
- `docs/reports/THOR_REPLAY_NATIVE_ISLAND_EIGHT_V2_20261013.{md,json}` — V2
  telemetry, coverage matrix, fresh replay comparison, and build dependency set;
  V1 remains unchanged.
- The fresh replay output and matching call traces are under ignored
  `build/multi-helper-e268-20261013/frozen-repro/debug-telemetry-v2/`.

# Secondary weapon selection and use pipeline (Bow)

- `src/game/player/secondary_weapon.hpp` — secondary weapon inventory model (`WeaponInventorySlot`, `WeaponInventory`), `ArrowProjectile` entity struct, and pure lifecycle function declarations.
- `src/game/player/secondary_weapon.cpp` — authentic secondary weapon cycle logic (`cycle_secondary_weapon`), secondary attack state starter (`try_start_secondary_attack`), State 14 animation/fire/recovery progression (`update_secondary_attack`), and arrow projectile physics/terrain/actor damage collisions (`update_arrow_projectile`).
- `tests/secondary_weapon_test.cpp` — 10 deterministic unit tests covering weapon selection, skipping unowned slots, rejecting no-ammo firing, use input state entry, frame 12 arrow spawning, facing direction transforms, single ammo decrement, enemy damage and knockback, projectile wall/lifetime removal, and frame 16 return to normal.

# M15 vertical slice connected recompilation audit and cluster selection

- `docs/reports/THOR_M15_GAMEPLAY_SLICE_CONNECTED_AUDIT_20261014.md` — comprehensive audit report documenting the 12-stage deterministic slice trajectory, execution ownership model, M68K PC coverage from 3,600-frame baseline replay, native-to-ROM boundary graph, top 10 bottlenecks evaluation, connected native regions analysis, selection rationale for Player State & Action Dispatcher (`0x004A20..0x004B80`), and full translation plan.

# Execution-driven hybrid recompilation registry and first-divergence oracle

- `src/tools/hybrid/hybrid_registry.hpp/.cpp` — central native replacement registry (`HybridExecutionRegistry`) managing `NativeRegion` descriptors, active/inactive region status, supported precondition gating, fail-closed fallback recording, and execution metrics.
- `src/tools/hybrid/player_dispatch_region.hpp/.cpp` — native region definition and handler for player dispatcher (`0x004A20` marked inactive StaleStaticCandidate; canonical `0x00557A` active), routing States 0, 10, 14, and 24 to verified native logic and eliminating child provenance rejects.
- `src/tools/hybrid/main_loop_tick_region.hpp/.cpp` — native region definition for Main Loop Frame Tick / Delay Coordinator (`0x0033D2..0x003424`), registered in instruction-stepped timing mode.
- `src/tools/hybrid/entity_oscillation_region.hpp/.cpp` — bounded native region definition and handler for Entity Oscillation / Bobbing Displacement Coordinator (`0x0613B2..0x0613F6`), displacing 26,233 ROM fallbacks across callers `0x061AC2` and `0x062576`.
- `src/tools/hybrid/m68k_613b2_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for `0x0613B2`, verified in dual canonical 3,600-frame replay (Run A: 26,233 ROM executions; Run B: 26,233 native executions, 0 divergences).
- `src/tools/hybrid/m68k_61e48_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Sprite Tile Allocation / Sub-Sprite Animation Cycling Coordinator (`0x061E48..0x061EDE`), verified in dual canonical 3,600-frame replay (Run A: 16,693 ROM executions; Run B: 16,693 native executions, 0 divergences).
- `src/tools/hybrid/m68k_61934_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Display List Priority Coordinator & Sprite Tier Driver (`0x061934..0x061F7E`), verified in dual canonical 3,600-frame replay (Run A: 20,916 ROM executions; Run B: 20,916 native executions, 0 divergences).
- `src/tools/hybrid/m68k_61934_step_moves.cpp` — dedicated modular move instruction stepping operations for `0x061934`.
- `src/tools/hybrid/m68k_61934_step_ops.hpp/.cpp` — ALU, branch, bitwise, memory manipulation, and CCR update stepping operations for `0x061934`.
- `src/tools/hybrid/m68k_61934_step_tail.hpp/.cpp` — tail instructions (`0x061C80..0x061F7E`), piece emission, and loop blocks for `0x061934`.
- `src/tools/hybrid/m68k_61934_step_tail_ops.hpp/.cpp` — auxiliary ALU, memory, and piece queuing helpers for the `0x061934` tail.
- `src/tools/hybrid/first_divergence_oracle.hpp/.cpp` — differential execution validator comparing D0-D7, A0-A7, SR/CCR/X, PC, stack top, and player entity record RAM.
- `tests/hybrid_registry_test.cpp` — unit test suite verifying canonical player ABI, stale region accounting, active region dispatch, main loop tick coordinator registration, fail-closed fallback recording, differential divergence detection, and E268 instruction step caller provenance (0x00359A, 0x0033E2, 0x00340E).
- `tests/m68k_61e48_instruction_step_test.cpp` — unit test suite verifying entry caller contract, early exit branches, mode 4/5 animation cycling, LCG random generator arithmetic, display output piece queueing, and D7 restoration for `0x061E48`.
- `tests/m68k_61934_instruction_step_test.cpp` — unit test suite verifying caller gating (6 proven callers `0x0601EE`, `0x0601FA`, `0x060206`, `0x060212`, `0x06021E`, `0x060260`), early exit flows, bitwise operations, piece emission, and child call routing to `0x061E48`.
- `src/tools/hybrid/m68k_6147e_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Coordinate Screen Projection & Auxiliary Component Driver (`0x06147E..0x061586`), verified in dual canonical 3,600-frame replay (Run A: 16,673 ROM executions; Run B: 16,673 native executions, 0 divergences, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_6147e_step_ops.hpp/.cpp` — core instruction-stepping primitives, bit operations, branch/jump handling, and register/RAM checks for `0x06147E`.
- `src/tools/hybrid/m68k_6147e_step_moves.cpp` — move, movem, and lea instruction-stepping operations for `0x06147E` with cycle-accurate bus refresh order and dummy postincrement reads.
- `src/tools/hybrid/m68k_6147e_step_math.cpp` — arithmetic, logic, shift, and rotate instruction-stepping operations for `0x06147E`.
- `src/tools/hybrid/m68k_6147e_table1.cpp` — jump table 1 layer merging handlers (`0x06159E..0x06163C`) for overlapping component layers.
- `src/tools/hybrid/m68k_6147e_component_step.cpp` — component timer countdown, direction sign negation, accumulator stepping, and base coordinate lookup (`0x06163E..0x0616B2`).
- `src/tools/hybrid/m68k_6147e_table2.cpp` — jump table 2 screen projection handlers (`0x0616CA..0x061932`) for directional coordinate scaling and clamping.
- `tests/m68k_6147e_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x061AC6`), component step dispatch, and exit at RTS for `0x06147E`.
- `src/tools/hybrid/m68k_623ac_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Script Interpreter & Sprite Animator (`0x0623AC..0x062892`), verified in dual canonical 3,600-frame replay (Run A: 13,944 ROM executions; Run B: 13,944 native executions, 0 divergences, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_623ac_step_ops.hpp/.cpp` — core instruction-stepping primitives, branch handling, and register/RAM/PSG access checks for `0x0623AC`.
- `src/tools/hybrid/m68k_623ac_step_moves.cpp` — move, movem, and lea instruction-stepping operations for `0x0623AC` with interleaved extension word fetch / memory reads and cycle-accurate bus refresh order.
- `src/tools/hybrid/m68k_623ac_step_math.cpp` — arithmetic operations (add, sub, mulu, cmp) with dynamic bit-count cycle models.
- `src/tools/hybrid/m68k_623ac_step_logic.cpp` — bitwise logic and shift/rotate operations with dynamic shift cycle addition before `SKIP_BUS_REFRESH()`.
- `src/tools/hybrid/m68k_623ac_block1.cpp` — modular instruction dispatch block 1 (`0x0623AC..0x0624BE`) for entity script command parsing and channel initialization.
- `src/tools/hybrid/m68k_623ac_block2.cpp` — modular instruction dispatch block 2 (`0x0624C0..0x06263A`) for opcode handlers and animation step timers.
- `src/tools/hybrid/m68k_623ac_block3.cpp` — modular instruction dispatch block 3 (`0x06263C..0x062776`) for coordinate updates and SN76489 PSG sound command generation.
- `src/tools/hybrid/m68k_623ac_block4.cpp` — modular instruction dispatch block 4 (`0x062778..0x062892`) for jump tables and terminal interpreter dispatch.
- `tests/m68k_623ac_instruction_step_test.cpp` — unit test suite verifying caller gating (4 callers: `0x060234`, `0x060242`, `0x060250`, `0x060276`), SN76489 PSG audio command dispatch, shift/movem dynamic cycles, and exit at RTS.
- `tests/hybrid_callee_623ac_test.cpp` — test fixture verifying `0x0623AC` callee contract and register preservation.
- `src/tools/hybrid/m68k_6273a_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for SN76489 PSG Channel Attenuation / Volume Synthesizer (`0x06273A..0x062788`), verified in dual canonical 3,600-frame replay (Run A: 11,303 ROM executions; Run B: 11,303 native executions, 0 fallbacks, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_6273a_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x062862`, `0x062620`), fail-closed registers, mute paths, distance attenuation arithmetic and clamping, and dual RTS exits for `0x06273A`.
- `src/tools/hybrid/m68k_62788_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for ADSR Sound Envelope Generator & Tremolo Modulator (`0x062788..0x06286E`), verified in dual canonical 3,600-frame replay (Run A: 9,287 ROM executions; Run B: 9,287 native executions, 0 fallbacks, 248,168 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_62788_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x062556`), fail-closed registers, ADSR attack/decay/sustain/release transitions, volume multiplication (`mulu.w`), tremolo modulation, connected call to child `0x06273A`, and exit at RTS.
- `src/tools/hybrid/m68k_60000_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Frame Audio/Graphics Coordinator (`0x060000..0x060284`), verified in dual canonical 3,600-frame replay (Run A: 3,486 ROM executions; Run B: 3,486 native executions, 0 fallbacks, 186,101 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_60000_step_ops.hpp/.cpp` — core instruction-stepping primitives, branch/call handling, and SR/MOVEM save/restore operations for `0x060000`.
- `src/tools/hybrid/m68k_60000_step_mem.hpp/.cpp` — memory read/write, immediate moves, and flag clearing operations for `0x060000`.
- `src/tools/hybrid/m68k_60000_step_alu.hpp/.cpp` — arithmetic, logical, and shift/rotate operations for `0x060000`.
- `src/tools/hybrid/m68k_60000_block1.hpp/.cpp` — modular instruction dispatch block 1 (`0x060000..0x060180`) for coordinator preamble, sound trigger parsing, and channel setup.
- `src/tools/hybrid/m68k_60000_block2.hpp/.cpp` — modular instruction dispatch block 2 (`0x060182..0x060284`) for coordinator orchestration of child subsystems `0x062AE0`, `0x061934`, `0x0623AC`, `0x060286` and epilogue.
- `tests/m68k_60000_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x002380`), frame-level pass branches, child call routing, stack preservation, and exit at RTS.
- `src/tools/hybrid/m68k_60286_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Ancient Music Driver Z80 Sound Command Commit & Bus Controller (`0x060286..0x060352`), verified in dual canonical 3,600-frame replay (Run A: 3,486 ROM executions; Run B: 3,486 native executions, 0 fallbacks, 445,912 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_60286_step_ops.hpp/.cpp` — instruction-stepping primitives, Z80 bus request/release protocol operations, memory copy loops (`move.b (a0)+, (a1)+` / `dbf d0`), and bit operations (`btst`/`bset`) for `0x060286`.
- `tests/m68k_60286_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x06027A`), bus request and wait semantics, BGM request flag transfer, voice activity clearing, buffer 0 and buffer 1 transfers, double buffer overflow path, and 3 RTS exits.
- `src/tools/hybrid/m68k_62ae0_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Ancient Music Driver FM Voice Parameter Processor / Channel 6 Sequencer (`0x062AE0..0x062D82`), verified in dual canonical 3,600-frame replay (Run A: 3,486 ROM executions; Run B: 3,486 native executions, 0 fallbacks, 27,085 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_62ae0_step_ops.hpp/.cpp` — instruction-stepping primitives for `0x062AE0`, covering post-increment byte moves (`move.b <ea>, (a4)+`), displacement byte test/moves (`btst #b, d(an)`, `move.b d(an), (a4)+`), and byte-store immediate.
- `tests/m68k_62ae0_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x0601E2`), Channel 6 context invariants (`A6 = 0x00FF05B2`, `D7 = 6`), BGM trigger parameter packet generation, muted channel skip, disabled channel buffer termination, and unobserved channel-enabled fail-closed fallback to ROM.
- `src/tools/hybrid/m68k_626bc_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Ancient Music Driver Sound Script Bytecode Lookahead & Loop Marker Detector (`0x0626BC..0x062722`), verified in dual canonical 3,600-frame replay (Run A: 2,970 ROM executions; Run B: 2,970 native executions, 0 fallbacks, 11,880 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_626bc_instruction_step_test.cpp` — unit test suite verifying caller gating (both `0x061ABA` and `0x062544`), immediate 0xFD sequence termination (`bset #1, 0(a6)`, `a0 += 1`), non-script skip, F4 sub-command lookahead patterns, and 7 RTS exits.
- `src/tools/hybrid/m68k_0027fc_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for VDP DMA Queue Record Consumer & Direct Hardware Committer (`0x0027FC..0x002898`), verified in dual canonical 3,600-frame replay (Run A: 2,806 ROM executions; Run B: 2,806 native executions, 0 fallbacks, 123,464 stepped instructions, 2,806 DMA transfers committed, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0027fc_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x002156` and `0x002678`), Z80 BUSREQ assert/grant/release protocol, Mode 2 DMA enable/disable shadow toggles, VDP register programming sequence, initial word port write, and stack frame restoration at RTS.
- `src/tools/hybrid/m68k_002992_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Mega Drive Hardware Gamepad Reader & Debouncer (`0x002992..0x002AE2`), verified in dual canonical 3,600-frame replay (Run A: 2,714 ROM executions; Run B: 2,714 native executions, 0 fallbacks, 279,482 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_002992_step_ops.hpp/.cpp` — instruction-stepping primitives for `0x002992`, covering Z80 BUSREQ assert/release, NOP timing delays, dynamic ASL timing (+28 master cycles), TH pin toggles, edge-triggered button state debouncing, and 6-button/3-button detection paths.
- `tests/m68k_002992_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x002188`), helper op logic (NOP, ASL dynamic cycles, ADDQ, ANDI), 6-button path, 3-button path, invalid handshake fallback path, and stack preservation at RTS.
- `src/tools/hybrid/m68k_6138e_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Sound Channel Envelope Parameter Staging & Modulation Init (`0x06138E..0x0613B2`), verified in dual canonical 3,600-frame replay (Run A: 2,609 ROM executions; Run B: 2,609 native executions, 0 fallbacks, 18,263 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_6138e_instruction_step_test.cpp` — unit test suite verifying caller gating (both `0x061A12` and `0x0624A0`), Musashi-exact memory access ordering (`fetch` -> `read` -> `fetch` -> `write`), dynamic shift cycle attribution (+14 master cycles on `lsr.w #1, d6`), channel envelope parameter copy/staging, and handoff to native RTS.
- `src/tools/hybrid/m68k_00e682_instruction_step.hpp/.cpp` — exact ROM-checked, 6-caller-gated instruction-stepped helper for Entity Action/Animation Freeze Timer & Damage Stun Gatekeeper (`0x00E682..0x00E6BA`), verified in dual canonical 3,600-frame replay (Run A: 2,104 ROM executions; Run B: 2,104 native executions, 0 fallbacks, 10,859 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e682_instruction_step_test.cpp` — unit test suite verifying caller gating (6 callers: `0x0057D0`, `0x01A54E`, `0x01BF84`, `0x01BBF8`, `0x01C4DC`, `0x01B7F2`), normal path 1, freeze timer countdown path 2a, freeze timer active caller stack bypass (`addq.l #4, sp` + RTS to grandparent) path 2b, stun hurt flash path 3a/3b, and fail-closed fallback.
- `src/tools/hybrid/m68k_0613f8_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Sound Channel Dual Auxiliary Voice / Pitch-Modulation Slot Initializer (`0x0613F8..0x06147E`), verified in dual canonical 3,600-frame replay (Run A: 1,687 ROM executions; Run B: 1,687 native executions, 0 fallbacks, 13,496 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0613f8_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x061A16`), MOVEM save/restore cycle timing (168 master cycles each), post-increment dummy read, Slot 1 / Slot 2 activation, default pitch path, table lookup pitch path, and stack preservation at RTS.
- `src/tools/hybrid/m68k_016ebc_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Interactive Entity / World Object Update & Initialization Handler (`0x016EBC..0x016F0E`), verified in dual canonical 3,600-frame replay (Run A: 1,310 ROM executions; Run B: 1,310 native executions, 0 fallbacks, 5,260 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_016ebc_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00DF1A`, `0x00FF88`), memory-to-memory move displacement ordering (`fetch` -> `read` -> `fetch` -> `write`), initialization path (`0x9E == 0`, `0x9E == 1`, `0x9E == 2`), update idle path (`0x66 == 0`), trigger path fallback, and stack preservation at RTS.
- `src/tools/hybrid/m68k_009ad6_instruction_step.hpp/.cpp` — exact ROM-checked, 8-caller-gated instruction-stepped helper for Terrain Ground Height Calculation & Slope Elevation Interpolation (`0x009AD6..0x009BF2`), verified in dual canonical 3,600-frame replay (Run A: 1,283 ROM executions; Run B: 1,283 native executions, 0 fallbacks, 31,624 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_009ad6_step_ops.hpp` — register push/pop, CCR flag calculation, and memory helper functions for `0x009AD6` instruction-stepped execution.
- `src/tools/hybrid/m68k_009ad6_step_slope.hpp/.cpp` — terrain slope interpolation calculation routine (`0x009BA6..0x009BEA`) with bit-exact Musashi `MULU.W` dynamic cycle formula and `DIVU.W` iterative non-restoring division cycle loop.
- `tests/m68k_009ad6_instruction_step_test.cpp` — unit test suite verifying caller gating (8 callers), register preservation (`D0-D3, D5-D7, A0, A3-A7`), MOVEM save/restore bus cycles (168 master cycles each), flat terrain calculation, void handling, slope height interpolation arithmetic, iterative division cycles, and stack preservation at RTS.
- `src/tools/hybrid/m68k_009bf2_instruction_step.hpp/.cpp` — exact ROM-checked, 8-caller-gated instruction-stepped helper for Entity Bounding-Box Terrain Grid Footprint Bitwise OR/AND Aggregator (`0x009BF2..0x009C40`), verified in dual canonical 3,600-frame replay (Run A: 1,283 ROM executions; Run B: 1,283 native executions, 0 fallbacks, 124,041 stepped instructions, 18,816 inner cell reads, 4,817 outer row steps, bit-identical canonical six-stream hashes across all 3,600 frames).
- `tests/m68k_009bf2_instruction_step_test.cpp` — unit test suite verifying caller gating (8 callers: `0x0090E8`, `0x00918A`, `0x0091D4`, `0x009270`, `0x009312`, `0x00935C`, `0x0098D4`, `0x009D26`), 2D cell footprint OR/AND aggregation, upper-word preservation, 16-bit predecrement `movem.l` writes and post-increment `movem.l` 32-bit reads + 16-bit dummy read, dynamic shift/DBF cycle timing, boundary coordinates, and fail-closed fallback.
- `src/tools/hybrid/m68k_0027a6_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Register-Parameterized Fixed-Buffer VDP Hardware DMA Committer (`0x0027A6..0x0027FC`), verified in dual canonical 3,600-frame replay (Run A: 1,171 ROM executions; Run B: 1,171 native executions, 0 fallbacks, 26,933 stepped instructions, 1,171 DMA transfers, bit-identical canonical six-stream hashes across all 3,600 frames).
- `tests/m68k_0027a6_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x001FC4` SAT DMA and `0x002252` CRAM DMA), Z80 BUSREQ assertion/wait/release protocol, VDP Mode 2 shadow bit-4 toggle, inline ROM source register table reads via `(A6)+` and `(A6)`, stack-staged DMA trigger write, first-word VDP Data Port rewrite (`(A5) -> $C00000`), and fail-closed fallback.
- `src/tools/hybrid/m68k_00b15e_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Entity Multi-Piece Sprite Compositor, VDP SAT Shadow Buffer Builder & Dynamic VRAM Tile DMA Request Stager (`0x00B15E..0x00B28C`), verified in dual canonical 3,600-frame replay (Run A: 1,165 ROM executions; Run B: 1,165 native executions, 0 fallbacks, 130,936 stepped instructions, 1,729 SAT records written, 396 DMA descriptors staged, 479 offscreen clipped pieces, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00b15e_step_ops.hpp/.cpp` — instruction-stepping primitives, CCR status flag calculations, 5-register MOVEM save/restore bus cycles (280 master cycles each) with postincrement dummy read, and branch execution for `0x00B15E`.
- `src/tools/hybrid/m68k_00b15e_step_setup.cpp` — frame initialization and master entity table indexing phase (`0x00B15E..0x00B1DE`) for `0x00B15E`, covering entity orientation calculation, frame change detection, animation tick updates, dynamic shift cycle adjustments, and `bset #31, d5` timing (+14 master cycles).
- `src/tools/hybrid/m68k_00b15e_step_piece.cpp` — multi-piece loop, SAT entry generation, link indexing, and screen coordinate clipping phase (`0x00B1E0..0x00B23E`) for `0x00B15E`.
- `src/tools/hybrid/m68k_00b15e_step_dma.cpp` — dynamic VRAM tile DMA descriptor queue staging (`0x00FF10B4..`), per-frame tile budget decrement (`0x00FF1652`), and piece loop footer / epilogue phase (`0x00B240..0x00B288`) for `0x00B15E`.
- `tests/m68k_00b15e_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00AB7A` and `0x00AC74`), multi-piece SAT shadow buffer construction, offscreen piece clipping, VRAM tile DMA descriptor staging, per-frame budget tracking, and fail-closed fallback to ROM.
- `src/tools/hybrid/m68k_00a8da_instruction_step.hpp/.cpp` — exact ROM-checked, 5-caller-gated instruction-stepped helper for Direct VDP Sprite Attribute Table Entry Appender & HUD Shadow Record Builder (`0x00A8DA..0x00A8F0`), verified in dual canonical 3,600-frame replay (Run A: 1,082 ROM executions; Run B: 1,082 native executions, 0 fallbacks, 9,738 stepped instructions, 1,082 SAT entries written, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00a8da_instruction_step_test.cpp` — unit test suite verifying caller gating (5 callers: `0x00A81E`, `0x00A82A`, `0x00A836`, `0x00A886`, `0x00A896`), SAT entry generation and word-stride writes, sprite count increment, 80-sprite limit check and termination branch, upper word register preservation, and fail-closed fallback to ROM.
- `src/tools/hybrid/m68k_017d12_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Collectible Dropped Item & Chest Entity Handler (`0x017D12..0x017E28`, Entity Type `0x1E` / 30), verified in dual canonical 3,600-frame replay (Run A: 985 ROM executions; Run B: 985 native executions, 0 fallbacks, 10,025 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_017d12_step_ops.hpp/.cpp` — instruction-stepping primitives, CCR status flag calculations, memory helpers, and child call delegation protocol (`step_jsr_abs` and `step_bsr_w`) under `namespace oasis::hybrid::step_017d12`.
- `src/tools/hybrid/m68k_017d12_step_init.cpp` — spawn initialization (`0x017D12..0x017D3C`), air falling / gravity integration, ground clamping, single bounce, and bounce sound dispatch (`0x017D3E..0x017D82`) for `0x017D12`.
- `src/tools/hybrid/m68k_017d12_step_update.cpp` — player contact detection, pickup state transition, sound dispatch, collection timer handling, inventory grant, persistent event registration, and chest open phase (`0x017D84..0x017E26`) for `0x017D12`.
- `tests/m68k_017d12_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00DF1A` and `0x00FF88`), spawn initialization with bit 7 clear/set, idle update, collection timer countdown, in-air gravity velocity integration, chest open deactivation, contact sound dispatch with child yield, and inventory item collection.
- `src/tools/hybrid/m68k_6281a_step_tail.hpp/.cpp` — shared release phase & attenuation tail dispatch (`0x06281A..0x062866`) under `namespace oasis::hybrid::step_6281a`, reused by both `0x062788` and `0x06281A` to eliminate conflicting instruction ownership and duplicated logic.
- `src/tools/hybrid/m68k_6281a_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Sound Channel Note-Off / Release Phase Attenuation Committer (`0x06281A..0x06286C`, caller `0x06243C`), verified in dual canonical 3,600-frame replay (Run A: 985 ROM executions; Run B: 985 native executions, 0 fallbacks, 19,773 stepped instructions, 985 child calls to `0x06273A`, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_6281a_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x06243C`), invalid register rejection, normal release decrement, underflow clamp to 0, master volume multiplication, optional tremolo modulation delta application, attenuation inversion, child call to hardware attenuation committer `0x06273A`, envelope caching, and RTS completion.
- `src/tools/hybrid/m68k_6282e_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Sound Channel Attenuation Calculator & Hardware Committer Sub-Tail Entry (`0x06282E..0x06286C`, caller `0x0624C8`), verified in dual canonical 3,600-frame replay (Run A: 922 ROM executions; Run B: 922 native executions, 0 fallbacks, 11,986 stepped instructions, 922 child calls to `0x06273A`, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_6282e_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x0624C8`), invalid register rejection, master volume multiplication (factor 1..16), tremolo modulation branch, attenuation inversion (`eori.b #15, d0`), child call to hardware attenuation committer `0x06273A`, envelope caching, and RTS completion.
- `src/tools/hybrid/m68k_01ac90_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Type 0x38 Lifecycle State Machine & Jump Table Dispatcher (`0x01AC90..0x01ACA2`, callers `0x00DF1A`, `0x00FD7A`, `0x00FDFE`), verified in dual canonical 3,600-frame replay (Run A: 914 ROM executions; Run B: 914 native executions, 0 fallbacks, 5,484 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01ac90_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00DF1A`, `0x00FD7A`, `0x00FDFE`), invalid register rejection, state index doubling, jump table offset indexing, jump dispatch across states, condition codes, and fail-closed rejection of bad callers, invalid SP, wrong entity type, and unsupported states.
- `src/tools/hybrid/m68k_01b102_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Type 0x38 Damage & Hit Processing Handler (`0x01B102..0x01B1BE`, callers `0x01B7F8`, `0x01BBFE`, `0x01BF8A`, `0x01C4E2`), verified in dual canonical 3,600-frame replay (Run A: 890 ROM executions; Run B: 890 native executions, 0 fallbacks, 2,121 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_01b102_step_hit.hpp/.cpp` — instruction-stepping primitives, CCR status flag calculations, damage formula, recoil/death state transitions, direction table lookup (`0x01074A`), and sound/animation child call protocol under `namespace oasis::hybrid::step_01b102`.
- `tests/m68k_01b102_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x01B7F8`, `0x01BBFE`, `0x01BF8A`, `0x01C4E2`), no-hit fast path, non-fatal recoil path, fatal damage with death hook (`0x00E4D0`), SFX child call (`0x00E7E8`), and fail-closed rejection of unexpected callers, bad return PC, and invalid SP.
- `src/tools/hybrid/m68k_00350e_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Title Screen "PRESS START BUTTON" Blinking String Dispatcher (`0x00350E..0x003534`, caller `0x0033FE`), verified in dual canonical 3,600-frame replay (Run A: 866 ROM executions; Run B: 866 native executions, 0 fallbacks, 7,139 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00350e_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x0033FE`), visible string path (`0x00003534`), blank spaces path (`0x00003547`), D6 upper word preservation, CCR flags, and fail-closed rejection of wrong callers, invalid SP, and corrupt ROM.
- `src/tools/hybrid/m68k_008e22_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Hardware Palette Cycling & Animated CRAM Engine (`0x008E22..0x008E90`, callers `0x0007B8` and `0x008B3E`), verified in dual canonical 3,600-frame replay (Run A: 842 ROM executions; Run B: 842 native executions, 0 fallbacks, 47,465 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_008e22_step_cycle.hpp/.cpp` — instruction-stepping primitives, record loop traversal, frame countdown, keyframe advance, loop wrap, and CRAM shadow buffer write logic under `namespace oasis::hybrid::step_008e22`.
- `tests/m68k_008e22_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x0007B8` and `0x008B3E`), disabled fast exit, countdown without advance, keyframe advance with CRAM buffer update and DMA request flag, keyframe loop wrap, and fail-closed rejections.
- `src/tools/hybrid/m68k_00a7e6_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for HUD Status Overlay Sprite Generator & VRAM DMA Transfer Queuer (`0x00A7E6..0x00A8DA`, callers `0x00A336` and `0x00A694`), verified in dual canonical 3,600-frame replay (Run A: 811 ROM executions; Run B: 811 native executions, 0 fallbacks, 12,017 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00a7e6_step_mode_a.hpp/.cpp` — Mode A sprite attributes, tile offsets, child `0x00A8DA` BSR calls, and DMA transfer queueing logic under `namespace oasis::hybrid::step_00a7e6`.
- `src/tools/hybrid/m68k_00a7e6_step_mode_b.hpp/.cpp` — Mode B sprite attributes, tile offsets, child `0x00A8DA` BSR calls, and DMA transfer queueing logic under `namespace oasis::hybrid::step_00a7e6`.
- `tests/m68k_00a7e6_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00A336` and `0x00A694`), disabled fast exit, mode A with and without DMA, mode B with DMA, child helper invocation, and fail-closed rejections.
- `src/tools/hybrid/m68k_008e90_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Active Entity Pool Scan & Master Physics/Animation Update Loop (`0x008E90..0x008EF6`, callers `0x008B42` and `0x00CD24`), verified in dual canonical 3,600-frame replay (Run A: 775 ROM executions; Run B: 775 native executions, 0 fallbacks, 430,427 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_008e90_step_ops.hpp/.cpp` — instruction-stepping primitives, CCR status flag calculations, sign-extended `movem.w` popping, dynamic shift cycles (`CYC_SHIFT`), and stack operations under `namespace oasis::hybrid::step_008e90`.
- `src/tools/hybrid/m68k_008e90_step_scan.hpp/.cpp` — triple-pool scan loops (`0xFF1770`, `0xFF19E8`, `0xFF2D8C`), active entity checks, and pool traversal logic under `namespace oasis::hybrid::step_008e90`.
- `src/tools/hybrid/m68k_008e90_step_anim.hpp/.cpp` — entity animation update, direction flags, and animation frame sequencing under `namespace oasis::hybrid::step_008e90`.
- `src/tools/hybrid/m68k_008e90_step_move_x.hpp/.cpp` — horizontal entity physics, velocity accumulation, terrain collision checks, and child call dispatch under `namespace oasis::hybrid::step_008e90`.
- `src/tools/hybrid/m68k_008e90_step_move_y.hpp/.cpp` — vertical entity physics, gravity accumulation, terrain collision checks, and child call dispatch under `namespace oasis::hybrid::step_008e90`.
- `src/tools/hybrid/m68k_008e90_step_move_alt.hpp/.cpp` — alternate movement path physics, collision responses, and coordinate updates under `namespace oasis::hybrid::step_008e90`.
- `tests/m68k_008e90_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B42`, `0x00CD24`), inactive entity pool scanning, child helper dispatch, fail-closed rejections, and register preservation.
- `src/tools/hybrid/m68k_00f788_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Event Script & Cutscene / Camera Scroll Sequencer (`0x00F788..0x00F920`, callers `0x008B2A` and `0x00CD02`), verified in dual canonical 3,600-frame replay (Run A: 775 ROM executions; Run B: 775 native executions, 0 fallbacks, 11,189 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00f788_step_ops.hpp/.cpp` — instruction-stepping primitives, absolute memory moves, long moves, byte logic flags, swap/neg, and stack operations under `namespace oasis::hybrid::step_00f788`.
- `src/tools/hybrid/m68k_00f788_step_seq.hpp/.cpp` — event script inhibit checks, countdown timer decrements, camera pan direction dispatch, script sequencer byte loop, and early child exit handling under `namespace oasis::hybrid::step_00f788`.
- `tests/m68k_00f788_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B2A`, `0x00CD02`), inhibit path, timer path, script command loop, early child exits, and fail-closed rejections.
- `src/tools/hybrid/m68k_00a196_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Sprite Table Committer & Multi-Pool Display Dispatcher (`0x00A196..0x00A33E`, callers `0x008B86` and `0x00CD28`), verified in dual canonical 3,600-frame replay (Run A: 773 ROM executions; Run B: 773 native executions, 0 fallbacks, 882,033 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00a196_step_ops.hpp/.cpp` — instruction-stepping primitives, bit manipulation (`bset` with upper-word cycle scaling), indexing, stack push/pop, and RTS primitives under `namespace oasis::hybrid::step_00a196`.
- `src/tools/hybrid/m68k_00a196_step_seq.hpp/.cpp` — multi-pool scan loops ($FF2954, $FF19C8, $FF19E8, $FF2ABC, $FF2D8C), indirect jump dispatch, HUD generator triggers, SAT finalization, and DMA staging under `namespace oasis::hybrid::step_00a196`.
- `tests/m68k_00a196_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B86`, `0x00CD28`), base sprite building, HUD triggers, SAT finalization, child return handling, and fail-closed rejections.
- `src/tools/hybrid/m68k_00a342_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Base Sprite Builder & VDP DMA Queue Stager (`0x00A342..0x00A436`, callers `0x00A19C` and `0x00A6A0`), verified in dual canonical 3,600-frame replay (Run A: 773 ROM executions; Run B: 773 native executions, 0 fallbacks, 55,011 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00a342_step_ops.hpp/.cpp` — instruction-stepping primitives, shadow SAT pointer displacement, sprite attribute generation, DBF loop stepping, and VDP DMA queue staging under `namespace oasis::hybrid::step_00a342`.
- `src/tools/hybrid/m68k_00a342_step_seq.hpp/.cpp` — sprite table template selection (tables 0x00A438 and 0x00A480), sprite linking loops, cursor sprite generation, and DMA queue staging under `namespace oasis::hybrid::step_00a342`.
- `tests/m68k_00a342_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00A19C`, `0x00A6A0`), sprite table building, DBF loop iteration, DMA queue staging, and fail-closed rejections.
- `src/tools/hybrid/m68k_00b34c_instruction_step.hpp/.cpp` — exact ROM-checked, dual-caller-gated instruction-stepped helper for Entity Display Mode & Visual Effects Dispatcher (`0x00B34C..0x00B3B0`, callers `0x00AD42` and `0x00B012`), verified in dual canonical 3,600-frame replay (Run A: 773 ROM executions; Run B: 773 native executions, 0 fallbacks, 12,368 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00b34c_step_ops.hpp/.cpp` — instruction-stepping primitives, absolute byte testing, memory bit test & clear (`bclr #2, 57(%fp)`), and RTS under `namespace oasis::hybrid::step_00b34c`.
- `src/tools/hybrid/m68k_00b34c_step_seq.hpp/.cpp` — global pause/freeze check (`$FF1927`), entity display mode flag bit-testing (`$39(%fp)`), and child delegation (`0x00B3D2`, `0x00B56E`, `0x00B45C`, `0x00B4FE`, `0x00B610`) under `namespace oasis::hybrid::step_00b34c`.
- `tests/m68k_00b34c_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00AD42`, `0x00B012`), freeze early exit, primary child dispatch, background child dispatch, overlay child dispatch with memory bit clear, and fail-closed rejections.
- `src/tools/hybrid/m68k_00c980_instruction_step.hpp/.cpp` — exact ROM-checked, multi-caller-gated instruction-stepped helper for Per-Frame Map Script Vector 0 Dispatcher (`0x00C980..0x00C996`, callers `0x008B60`, `0x00CD20`, and `0x006A3E`), verified in dual canonical 3,600-frame replay (Run A: 773 ROM executions; Run B: 773 native executions, 0 fallbacks, 3,805 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00c980_step_ops.hpp/.cpp` — instruction-stepping primitives, absolute bit testing (`btst #1, $FF17B8`), address register loading (`movea.l $FF16EC, %a1`), indexed addition (`adda.w (%a1), %a1`), indirect tail jumping (`jmp (%a1)`), and RTS under `namespace oasis::hybrid::step_00c980`.
- `tests/m68k_00c980_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B60`, `0x00CD20`, `0x006A3E`), corrupt ROM rejection, script inhibit early exit, vector 0 dispatch path, and interruption/reentrancy protection.
- `src/tools/hybrid/m68k_00c998_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Per-Frame Map Script Vector 1 Dispatcher (`0x00C998..0x00C9B0`, caller `0x008B52`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 4,542 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00c998_step_ops.hpp/.cpp` — instruction-stepping primitives, address register quick-addition (`addq.l #2, %a1`) under `namespace oasis::hybrid::step_00c998`.
- `tests/m68k_00c998_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B52`), corrupt ROM rejection, script inhibit early exit, vector 1 dispatch path, and interruption/reentrancy protection.
- `src/tools/hybrid/m68k_008dd4_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for HUD / Screen Spirit / Special Counter Step Handler (`0x008DD4..0x008E20`, caller `0x008B46`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 4,632 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_008dd4_step_ops.hpp/.cpp` — instruction-stepping primitives, absolute word testing (`tst.w $FF1AA4`) and immediate comparison (`cmpi.w #$0014, $FF199E`) under `namespace oasis::hybrid::step_008dd4`.
- `tests/m68k_008dd4_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B46`), corrupt ROM rejection, active increment path, inactive zero-exit path, and interruption/reentrancy protection.
- `src/tools/hybrid/m68k_00557a_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Player Per-Frame State, Footstep Audio & Shadow Coordinator (`0x00557A..0x00566E`, caller `0x008B2E`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 24,968 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00557a_step_ops.hpp/.cpp` — instruction-stepping primitives, absolute flag clear (`sf $FF1986`), displacement register operations, bit testing (`btst #0, 23(%a6)`), register swapping (`exg %d1, %d2`), immediate comparison, arithmetic, sound event dispatch, and shadow coordinate updates under `namespace oasis::hybrid::step_00557a`.
- `src/tools/hybrid/m68k_00557a_step_block1.hpp/.cpp` — block 1 instruction dispatch sequence for `0x00557A..0x0055DC`.
- `src/tools/hybrid/m68k_00557a_step_block2.hpp/.cpp` — block 2 instruction dispatch sequence for `0x0055E2..0x00566E`.
- `tests/m68k_00557a_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B2E`), corrupt ROM rejection, shadow skip path, shadow update path, and interruption/reentrancy protection.
- `src/tools/hybrid/m68k_005670_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Player Pre-Action Input, Timers & Action Dispatcher (`0x005670..0x0059B8` + `0x005B3C..0x005B9E`, caller `0x005586`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 47,655 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_005670_step_ops.hpp`, `src/tools/hybrid/m68k_005670_step_ops1.cpp`, `src/tools/hybrid/m68k_005670_step_ops2.cpp` — instruction-stepping primitives, input/timer evaluations, table lookups, arithmetic, and hardware bus refresh cycles under `namespace oasis::hybrid::step_005670`.
- `src/tools/hybrid/m68k_005670_step_block1.hpp/.cpp` — block 1 instruction dispatch sequence for `0x005670..0x0057D0`.
- `src/tools/hybrid/m68k_005670_step_block2.hpp/.cpp` — block 2 instruction dispatch sequence for `0x0057D6..0x0059B8`.
- `src/tools/hybrid/m68k_005670_step_block3.hpp/.cpp` — block 3 instruction dispatch sequence for `0x005B3C..0x005B9E`.
- `tests/m68k_005670_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x005586`), corrupt ROM rejection, odd SP rejection, action dispatch path, and interruption/reentrancy protection.
- `src/tools/hybrid/m68k_008b8e_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Per-Frame State Gate & Mode Transition Dispatcher (`0x008B8E..0x008BAC`, caller `0x008B26`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 2,316 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_008b8e_step_ops.hpp/.cpp` — instruction-stepping primitives, absolute bit clear (`bclr #7, $00FF1864`) and mode tail branches under `namespace oasis::hybrid::step_008b8e`.
- `tests/m68k_008b8e_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B26`), corrupt ROM rejection, odd SP rejection, zero-exit path, bra 0x530c path, bra 0x7cc6 path, and interruption/reentrancy handling.
- `src/tools/hybrid/m68k_008bac_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Player Menu, Pause & Inventory Coordinator (`0x008BAC..0x008C5A`, caller `0x008B22`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 17,262 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_008bac_step_ops.hpp/.cpp` — instruction-stepping primitives, controller state reads, inhibit checks, and menu jump handlers under `namespace oasis::hybrid::step_008bac`.
- `tests/m68k_008bac_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B22`), corrupt ROM rejection, odd SP rejection, clean RTS path, inhibit RTS path, and menu jump path.
- `src/tools/hybrid/m68k_00deec_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Master Active Entity Tick & Isometric Projection Loop (`0x00DEEC..0x00DF52`, caller `0x008B32`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 161,708 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00deec_step_ops.hpp/.cpp` — instruction-stepping primitives, entity loop state evaluation, movem stack operations, isometric velocity projection, and dbf cycle adjustments under `namespace oasis::hybrid::step_00deec`.
- `tests/m68k_00deec_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B32`), corrupt ROM rejection, odd SP rejection, inhibit early exit path, full entity loop path, and reentrancy/state handling.
- `src/tools/hybrid/m68k_00e0ba_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Secondary / Auxiliary Entity Slot Dispatch Loop (`0x00E0BA..0x00E0F4`, caller `0x008B36`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 21,608 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00e0ba_step_ops.hpp/.cpp` — instruction-stepping primitives, auxiliary entity slot active evaluation, movem stack operations, and dbf cycle adjustments under `namespace oasis::hybrid::step_00e0ba`.
- `tests/m68k_00e0ba_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B36`), corrupt ROM rejection, odd SP rejection, inhibit early exit path, empty slots loop path, and active child slot dispatch path.
- `src/tools/hybrid/m68k_00e0f4_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Tertiary / Interactive Entity Slot Dispatch & Table Jump Loop (`0x00E0F4..0x00E140`, caller `0x008B3A`), verified in dual canonical 3,600-frame replay (Run A: 772 ROM executions; Run B: 772 native executions, 0 fallbacks, 32,010 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00e0f4_step_ops.hpp/.cpp` — instruction-stepping primitives, countdown timer decrement, tertiary entity slot evaluation, jump table offset indexing, movem stack operations, and dbf cycle adjustments under `namespace oasis::hybrid::step_00e0f4`.
- `tests/m68k_00e0f4_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x008B3A`), corrupt ROM rejection, odd SP rejection, timer countdown & inhibit early exit path, empty slots loop path, and active child slot dispatch path.
- `src/tools/hybrid/m68k_00b3b2_instruction_step.hpp/.cpp` — exact ROM-checked, single-caller-gated instruction-stepped helper for Player / Entity Pause Freeze Gate & Flag Dispatch Subroutine (`0x00B3B2..0x00B3D2`, caller `0x00AD4E`), verified in dual canonical 3,600-frame replay (Run A: 759 ROM executions; Run B: 759 native executions, 0 fallbacks, 4,554 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `src/tools/hybrid/m68k_00b3b2_step_ops.hpp/.cpp` — instruction-stepping primitives, global pause test, entity attribute inspection, and child routine dispatch under `namespace oasis::hybrid::step_00b3b2`.
- `tests/m68k_00b3b2_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00AD4E`), corrupt ROM rejection, odd SP rejection, pause exit path, bit 1 clear exit path, and bit 1 set child bsr path.
- `src/tools/hybrid/m68k_camera_scroll_step_ops.hpp/.cpp` and `m68k_camera_scroll_step_ops_alu.cpp` — modular instruction-stepping primitives, branch adjustments, ALU operations, shifts, and memory reads/writes for the Camera Viewport Scroll subsystem under `namespace oasis::hybrid::step_camera`.
- `src/tools/hybrid/m68k_00ceea_instruction_step.hpp/.cpp` and `m68k_00ceea_line_buffer.hpp/.cpp` — instruction-stepped native helper for Camera Viewport Boundary Clamping & VDP Scroll Line Buffer Pipeline (`0x00CEEA..0x00D0C4`, callers `0x02E604`, `0x02E6F6`, `0x02E87C`).
- `src/tools/hybrid/m68k_00d188_instruction_step.hpp/.cpp` — instruction-stepped native helper for Camera Sub-pixel Velocity & Fractional Position Accumulator (`0x00D188..0x00D266`, caller `0x00D0BC`).
- `src/tools/hybrid/m68k_00d344_instruction_step.hpp/.cpp` — instruction-stepped native helper for Camera VDP Scroll Plane Table Committal & Inter-frame Delta Step (`0x00D344..0x00D398`, caller `0x00D2B4`).
- `src/tools/hybrid/m68k_00d398_instruction_step.hpp/.cpp` — instruction-stepped native helper for Camera Plane A/B Offset Sync & VDP Queue Trigger (`0x00D398..0x00D404`, callers `0x02E60A`, `0x02E6FC`, `0x02E882`).
- `tests/m68k_camera_scroll_instruction_step_test.cpp` — unit test suite verifying all 4 camera scroll routines, caller gatings, ROM checks, and state transitions.
- `src/tools/hybrid/m68k_terrain_collision_step_ops.hpp/.cpp` and `m68k_terrain_collision_step_ops_alu.cpp` — modular instruction-stepping primitives, coordinate delta math, elevation lookups, bit operations with hardware writeback cycle calibration, and bounds checking for the Terrain Heightmap Collision subsystem under `namespace oasis::hybrid::step_terrain`.
- `src/tools/hybrid/m68k_009d4e_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 0 (Right) Terrain Elevation Delta Step (`0x009D4E..0x009D70`).
- `src/tools/hybrid/m68k_009d70_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 1 (Left) Terrain Elevation Delta Step (`0x009D70..0x009D92`).
- `src/tools/hybrid/m68k_009d92_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 2 (Down) Terrain Elevation Delta Step (`0x009D92..0x009DB4`).
- `src/tools/hybrid/m68k_009db4_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 3 (Up) Terrain Elevation Delta Step (`0x009DB4..0x009DD6`).
- `src/tools/hybrid/m68k_009dd6_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 0 (Right) Entity Bounding Box Collision Loop (`0x009DD6..0x009EBA`).
- `src/tools/hybrid/m68k_009eba_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 1 (Left) Entity Bounding Box Collision Loop (`0x009EBA..0x009FA0`).
- `src/tools/hybrid/m68k_009fa0_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 2 (Down) Entity Bounding Box Collision Loop (`0x009FA0..0x00A09A`).
- `src/tools/hybrid/m68k_00a09a_instruction_step.hpp/.cpp` — instruction-stepped native helper for Direction 3 (Up) Entity Bounding Box Collision Loop (`0x00A09A..0x00A196`).
- `tests/m68k_terrain_collision_instruction_step_test.cpp` — comprehensive unit test suite covering all 8 terrain heightmap collision routines, coordinate delta math, bit 31 set/clear branches, and boundary loops.
- `src/tools/hybrid/m68k_player_weapon_step_ops.hpp/.cpp` and `m68k_player_weapon_step_ops_alu.cpp` — modular instruction-stepping primitives, ALU operations, MOVEM 168-cycle stack operations, and dynamic shift timing for the Player Weapon Sprite Assembly subsystem under `namespace oasis::hybrid::step_player_weapon`.
- `src/tools/hybrid/m68k_00b3d2_instruction_step.hpp/.cpp` — instruction-stepped native helper for Player Primary Weapon Sprite Piece Layout Step (`0x00B3D2..0x00B42C`, caller `0x00B3CA`).
- `src/tools/hybrid/m68k_00b56e_instruction_step.hpp/.cpp` — instruction-stepped native helper for Player Secondary Weapon / Bomb Sprite Piece Step (`0x00B56E..0x00B5C8`, caller `0x00B3CE`).
- `src/tools/hybrid/m68k_00b6a6_instruction_step.hpp/.cpp` — instruction-stepped native helper for Weapon Sprite SAT Attribute Assembly & Screen Clipping Dispatch (`0x00B6A6..0x00B730`, callers `0x00B422`, `0x00B5BE`).
- `tests/m68k_player_weapon_instruction_step_test.cpp` — unit test suite covering all 3 player weapon sprite routines, negative-piece terminal exits, zero-frame exits, and sprite emission.
- `src/tools/hybrid/m68k_00c9b2_step_ops.hpp/.cpp` — instruction-stepping primitives, absolute byte testing (`tst.b ($FFxxxx)`), and register updates under `namespace oasis::hybrid::step_00c9b2`.
- `src/tools/hybrid/m68k_00c9b2_instruction_step.hpp/.cpp` — exact ROM-checked, multi-caller-gated instruction-stepped helper for Room Transition Script Abort & Mode Gatekeeper (`0x00C9B2..0x00C9EC`, callers `0x02E5B0`, `0x02E64E`, `0x02E774`), verified in dual canonical 3,600-frame replay (Run A: 742 ROM executions; Run B: 742 native executions, 0 fallbacks, 9,646 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00c9b2_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x02E5B0`, `0x02E64E`, `0x02E774`), corrupt ROM rejection, all 6 flag abort return branches (`ADDQ.L #4, SP; RTS`), normal exit path (`RTS`), and interrupt/reentrancy protection.
- `data/thor-evidence/c9b2-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00C9B2` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00c1ce_step_ops.hpp/.cpp` — instruction-stepping primitives, 8-register MOVEM stack push/pop, dynamic shift timing, player position subtraction, and CCR bit manipulation under `namespace oasis::hybrid::step_00c1ce`.
- `src/tools/hybrid/m68k_00c1ce_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Proximity AABB Intersect Collision Test (`0x00C1CE..0x00C222`, caller `0x02E796`), verified in dual canonical 3,600-frame replay (Run A: 463 ROM executions; Run B: 463 native executions, 0 fallbacks, 13,427 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00c1ce_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x02E796`), corrupt ROM rejection, collision hit path (`ORI #1, CCR`, C=1), collision miss path (`ANDI #$FE, CCR`, C=0), and MOVEM register preservation.
- `data/thor-evidence/c1ce-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00C1CE` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00e0b8_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Null Subroutine & Idle Slot Return Stub (`0x00E0B8..0x00E0BA`, callers `0x00DF1A`, `0x00FF88`), completing the native `ENTITY_LIFECYCLE` dispatch group and eliminating inter-domain exits from `0x00DEEC`, verified in dual canonical 3,600-frame replay (Run A: 464 ROM executions; Run B: 464 native executions, 0 fallbacks, 464 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e0b8_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00DF1A`, `0x00FF88`), corrupt ROM rejection, SP popping, return state prefetch alignment, and cycle accounting.
- `data/thor-evidence/e0b8-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E0B8` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00b856_step_ops.hpp/.cpp` — instruction-stepping primitives, 8-register MOVEM stack/memory push/pop, 10-cycle `lsr.l #1` shift timing compensation, bounding box subtraction/comparisons, and CCR bit manipulation under `namespace oasis::hybrid::step_00b856`.
- `src/tools/hybrid/m68k_00b856_instruction_step.hpp/.cpp` — exact ROM-checked, multi-entry instruction-stepped helper for Entity Overlap AABB Query Generator & Iteration Engine (`0x00B852..0x00B922`, entry `0x00B856`, prefix `0x00B852`, resume `0x00B912`, callers `0x00B7CA`, `0x00B848`, `0x008332`), verified in dual canonical 3,600-frame replay (Run A: 154 target entries, 41 resume entries; Run B: 154 native target calls, 41 resume calls, 195 total completed, 0 fallbacks, 39,325 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00b856_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x00B7CA`, `0x00B848`, `0x008332`), corrupt ROM rejection, AABB query hit path (`ANDI #$FE, CCR`), miss path (`ORI #1, CCR`), and resume iterator loop.
- `data/thor-evidence/b856-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00B856` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00b79a_step_ops.hpp/.cpp` — instruction-stepping primitives, directional facing vector setup, displacement comparisons, child BSR invocation, and CCR arithmetic/logic manipulation under `namespace oasis::hybrid::step_00b79a`.
- `src/tools/hybrid/m68k_00b79a_instruction_step.hpp/.cpp` — exact ROM-checked, multi-caller-gated instruction-stepped helper for Player Combat Hitbox Directional Scan & Reaction Dispatcher (`0x00B79A..0x00B852`, callers `0x005D46`, `0x00E53C`), verified in dual canonical 3,600-frame replay (Run A: 139 ROM executions; Run B: 139 native executions, 0 fallbacks, 2,844 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00b79a_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x005D46`, `0x00E53C`), corrupt ROM rejection, direction-based search bounds adjustment, child BSR handling (`0x00B856`), and RTS exit.
- `data/thor-evidence/b79a-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00B79A` displacement across all 8 canonical streams.
- `data/thor-evidence/combat-hitbox-replay-differential-20261018.json` — composite machine-readable verification receipt proving bit-for-bit replay equivalence of the complete `PLAYER_ACTION_COMBAT_HITBOX` cluster (`0x00B79A` + `0x00B856`) across all 8 canonical streams.
- `src/tools/hybrid/m68k_061ef4_step_ops.hpp/.cpp` — instruction-stepping primitives, bit test, dynamic shift/rotate cycle compensation, memory operand reads, register updates, and CCR bit manipulation under `namespace oasis::hybrid::step_061ef4`.
- `src/tools/hybrid/m68k_061ef4_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for YM2612 4-Operator FM Envelope Parameter Formatter & Sound Queue Appender (`0x061EF4..0x061F56`, caller `0x061EE4`), verified in dual canonical 3,600-frame replay (Run A: 731 ROM calls; Run B: 731 native calls, 0 fallbacks, 34,155 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_061ef4_instruction_step_test.cpp` — unit test suite verifying caller gating (`0x061EE4`), corrupt ROM rejection, early exit branch path, and 4-operator loop execution path.
- `src/tools/hybrid/m68k_061cd2_step_ops.hpp/.cpp` — instruction-stepping primitives, register/accumulator increment/decrement, DBF loop branch compensation, sound queue post-increment memory writes, and CCR arithmetic manipulation under `namespace oasis::hybrid::step_061cd2`.
- `src/tools/hybrid/m68k_061cd2_instruction_step.hpp/.cpp` — exact ROM-checked, multi-caller-gated instruction-stepped helper for YM2612 4-Operator Register Index Sequence Step & Queue Allocator (`0x061CD2..0x061CEE`, callers `0x061D28`, `0x061DB6`, `0x06219A`), verified in dual canonical 3,600-frame replay (Run A: 121 ROM calls; Run B: 121 native calls, 0 fallbacks, 3,025 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_061cd2_instruction_step_test.cpp` — unit test suite verifying multi-caller gating (`0x061D28`, `0x061DB6`, `0x06219A`), corrupt ROM rejection, 4-operator register index step calculation, and sound queue write verification.
- `data/thor-evidence/audio-envelope-replay-differential-20261018.json` — composite machine-readable verification receipt proving bit-for-bit replay equivalence of the complete `YM2612_ENVELOPE_FORMATTER` cluster (`0x061EF4` + `0x061CD2`) across all 8 canonical streams.
- `src/tools/hybrid/m68k_01b580_step_ops.hpp/.cpp` — instruction-stepping primitives, dynamic shift cycle compensation (`lsr.w #1`), stack memory operations (`step_movem_to_stack_d0`), branch adjustments, and CCR flag manipulation under `namespace oasis::hybrid::step_01b580`.
- `src/tools/hybrid/m68k_01b580_instruction_step.hpp/.cpp` — exact ROM-checked, 7-caller-gated instruction-stepped helper for Entity-to-Player Front/Rear Dual-Sector Proximity & Detection Scanner (`0x01B580..0x01B5DA`, callers `0x01B558`, `0x01B89A`, `0x01B95C`, `0x01BC40`, `0x01BCBE`, `0x01BFD4`, `0x01C04A`), verified in dual canonical 3,600-frame replay (Run A: 406 ROM calls; Run B: 406 native calls, 0 fallbacks, 6,484 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01b580_instruction_step_test.cpp` — unit test suite verifying 7-caller gating, corrupt ROM rejection, inhibit exit path, front detect hit path, and rear detect jump path.
- `data/thor-evidence/01b580-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01B580` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0064c4_step_ops.hpp/.cpp` — instruction-stepping primitives, dynamic arithmetic shift right cycle compensation (`asr.l #1`), facing direction bitmasking, and CCR arithmetic manipulation under `namespace oasis::hybrid::step_0064c4`.
- `src/tools/hybrid/m68k_0064c4_instruction_step.hpp/.cpp` — exact ROM-checked, 5-caller-gated instruction-stepped helper for Player Isometric Diagonal & Surface Velocity Scaler (`0x0064C4..0x006516`, callers `0x0062FC`, `0x00653A`, `0x006692`, `0x006720`, `0x0067D0`), verified in dual canonical 3,600-frame replay (Run A: 269 ROM calls; Run B: 269 native calls, 0 fallbacks, 3,131 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0064c4_instruction_step_test.cpp` — unit test suite verifying 5-caller gating, corrupt ROM rejection, orthogonal facing preserve path, diagonal facing scale path, slowdown mode halving path, and running lock skip path.
- `data/thor-evidence/0064c4-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0064C4` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_010930_step_ops.hpp/.cpp` — instruction-stepping primitives, Z-delta height arithmetic, direction table indexing, and Carry flag manipulation under `namespace oasis::hybrid::step_010930`.
- `src/tools/hybrid/m68k_010930_instruction_step.hpp/.cpp` — exact ROM-checked, 4-entry-point instruction-stepped helper for Multi-Sector Player Bounding Box Hitbox & Z-Height Proximity Test (`0x010916..0x010A74`, entry points `0x010916`, `0x010920`, `0x010926`, `0x010930`), closing the native-to-ROM hole from `0x01B580`, verified in dual canonical 3,600-frame replay (Run A: 669 ROM calls; Run B: 669 native calls, 0 fallbacks, 8,232 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_010930_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, Z-reject miss exit, each cardinal direction hit and miss paths, and all 4 entry points.
- `data/thor-evidence/010930-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x010930` cluster displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_02e704_step_ops.hpp/.cpp` — instruction-stepping primitives, entity slot loop pointer math, elevation addition, and dbra loop timing under `namespace oasis::hybrid::step_02e704`.
- `src/tools/hybrid/m68k_02e704_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Elevation & Shadow Layer Delta Adjustment Loop (`0x02E704..0x02E748`, callers `0x02E64A` and `0x02E770`), verified in dual canonical 3,600-frame replay (Run A: 653 ROM calls; Run B: 653 native calls, 0 fallbacks, 76,968 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_02e704_instruction_step_test.cpp` — unit test suite verifying disabled mode, unsupported caller rejection, corrupt ROM rejection, all inactive slots, active slot elevation update, and filter condition matrix.
- `data/thor-evidence/02e704-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x02E704` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0178ea_step_ops.hpp/.cpp` — instruction-stepping primitives, entity substate testing, animation doubling, and vertical velocity kick under `namespace oasis::hybrid::step_0178ea`.
- `src/tools/hybrid/m68k_0178ea_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Substate-0 First-Tick Initializer & Z-Velocity Kick (`0x0178EA..0x017908`, callers `0x00DF1A` and `0x00FF88`), verified in dual canonical 3,600-frame replay (Run A: 541 ROM calls; Run B: 541 native calls, 0 fallbacks, 1,633 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0178ea_instruction_step_test.cpp` — unit test suite verifying disabled mode, unsupported caller rejection, corrupt ROM rejection, substate skip path, and substate init paths for both callers.
- `data/thor-evidence/0178ea-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0178EA` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00baac_step_ops.hpp/.cpp` — instruction-stepping primitives, entity 3D bounding box checks, flag tests, and cycle-accurate MOVEM.W dummy-read/timing operations under `namespace oasis::hybrid::step_00baac`.
- `src/tools/hybrid/m68k_00baac_instruction_step.hpp/.cpp` — exact ROM-checked, 9-caller-gated instruction-stepped helper for Entity Bounding Box Collision Check against Player (`0x00BAAC..0x00BBA8`), verified in dual canonical 3,600-frame replay (Run A: 428 ROM calls; Run B: 428 native calls, 0 fallbacks, 25,259 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00baac_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM rejection, unsupported caller rejection, hit player path, miss player path, and inactive player path.
- `data/thor-evidence/00baac-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00BAAC` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00f91c_step_ops.hpp/.cpp` — instruction-stepping primitives, stack unwinding (`ADDQ.L #4, SP` without CCR alteration), RTS outer caller return, and metrics tracking under `namespace oasis::hybrid::step_00f91c`.
- `src/tools/hybrid/m68k_00f91c_instruction_step.hpp/.cpp` — exact ROM-checked instruction-stepped helper for Cutscene/Event Script Sequencer Command 00 — Script Terminate / Stack-Unwind Return (`0x00F91C..0x00F920`, caller `0x00F7C6`), verified in dual canonical 3,600-frame replay (Run A: 630 ROM calls; Run B: 632 native calls, 0 fallbacks, 1,264 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00f91c_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM rejection, unsupported caller rejection, outer return 0x008B2E path, outer return 0x00CD06 path, and bad outer return PC rejection.
- `data/thor-evidence/00f91c-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00F91C` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0107aa_step_ops.hpp/.cpp` — instruction-stepping primitives, Z-height comparisons, cardinal jump-table dispatch, bounding box calculations, and CCR Carry set/clear operations under `namespace oasis::hybrid::step_0107aa`.
- `src/tools/hybrid/m68k_0107aa_instruction_step.hpp/.cpp` — exact ROM-checked, 8-caller-gated instruction-stepped helper for Spatial Detection Sector Hitbox & Range Scanner (`0x0107AA..0x010916`), verified in dual canonical 3,600-frame replay (Run A: 337 ROM calls; Run B: 337 native calls, 0 fallbacks, 5,772 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0107aa_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM rejection, unsupported caller rejection, Z reject path, Sector 0..3 hit and miss paths, and exact CCR preservation/modification.
- `data/thor-evidence/0107aa-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0107AA` displacement across all 8 canonical streams.
- `scratch/run_ab_0107aa.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x0107AA`.
- `src/tools/hybrid/m68k_00e192_step_ops.hpp/.cpp` — instruction-stepping primitives, bounce math, weapon slot pointer logic, flicker manipulation, and cycle-accurate condition evaluation under `namespace oasis::hybrid::step_00e192`.
- `src/tools/hybrid/m68k_00e192_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Secondary State Transition & Animation Sequencer (`0x00E192..0x00E20A`, loop caller `0x00E130` and indirect dispatcher `0x00DF1A`), verified in dual canonical 3,600-frame replay (Run A: 404 ROM calls; Run B: 468 native calls, 0 fallbacks, 5,543 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e192_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM rejection, unsupported caller rejection, normal bounce active, bounce clamp to zero, weapon slot despawn, flicker phase, and timeout despawn paths.
- `data/thor-evidence/00e192-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E192` displacement across all 8 canonical streams.
- `scratch/run_ab_00e192.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x00E192`.
- `src/tools/hybrid/m68k_00e502_step_ops.hpp/.cpp` — instruction-stepping primitives, coordinate math, velocity integration, sector classification, and MOVEM push/pop cycle adjustments under `namespace oasis::hybrid::step_00e502`.
- `src/tools/hybrid/m68k_00e502_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Coordinate Integration & Physics Position Step (`0x00E502..0x00E5AE`, callers `0x01B1E8` and `0x01A95A`), verified in dual canonical 3,600-frame replay (Run A: 329 ROM calls; Run B: 329 native calls, 0 fallbacks, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e502_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM rejection, unsupported caller rejection, coordinate advancement, and MOVEM register preservation.
- `data/thor-evidence/00e502-replay-differential-20261018.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E502` displacement across all 8 canonical streams.
- `scratch/run_ab_00e502.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x00E502`.
- `src/tools/hybrid/m68k_00e5ae_step_ops.hpp/.cpp` — instruction-stepping primitives, camera coordinate loading, bit testing, signed screen projection math, margin bounding box calculations, and CCR Carry flag manipulation under `namespace oasis::hybrid::step_00e5ae`.
- `src/tools/hybrid/m68k_00e5ae_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for Entity 3D Camera Viewport Frustum Culling & Visibility Test (`0x00E5AE..0x00E610`, callers `0x00EC02`, `0x01C1E4`, `0x01C5F0`), verified in dual canonical 3,600-frame replay (Run A: 215 ROM calls; Run B: 215 native calls, 0 fallbacks, 4,007 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e5ae_instruction_step_test.cpp` — unit test suite verifying Plane 0 and Plane 1 camera selection, in-frustum visible return (Carry=0), left/top/right/bottom frustum culling (Carry=1), disabled mode, and corrupt ROM rejection.
- `data/thor-evidence/00e5ae-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E5AE` displacement across all 8 canonical streams.
- `scratch/run_ab_00e5ae.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x00E5AE`.
- `src/tools/hybrid/m68k_0096ba_step_ops.hpp/.cpp` — instruction-stepping primitives, coordinate math, tilemap address calculation, shift timing, and horizontal span collision accumulation loop under `namespace oasis::hybrid::step_0096ba`.
- `src/tools/hybrid/m68k_0096ba_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Terrain Tilemap Collision Span Evaluator (`0x0096BA..0x0096E8`, callers `0x0094F2` and `0x009680`), verified in dual canonical 3,600-frame replay (Run A: 226 ROM calls; Run B: 226 native calls, 0 fallbacks, 6,940 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0096ba_instruction_step_test.cpp` — unit test suite verifying multi-tile loop accumulation, 1-tile loop, boundary coordinates, corrupt ROM rejection, and disabled mode.
- `data/thor-evidence/0096ba-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0096BA` displacement across all 8 canonical streams.
- `scratch/run_ab_0096ba.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x0096BA`.
- `src/tools/hybrid/m68k_0101ec_step_ops.hpp/.cpp` — instruction-stepping primitives, entity table scanning, active state predicate evaluation, and DBF loop timing under `namespace oasis::hybrid::step_0101ec`.
- `src/tools/hybrid/m68k_0101ec_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Active Entity Slot Counter (`0x0101EC..0x010214`, callers `0x00F9A6`, `0x00FCCE`, `0x00FD8C`, `0x017988`), verified in dual canonical 3,600-frame replay (Run A: 194 ROM calls; Run B: 194 native calls, 0 fallbacks, 16,557 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0101ec_instruction_step_test.cpp` — unit test suite verifying all 17 slots, zero active slots, flag 58 rejection, DBF paths, upper-word preservation, corrupt ROM rejection, and disabled mode.
- `data/thor-evidence/0101ec-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0101EC` displacement across all 8 canonical streams.
- `scratch/run_ab_0101ec.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x0101EC`.
- `src/tools/hybrid/m68k_0094d2_step_ops.hpp/.cpp` — instruction-stepping primitives, 4-register MOVEM push/pop, child BSR invocation, elevation delta comparison, Work RAM height table lookup, and CCR Carry manipulation under `namespace oasis::hybrid::step_0094d2`.
- `src/tools/hybrid/m68k_0094d2_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Terrain Collision Coordinate Normalizer (`0x0094D2..0x0095AA`, caller `0x0092F0`), verified in dual canonical 3,600-frame replay (Run A: 194 ROM calls; Run B: 194 native calls, 0 fallbacks, 4,958 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0094d2_instruction_step_test.cpp` — unit test suite verifying disabled gate, same level free move, wall block (delta >= 2), elevation delta table lookup (delta <= -2), corrupt ROM rejection, and invalid child return handling.
- `data/thor-evidence/0094d2-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0094D2` displacement across all 8 canonical streams.
- `scratch/run_ab_0094d2.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x0094D2`.
- `src/tools/hybrid/m68k_010a74_step_ops.hpp/.cpp` — instruction-stepping primitives, 4-way PC-relative jump table dispatch, Z-elevation comparison, 2D lateral span and reach test, and CCR Carry flag manipulation under `namespace oasis::hybrid::step_010a74`.
- `src/tools/hybrid/m68k_010a74_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Line-of-Sight Range Test (`0x010A74..0x010BF0`, callers `0x01BC36`, `0x01BCB4`), verified in dual canonical 3,600-frame replay (Run A: 193 ROM calls; Run B: 193 native calls, 0 fallbacks, 965 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_010a74_instruction_step_test.cpp` — unit test suite verifying disabled mode, caller validation, player inactive miss, and hit paths for UP and DOWN cardinal directions.
- `data/thor-evidence/010a74-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x010A74` displacement across all 8 canonical streams.
- `scratch/run_ab_010a74.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x010A74`.
- `src/tools/hybrid/m68k_01c1d6_step_ops.hpp/.cpp` — instruction-stepping primitives, bit test, child JSR calls, 160 px outer radius / 64 px inner deadzone threshold tests, child slot allocation, and CCR Carry manipulation under `namespace oasis::hybrid::step_01c1d6`.
- `src/tools/hybrid/m68k_01c1d6_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for Entity Annulus Proximity Detection & Child Spawn Trigger (`0x01C1D6..0x01C23E`, callers `0x01BF1A`, `0x01BFCC`, `0x01C042`), verified in dual canonical 3,600-frame replay (Run A: 191 ROM calls; Run B: 191 native calls, 0 fallbacks, 1,552 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01c1d6_instruction_step_test.cpp` — unit test suite verifying disabled mode, caller gating, bit 3 set miss, frustum culling, outer range miss, inner deadzone miss, hit path, and corrupt ROM rejection.
- `data/thor-evidence/01c1d6-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01C1D6` displacement across all 8 canonical streams.
- `scratch/run_ab_01c1d6.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x01C1D6`.
- `src/tools/hybrid/m68k_00e144_step_ops.hpp/.cpp` — instruction-stepping primitives, indexed LEA calculation from PC-relative table base 0x00E140, signed word table addition, and tail JMP (An) dispatch under `namespace oasis::hybrid::step_00e144`.
- `src/tools/hybrid/m68k_00e144_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Movement Sub-Mode Dispatcher Stub (`0x00E144..0x00E150`, caller `0x00DF1A`), verified in dual canonical 3,600-frame replay (Run A: 106 ROM calls; Run B: 106 native calls, 0 fallbacks, 424 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e144_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM, invalid caller gating, dispatch index 0, dispatch index 2, stack preservation, and metrics reporting.
- `data/thor-evidence/00e144-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E144` displacement across all 8 canonical streams.
- `scratch/run_ab_00e144.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x00E144`.
- `src/tools/hybrid/m68k_0083d4_step_ops.hpp/.cpp` — instruction-stepping primitives, displacement read from A6, byte subtraction, short branch, word branch, moveq, and RTS under `namespace oasis::hybrid::step_0083d4`.
- `src/tools/hybrid/m68k_0083d4_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for Entity 4-Way Compass Direction Code Normalization Helper (`0x0083D4..0x0083F8`, callers `0x005B70`, `0x006346`, `0x006524`), verified in dual canonical 3,600-frame replay (Run A: 94 ROM calls; Run B: 94 native calls, 0 fallbacks, 476 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0083d4_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM, invalid caller gating, direction mappings 0->0, 1->3, 2->1, 3->2, and caller attribution.
- `data/thor-evidence/0083d4-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0083D4` displacement across all 8 canonical streams.
- `scratch/run_ab_0083d4.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x0083D4`.
- `src/tools/hybrid/m68k_01a244_step_ops.hpp/.cpp` — instruction-stepping primitives, bit test on displacement from A6, conditional short branch, word displacement write from D0 and immediate to A6, immediate word addition to D0, and RTS under `namespace oasis::hybrid::step_01a244`.
- `src/tools/hybrid/m68k_01a244_instruction_step.hpp/.cpp` — exact ROM-checked, 6-caller-gated instruction-stepped helper for Entity Animation Facing Direction Slot & Frame Offset Dispatcher (`0x01A244..0x01A288`, callers `0x01A4E2`, `0x01A51C`, `0x01A5D2`, `0x01A606`, `0x01A96E`, `0x01A9DE`), verified in dual canonical 3,600-frame replay (Run A: 90 ROM calls; Run B: 90 native calls, 0 fallbacks, 717 stepped instructions, 100% path coverage, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01a244_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM, invalid caller gating, all 4 orientation paths, and all 6 caller attributions.
- `data/thor-evidence/01a244-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01A244` displacement across all 8 canonical streams.
- `scratch/run_ab_01a244.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x01A244`.
- `src/tools/hybrid/m68k_010660_step_ops.hpp/.cpp` — instruction-stepping primitives, word register test, word conditional branch, immediate address load to A0, immediate long shift (112 master cycles), unsigned word division (with Musashi `calc_divu_cycles` and `skip_bus_refresh`), word register clear, post-increment word compare, word unsigned branch lower-or-same, immediate word addition to D0, short unconditional branch, and RTS under `namespace oasis::hybrid::step_010660`.
- `src/tools/hybrid/m68k_010660_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Vector Angle & Fixed-Point Tangent Octant Boundary Search Routine (`0x010660..0x010684`, parent caller `0x010594` via callsites `0x0105B8`, `0x0105DA`, `0x0105AE`, `0x0105CA`), verified in dual canonical 3,600-frame replay (Run A: 68 ROM calls; Run B: 68 native calls, 0 fallbacks, 2,124 stepped instructions, 446 table loop iterations, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_010660_instruction_step_test.cpp` — unit test suite verifying disabled mode, corrupt ROM, invalid caller gating, all 4 callers, D0=0 early exit, all 9 table buckets, exact 24 captured canonical call vectors, and DIVU cycle calculation (16 test cases).
- `data/thor-evidence/010660-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x010660` displacement across all 8 canonical streams.
- `scratch/run_ab_010660.py` — automated dual-run A/B replay harness verifying 3,600-frame state equivalence and generating verification receipts for `0x010660`.
- `src/tools/hybrid/m68k_010594_step_ops.hpp/.cpp` & `m68k_010594_step_math.cpp` — instruction-stepping primitives, coordinate difference calculations, quadrant sign evaluation, child BSR call invocation, angle quadrant transformation, ROM sine/cosine table lookup, and speed vector multiplications under `namespace oasis::hybrid::step_010594`.
- `src/tools/hybrid/m68k_010594_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for Vector Angle & Velocity Component Calculator (`0x010594..0x010660`, callers `0x01B9A2`, `0x01BD04`, `0x01C090`), verified in dual canonical 3,600-frame replay (Run A: 68 ROM calls; Run B: 68 native calls, 0 fallbacks, 3,691 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_010594_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, re-entry protection, all 4 quadrants, child calls to `0x010660`, and memory write assertions (11 test cases).
- `data/thor-evidence/010594-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x010594` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00e150_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Airborne Gravity & Landing Handler (`0x00E150..0x00E192`, callers `0x00E130` and `0x00DF1A` via tail dispatcher `0x00E144`), verified in dual canonical 3,600-frame replay (Run A: 63 ROM calls; Run B: 105 native entries, 0 fallbacks, 1,041 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e150_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, airborne path, ground landing clamp path, and sub-mode transitions (7 test cases).
- `data/thor-evidence/00e150-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E150` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00e25c_step_ops.hpp/.cpp` — instruction-stepping primitives, word subtraction with immediate 1 on entity lifetime counter, conditional branch, and entity slot clear under `namespace oasis::hybrid::step_00e25c`.
- `src/tools/hybrid/m68k_00e25c_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Lifecycle Timer Decrement & Despawn Handler (`0x00E25C..0x00E268`, caller `0x00DF1A`), verified in dual canonical 3,600-frame replay (Run A: 40 ROM calls; Run B: 40 native calls, 0 fallbacks, 120 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e25c_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, decrement non-zero branch path, and zero despawn path (5 test cases).
- `data/thor-evidence/00e25c-replay-differential-20261019.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E25C` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0094a2_step_ops.hpp/.cpp` — instruction-stepping primitives, coordinate word subtraction, bitwise AND mask, table pointer indexing, tile height comparison, and RTS under `namespace oasis::hybrid::step_0094a2`.
- `src/tools/hybrid/m68k_0094a2_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Vertical Terrain Span Evaluator (`0x0094A2..0x0094D2`, caller `0x00947A`), verified in dual canonical 3,600-frame replay (Run A: 66 ROM calls; Run B: 66 native calls, 0 fallbacks, 792 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0094a2_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, span calculation paths, and bounds checks (6 test cases).
- `data/thor-evidence/0094a2-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0094A2` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_009460_step_ops.hpp/.cpp` — instruction-stepping primitives, elevation scan loop, child BSR call invocation to native `0x0094A2`, step-delta accumulation, and RTS under `namespace oasis::hybrid::step_009460`.
- `src/tools/hybrid/m68k_009460_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Surface Elevation Scanner / Vertical Raycast (`0x009460..0x0094A2`, caller `0x0093A6`), verified in dual canonical 3,600-frame replay (Run A: 66 ROM calls; Run B: 66 native calls, 0 fallbacks, 528 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_009460_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, child invocation to native 0x0094A2, and scan loop bounds (7 test cases).
- `data/thor-evidence/009460-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x009460` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00938e_step_ops.hpp/.cpp` — instruction-stepping primitives, MOVEM push/pop register bus timing, word branch not-taken adjustment, elevation delta comparison, collision obstacle flag bit setting, and RTS under `namespace oasis::hybrid::step_00938e`.
- `src/tools/hybrid/m68k_00938e_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Terrain Bounding Box Evaluator (`0x00938E..0x009460`, callers `0x0090C6` and `0x009168`), verified in dual canonical 3,600-frame replay (Run A: 66 ROM calls; Run B: 66 native calls, 0 fallbacks, 1,384 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00938e_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, obstacle detection paths, carry flag contract, and cycle timing (10 test cases).
- `data/thor-evidence/00938e-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00938E` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0099d6_step_ops.hpp/.cpp` — instruction-stepping primitives, coordinate tile division, pitch shift exponent multiplication, linear tile offset computation, tile byte read from A1, and RTS under `namespace oasis::hybrid::step_0099d6`.
- `src/tools/hybrid/m68k_0099d6_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Map Tile Fetcher & Grid Coordinate Evaluator (`0x0099D6..0x0099F0`, caller `0x009932`), verified in dual canonical 3,600-frame replay (Run A: 30 ROM calls; Run B: 30 native calls, 0 fallbacks, 300 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0099d6_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, tile index calculation, and byte fetch (5 test cases).
- `data/thor-evidence/0099d6-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0099D6` displacement across all 8 canonical streams.

- `src/tools/hybrid/m68k_009678_step_ops.hpp/.cpp` — instruction-stepping primitives, half-width/depth probe calculations, child BSR call invocation to native `0x0096BA`, elevation obstruction normalization, and RTS under `namespace oasis::hybrid::step_009678`.
- `src/tools/hybrid/m68k_009678_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Surface Raycast Scanner (`0x009678..0x0096BA`, caller `0x0095C8`), verified in dual canonical 3,600-frame replay (Run A: 32 ROM calls; Run B: 32 native calls, 0 fallbacks, 416 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_009678_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, probe bounds, and child call to 0x0096BA (7 test cases).
- `data/thor-evidence/009678-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x009678` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0095aa_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Diagonal Terrain Bounding Box Evaluator (`0x0095AA..0x009678`, callers `0x009540` and `0x00956E`), verified in dual canonical 3,600-frame replay (Run A: 32 ROM calls; Run B: 32 native calls, 0 fallbacks, 1,328 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0095aa_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, diagonal collision branches, obstacle flags, and child call to 0x009678 (8 test cases).
- `data/thor-evidence/0095aa-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0095AA` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_009ad0_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Terrain Height Interpolation Alternate Entry (`0x009AD0..0x009AD6`, caller `0x0098FA`), verified in dual canonical 3,600-frame replay (Run A: 30 ROM calls; Run B: 30 native calls, 0 fallbacks, 60 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_009ad0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, coordinate loading, and fallthrough to 0x009AD6 (5 test cases).
- `data/thor-evidence/009ad0-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x009AD0` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_009d00_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Tile Attribute / Height Descriptor Query (`0x009D00..0x009D4E`, multiple callers), verified in dual canonical 3,600-frame replay (Run A: 42 ROM calls; Run B: 42 native calls, 0 fallbacks, 714 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_009d00_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, MOVEM push/pop framing, descriptor selection, and child calls to 0x009BF2 and 0x009AD6 (6 test cases).
- `data/thor-evidence/009d00-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x009D00` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0103b0_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Offset / Coordinate Setter Leaf (`0x0103B0..0x0103E0`, callers `0x010260` and `0x010372`), verified in dual canonical 3,600-frame replay (Run A: 11 ROM calls; Run B: 11 native calls, 0 fallbacks, 132 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0103b0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and offset calculations (5 test cases).
- `data/thor-evidence/0103b0-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0103B0` displacement across all 8 canonical streams (receipt #86).
- `src/tools/hybrid/m68k_010382_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity ID Proximity Search Leaf (`0x010382..0x0103B0`, callers `0x01034A` and `0x014F96`), verified in dual canonical 3,600-frame replay (Run A: 10 ROM calls; Run B: 10 native calls, 0 fallbacks, 350 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_010382_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, pool scanning, and match conditions (6 test cases).
- `data/thor-evidence/010382-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x010382` displacement across all 8 canonical streams (receipt #87).
- `src/tools/hybrid/m68k_00e398_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity-to-Player Vector / Quadrant Direction Leaf (`0x00E398..0x00E3E6`, callers `0x01A2D0` and `0x01B91C`), verified in dual canonical 3,600-frame replay (Run A: 20 ROM calls; Run B: 20 native calls, 0 fallbacks, 359 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e398_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, delta vector math, and 4-quadrant cardinal classification (8 test cases).
- `data/thor-evidence/00e398-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E398` displacement across all 8 canonical streams (receipt #88).
- `src/tools/hybrid/m68k_01ad14_instruction_step.hpp/.cpp` — exact ROM-checked, 8-caller-gated instruction-stepped helper for Entity Path / Trajectory Data Loader Leaf (`0x01AD14..0x01AD38`, 8 callers), verified in dual canonical 3,600-frame replay (Run A: 11 ROM calls; Run B: 11 native calls, 0 fallbacks, 99 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01ad14_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, ROM table offsets, shift cycle timing, and trajectory loading (5 test cases).
- `data/thor-evidence/01ad14-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01AD14` displacement across all 8 canonical streams (receipt #89).
- `src/tools/hybrid/m68k_00e2d4_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for 8-Direction Velocity / Delta-Position Transform Leaf (`0x00E2D4..0x00E338`, callers `0x00C722` and `0x00DB7A`), verified in dual canonical 3,600-frame replay (Run A: 37 ROM calls; Run B: 37 native calls, 0 fallbacks, 625 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e2d4_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, 8-direction velocity decomposition, and speed scaling (7 test cases).
- `data/thor-evidence/00e2d4-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E2D4` displacement across all 8 canonical streams (receipt #90).
- `src/tools/hybrid/m68k_06231e_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Parameter Scaler Leaf (`0x06231E..0x06232C`, callers `0x062092` and `0x062300`), verified in dual canonical 3,600-frame replay (Run A: 40 ROM calls; Run B: 40 native calls, 0 fallbacks, 120 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_06231e_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, threshold branching, signed multiplication, and arithmetic shift (6 test cases).
- `data/thor-evidence/06231e-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x06231E` displacement across all 8 canonical streams (receipt #91).
- `src/tools/hybrid/m68k_0622d0_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Animation Attribute Loader Leaf (`0x0622D0..0x0622DC`, caller `0x0622A8`), verified in dual canonical 3,600-frame replay (Run A: 30 ROM calls; Run B: 30 native calls, 0 fallbacks, 120 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0622d0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, stream fetching, and dual entity field stores (4 test cases).
- `data/thor-evidence/0622d0-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0622D0` displacement across all 8 canonical streams (receipt #92).
- `src/tools/hybrid/m68k_0622dc_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Animation Auxiliary Attribute Loader Leaf (`0x0622DC..0x0622E8`, caller `0x0622AC`), verified in dual canonical 3,600-frame replay (Run A: 30 ROM calls; Run B: 30 native calls, 0 fallbacks, 120 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0622dc_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, stream fetching, and dual auxiliary entity field stores (4 test cases).
- `data/thor-evidence/0622dc-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0622DC` displacement across all 8 canonical streams (receipt #93).
- `src/tools/hybrid/m68k_06230e_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Animation Step Parameter Scaler Leaf (`0x06230E..0x06231E`, caller `0x0622B4`), verified in dual canonical 3,600-frame replay (Run A: 30 ROM calls; Run B: 30 native calls, 0 fallbacks, 180 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_06230e_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, zero-extension, shift timing, and dual parameter stores (4 test cases).
- `data/thor-evidence/06230e-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x06230E` displacement across all 8 canonical streams (receipt #94).
- `src/tools/hybrid/m68k_0622f4_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Animation Little-Endian Word Parameter Reader & Scaler (`0x0622F4..0x06230E`, callers `0x0622B0` and `0x0622E8`), verified in dual canonical 3,600-frame replay (Run A: 30 ROM calls; Run B: 30 native calls, 0 fallbacks, 270 stepped instructions, 30 child native calls to `0x06231E`, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0622f4_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, little-endian word assembly, child native call integration, and scaled parameter stores (4 test cases).
- `data/thor-evidence/0622f4-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0622F4` displacement across all 8 canonical streams (receipt #95).
- `src/tools/hybrid/m68k_00129a_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for SRAM Header Signature & Checksum Validator (`0x00129A..0x0012BC`), verified in dual canonical 3,600-frame replay (Run A: 16 ROM calls; Run B: 16 native calls, 0 fallbacks, 144 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00129a_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, magic bytes validation, and checksum calculation (4 test cases).
- `data/thor-evidence/00129a-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00129A` displacement across all 8 canonical streams (receipt #96).
- `src/tools/hybrid/m68k_0005ec_instruction_step.hpp/.cpp` — exact ROM-checked, 5-caller-gated instruction-stepped helper for Decompressed Graphics Asset Buffer Alignment Leaf (`0x0005EC..0x0005F8`), verified in dual canonical 3,600-frame replay (Run A: 10 ROM calls; Run B: 10 native calls, 0 fallbacks, 50 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0005ec_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and pointer alignment calculation (4 test cases).
- `data/thor-evidence/0005ec-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0005EC` displacement across all 8 canonical streams (receipt #97).
- `src/tools/hybrid/m68k_00d9be_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Dynamic Slot Allocation Table Scanner (`0x00D9BE..0x00D9F0`), verified in dual canonical 3,600-frame replay (Run A: 9 ROM calls; Run B: 9 native calls, 0 fallbacks, 96 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00d9be_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, table traversal, and slot indexing (4 test cases).
- `data/thor-evidence/00d9be-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00D9BE` displacement across all 8 canonical streams (receipt #98).
- `src/tools/hybrid/m68k_008ef8_instruction_step.hpp/.cpp` — exact ROM-checked, 14-caller-gated instruction-stepped helper for Entity Animation Action State Reset Leaf (`0x008EF8..0x008F10`), verified in dual canonical 3,600-frame replay (Run A: 14 ROM calls; Run B: 14 native calls, 0 fallbacks, 84 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_008ef8_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, direct call and tail-jump entries, and field stores (4 test cases).
- `data/thor-evidence/008ef8-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x008EF8` displacement across all 8 canonical streams (receipt #99).
- `src/tools/hybrid/m68k_008cac_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for Entity Record Block Clearer (`0x008CAC..0x008CCE`), verified in dual canonical 3,600-frame replay (Run A: 9 ROM calls; Run B: 9 native calls, 0 fallbacks, 441 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_008cac_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, bounded stride loop, and record field initialization (4 test cases).
- `data/thor-evidence/008cac-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x008CAC` displacement across all 8 canonical streams (receipt #100).
- `src/tools/hybrid/m68k_008cd0_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Secondary Record Clearer Leaf (`0x008CD0..0x008CDA`), verified in dual canonical 3,600-frame replay (Run A: 3 ROM calls; Run B: 3 native calls, 0 fallbacks, 39 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_008cd0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, stride stepping, and record clearing (4 test cases).
- `data/thor-evidence/008cd0-replay-differential-20261020.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x008CD0` displacement across all 8 canonical streams (receipt #101).
- `src/tools/hybrid/m68k_00d7b0_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Camera / Map Chunk Stride Bit-Shift Exponent Calculator (`0x00D7B0..0x00D7C0`), verified in dual canonical 3,600-frame replay (Run A: 6 ROM calls; Run B: 6 native calls, 0 fallbacks, 114 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00d7b0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, bit-shift loop execution, and shift cycle timing parity (3 test cases).
- `data/thor-evidence/00d7b0-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00D7B0` displacement across all 8 canonical streams (receipt #102).
- `src/tools/hybrid/m68k_0101e4_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Active Entity Header Buffer Pointer Getter (`0x0101E4..0x0101EC`), verified in dual canonical 3,600-frame replay (Run A: 2 ROM calls; Run B: 2 native calls, 0 fallbacks, 4 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0101e4_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and buffer address loading (3 test cases).
- `data/thor-evidence/0101e4-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0101E4` displacement across all 8 canonical streams (receipt #103).
- `src/tools/hybrid/m68k_007d8a_instruction_step.hpp/.cpp` — exact ROM-checked, 5-caller-gated instruction-stepped helper for Controller Input History Buffer Bitmask Accumulator (`0x007D8A..0x007DC8`), verified in dual canonical 3,600-frame replay (Run A: 8 ROM calls; Run B: 8 native calls, 0 fallbacks, 376 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_007d8a_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, buffer traversal, wrapping, and bitmask accumulation (4 test cases).
- `data/thor-evidence/007d8a-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x007D8A` displacement across all 8 canonical streams (receipt #104).
- `src/tools/hybrid/m68k_0085e2_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Directional Velocity and Facing Resolver (`0x0085E2..0x0086B4`), verified in dual canonical 3,600-frame replay (Run A: 283 ROM calls; Run B: 283 native calls, 0 fallbacks, 3,254 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0085e2_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and vector calculation.
- `data/thor-evidence/0085e2-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0085E2` displacement across all 8 canonical streams (receipt #105).
- `src/tools/hybrid/m68k_0604bc_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Sound Effect Reset & Sub-Track Muter (`0x0604BC..0x0604E6`), verified in dual canonical 3,600-frame replay (Run A: 10 ROM calls; Run B: 10 native calls, 0 fallbacks, 100 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0604bc_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and audio RAM clears.
- `data/thor-evidence/0604bc-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0604BC` displacement across all 8 canonical streams (receipt #106).
- `src/tools/hybrid/m68k_01a288_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Facing / Animation Sub-State Lookup (`0x01A288..0x01A29C`), verified in dual canonical 3,600-frame replay (Run A: 5 ROM calls; Run B: 5 native calls, 0 fallbacks, 30 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01a288_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and sub-state table lookup.
- `data/thor-evidence/01a288-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01A288` displacement across all 8 canonical streams (receipt #107).
- `src/tools/hybrid/m68k_0062cc_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Actor Motion State and Timer Clearer (`0x0062CC..0x0062E4`), reclassified as PARTIAL_NATIVE_INTERCEPT and disabled from the active native helpers baseline due to 145 tail-fallthrough entries from ~30 callers.
- `tests/m68k_0062cc_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and timer field clearing.
- `data/thor-evidence/0062cc-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0062CC` displacement across all 8 canonical streams (receipt #108).
- `src/tools/hybrid/m68k_0101d0_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Secondary Entity Header Pointer Evaluator (`0x0101D0..0x0101E4`), verified in dual canonical 3,600-frame replay (Run A: 2 ROM calls; Run B: 2 native calls, 0 fallbacks, 16 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0101d0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and address pointer evaluation.
- `data/thor-evidence/0101d0-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0101D0` displacement across all 8 canonical streams (receipt #109).
- `src/tools/hybrid/m68k_01b024_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Actor Animation Sub-State Dispatcher D (`0x01B024..0x01B046`), verified in dual canonical 3,600-frame replay (Run A: 4 ROM calls; Run B: 4 native calls, 0 fallbacks, 44 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01b024_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and actor animation sub-state dispatching.
- `data/thor-evidence/01b024-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01B024` displacement across all 8 canonical streams (receipt #110).
- `src/tools/hybrid/m68k_0085c4_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Player Extended Motion Flag Setter (`0x0085C4..0x0085E2`), verified in dual canonical 3,600-frame replay (Run A: 1 ROM call; Run B: 1 native call, 0 fallbacks, 6 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0085c4_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, threshold comparison, and motion flag mutation.
- `data/thor-evidence/0085c4-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0085C4` displacement across all 8 canonical streams (receipt #111).
- `src/tools/hybrid/m68k_01a21e_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Actor Animation Sub-State Dispatcher A (`0x01A21E..0x01A23C`), verified in dual canonical 3,600-frame replay (Run A: 1 ROM call; Run B: 1 native call, 0 fallbacks, 8 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01a21e_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and sub-state table lookup.
- `data/thor-evidence/01a21e-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01A21E` displacement across all 8 canonical streams (receipt #112).
- `src/tools/hybrid/m68k_01ae7e_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Actor Animation Sub-State Dispatcher B (`0x01AE7E..0x01AE9C`), verified in dual canonical 3,600-frame replay (Run A: 1 ROM call; Run B: 1 native call, 0 fallbacks, 8 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01ae7e_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and sub-state table lookup.
- `data/thor-evidence/01ae7e-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01AE7E` displacement across all 8 canonical streams (receipt #113).
- `src/tools/hybrid/m68k_01af3a_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Actor Animation Sub-State Dispatcher C (`0x01AF3A..0x01AF58`), verified in dual canonical 3,600-frame replay (Run A: 1 ROM call; Run B: 1 native call, 0 fallbacks, 8 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01af3a_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and sub-state table lookup.
- `data/thor-evidence/01af3a-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01AF3A` displacement across all 8 canonical streams (receipt #114).

- `src/tools/hybrid/m68k_00c90e_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Actor Parameter Table Lookup and Displacement Resolver (`0x00C90E..0x00C924`), verified in dual canonical 3,600-frame replay (Run A: 3 ROM calls; Run B: 3 native calls, 0 fallbacks, 33 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00c90e_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and actor parameter table lookup.
- `data/thor-evidence/00c90e-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00C90E` displacement across all 8 canonical streams (receipt #115).
- `src/tools/hybrid/m68k_00cecc_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Actor Collision / Boundary Coordinate Delta Resolver (`0x00CECC..0x00CEFA`), verified in dual canonical 3,600-frame replay (Run A: 3 ROM calls; Run B: 3 native calls, 0 fallbacks, 210 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00cecc_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and actor bounding box coordinate calculation.
- `data/thor-evidence/00cecc-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00CECC` displacement across all 8 canonical streams (receipt #116).
- `src/tools/hybrid/m68k_010086_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity State Reset and Attribute Register Clear (`0x010086..0x0100AC`), verified in dual canonical 3,600-frame replay (Run A: 2 ROM calls; Run B: 2 native calls, 0 fallbacks, 50 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_010086_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and entity struct clearing.
- `data/thor-evidence/010086-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x010086` displacement across all 8 canonical streams (receipt #117).
- `src/tools/hybrid/m68k_00f920_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Coordinate Range Clamping & Direction Evaluator (`0x00F920..0x00F94C`), verified in dual canonical 3,600-frame replay (Run A: 2 ROM calls; Run B: 2 native calls, 0 fallbacks, 86 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00f920_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, coordinate delta calculation, and compass direction determination.
- `data/thor-evidence/00f920-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00F920` displacement across all 8 canonical streams (receipt #118).
- `src/tools/hybrid/m68k_002d66_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for VDP Sprite Attribute Buffer Pack and Transfer Subroutine (`0x002D66..0x002D88`), verified in dual canonical 3,600-frame replay (Run A: 6 ROM calls; Run B: 6 native calls, 0 fallbacks, 458 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_002d66_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and sprite attribute block copying.
- `data/thor-evidence/002d66-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x002D66` displacement across all 8 canonical streams (receipt #119).
- `src/tools/hybrid/m68k_061032_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Level Transition Entity Parser (`0x061032..0x06108C`), verified in dual canonical 3,600-frame replay (Run A: 27 ROM calls; Run B: 27 native calls, 0 fallbacks, 1,215 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_061032_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and transition entity record parsing.
- `data/thor-evidence/061032-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x061032` displacement across all 8 canonical streams (receipt #120).
- `src/tools/hybrid/m68k_0086b4_instruction_step.hpp/.cpp`, `src/tools/hybrid/m68k_0086b4_step_ops.hpp/.cpp` — exact ROM-checked, 4-caller-gated modular instruction-stepped helper for Player Direction Vector & Angle Range Mapper (`0x0086B4..0x008778`), verified in dual canonical 3,600-frame replay (Run A: 8 ROM calls; Run B: 8 native calls, 0 fallbacks, 1,926 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0086b4_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and player vector/angle mapping.
- `data/thor-evidence/0086b4-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0086B4` displacement across all 8 canonical streams (receipt #121).
- `src/tools/hybrid/m68k_00f740_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Lifecycle Flag Evaluator (`0x00F740..0x00F756`), verified in dual canonical 3,600-frame replay (Run A: 7 ROM calls; Run B: 7 native calls, 0 fallbacks, 71 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00f740_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and lifecycle flag condition code evaluations.
- `data/thor-evidence/00f740-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00F740` displacement across all 8 canonical streams (receipt #122).
- `src/tools/hybrid/m68k_008950_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Player Animation Motion Buffer Matcher (`0x008950..0x00898E`), verified in dual canonical 3,600-frame replay (Run A: 10 ROM calls; Run B: 10 native calls, 0 fallbacks, 1,409 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_008950_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and animation buffer matching logic.
- `data/thor-evidence/008950-replay-differential-20261021.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x008950` displacement across all 8 canonical streams (receipt #123).
- `src/tools/hybrid/m68k_00c012_instruction_step.hpp/.cpp`, `src/tools/hybrid/m68k_00c012_step_ops.hpp/.cpp` — instruction-stepped helper for Collision Box Overlap & Distance Comparator (`0x00C012..0x00C0DE`). Unit tested in `tests/m68k_00c012_instruction_step_test.cpp`. Evaluated in dual 3,600-frame canonical replay; showed instruction timing divergence at frame 2053. Classified as `RESEARCHED_NOT_DISPLACED` / `REJECTED_FULL_DISPLACEMENT_DISABLED`, disabled in runtime island.
- `data/thor-evidence/00c012-replay-differential-20261021.json` — machine-readable verification receipt recording rejection of `0x00C012` full displacement due to timing divergence.
- `src/tools/hybrid/m68k_0610c8_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for FM Sound Channel Track Context Initializer (`0x0610C8..0x06112C`), verified in dual canonical 3,600-frame replay (Run A: 9 ROM calls; Run B: 9 native calls, 0 fallbacks, 243 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0610c8_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and FM channel context initialization logic.
- `data/thor-evidence/0610c8-replay-differential-20261022.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0610C8` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0008a2_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Framebuffer RAM Block Clear Loop (`0x0008A2..0x0008B4`), verified in dual canonical 3,600-frame replay (Run A: 2 ROM calls; Run B: 2 native calls, 0 fallbacks, 3,848 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0008a2_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and RAM clear loop execution.
- `data/thor-evidence/0008a2-replay-differential-20261022.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0008A2` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0008b6_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Screen Region Setup and Dimension Pre-calculator (`0x0008B6..0x0008E8`), verified in dual canonical 3,600-frame replay (Run A: 2 ROM calls; Run B: 2 native calls, 0 fallbacks, 620 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0008b6_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and nested DBRA screen dimension loops.
- `data/thor-evidence/0008b6-replay-differential-20261022.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0008B6` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_008910_instruction_step.hpp/.cpp` - exact ROM-checked, 4-caller-gated instruction-stepped helper for Motion Sequence Buffer Bit-Test (`0x008910..0x008950`), verified in dual canonical 3,600-frame replay (Run A: 17 ROM calls; Run B: 17 native calls, 0 fallbacks, 1,488 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_008910_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, and motion sequence bit-test loop.
- `data/thor-evidence/008910-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x008910` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_061054_instruction_step.hpp/.cpp` - exact ROM-checked, 1-caller-gated instruction-stepped helper for Sound Channel State Reset & Track Initializer (`0x061054..0x0610C8`), verified in dual canonical 3,600-frame replay (Run A: 7 ROM calls; Run B: 7 native calls, 0 fallbacks, 189 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_061054_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, and channel state reset initialization.
- `data/thor-evidence/061054-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x061054` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_06121a_instruction_step.hpp/.cpp` - exact ROM-checked, 2-caller-gated instruction-stepped helper for PSG 4-Channel Mute & Attenuation Setup (`0x06121A..0x061232`), verified in dual canonical 3,600-frame replay (Run A: 11 ROM calls; Run B: 11 native calls, 0 fallbacks, 66 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_06121a_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, and PSG port attenuation writes.
- `data/thor-evidence/06121a-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x06121A` displacement across all 8 canonical streams.
- `data/thor-evidence/00e3e8-replay-differential-20261022.json` - machine-readable verification receipt recording rejection of `0x00E3E8` full displacement due to timing divergence on complex EA displacement calculation.
- `src/tools/hybrid/m68k_017ff2_instruction_step.hpp/.cpp` — instruction-stepped helper for Sprite Table Index Adjuster (`0x017FF2..0x018002`). Unit tested in `tests/m68k_017ff2_instruction_step_test.cpp`. Evaluated in dual 3,600-frame canonical replay; showed raster/interrupt timing divergence at frame 2181 (`first_different_frame=2181`, `different_frame_count=937`). Classified as `RESEARCHED_NOT_DISPLACED` / `REJECTED_FULL_DISPLACEMENT_DISABLED`, disabled in runtime island.
- `data/thor-evidence/017ff2-timing-rejection-20261022.json` — machine-readable verification receipt recording rejection of `0x017FF2` full displacement due to timing divergence.
- `data/thor-evidence/aggregate-136-to-139-20261022.json` — aggregate canonical 3,600-frame replay verification receipt advancing baseline from 136 to 139 active native helpers.
- `src/tools/hybrid/m68k_060fb2_instruction_step.hpp/.cpp` - exact ROM-checked, 1-caller-gated instruction-stepped helper for Sound Channel Context Initializer (`0x060FB2..0x061032`), verified in dual canonical 3,600-frame replay (Run A: 90 ROM calls; Run B: 90 native calls, 0 fallbacks, 2,700 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_060fb2_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, and channel context setup.
- `data/thor-evidence/060fb2-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x060FB2` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_002d58_instruction_step.hpp/.cpp` - exact ROM-checked, 1-caller-gated instruction-stepped helper for Screen Transition Flag Setter & Child Invoker (`0x002D58..0x002D66`), verified in dual canonical 3,600-frame replay (Run A: 4 ROM calls; Run B: 4 native calls, 0 fallbacks, 12 stepped instructions, 4 connected native child calls, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_002d58_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, and child invoker execution.
- `data/thor-evidence/002d58-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x002D58` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0063da_instruction_step.hpp/.cpp` - exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Record Dispatch & State Updater (`0x0063DA..0x006464`), verified in dual canonical 3,600-frame replay (Run A: 1 ROM call; Run B: 1 native call, 0 fallbacks, 21 stepped instructions, bit-identical stream hashes across all 3,600 frames). Repaired in M15.187C for opcode prefetch consumption and 4-byte displacement decoding.
- `tests/m68k_0063da_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, entity record dispatch, and un-elidable full instruction stepping assertions.
- `data/thor-evidence/0063da-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0063DA` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00638c_instruction_step.hpp/.cpp` - exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Record Validation & Branch Routine (`0x00638C..0x0063DA`), verified in dual canonical 3,600-frame replay (Run A: 1 ROM call; Run B: 1 native call, 0 fallbacks, 4 stepped instructions along proven path `bne.w 0x63d8 -> rts`, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00638c_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, and branch path execution.
- `data/thor-evidence/00638c-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00638C` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00297c_instruction_step.hpp/.cpp` - exact ROM-checked, 1-caller-gated instruction-stepped helper for Genesis Controller Port Direction Initializer (`0x00297C..0x002992`), verified in dual canonical 3,600-frame replay (Run A: 1 ROM call; Run B: 1 native call, 0 fallbacks, 5 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00297c_instruction_step_test.cpp` - unit test suite verifying caller gating, corrupt ROM rejection, and controller port IO writes.
- `data/thor-evidence/00297c-replay-differential-20261022.json` - machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00297C` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_gameplay_step_metrics.cpp` - helper source separating metrics emission to preserve strict <= 500 lines limit in step set.
- `data/thor-evidence/aggregate-142-to-147-20261022.json` — aggregate canonical 3,600-frame replay verification receipt advancing baseline from 142 to 147 active native helpers.
- `src/tools/hybrid/m68k_gameplay_step_registry.hpp/.cpp` — modular step registration helper separating legacy step slots `0..100` from `m68k_gameplay_step_set.cpp` (slots `101..162`) to keep both files well below the 500-line limit with zero runtime/semantic changes.
- `src/tools/hybrid/m68k_01a2ac_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Player-Proximity & Quadrant Evaluator (`0x01A2AC..0x01A322`, caller `0x01A4CA`), verified in dual canonical 3,600-frame replay (Run A: 10 ROM calls; Run B: 10 native calls, 0 fallbacks, 150 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01a2ac_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, Z-height threshold, and X/Y proximity evaluation.
- `data/thor-evidence/01a2ac-replay-differential-20261023.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01A2AC` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_00e4d0_instruction_step.hpp/.cpp` — exact ROM-checked, 2-caller-gated instruction-stepped helper for Entity Knockback Velocity Initializer (`0x00E4D0..0x00E502`, callers `0x01A5B6` and `0x01B170`), verified in dual canonical 3,600-frame replay (Run A: 7 ROM calls; Run B: 7 native calls, 0 fallbacks, 77 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_00e4d0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, knockback state initialization, and child `0x00E2D4` velocity transform.
- `data/thor-evidence/00e4d0-replay-differential-20261023.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E4D0` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_0102be_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Spawn ID Table Range Scanner — Offset +2 (`0x0102BE..0x010300`, caller `0x00FDA2`), verified in dual canonical 3,600-frame replay (Run A: 5 ROM calls; Run B: 5 native calls, 0 fallbacks, 115 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_0102be_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, active entity table scan at offset `+2`, and carry flag outputs.
- `data/thor-evidence/0102be-replay-differential-20261023.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0102BE` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_01027c_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Spawn ID Table Range Scanner — Offset +16 (`0x01027C..0x0102BE`, caller `0x00FCE4`), verified in dual canonical 3,600-frame replay (Run A: 2 ROM calls; Run B: 2 native calls, 0 fallbacks, 73 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_01027c_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, active entity table scan at offset `+16`, and carry flag outputs.
- `data/thor-evidence/01027c-replay-differential-20261023.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x01027C` displacement across all 8 canonical streams.
- `data/thor-evidence/aggregate-147-to-151-20261023.json` — aggregate canonical 3,600-frame replay verification receipt advancing baseline from 147 to 151 active native helpers.
- `src/tools/hybrid/m68k_008cdc_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Entity Parameter Reset Loop (`0x008CDC..0x008D06`, caller `0x008C8A`), verified in dual canonical 3,600-frame replay (Run A: 3 ROM calls; Run B: 3 native calls, 0 fallbacks, 516 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_008cdc_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, loop body execution, and parameter reset.
- `data/thor-evidence/008cdc-replay-differential-20261023.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x008CDC` displacement across all 8 canonical streams.
- `src/tools/hybrid/m68k_062156_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for Audio Driver SFX Dispatcher (`0x062156..0x062186`, callers `0x061032`, `0x061054`, `0x0610C8`), verified in dual canonical 3,600-frame replay (Run A: 20 ROM calls; Run B: 20 native calls, 0 fallbacks, 400 stepped instructions, bit-identical stream hashes across all 3,600 frames).
- `tests/m68k_062156_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, effect code masking, pointer table indexing, and slot committal.
- `data/thor-evidence/062156-replay-differential-20261023.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x062156` displacement across all 8 canonical streams.
- `tools/bizhawk-native-ring/evidence_hold_directives.py` — modular hold and deferral directives dictionary extracted from `evidence_knowledge_bridge.py` to maintain strict <= 500 line size limit.
- `data/thor-evidence/aggregate-151-to-153-20261023.json` — aggregate canonical 3,600-frame replay verification receipt advancing baseline from 151 to 153 active native helpers.
- `src/tools/hybrid/m68k_003064_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for CRAM RGB Channel Linear Interpolator Leaf (`0x003064..0x00307A`, caller `0x00303C`), verified in expanded scenario `beach_combat_transition_6000` (Run A: 3,168 ROM calls; Run B: 3,168 native calls, 0 fallbacks, 34,848 stepped instructions, 0 diff frames).
- `tests/m68k_003064_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, RGB channel math, and blend accumulation.
- `data/thor-evidence/003064-replay-differential-20261026.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x003064` displacement.
- `src/tools/hybrid/m68k_02edca_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Entity Child Slot Relative Coordinate Updater (`0x02EDCA..0x02EDE6`, callers `0x02EDAA`, `0x02EDB2`, `0x02EDBA`, `0x02EDC2`), verified in expanded scenario `beach_combat_transition_6000` (Run A: 2,020 ROM calls; Run B: 2,020 native calls, 0 fallbacks, 14,140 stepped instructions, 0 diff frames).
- `tests/m68k_02edca_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, coordinate offsets, and slot memory writes.
- `data/thor-evidence/02edca-replay-differential-20261026.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x02EDCA` displacement.
- `src/tools/hybrid/m68k_02eea0_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Entity Child Slot 3D Position & Offset Subtractor (`0x02EEA0..0x02EEC4`, callers `0x02EE82`, `0x02EE8A`, `0x02EE92`, `0x02EE9A`), verified in expanded scenario `beach_combat_transition_6000` (Run A: 2,020 ROM calls; Run B: 2,020 native calls, 0 fallbacks, 22,220 stepped instructions, 0 diff frames).
- `tests/m68k_02eea0_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, 3D subtraction logic, and memory updates.
- `data/thor-evidence/02eea0-replay-differential-20261026.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x02EEA0` displacement.
- `src/tools/hybrid/m68k_001d30_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for Menu Text / Sprite Tile Descriptor Writer Leaf (`0x001D30..0x001D5A`, callers `0x001C64`, `0x001C6C`, `0x001C74`), verified in expanded scenario `file_select_name_entry_3600` (Run A: 117 ROM calls; Run B: 117 native calls, 0 fallbacks, 1,197 stepped instructions, 0 diff frames).
- `tests/m68k_001d30_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, character mapping, and sprite attribute writes.
- `data/thor-evidence/001d30-replay-differential-20261026.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x001D30` displacement.
- `src/tools/hybrid/m68k_03ac92_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Title / Intro Scroll Dual Fixed-Point Accumulator Stepper (`0x03AC92..0x03ACA8`, caller `0x03AAA8`), verified in expanded scenario `attract_title_story_demo_3600` (Run A: 89 ROM calls; Run B: 89 native calls, 0 fallbacks, 267 stepped instructions, 0 diff frames).
- `tests/m68k_03ac92_instruction_step_test.cpp` — unit test suite verifying caller gating, corrupt ROM rejection, and dual accumulator updates.
- `data/thor-evidence/03ac92-replay-differential-20261026.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03AC92` displacement.
- `data/thor-evidence/aggregate-153-to-158-20261026.json` — aggregate canonical 3,600-frame replay verification receipt advancing baseline from 153 to 158 active native helpers with exact 8/8 canonical stream match and 0 diff frames.
- `src/tools/hybrid/m68k_02e30e_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for Name Entry Cursor Position Clamper Leaf (`0x02E30E..0x02E3A8`, caller `0x02E290`), verified in expanded scenario `file_select_name_entry_3600` (125 native calls, 0 fallbacks, 1,562 stepped instructions, 0 diff frames).
- `tests/m68k_02e30e_instruction_step_test.cpp` — unit test suite for `0x02E30E` verifying caller gating, all 4 branch paths, bounds clamping, and flag manipulation.
- `data/thor-evidence/02e30e-replay-differential-20261027.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x02E30E`.
- `src/tools/hybrid/m68k_03c584_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Menu Entity Slot Coordinator Leaf (`0x03C584..0x03C5B6`, callers `0x03C34A`, `0x03C3CC`, `0x03C402`, `0x03C432`), verified in expanded scenario `menu_pause_inventory_3600` (3 native calls, 0 fallbacks, 192 stepped instructions, 0 diff frames).
- `tests/m68k_03c584_instruction_step_test.cpp` — unit test suite for `0x03C584` verifying caller gating, entity slot iteration, and threshold clearing.
- `data/thor-evidence/03c584-replay-differential-20261027.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03C584`.
- `data/thor-evidence/aggregate-167-to-169-20261027.json` — aggregate canonical 3,600-frame replay verification receipt advancing baseline from 167 to 169 active native helpers with exact 8/8 canonical stream matches and 0 diff frames.
- `src/tools/hybrid/m68k_00b730_instruction_step.hpp/.cpp` — exact ROM-checked, 4-caller-gated instruction-stepped helper for Sprite Attribute Table (SAT) Builder Loop (`0x00B730..0x00B79A`, callers `0x000EFE`, `0x004B48`, `0x03B448`, `0x03CE86`), verified in expanded scenario `attract_title_story_demo_3600` (1,396 native calls, 0 fallbacks, 171,475 stepped instructions, 0 diff frames).
- `tests/m68k_00b730_instruction_step_test.cpp` — unit test suite for `0x00B730` verifying caller gating, corrupt ROM rejection, and sprite piece clamping.
- `data/thor-evidence/00b730-replay-differential-20261027.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00B730`.
- `data/thor-evidence/aggregate-169-to-170-20261027.json` — aggregate canonical 3,600-frame replay verification receipt advancing baseline from 169 to 170 active native helpers with exact 8/8 canonical stream matches and 0 diff frames.
- `src/tools/hybrid/m68k_03b832_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped prototype for Title/Intro Palette Cross-Fader Leaf (`0x03B832..0x03B8A6`, caller `0x03AB8C`), slot 191 registered in step set, classified as TIMING_MODEL_BUG / DEFERRED pending `muls.w`/`divs.w` hardware cycle calibration.
- `tests/m68k_03b832_instruction_step_test.cpp` — unit test suite for `0x03B832` verifying caller gating, corrupt ROM rejection, and palette interpolation.
- `src/tools/hybrid/m68k_002b8a_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for VDP H-Scroll Table Reset Leaf (`0x002B8A..0x002BA0`, caller `0x002B40`), verified in scenarios `canonical_3600` and `attract_3600` (10 / 13 native calls, 0 fallbacks, 39 stepped instructions, 0 diff frames).
- `tests/m68k_002b8a_instruction_step_test.cpp` — unit test suite for `0x002B8A` verifying caller gating, corrupt ROM rejection, and VDP port writes.
- `data/thor-evidence/002b8a-replay-differential-20261028.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x002B8A`.
- `src/tools/hybrid/m68k_002ba0_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for VDP Plane Table Clear Loop Leaf (`0x002BA0..0x002BBC`, caller `0x002B44`), verified in scenarios `canonical_3600` and `attract_3600` (10 / 13 native calls, 0 fallbacks, 1,105 stepped instructions, 0 diff frames).
- `tests/m68k_002ba0_instruction_step_test.cpp` — unit test suite for `0x002BA0` verifying caller gating, corrupt ROM rejection, and DBRA plane zeroing.
- `data/thor-evidence/002ba0-replay-differential-20261028.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x002BA0`.
- `src/tools/hybrid/m68k_002b6e_instruction_step.hpp/.cpp` — exact ROM-checked, 1-caller-gated instruction-stepped helper for VDP V-Scroll Table Clear Leaf (`0x002B6E..0x002B8A`, caller `0x002B5E`), verified in scenarios `canonical_3600` and `attract_3600` (10 / 13 native calls, 0 fallbacks, 52 stepped instructions, 0 diff frames).
- `tests/m68k_002b6e_instruction_step_test.cpp` — unit test suite for `0x002B6E` verifying caller gating, corrupt ROM rejection, and VSRAM table clear.
- `data/thor-evidence/002b6e-replay-differential-20261028.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x002B6E`.
- `src/tools/hybrid/m68k_03c956_instruction_step.hpp/.cpp` — exact ROM-checked, multi-caller-gated instruction-stepped helper for Display/Window Critical Mode Transition Leaf (`0x03C956..0x03C98A`, 23 ROM callers), verified in scenarios `canonical_3600` and `attract_3600` (170 native calls, 0 fallbacks, 1,530 stepped instructions, 0 diff frames).
- `tests/m68k_03c956_instruction_step_test.cpp` — unit test suite for `0x03C956` verifying caller gating, corrupt ROM rejection, SR interrupt masking, and RAM flag updates.
- `data/thor-evidence/03c956-replay-differential-20261028.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03C956`.
- `src/tools/hybrid/m68k_002cbc_instruction_step.hpp/.cpp` — exact ROM-checked, 6-caller-gated instruction-stepped helper for VDP Plane Tile Rectangle Block Writer Leaf (`0x002CBC..0x002CE4`), verified across all 5 scenarios (2 native calls, 0 fallbacks, 2,642 stepped instructions, 0 diff frames).
- `tests/m68k_002cbc_instruction_step_test.cpp` — unit test suite for `0x002CBC` verifying caller gating, corrupt ROM rejection, and VDP tile block copy loop.
- `data/thor-evidence/002cbc-replay-differential-20261028.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x002CBC`.
- `src/tools/hybrid/m68k_shared_tail_contract.hpp` — reusable Shared Tail Execution Contract providing caller/branch validation, transfer provenance extraction, stack frame validation, and register/CCR safety for shared tail routines.
- `src/tools/hybrid/m68k_0062cc_instruction_step.hpp/.cpp` — shared-tail contract instruction stepper for Stop Entity / Reset Movement Velocity (`0x0062CC..0x0062E4`), supporting both direct calls (BSR/JSR) and tail branches (BRA/Bcc/JMP), verified across canonical_3600 (148 entries) and beach_6000 (372 entries) with 0 fallbacks and 0 diff frames.
- `tests/m68k_0062cc_instruction_step_test.cpp` — 12-case unit test suite for `0x0062CC` shared tail execution contract verifying direct BSR, BRA, Bcc, RTS, SP balance, CCR flags, X preservation, RAM writes, invalid entry, and stack integrity.
- `data/thor-evidence/0062cc-replay-differential-20261101.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0062CC` under shared tail contract (Milestone M15.184).
- `src/tools/hybrid/m68k_00663e_instruction_step.hpp/.cpp` — exact ROM-checked instruction stepper for Entity Movement Friction Deceleration & Subpixel Drift Applicator (`0x00663E..0x0066AA`, Action 6 in Player Dispatcher `0x005670`), supporting arithmetic friction deceleration, zero clamping, subpixel coordinate accumulation, child call dispatch (`0x0064C4`, `0x060004`), tail call to `0x0062CC`, and RTS return, verified across canonical_3600 (13 entries) and beach_6000 (52 entries) with 0 fallbacks and 0 diff frames.
- `tests/m68k_00663e_instruction_step_test.cpp` — 6-case unit test suite for `0x00663E` verifying normal completion, tail call to `0x0062CC`, negative velocity damping, SFX trigger path, unhandled caller rejection, and ROM byte mismatch.
- `data/thor-evidence/00663e-replay-differential-20261102.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00663E` displacement across all 8 canonical and beach streams (Milestone M15.186).
- `src/tools/hybrid/m68k_0062e4_instruction_step.hpp/.cpp` — exact ROM-checked instruction stepper for Entity Velocity / Subpixel Movement Applicator (`0x0062E4..0x006336`, Action 2 in Player Dispatcher `0x005670`), supporting subpixel coordinate accumulation, child call dispatch (`0x007DC8`, `0x0085E2`, `0x0064C4`, `0x060004`), tail call to `0x0062CC`, and stack unwind abort handling, verified across canonical_3600 (84 entries) and beach_6000 (296 entries) with 0 fallbacks and 0 diff frames.
- `tests/m68k_0062e4_instruction_step_test.cpp` — 9-case unit test suite for `0x0062E4` verifying normal completion, tail call to `0x0062CC`, stack abort early exit, unhandled caller rejection, ROM byte mismatch, external IRQ handling, SFX trigger path, and subpixel math accumulation.
- `data/thor-evidence/0062e4-replay-differential-20261102.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0062E4` displacement across all 8 canonical and beach streams (Milestone M15.185).
- `src/tools/hybrid/m68k_006516_instruction_step.hpp/.cpp` and `m68k_006516_step_block1.hpp/.cpp` — exact ROM-checked instruction stepper partitioned across two <= 500 line translation units for Secondary Locomotion / Diagonal Movement Integration (`0x006516..0x00663E`, Action 2 in Player Action Dispatcher `0x005670`), supporting coordinate accumulation, child call dispatch (`0x007DC8`, `0x0083D4`, `0x0064C4`, `0x008EF8`, `0x060004`), tail call to `0x0062CC`, direct fallthrough into active helper `0x00663E`, and stack unwind abort handling, verified across 1,592 gameplay continuation frames from checkpoint `canonical_frame2008.state` (97 entries, 0 fallbacks, 3,049 stepped instructions, 0 diff frames).
- `tests/m68k_006516_instruction_step_test.cpp` — 6-case unit test suite for `0x006516` using non-elidable `TEST_ASSERT` macros verifying entry validation, child returns, branch conditions, sound trigger, unhandled caller rejection, ROM byte mismatch, and direct fallthrough to `0x00663E`.
- `data/thor-evidence/006516-gameplay-checkpoint-ab-20261103.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x006516` displacement across all 8 output streams under the full production baseline (Milestone M15.188).
- `src/tools/hybrid/m68k_007074_instruction_step.hpp/.cpp` and `m68k_007074_step_ops.hpp/.cpp` — modular ROM-checked instruction stepper partitioned across dedicated <= 500 line translation units for Player Locomotion Jump Gravity & Landing Collision Check (`0x007074..0x0070F8`, called from Action 0x72 `0x006FE4` and Action 0x74 `0x00701A`), supporting floor collision probe `0x00BCC4`, sound dispatch `0x060004`, landing state `0x00E268`, and animation update `0x01AD14`. Repaired operand inversion in three `ADD.L` instructions (`ADD.L Dn, d16(An)`), verified across 1,592 continuation frames with 42 native calls, 0 fallbacks, 600 stepped instructions, and 8/8 stream bit-for-bit equivalence; active helper #187 (Milestone M15.190).
- `tests/m68k_007074_instruction_step_test.cpp` — unit test suite for `0x007074` using non-elidable `TEST_ASSERT` macros verifying RAM writes to altitude (`16(a6)`), X-pos (`114(a6)`), Y-pos (`118(a6)`), velocity register preservation, fallthrough to `0x00BCC4`, ground branch with sound trigger `0x060004`, and ground collision active path with Carry set.
- `data/thor-evidence/007074-timing-rejection-20261103.json` — machine-readable rejection receipt recording 5/8 stream matches, 42/42 completed calls, 0 contract faults, and frame 771 DRAM refresh timing shift details (Milestone M15.189).
- `data/thor-evidence/007074-gameplay-checkpoint-ab-20261104.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x007074` across all 8 canonical output streams over 1,592 continuation frames from checkpoint `canonical_frame2008.state` under production baseline 187 (Milestone M15.190).
- `src/tools/hybrid/m68k_006fe4_instruction_step.hpp/.cpp` and `m68k_006fe4_step_ops.hpp/.cpp` — modular ROM-checked instruction stepper partitioned across dedicated <= 500 line translation units for Player Action 0x72 Jump Handler (`0x006FE4..0x00701A`, called from Action Table Jump via 0x00558A loop), supporting child physics call `0x007074`, input read `0x007D8A`, sound dispatch `0x060004`, carry-clear early exit, and transition branch to `0x007066`. Verified across 1,592 continuation frames with 24 native calls, 0 fallbacks, 87 stepped instructions, and 8/8 stream bit-for-bit equivalence; active helper #188 (Milestone M15.191).
- `tests/m68k_006fe4_instruction_step_test.cpp` — unit test suite for `0x006FE4` using non-elidable `TEST_ASSERT` macros verifying carry-clear airborne return, landing branch to `0x007066`, sound trigger dispatch `0x060004`, unhandled caller rejection, and corrupt ROM rejection.
- `data/thor-evidence/006fe4-gameplay-checkpoint-ab-20261104.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x006FE4` across all 8 canonical output streams over 1,592 continuation frames from checkpoint `canonical_frame2008.state` under production baseline 188 (Milestone M15.191).
- `src/tools/hybrid/m68k_03ace4_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Title / Intro Sequence Entity Initializer (`0x03ACE4..0x03AD0C`, proven caller `0x03AAA8`), verified in scenarios `attract_3600` and `canonical_3600` (158 native calls, 0 fallbacks, 474 stepped instructions, 0 diff frames).
- `tests/m68k_03ace4_instruction_step_test.cpp` — unit test suite for `0x03ACE4` verifying caller gating, corrupt ROM rejection, child call dispatch, and memory word writes.
- `data/thor-evidence/03ace4-replay-differential-20261101.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03ACE4` (Milestone M15.183).
- `tests/thor_auto_migrator_phase18_test.py` — regression test suite for Milestone M15.183 production integration, step set slot 205, receipt index 173 entries, and baseline 182 integrity.
- `src/tools/hybrid/m68k_00e36e_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Collision / Object Bound Updater (`0x00E36E..0x00E398`, proven caller `0x00EBC4`), verified in scenarios `canonical_3600` and `beach_6000` (48 native calls, 0 fallbacks, 336 stepped instructions, 0 diff frames).
- `tests/m68k_00e36e_instruction_step_test.cpp` — unit test suite for `0x00E36E` verifying caller gating, corrupt ROM rejection, child call dispatch, and bit flag operations.
- `data/thor-evidence/00e36e-replay-differential-20261101.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E36E` (Milestone M15.182).
- `tests/thor_auto_migrator_codegen_extension_test.py` — unit test suite for generalized M68000 instruction codegen extensions (displacement byte transfer, bit ops, immediate arithmetic).
- `tests/thor_auto_migrator_phase17_test.py` — regression test suite for Milestone M15.182 production integration, step set slot 204, receipt index 172 entries, and baseline 181 integrity.
- `src/tools/hybrid/m68k_03abda_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Title / Intro Sequence Stepper (`0x03ABDA..0x03AC16`, proven caller `0x03AAA8`), verified in scenarios `attract_3600` and `canonical_3600` (119 native calls, 0 fallbacks, 526 stepped instructions, 0 diff frames).
- `tests/m68k_03abda_instruction_step_test.cpp` — unit test suite for `0x03ABDA` verifying caller gating, corrupt ROM rejection, child call dispatch, and multi-frame branch stepping.
- `data/thor-evidence/03abda-replay-differential-20261101.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03ABDA` (Milestone M15.181).
- `tests/thor_auto_migrator_phase16_test.py` — regression test suite for Milestone M15.181 production integration, step set slot 203, receipt index 171 entries, and baseline 180 integrity.
- `src/tools/hybrid/m68k_03ab98_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Title / Intro Entity Initialization Sequence Leaf (`0x03AB98..0x03ABDA`, proven caller `0x03AA28`), verified in scenarios `attract_3600` and `canonical_3600` (1 native call, 0 fallbacks, 10 stepped instructions, 0 diff frames).
- `tests/m68k_03ab98_instruction_step_test.cpp` — unit test suite for `0x03AB98` verifying caller gating, corrupt ROM rejection, child call dispatch, and entity RAM flag updates.
- `data/thor-evidence/03ab98-replay-differential-20261101.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03AB98` (Milestone M15.180).
- `tests/thor_auto_migrator_phase15_test.py` — regression test suite for Milestone M15.180 production integration, step set slot 202, receipt index 170 entries, and baseline 179 integrity.
- `src/tools/hybrid/m68k_03ac16_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Title / Intro Entity Initialization Sequence Leaf (`0x03AC16..0x03AC68`, proven caller `0x03AA28`), verified in scenarios `attract_3600` and `canonical_3600` (1 native call, 0 fallbacks, 12 stepped instructions, 0 diff frames).
- `tests/m68k_03ac16_instruction_step_test.cpp` — unit test suite for `0x03AC16` verifying caller gating, corrupt ROM rejection, child call dispatch, and entity RAM flag updates.
- `data/thor-evidence/03ac16-replay-differential-20261031.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03AC16` (Milestone M15.179).
- `tests/thor_auto_migrator_phase14_test.py` — regression test suite for Milestone M15.179 production integration, step set slot 201, receipt index 169 entries, and baseline 178 integrity.
- `src/tools/hybrid/m68k_03ac6e_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Title / Intro Entity Initialization Sequence Leaf (`0x03AC6E..0x03AC92`, proven caller `0x03AA28`), verified in scenarios `attract_3600` and `canonical_3600` (1 native call, 0 fallbacks, 6 stepped instructions, 0 diff frames).
- `tests/m68k_03ac6e_instruction_step_test.cpp` — unit test suite for `0x03AC6E` verifying caller gating, corrupt ROM rejection, child call dispatch, and entity RAM flag updates.
- `data/thor-evidence/03ac6e-replay-differential-20261031.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x03AC6E` (Milestone M15.178).
- `tests/thor_auto_migrator_phase13_test.py` — regression test suite for Milestone M15.178 production integration, step set slot 200, receipt index 168 entries, and baseline 177 integrity.
- `src/tools/hybrid/m68k_00d962_instruction_step.hpp/.cpp` — exact ROM-checked, 3-caller-gated instruction-stepped helper for VDP Plane Clear/Fill Rectangle Loop Leaf (`0x00D962..0x00D9A4`), verified across all 5 scenarios (2 canonical / 2 attract calls, 0 fallbacks, 5,016 stepped instructions, 0 diff frames).
- `tests/m68k_00d962_instruction_step_test.cpp` — unit test suite for `0x00D962` verifying caller gating, corrupt ROM rejection, and VDP rectangular fill pattern loop.
- `data/thor-evidence/00d962-replay-differential-20261028.json` — machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00D962`.
- `src/tools/hybrid/m68k_0007c4_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for VDP DMA/Command FIFO Enqueue Leaf (`0x0007C4..0x0007E2`), registered slot 196 marked deferred pending interrupt mask / SR latency calibration.
- `tests/m68k_0007c4_instruction_step_test.cpp` — unit test suite for `0x0007C4` verifying caller gating, corrupt ROM rejection, and FIFO queue updates.
- `src/tools/hybrid/m68k_007198_instruction_step.hpp/.cpp` — exact ROM-checked, caller-gated instruction-stepped helper for Entity Friction & Inertia Velocity Scaler Leaf (`0x007198..0x0071DA`), registered slot 199 marked deferred pending scheduler cycle timing calibration (confirming historical `ADR-M15-HOLD-007198`).
- `tests/m68k_007198_instruction_step_test.cpp` — unit test suite for `0x007198` verifying caller gating, corrupt ROM rejection, and velocity scaling calculations.

## Automated Routine Migration Tooling (tools/thor_auto_migrator/)
- thor-auto-migrator.py — Top-level CLI launcher for the automated migrator.
- tools/thor_auto_migrator/__init__.py — Package initialization and versioning.
- tools/thor_auto_migrator/config.py — Migration limits, canonical ROM SHA256, and known hold/deferral tables.
- tools/thor_auto_migrator/state_machine.py — 3-dimensional state machine (historical, discovery, migration), attempt tracking, and persistent JSON database.
- tools/thor_auto_migrator/status_indexer.py — Comprehensive historical exclusions aggregator generating data/thor-auto-migrator/historical-status-index.json.
- tools/thor_auto_migrator/status_adapter.py — Authoritative status ledger adapter with active supersession and regression verification.
- tools/thor_auto_migrator/doctor.py — Environment and toolchain capability diagnostics (ROM, active set, censuses, hybrid cores, WSL, strict oracles).
- tools/thor_auto_migrator/discovery.py — Two-stage candidate discovery engine across all 5 runtime censuses with zero contamination guarantee.
- tools/thor_auto_migrator/m68k_opcodes.py — M68K instruction decoder and auto-safe opcode whitelist.
- tools/thor_auto_migrator/ir.py — Instruction and RoutineIR representation with branch/exit detection.
- tools/thor_auto_migrator/precheck.py — Conservative safety validator enforcing authoritative exclusions and hardware polling checks.
- tools/thor_auto_migrator/codegen.py — Native C++ instruction step generator with self-checking.
- tools/thor_auto_migrator/testgen.py — Deterministic unit test generator enforcing Rule 7 non-elidable TEST_ASSERT assertions.
- tools/thor_auto_migrator/build_gate.py — Isolated build and unit test execution gate.
- tools/thor_auto_migrator/ownership.py — Native C++ routine ownership index classifier (FULL_ROUTINE, PARTIAL_ROUTINE, EMBEDDED_BLOCK, SHARED_TAIL, INTERNAL_ENTRY, UNKNOWN).
- tools/thor_auto_migrator/runner_build.py — Isolated Linux runner build adapter compiling/linking staged overlays into build/auto-migrator/<target>/linux-runner/.
- tools/thor_auto_migrator/ab_gate.py — Strict differential A/B replay runner and 8-stream hash verifier.
- tools/thor_auto_migrator/cache.py — Evidence and execution cache manager with 8 authoritative keys preventing redundant analysis.
- tools/thor_auto_migrator/transaction.py — Isolated transaction directory manager (build/auto-migrator/<target>/).
- tools/thor_auto_migrator/fetch_contract_audit.py — Static pattern validator auditing opcode fetch and PC advancement contracts across hybrid step helpers.
- tools/thor_auto_migrator/run_03ac6e_verification.py — Target-specific reachability and strict differential A/B verification runner for prototype targets.
- tools/thor_auto_migrator/cli.py — CLI commands implementation (doctor, candidates, explain, scan, inspect, dry-run, migrate, status).
- data/thor-auto-migrator/historical-status-index.json — Machine-readable index of all authoritative project exclusions and active statuses.
- data/thor-auto-migrator/native-ownership-index.json — Machine-readable native C++ code coverage and routine ownership index.
- data/thor-auto-migrator/state.json — Persistent state database recording all evaluated targets and 3-dimensional status dimensions.
- docs/AUTO_MIGRATOR.md — Documentation guide for thor-auto-migrator operation, doctor diagnostics, and failure recovery.
- tests/thor_auto_migrator_test.py — Unit test suite covering StatusAdapter, supersession, doctor, candidate discovery, and state machine invariants.
- tests/thor_auto_migrator_phase11_test.py — Regression test suite for dual-run full-baseline A/B gate, native ownership, and cache invalidation.
- tests/thor_auto_migrator_baseline_regression_test.py — Regression test suite for baseline crash classification, contract fault repair verification, and A/B readiness.

- `src/tools/hybrid/m68k_00701a_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Player Action 0x74 Fall Handler, Actions 0x76/0x78 & Landing Transition (`0x00701A..0x007074`, 90 ROM bytes, 21 instructions). Handles primary entry `0x00701A`, secondary landing entry `0x007066`, and external secondary entries `0x007050` / `0x00705A` natively with zero ROM fallbacks.
- `tests/m68k_00701a_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering carry-clear airborne path, landing branch, SFX dispatch path, tail transfer to `0x0062CC`, caller rejection, and corrupt ROM rejection.
- `data/thor-evidence/00701a-gameplay-checkpoint-ab-20261104.json` — Machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00701A` across 1,592 continuation frames from `canonical_frame2008.state` (8/8 streams match, 48 native calls, 0 ROM fallbacks).
- `src/tools/hybrid/m68k_007198_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Player Inertia & Velocity Damping Leaf Subroutine (`0x007198..0x0071DA`, 66 ROM bytes, 27 instructions). Implements shift, clamp, and conditional branch damping logic with instruction-exact Musashi cycle accounting; promoted to Active Native Helper #190 in Milestone M15.194.
- `src/tools/hybrid/m68k_0064c4_step_ops.hpp/.cpp` — Shared M68000 arithmetic and branch step operations, including cycle-exact `step_asr_l_1` (+14 master clock dynamic adjustment) and `step_bcc_s` (-14 master clock not-taken branch adjustment).
- `tests/m68k_007198_instruction_step_test.cpp` — Dedicated unit test suite covering velocity damping, positive/negative clamping, zero thresholds, branch cycles, and Frame 245 regression fixture with non-elidable `TEST_ASSERT` assertions.
- `data/thor-evidence/007198-timing-rejection-20261104.json` — Machine-readable verification receipt documenting initial differential A/B evaluation of candidate `0x007198`, Frame 245 28-cycle drift isolation, and preserved Baseline 189 historical hold (M15.193).
- `data/thor-evidence/007198-gameplay-checkpoint-ab-20261104.json` — Machine-readable verification receipt proving bit-for-bit replay equivalence of `0x007198` across 1,592 continuation frames from `canonical_frame2008.state` (8/8 streams match, 49 native calls, 0 ROM fallbacks, 921 stepped instructions) and baseline promotion to 190 active helpers (M15.194).
- `src/tools/hybrid/m68k_0070f8_step_ops.hpp/.cpp` — Specialized M68000 instruction step operations for `0x0070F8`, implementing `step_tst_l_disp`, `step_cmpi_w_disp`, and `step_andi_w_disp` with exact operand direction and CCR calculation.
- `src/tools/hybrid/m68k_0070f8_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Player Jump Collision Interaction Callback (`0x0070F8..0x007112`, 26 ROM bytes, 7 instructions). Displaces callback registered into `$FF1940` by `0x007074` and invoked at `0x00BE5A`, extending the contiguous native jump/fall pipeline `[0x006FE4, 0x007112)` (302 contiguous ROM bytes); promoted to Active Native Helper #191 in Milestone M15.195.
- `tests/m68k_0070f8_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering caller gating, child call dispatch to `0x00BE98`, altitude active branch, action 0x66 branch, and ground contact mask application with `TEST_ASSERT` assertions.
- `data/thor-evidence/0070f8-gameplay-checkpoint-ab-20261104.json` — Machine-readable verification receipt proving bit-for-bit replay equivalence of `0x0070F8` across 1,592 continuation frames from `canonical_frame2008.state` (8/8 streams match, 3 native calls, 0 ROM fallbacks, 21 stepped instructions) and baseline promotion to 191 active helpers (M15.195).
- `src/tools/hybrid/m68k_00e2a2_step_ops.hpp/.cpp` — Specialized M68000 instruction step operations for `0x00E2A2`, implementing `step_tst_w_reg`, `step_move_l_reg_predec_sp`, `step_move_w_reg_reg`, `step_mulu_w_reg_reg`, `step_swap_reg`, `step_move_l_postinc_sp_reg`, and `step_rts` with cycle-exact unsigned MULU dynamics.
- `src/tools/hybrid/m68k_00e2a2_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Bounded Pseudo-Random Number Scaling Subroutine `GetRandomBounded(D0)` (`0x00E2A2..0x00E2BA`, 24 ROM bytes, 12 words, 10 instructions). Gated across 5 proven continuation callers (`0x01A4FC`, `0x01B242`, `0x01B7D6`, `0x01BBDC`, `0x01BF68`), handles child LFSR PRNG call to `0x00E268`, and provides uniform bounded random integer scaling; promoted to Active Native Helper #192 in Milestone M15.196.
- `tests/m68k_00e2a2_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering caller set gating, zero-bound early exit, and normal scaling with child return (including MSB-set `0xC000` test vector) with `TEST_ASSERT` assertions.
- `data/thor-evidence/00e2a2-gameplay-checkpoint-ab-20261104.json` — Machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E2A2` across 1,592 continuation frames from `canonical_frame2008.state` (8/8 streams match, 14 native calls, 0 ROM fallbacks, 140 stepped instructions) and baseline promotion to 192 active helpers (M15.196).

- `src/tools/hybrid/m68k_00e268_step_ops.hpp/.cpp` — Specialized M68000 instruction step operations for `0x00E268`, implementing `step_move_w_reg_predec_sp`, `step_move_w_abs_reg`, `step_move_w_reg_reg`, `step_rol_w_imm_reg`, `step_eor_w_reg_reg`, `step_lsr_w_imm_reg`, `step_addx_w_reg_reg`, `step_move_w_reg_abs`, `step_move_w_postinc_sp_reg`, and `step_rts` with exact Musashi cycle and CCR calculation.
- `src/tools/hybrid/m68k_00e268_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Core 16-bit Galois/Fibonacci LFSR Pseudo-Random Number Generator `GetRandom16()` (`0x00E268..0x00E2A2`, 58 ROM bytes, 29 words, 25 instructions). Manages state seed at `$00FF1928.W`, preserves D1.L across stack, and provides uniform 16-bit random word in D0.W; promoted to Active Native Helper #193 in Milestone M15.197.
- `src/tools/hybrid/CMakeLists_captures.cmake` — Sub-CMake module containing extracted test capture and oracle tool target definitions to maintain `src/tools/hybrid/CMakeLists.txt` strictly <= 500 lines.
- `tests/m68k_multiply_semantics_regression_test.cpp` — Dedicated test suite verifying M68000 unsigned MULU vs signed MULS operations, 32-bit register replacement, condition code flags, and dynamic cycle calculation against Musashi reference.
- `tests/m68k_00e268_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering 0x00E268 alignment checks, corrupt ROM rejection, stack preservation, and full 25-instruction execution cycle with non-elidable `TEST_ASSERT` assertions.
- `data/thor-evidence/00e268-gameplay-checkpoint-ab-20261104.json` — Machine-readable verification receipt proving bit-for-bit replay equivalence of `0x00E268` across 1,592 continuation frames from `canonical_frame2008.state` (8/8 streams match, 61 native calls, 0 ROM fallbacks, 1,525 stepped instructions) and baseline promotion to 193 active helpers (M15.197).

- `src/tools/hybrid/m68k_0083f8_step_ops.hpp/.cpp` — Specialized M68000 instruction step operations for `0x0083F8`, implementing indexed memory reading `step_move_w_indexed_disp_reg`, bit testing/setting `step_btst_imm_reg` / `step_bset_imm_reg`, and conditional loop branch operations with Musashi cycle calculations.
- `src/tools/hybrid/m68k_0083f8_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Entity Collision Ring Buffer Scanner `ScanEntityCollisionRing` (`0x0083F8..0x00846C`, 116 ROM bytes, 58 words, 33 instructions). Recompiled and evaluated in Milestone M15.198; quarantined in `HISTORICAL_HOLD` due to accumulated micro-cycle drift shifting frame boundary.
- `tests/m68k_0083f8_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering 0x0083F8 match exit condition, ring exhaustion, sequential calls, register preservation, and memory read assertions with non-elidable `TEST_ASSERT` assertions.
- `data/thor-evidence/0083f8-timing-rejection-20261104.json` — Machine-readable rejection receipt recording cycle-timing divergence forensics of `0x0083F8` across 1,592 continuation frames from `canonical_frame2008.state` (28 calls, 3,933 instructions displaced, 0 contract faults, 28-cycle boundary drift) and quarantine into `HISTORICAL_HOLD` (M15.198).
- `tests/hybrid_registry_test.cpp` — Focused unit test suite verifying `HybridExecutionRegistry` player ABI canon, state tracking, main loop tick, divergence oracle, and entity oscillation region dispatch.
- `tools/candidate_selection_gate.py` — Fail-closed 7-step candidate selection gate preventing accidental promotion or migration of historical failed, held, active, or overlapping routines without approved ADR supersession tokens (M15.199).
- `tests/candidate_selection_gate_test.py` — Dedicated test suite verifying `CandidateSelectionGate` rejection of `0x0083F8`, `0x007DC8`, `0x00BCC4`, `0x00C012`, active routines, overlapping ranges, high execution count invariance, and authorized supersession (M15.199).
- `data/thor-evidence/canonical-beach-frame3600-checkpoint-20261105.json` — Machine-readable checkpoint verification receipt for `canonical_beach_frame3600.state` establishing 100% bit-for-bit deterministic resume proof across 600 continuation frames, 26 new ROM procedures, and confirmed combat gameplay mechanics (M15.199).

- `src/tools/hybrid/m68k_00b610_step_ops.hpp/.cpp` — Specialized M68000 instruction step operations for `0x00B610`, implementing arithmetic shift right `step_asr_w_imm_reg` (`0xE448`, count=2 with `count * 14` cycle calculation), address addition `step_adda_w_reg_reg`, register-to-stack / stack-to-register moves `step_move_w_reg_predec_sp` / `step_move_w_postinc_sp_reg`, and sub-piece table lookups.
- `src/tools/hybrid/m68k_00b610_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Player Weapon Dynamic Collision & Sub-Piece Animation Sequencer (`0x00B610..0x00B66A`, 90 ROM bytes, 26 instructions + 10-byte lookup table). Displaces caller `0x00B3A4`, coordinates child `0x00B6A6`, correctly applies `ASR.W #2, D0` and cycle-adjusted `movem.l` operations; promoted to Active Native Helper #194 in Milestone M15.200.
- `tests/m68k_00b610_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering 0x00B610 instruction stepping, arithmetic shift right bit-exact behavior, child call dispatch, register preservation, and stack alignment with non-elidable `TEST_ASSERT` assertions.
- `data/thor-evidence/00b610-dual-scenario-ab-20261105.json` — Machine-readable dual-scenario verification receipt proving bit-for-bit replay equivalence across BOTH authentic scenarios: Scenario A (`canonical_frame2008.state`, 1,592 frames, 8/8 streams match) and Scenario B (`canonical_beach_frame3600.state`, 600 frames, 8/8 streams match, 5 native calls, 120 stepped instructions, 0 ROM fallbacks), establishing baseline promotion to 194 active helpers (`ACTIVE_SET_SHA256 = 89c33e7e9946e15ddf58b2611fbcc8c928897b9fa01c10dc7d50b69a978ec43f`).


- `src/tools/hybrid/m68k_0082ae_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Player Interaction / Item Contact Scanner & Landing Dispatcher (`0x0082AE..0x0082F8`, 74 ROM bytes, 37 ROM words, 21 instructions + 16 extension words). Displaces caller `0x007FE8`, coordinates child `0x00B9EC` (registered return PC `0x0082BE`), and executes landing dispatch logic; promoted to Active Native Helper #195 in Milestone M15.201.
- `tests/m68k_0082ae_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering 0x0082AE instruction stepping, child call dispatch to `0x00B9EC`, stack preservation, and landing dispatch with non-elidable `TEST_ASSERT` assertions.
- `tests/m68k_00ceea_cmpa_regression_test.cpp` — Dedicated regression test suite verifying `CMPA.L #imm, A0` vs `A1` condition code register differences across line buffer sites `0x00D2EC`, `0x00D316`, and `0x00D336` preventing M15.200 register mismatch regressions.
- `data/thor-evidence/0082ae-dual-scenario-ab-20261105.json` — Machine-readable dual-scenario verification receipt proving bit-for-bit replay equivalence across BOTH authentic scenarios: Scenario A (`canonical_frame2008.state`, 1,592 frames, 8/8 streams match) and Scenario B (`canonical_beach_frame3600.state`, 600 frames, 8/8 streams match, 12 dynamic entries, 156 stepped instructions, 0 ROM fallbacks), establishing baseline promotion to 195 active helpers (`ACTIVE_SET_SHA256 = efa86699a2d3714e23f51b163de3f067e0da2bd59eea996dec77035afb377ffb`).

- `src/tools/hybrid/m68k_00b66a_step_ops.hpp/.cpp` — Specialized M68000 instruction step operations for `0x00B66A`, implementing address subtraction `step_suba_w_reg_areg` (`SUBA.W D0, A4`) and immediate addition `step_addq_w_imm_reg` (`ADDQ.W #1, D0`).
- `src/tools/hybrid/m68k_00b66a_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Player Weapon Dynamic Collision & Sub-Piece Animation Sequencer #2 (`0x00B66A..0x00B6A6`, 60 ROM bytes, 30 ROM words, 19 instructions). Displaces caller `0x00B3CA` (displacement `0x029E`), coordinates child `0x00B6A6`, and steps animation sequencing logic; promoted to Active Native Helper #196 in Milestone M15.203.
- `tests/m68k_00b66a_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering 0x00B66A instruction stepping, caller validation, displacement calculation, branch taken paths, child call dispatch, and register preservation with non-elidable `TEST_ASSERT` assertions.
- `tests/m68k_00b856_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering 0x00B856 composite instruction stepping, prefix entry verification at `0x00B852`, miss paths, and resume paths with non-elidable `TEST_ASSERT` assertions.
- `data/thor-evidence/00b66a-dual-scenario-ab-20261105.json` — Machine-readable dual-scenario verification receipt proving bit-for-bit replay equivalence across BOTH authentic scenarios: Scenario A (`canonical_frame2008.state`, 1,592 frames, 8/8 streams match) and Scenario B (`canonical_beach_frame3600.state`, 600 frames, 8/8 streams match, 5 native calls, 90 stepped instructions, 0 ROM fallbacks), establishing baseline promotion to 196 active helpers (`ACTIVE_SET_SHA256 = 17396fdaf9c566d36ca8317d40bb334f0f3467c73f1aebe0e26a36f8d4951867`).

- `tools/thor_evidence/receipt_verifier.py` — Deterministic machine receipt verification pipeline validating 64-hex SHA-256 formats, computing physical disk byte hashes, verifying runner SHA formats and paths, detecting duplicate Z80 RAM digests, inspecting and recalculating SHA-256 of all 48 stream files directly from disk across 3 scenarios x 2 runs A/B x 8 streams (`STREAM_ARTIFACT_COUNT = 48`, `STREAM_ARTIFACTS_VALID = 48`), and differentiating arbitrary hash mismatches from truncated logger prefix hallucinations (`HALLUCINATED_SHA256_SUFFIX`) with zero tolerance for LLM-transcribed cryptographic claims (M15.204..M15.210).
- `tests/receipt_verifier_test.py` — Dedicated unit test suite verifying `ReceiptVerifier` functionality with 17 comprehensive test cases covering 1-byte corruptions, missing files, omitted streams, duplicate Z80 digests, fabricated SHA suffixes, byte size mismatches, invalid runner SHA formats, stream artifact count mismatches, and physical verification of superseded, active, and qualification receipt files (M15.204..M15.210).
- `data/thor-evidence/canonical-beach-frame3600-checkpoint-20261105.m15_199_superseded.json` — Immutable byte-exact copy of the original superseded M15.199 receipt with authentic LF newlines preserving historical evidence integrity (`OLD_RECEIPT_CONTENT_SHA256 = 310645d2bd4502ea1a0e6aaf1433aa35fbed482c0dcaff57c4ed81e15ed346e2`).
- `data/thor-evidence/supersession-beach-frame3600-20261105.json` — Cryptographic supersession record linking the superseded M15.199 receipt and updated M15.204 active receipt with verified physical disk SHA-256 values.
- `tools/audit_steppers.py` — AST-based static instruction stepper semantic analysis tool auditing all 196 active production helpers for multi-word operand ordering, branch synchronization, elidable test assertions, operand directionality, MULU/MULS signedness, and CMPA target registers. Updated in M15.205 with `verify_production_stepper_inventory()`, 20 validation gate fixtures, and complete 196-helper inventory enforcement.
- `src/tools/hybrid/m68k_00b45c_step_ops.hpp/.cpp` — Specialized M68000 instruction step operations for `0x00B45C`, implementing memory test `step_tst_b_disp_reg` (`TST.B d16(An)`), branch lower or same `step_bls_s` (`BLS.S`), and sub-piece parameter table lookups.
- `src/tools/hybrid/m68k_00b45c_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Player Weapon Sub-Piece Animation Sequencer #1 (`0x00B45C..0x00B4C8`, 108 ROM bytes, 38 instructions + 54-byte lookup tables). Displaces caller `0x00B394` (displacement `0x00C6`), coordinates child `0x00B6A6`, and steps animation sequencing logic. Repaired in M15.205 for authentic `DC 40` `ADD.W D0, D6` ($3 \times i$ index) and caller displacement `0x00C6`. Registered disabled in slot 217 awaiting authentic scenario reachability.
- `tests/m68k_00b45c_instruction_step_test.cpp` — Dedicated non-elidable unit test suite covering 0x00B45C instruction stepping, caller validation against authentic ROM displacement, variant lookups, table 1 and table 2 dispatch paths with $3 \times i$ index verification, child call dispatch to `0x00B6A6`, and register preservation with non-elidable `TEST_ASSERT` assertions (4/4 PASS).
- `tools/thor_auto_migrator/production_inventory.py` — Dynamic production inventory discovery and validation module for the auto-migrator pipeline (M15.206). Eliminates manual versioned list imports, parses active ranges from `native-ownership-index.json`, correlates with C++ selectable/disabled wiring in `m68k_gameplay_step_registry.cpp` and `m68k_gameplay_step_set.cpp`, and verifies existence of physical C++ source files on disk.
- `tests/production_inventory_test.py` — Comprehensive regression test suite for `production_inventory.py` implementing 7 negative validation fixtures (active omission, disabled inclusion, duplicate registration, invalid range, missing source, historical hold enabled, adapter disagree) and verifying 100% active set consistency (7/7 PASS).
- `src/tools/hybrid/m68k_00302e_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Palette Blender & 3-Channel RGB Interpolator (`0x00302E..0x003064`, 54 ROM bytes, 15 instructions). Recompiles register save/restore (`MOVEM.L`), dual source palette reading `(A1)+` and `(A2)+`, child invocation of color channel blender #171 `0x003064` (3 calls per color word), destination writing `(A0)+`, palette line loop `DBRA D1`, and VDP dirty flag setting `BSET.B #0, ($00FF164D)`. Registered in slot 218.
- `tests/m68k_00302e_instruction_step_test.cpp` — Dedicated non-elidable unit test suite for `0x00302E` covering 16-color loop execution, output color blending, register restoration, dirty flag assertion, non-matching PC rejection, disabled bypass mode, and 15 authentic ROM caller validations using `TEST_ASSERT` (4/4 PASS).
- `src/tools/hybrid/m68k_00f0d2_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Euclidean 2D Distance Calculator (`0x00F0D2..0x00F0EC`, 26 ROM bytes, 13 words, 10 instructions). Recompiles register save/restore (`MOVEM.L D3-D4`), coordinate subtractions (`SUB.W`), signed multiplications (`MULS.W`) using bit-exact Musashi cycle formula $38 \times 7 + 14 \times \text{bits}$, sum of squares accumulation (`ADD.L`), child integer square root dispatch (`BSR.W 0x00E6BA`), stack restore with postincrement dummy read, and return (`RTS`). Registered in slot 219 (M15.212).
- `tests/m68k_00f0d2_instruction_step_test.cpp` — Dedicated non-elidable unit test suite for `0x00F0D2` covering Euclidean distance vector arithmetic ($30^2 + 40^2 \rightarrow 50$), signed multiplication limits, negative numbers, zero handling, MOVEM stack pointer pre-decrement/post-increment (-8/+8 bytes), +112 master clocks extra, dummy read at new SP, stack sentinels (`0xCAFEBABE`, `0xDEADBEEF`), register preservation, child `0x00E6BA` invocation, caller validation against authentic ROM caller `0x02581C`, and metrics recording (M15.213).
- `tools/run_00f0d2_three_scenario_ab.py` — Automated three-scenario differential A/B replay runner for `0x00F0D2` across Scenario A (1,592 frames), Scenario B (600 frames), and Scenario C (1,800 frames), verifying 24/24 streams bit-for-bit exact match and 0 ROM fallbacks (M15.213).
- `data/thor-evidence/00f0d2-three-scenario-qualification-20261107.json` — Machine-readable three-scenario qualification receipt documenting bit-exact single-variable differential A/B equivalence across Scenario A (1,592 frames, 8/8 streams match), Scenario B (600 frames, 8/8 streams match), and Scenario C (1,800 frames, 8/8 streams match, 1,202 native calls, 12,020 instructions stepped, 1,202 child calls, 0 ROM fallbacks), verified by `ReceiptVerifier` across 52 physical disk files (M15.213).
- `src/tools/hybrid/m68k_gameplay_step_metrics.cpp` — Global gameplay step metrics serialization implementation split from `m68k_gameplay_step_set.cpp` to maintain source files $\le 500$ lines. Writes JSON metrics for all registered gameplay helper steppers (including slots 218 `0x00302E` and 219 `0x00F0D2`).
- `scratch/run_00302e_dual_scenario.py` — Dual-scenario differential replay test script executing single-variable A/B runs across Scenario A (1592 frames) and Scenario B (600 frames) on WSL, verifying bit-for-bit identity across all 8 stream artifacts.
- `tools/prepare_target_active_session.py` — Generates authentic target-active verification scenario `target_active_beach_1800` (1,800 frames resuming from `canonical_beach_frame3600.state`, covering original timeline frames 3601..5400) by slicing and renumbering input polls and frame landmarks from `beach_combat_transition_6000` (M15.207).
- `tools/run_target_active_ab.py` — Single-variable differential A/B replay runner for `target_active_beach_1800` on WSL, comparing 196 baseline vs 196 + 00302E, verifying native execution telemetry (66 calls, 11,880 stepped instructions, 3,168 child calls), and evaluating raw byte-exact equivalence across all 8 streams (M15.207).
- `data/thor-evidence/00302e-three-scenario-qualification-20261106.json` — Machine-readable three-scenario qualification receipt documenting bit-exact single-variable differential A/B equivalence across Scenario A (1,592 frames, 8/8 streams match), Scenario B (600 frames, 8/8 streams match), and Scenario C (1,800 frames, 8/8 streams match, 66 native calls, 11,880 instructions stepped, 3,168 child calls, 0 ROM fallbacks), verified by `ReceiptVerifier` (M15.209).
- `src/tools/hybrid/replay_finalization.hpp/.cpp` — Explicit runner finalization and replay lifecycle coordinator (`finalize_diagnostic_replay()`) ensuring all diagnostic traces, journals, external event streams, and manifests are durably flushed, verified, and closed before calling `retro_unload_game()` and `retro_deinit()`. Completely eliminates unsafe `std::quick_exit(0)` and prevents buffered file truncations (M15.214).
- `tests/gameplay_session_termination_test.py` — Dedicated pytest suite verifying clean return codes (exit 0), `SAFE_CLEANUP_SUCCESS` teardown status, full JSONL record counts, and fail-closed termination under journal truncation, missing checkpoints, corrupted states, and manifest mismatches (M15.214).
- `tests/m68k_0099d6_timing_regression_test.cpp` — Cycle-timing regression test verifying M68000 bus-refresh cycle suppression and shift timing order for helper `0x0099D6` (`LSL.W D3, D4`), proving that adding shift cycles before invoking `skip_bus_refresh()` correctly suppresses refresh when crossing the 896-cycle boundary (M15.214).
- `data/thor-evidence/00f0d2-three-scenario-qualification-20261107.json` — Machine-readable three-scenario qualification receipt documenting bit-exact single-variable differential A/B equivalence across Scenario A (1,592 frames, 8/8 streams match), Scenario B (600 frames, 8/8 streams match), and Scenario C (1,800 frames, 8/8 streams match, 1,202 native calls, 12,020 instructions stepped, 1,202 child calls, 0 ROM fallbacks), verified by `ReceiptVerifier` across 56 distinct physical disk files (57 total checks: 48 streams + 2 checkpoints + 3 inputs + 1 ROM + 1 Core + 1 Runner) (M15.214).
- `src/tools/hybrid/m68k_00d168_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Camera Viewport Scroll Offset Wrapper / Coordinate Delta Accumulator (`0x00D168..0x00D266`, 254 ROM bytes, 73 instructions). Evaluates camera tracking active flag (`btst #0, 39(%a0)`), conditionally branches to RTS (`0x00D264`), accumulates coordinate deltas (`add.w (%a0), %d1` and `add.w 4(%a0), %d2`), and branches to viewport clamping logic (`0x00D188..0x00D264`) via `step_camera::step_camera_scroll_op()`. Registered in slot 220 (M15.215).
- `tests/m68k_00d168_instruction_step_test.cpp` — Dedicated non-elidable unit test suite for `0x00D168` covering tracking-enabled branch (delta accumulation and clamping branch), tracking-disabled branch (immediate RTS bypass), caller verification (`0x02EB24`), ROM opcode prefix verification, and register preservation using `TEST_ASSERT` (M15.215).
- `tools/run_00d168_three_scenario_ab.py` — Automated three-scenario differential A/B replay runner for `0x00D168` across Scenario A (1,592 frames), Scenario B (600 frames), and Scenario C (1,800 frames), verifying 24/24 streams bit-for-bit exact match, 114 native calls, 5,965 stepped instructions, 0 fallbacks, 0 contract faults, and generating authoritative evidence receipt (M15.215).
- `data/thor-evidence/00d168-three-scenario-qualification-20261107.json` — Machine-readable three-scenario qualification receipt documenting bit-exact single-variable differential A/B equivalence across Scenario A (1,592 frames, 8/8 streams match), Scenario B (600 frames, 8/8 streams match), and Scenario C (1,800 frames, 8/8 streams match, 114 native calls, 5,965 instructions stepped, 0 ROM fallbacks, 0 contract faults), verified by `ReceiptVerifier` across 56 distinct physical disk files (57 total checks: 48 streams + 2 checkpoints + 3 inputs + 1 ROM + 1 Core + 1 Runner) (M15.215, ownership updated in M15.216).
- `tests/m68k_00d188_semantic_regression_test.cpp` — Dedicated semantic regression test suite for `0x00D188` and `0x00D168` covering caller isolation, mutual caller rejection, tracking flag branch, coordinate delta accumulation, viewport boundary clamping, RTS return state, and register preservation using non-elidable `TEST_ASSERT` (M15.216).
- `data/thor-evidence/00d168-ownership-supplement-20261107.json` — Machine-readable ownership supplement receipt documenting the dynamic step histogram for candidate `0x00D168` across 114 native calls (570 prefix steps + 5,395 shared-tail steps = 5,965 total native steps, 0 early RTS branches), resolving M15.216 evidence accounting gaps without repeating full replays (M15.217).
- `src/tools/hybrid/m68k_00dbca_instruction_step.hpp/.cpp` — Native C++ instruction stepper helper for Camera Viewport Buffer 2D Block Copy Math Leaf (`0x00DBCA..0x00DC26`, 92 ROM bytes, 28 instructions). Simulates tile buffer 2D row/column block copy, coordinate pitch indexing, register shifts, and loop control via `dbra`. Enforces exact Musashi bus refresh timing bridge and caller whitelist (`0x00DB64`, `0x010130`). Registered in slot 221 (M15.217).
- `tests/m68k_00dbca_instruction_step_test.cpp` — Dedicated unit test suite for `0x00DBCA` covering caller whitelisting, ROM byte guard verification, empirical Scenario C execution fixture (62 steps, 2D tile block copy memory and register parity), and non-elidable `TEST_ASSERT` validation in Debug and Release (M15.217).
- `tools/run_00dbca_three_scenario_ab.py` — Automated three-scenario differential A/B replay runner for `0x00DBCA` across Scenario A (1,592 frames), Scenario B (600 frames), and Scenario C (1,800 frames), verifying 24/24 streams bit-for-bit exact match, 2 native calls, 235 stepped instructions, 0 fallbacks, 0 contract faults, and generating authoritative evidence receipt (M15.217).
- `data/thor-evidence/00dbca-three-scenario-qualification-20261107.json` — Machine-readable three-scenario qualification receipt documenting bit-exact single-variable differential A/B equivalence across Scenario A (1,592 frames, 8/8 streams match), Scenario B (600 frames, 8/8 streams match), and Scenario C (1,800 frames, 8/8 streams match, 2 native calls, 235 instructions stepped, 0 ROM fallbacks, 0 contract faults), verified by `ReceiptVerifier` across 56 distinct physical disk files (57 total checks: 48 streams + 2 checkpoints + 3 inputs + 1 ROM + 1 Core + 1 Runner) (M15.217).

- `src/tools/hybrid/m68k_entity_cluster_types.hpp` — Common type definitions, transfer descriptors, execution outcomes, and constant definitions for the `0x0254EA` entity state engine execution cluster (M15.219).
- `src/tools/hybrid/m68k_entity_cluster_context.hpp/.cpp` — Shared explicit `GuestExecutionContext` wrapping register state, guest memory reads/writes, CCR flags, and cycle tracking without altering guest semantics (M15.219).
- `src/tools/hybrid/m68k_entity_cluster_dispatch.hpp/.cpp` — Position-independent jump table calculation engine (`0x02575C..0x025766`) reconstructing index validation, relative table arithmetic, and fail-closed dispatch to 13 targets (M15.219).
- `src/tools/hybrid/m68k_entity_cluster_state0.hpp/.cpp` — State 0 timer decrement handler (`0x025896`) managing timer exhaustion, loopback transfers, and original return points (M15.219).
- `src/tools/hybrid/m68k_entity_cluster_target10.hpp/.cpp` — Target 10 proximity check & coordinate math handler (`0x0257EE`) coordinating child call `0x00F0D2` and common return joins (M15.219).
- `src/tools/hybrid/m68k_entity_cluster_target11_12.hpp/.cpp` — Target 11 elevation math (`0x02583E`) and Target 12 stun loopback (`0x025880`) handlers (M15.219).
- `src/tools/hybrid/m68k_entity_cluster_coordinator.hpp/.cpp` — Execution cluster coordinator orchestrating multi-module control transfers between native dispatcher and native state handlers with zero synthetic stack pushes or duplicate instructions (M15.219).
- `src/tools/hybrid/m68k_entity_cluster_metrics.hpp/.cpp` — Cluster telemetry and instruction step accounting tracking authentic ROM instruction displacement across all cluster modules (M15.219).
- `tests/m68k_entity_cluster_test.cpp` — Dedicated test suite with 7 test fixtures verifying jump table calculation, dispatcher stepping, State 0, Target 10, Targets 11/12, and coordinator chaining across Debug and Release (M15.219).
- `scratch/extract_cluster_cfg.py` — Automated CFG and basic block extraction script analyzing 1,608 bytes of ROM at `[0x0254EA, 0x025B32)` across 111 basic blocks and authentic Scenario C trace counts (M15.219).
- `scratch/rank_handlers.py` — Dynamic workload ranking script computing authentic Scenario C entries and instruction step displacement across all 13 jump table targets (M15.219).
- `src/tools/hybrid/m68k_02575c_instruction_step.hpp/.cpp` — Native C++ hybrid instruction stepper bridging the entity engine cluster (`0x02575C..0x02589E`) to the live M68K runner. Implements fail-closed unsupported state decline, child call coordination, State 0, State 7, Target 12, routing, and metrics serialization (M15.220, M15.221, M15.222).
- `src/tools/hybrid/m68k_02575c_step_ops.hpp` — Shared internal step declarations, CCR update helpers (`do_bset_b`, `do_bclr_b`, etc.), and instruction address boundaries for the modular `0x02575C` entity cluster stepper (M15.221, M15.222).
- `src/tools/hybrid/m68k_02575c_target10_step.cpp` — Dedicated native stepper module for Target 10 (`0x0257EE..0x02583C`), implementing proximity checks, coordinate loading, ROM child call `0x00F0D2`, and return continuation at `0x025822` (1,202 calls, 15,629 native steps) (M15.221, M15.222).
- `src/tools/hybrid/m68k_02575c_target11_step.cpp` — Dedicated native stepper module for Target 11 (`0x02583E..0x02587A`), implementing elevation/gravity integration, velocity updates, and intentional tail continuation to `0x00E7E8` (27 calls, 166 native steps) (M15.221, M15.222).
- `src/tools/hybrid/m68k_02575c_state1_step.cpp` — Dedicated native stepper module for State 1 (`0x0258A0..0x025940`), implementing attack/movement processing and child call `0x00BCD8` (229 calls, 3,196 steps) (M15.222).
- `src/tools/hybrid/m68k_02575c_state2_3_4_step.cpp` — Dedicated native stepper module for States 2, 3, and 4 (`0x025940..0x0259CE`), implementing deceleration, AI alert mode with child `0x00E502` and tail jump `0x00E7E8`, and reaction checks (M15.222).
- `src/tools/hybrid/m68k_02575c_state5_step.cpp` — Dedicated native stepper module for State 5 (`0x0259CE..0x025A72`), implementing entity lookup and displacement math (M15.222).
- `src/tools/hybrid/m68k_02575c_state6_step.cpp` — Dedicated native stepper module for State 6 (`0x025A72..0x025B22`), implementing bounding box checks and child calls `0x00B852` and `0x00E7E8` (32 calls, 618 steps) (M15.222).
- `tools/build_02575c_runner.py` — Build orchestration script generating the Linux gameplay runner `oasis_gameplay_session` (SHA256: `cdd7f58d0215fe9418b8fed5b1f84bd3c84a913c3a92e960f4c1707ed85819c8`) with slot 222 wired to `step_m68k_02575c` (M15.220, M15.221, M15.222).
- `tools/run_02575c_three_scenario_ab.py` — Automated three-scenario differential A/B replay runner for `0x02575C` across Scenario A (1,592 frames), Scenario B (600 frames), and Scenario C (1,800 frames), verifying 24/24 streams bit-for-bit exact match (M15.220, M15.221, M15.222).
- `data/thor-evidence/02575c-three-scenario-qualification-20261108.json` — Machine-readable three-scenario qualification evidence receipt for State 0, validated via `ReceiptVerifier` across 55 checked items (M15.220).
- `data/thor-evidence/02575c-three-scenario-qualification-m15-221-20261108.json` — Machine-readable three-scenario qualification evidence receipt for the expanded multi-handler cluster (State 0, Target 10, Target 11, Target 12), validated via `ReceiptVerifier` across all 56 distinct physical files (58 check operations) with status `VERIFIED` (M15.221).
- `data/thor-evidence/02575c-three-scenario-qualification-m15-222-20261108.json` — Machine-readable three-scenario qualification evidence receipt for M15.222 (States 0..4, 6..7, Targets 10..12), validated via `ReceiptVerifier` across all 56 distinct physical files (58 check operations) with status `VERIFIED` (M15.222).
- `tools/generate_02575c_m15_223_receipt.py` — Automated receipt generator for M15.223, capturing all 48 stream artifacts across Scenarios A, B, and C with exact byte counts and SHA256 hashes (M15.223).
- `data/thor-evidence/02575c-three-scenario-qualification-m15-223-20261108.json` — Machine-readable three-scenario qualification evidence receipt for M15.223 (subsystem completion, root CFG integration, transfer ledger, 24/24 streams), validated via `ReceiptVerifier` across 58 check operations with status `VERIFIED` (M15.223).
- `src/tools/hybrid/m68k_02575c_root_step.hpp/.cpp` — Dedicated native stepper module for Root Entry `0x0254EA`, implementing Mode 0 path into dispatcher `0x02575C`, child motion helper `0x00E682` coordination, `134(a6) == 0` check, preflight decline of complex paths (Modes 1..5, motion stack pop, non-zero 134), and metrics tracking (3,192 calls, 15,960 native steps) (M15.224).
- `tools/generate_02575c_m15_224_receipt.py` — Automated receipt generator for M15.224, capturing all 48 stream artifacts across Scenarios A, B, and C with exact byte counts and SHA256 hashes (M15.224).
- `data/thor-evidence/02575c-three-scenario-qualification-m15-224-20261108.json` — Machine-readable three-scenario qualification evidence receipt for M15.224 (State 5 safe paths, Root Entry `0x0254EA` execution, 53,508 stepped instructions, 24/24 streams), validated via `ReceiptVerifier` across 58 check operations with status `VERIFIED` (M15.224).

- `src/core/sha256.hpp/.cpp` — Modular standard C++20 SHA-256 calculation utility decoupled from platform and emulator code (M16.0).
- `src/genesis/vdp_state.hpp/.cpp` — Explicit Sega Genesis VDP state representation (64 KiB VRAM, 128-byte CRAM, 80-byte VSRAM, 32 registers), table address calculations, and zero-dependency JSON serialization (M16.0).
- `src/genesis/vdp_rasterizer.hpp/.cpp` — Standalone native C++ VDP rasterizer implementing Genesis backdrop, Plane B, Plane A, Window plane, SAT sprite traversal (link order, column-major tiles, scanline limits), priority layering, and 9-bit color expansion (M16.0).
- `src/tools/standalone_first_pixel.cpp` — Headless standalone executable `oasis_standalone_first_pixel` with PPM output, frame SHA256 computation, and per-pixel reference comparison (M16.0).
- `tests/vdp_rasterizer_test.cpp` — Comprehensive unit test suite for native VDP state and rasterizer, including bit-exact Frame 779 and Frame 200 pixel validation (M16.0).
- `data/standalone-fixtures/frame_779_vdp_state.json` — Versioned VDP state fixture for Frame 779 (VRAM, CRAM, VSRAM, registers) (M16.0).
- `data/standalone-fixtures/frame_779_reference.rgb24` — Authentic reference RGB24 framebuffer for Frame 779 (SHA256: `fcfceb5833abef88a33c13339f2d40d38130e3ccda9b35699bd49a8d6125f690`) (M16.0).
- `data/standalone-fixtures/frame_200_vdp_state.json` — Versioned VDP state fixture for Frame 200 (VRAM, CRAM, VSRAM, registers) (M16.0).
- `data/standalone-fixtures/frame_200_reference.rgb24` — Authentic reference RGB24 framebuffer for Frame 200 (SHA256: `97287665eb8fc1e690fefeff5af4ea833b34d2384ffba2e110b5c493670d20f0`) (M16.0).
- `docs/SEMANTIC_MAP.md` — Navigational map of canonical Beyond Oasis ROM by subsystem with explicit evidence levels and recompilation statuses (M16.0).

- `src/game/render/vdp_dma_queue.hpp/.cpp` — Reconstructed Genesis `$FF10B4..$FF1892` DMA queue model (`DmaTransferEvent`, `VdpDmaQueue::drain_lifo`) with VDP R15/R19..R23 state updates (M16.38).
- `tests/m16_38_dynamic_pattern_dma_test.cpp` — Differential qualification suite for M16.37 metric reconciliation, F779 pattern/PlaneA/CRAM/VSRAM/register convergence, 14-snapshot Holdout+Canonical checks, PPM diff outputs, and 7 DMA/VDP negative mutations (M16.38).

- `tests/m16_39_sat_forensics_pixel_exact_test.cpp` — Differential M16.39 test verifying VDP `x==0` sprite masking, 100% bit-exact active frame convergence at Canonical `F779` (`0/71,680` pixels, `0/65,536` VRAM, `0/640` SAT), 14-snapshot VDP/pixel summary, and 4 M16.39 mutation guards (M16.39).

- `data/standalone-fixtures/town_canonical_continuous_f775_f785.json` — Pinned continuous 11-frame GPGX VDP state fixture (F775..F785) with full VRAM, CRAM, VSRAM, registers, and SAT chain (M16.40).
- `tests/m16_40_continuous_exact_prefix_test.cpp` — Differential qualification suite for 10-frame continuous exact prefix (F775..F784, 0 diff px, 0 VRAM, 0 CRAM, 0 VSRAM), F785 first-divergence quantification (1,352 px, 802 VRAM bytes), 14-snapshot regression matrix, and 4 mutation guards (M16.40).

- `data/standalone-fixtures/town_canonical_continuous_f775_f795.json` — Pinned continuous 21-frame GPGX VDP state fixture (F775..F795) with full VRAM, CRAM, VSRAM, registers, and SAT active chain (M16.41).
- `tests/m16_41_sword_attack_dma_test.cpp` — Differential qualification suite for 12-frame continuous exact prefix (F775..F786, 0 diff px, 0 VRAM diff), F785 exactness gate, zero Pattern RAM diff across all 21 continuous frames, earliest divergence identification at F787, 14-snapshot regression matrix, and 4 mutation guards (M16.41).
