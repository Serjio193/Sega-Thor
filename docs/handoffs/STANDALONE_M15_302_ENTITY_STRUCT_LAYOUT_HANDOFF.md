# Standalone Handoff: M15.302 — Entity Struct Constructor & `0x03F2FA` Field Map (`0x008D06`)

## Summary
The universal entity constructor (`0x008D06`) zeroes `+0x02..+0xBB` (`0x5D` words for `< $FF2954`, `0x2C` words for lightweight slots `>= $FF2954`) and unpacks the 32-byte master descriptor at `0x03F2FA + (entity_id - 2) * 16` into the `0xBC`-byte entity struct:
- `0x00` (`u16`) -> `+0x4A` (`height_z`), `0x02` (`u8`) -> `+0x46/+0x42/+0x44` (`radius_extent`), `0x03..0x04` -> `+0x48/+0x49` (`hitbox_half_w/h`), `0x05` -> `+0x3D` (`weight`), `0x06` -> `+0x4C` (`shadow_z`), `0x07..0x0B` -> `+0x3A/+0x36/+0x37/+0x38/+0x3C` (flags), `0x0C..0x18` -> `+0x1A/+0x1E/+0x26/+0x22` (tile/frame/anim/hitbox ptrs), `0x1C` -> `+0x5E` (secondary AI ptr, `< $FF2954` only).

## C++20 Standalone Target
- Implement `EntityDescriptor32` & `init_entity_from_descriptor()` in `src/game/entities/`.
- Evidence artifact: `data/thor-evidence/rom_entity_struct_layout_m15_302.json`.
