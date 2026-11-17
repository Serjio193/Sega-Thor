# Standalone Handoff: M15.307 — WRAM Entity & Effect Pool Allocators (`0x00D9BE`..`0x00DA60`)

## Summary
Beyond Oasis allocates runtime entities and effects across **4 fixed-stride WRAM pools** (scanning for `word[+0] == 0`):
1. `0x00D9BE` (`effect_pool_8`): `$FF2ABC`, `8` slots, stride `0x5A` (`90`B), assigns VRAM tile `0x694 + idx * 9`.
2. `0x00D9F0` (`special_pool_6`): `$FF2D8C`, `6` slots, stride `0x5A` (`90`B).
3. `0x00DA1A` (`full_entity_pool_18`): `$FF1C1C`, `18` slots, stride `0xBC` (`188`B).
4. `0x00DA2A` (`enemy_entity_pool_17`): `$FF1CD8`, `17` slots, stride `0xBC` (`188`B); clears `$FF184F` if the reused slot matches `$FF1850`.

## C++20 Standalone Target
- Implement `EntityPoolAllocators` in `src/game/entities/` with exact capacities (`8, 6, 18, 17`), VRAM tile step (`0x694 + idx * 9`), and `$FF184F` stale-HUD invalidation.
- Evidence artifact: `data/thor-evidence/rom_entity_pool_allocators_m15_307.json`.
