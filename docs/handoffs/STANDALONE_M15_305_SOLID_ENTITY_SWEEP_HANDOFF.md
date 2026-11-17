# Standalone Handoff: M15.305 — 3D AABB Solid Entity Sweep & Push (`0x009DD6`..`0x009EB8`)

## Summary
Solid entity-vs-entity separation (`0x009DD6`) sweeps along `+X` across all `21` active entity slots (`$FF19E8..`, stride `0xBC`), skipping non-solid entities (`$3A` bit 1 or `$38` bit 2), testing 3D AABB overlap in `(x +/- $42, y + floor_z +/- $44, z + floor_z .. +$4A)`. On contact it clamps `dx` to clearance, sets `$40` bit 7, records `$68 = blocker_ptr`, sets a `3`-frame push timer (`$6C = 3`), and nudges pushable blockers by `+1px` when clearance is `0`.

## C++20 Standalone Target
- Implement `sweep_solid_x_pos()` and `3`-frame push-contact state in `src/game/entities/`.
- Evidence artifact: `data/thor-evidence/rom_solid_entity_sweep_m15_305.json`.
