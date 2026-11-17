# Standalone Handoff: M15.293 — Checkpoint Snapshot & Retry Restore (`0x0017DA`..`0x001900`)

## Summary
Beyond Oasis maintains an in-RAM checkpoint snapshot for seamless Game-Over Retry:
- **Snapshot (`0x0017DA`, skipped in demo `$FF1996 != 0`):** Records `(room_id, cam_xy, cam2, ali_xy)` at `$FF0E42`, copies `0xBC` words (`376` bytes = Ali `$FF19E8` + Spirit `$FF1AA4`) into `$FF0BFE`, and copies `0x12A` words (`596` bytes) into `$FF0E52`.
- **Retry Restore (`0x001828`):** Preserves `$FF0D76`, `$FF1A7E`, and `$FF1A70` (`MaxHP`), restores `$FF0E52 -> $FF0BFE -> $FF19E8`, sets **`HP = MaxHP`**, reloads spirit resources (`0x014522`), increments continue counter `$FF0E3E += 1`, and grants `30` (`0x1E`) invulnerability frames (`$FF198E`).

## C++20 Standalone Target
- Implement `CheckpointRetryManager` in `src/game/` matching exact snapshot/restore preservation and `HP = MaxHP` + `30`-frame invulnerability semantics.
- Evidence artifact: `data/thor-evidence/rom_checkpoint_retry_m15_293.json`.
