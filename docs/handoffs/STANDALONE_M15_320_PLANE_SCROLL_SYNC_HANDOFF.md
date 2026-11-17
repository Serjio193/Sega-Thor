# Standalone Handoff — M15.320: Plane Scroll Delta & VDP H/V-Scroll Buffer Sync (`0x00D188..0x00D342`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x00D188..0x00D264`: `update_plane_scroll_deltas`
  - `0x00D266..0x00D2B2`: `wrap_camera_to_map_bounds`
  - `0x00D2B4..0x00D342`: `flush_hscroll_to_vdp_buffer`
- **Evidence JSON:** `data/thor-evidence/rom_plane_scroll_sync_m15_320.json`
- **Python Extractor & Tests:** `tools/extract_rom_plane_scroll_sync.py`, `tests/rom_plane_scroll_sync_test.py`

## Behavior
1. Computes `dx = target_x - cam_x` and `dy = target_y - cam_y`.
2. When `!(0x27(a0) & 2)`: computes tile crossings `(target >> 3) - (old >> 3)` into `pending_dx_tiles` (`0x1C`) / `pending_dy_tiles` (`0x1E`), setting dirty bits `0`/`1`.
3. Updates VDP H-scroll (`(hscroll - dx) & 0x1FF`, dirty bit `2`) and V-scroll (`(vscroll + dy) & 0xFF`, dirty bit `3`).
4. Guards `cam_x`/`cam_y` against `[-10000, +10000]` overflow by adding/subtracting `2 * map_px_size`.
5. `flush_hscroll_to_vdp_buffer` writes `vdp_hscroll` to `$FFFB5C` (per-line / per-cell) or `$FF1344` (full-screen), offset `+0` for Plane A (`$FF1716`) or `+2` for Plane B.
