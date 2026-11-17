# Standalone Handoff — M15.386: ROM Ali Item Pickup Dispatcher, Inventory Storage, Jewel Upgrades & Full-Bag Audio Rejection (`0x007A28..0x007C3C`)

## Summary
- **Source routines in Beyond Oasis (USA):**
  - `0x007A28..0x007A68`: `ali_item_pickup_dispatcher` (categorizes item ID in `$FF1976`: weapons `0x01..0x0C`, jewels `0x0D..0x10`, rank upgrade `0x11`, spirit jewels `0x12..0x15`, keys `0x16..0x19`, food/consumables `0x1A..0x3F`).
  - `0x007A6C..0x007AF0`: `ali_item_consume_sfx_classifier` (reads classification table `0x05CE56.l`; maps category 3 to SFX `#$508`, `state = 0x44, anim = 0x4A`; category 4 to SFX `#$50D`, `state = 0x44, anim = 0x4A`; category 0 to `#$C0035F`; category 1 to `#$C00360`; category 2 to `#$C00361`; clears mask bits in `$FF17B8` and returns to idle/walk).
  - `0x007AF0..0x007B28`: `ali_chest_collection_bit_recorder` (records persistent chest/item collection state: table `$FF0DBC` for map 1, `$FF0E1C` for map 2, `$FF0E2C` for map 3; sets bit `d0 & 7` at byte offset `d0 >> 3`).
  - `0x007B64..0x007B82`: `ali_weapon_inventory_storage` (scans 8 16-bit weapon slots at `$FF0D80`; stores item into first free slot `[a0] == 0`; if all 8 slots are occupied, branches to `0x007C20`).
  - `0x007B84..0x007BA2`: `ali_food_inventory_storage` (scans 16 16-bit food slots at `$FF0D90`; stores item into first free slot `[a0] == 0`; if all 16 slots are occupied, branches to `0x007C20`).
  - `0x007BA4..0x007BD2`: `ali_spirit_jewel_tracker` (increments spirit jewel level in `$FF0DB6 + (id - 0x12)` clamped to 15; if spirit active `$FF1A74 != 0`, boosts current and max SP capacities by +5).
  - `0x007BD4..0x007BE6`: `ali_gem_counter` (increments gem count at `$FF0DB2 + (id - 0x0D)`).
  - `0x007BE8..0x007BF4`: `ali_key_bits` (sets key bit `(id - 0x16)` in persistent bitmask `$FF0DBA`).
  - `0x007BF6..0x007C1E`: `ali_rank_upgrade` (checks level at `$FF1A7E` < 200; increments level; updates max HP `$FF1A70 = level * 2 + 0xC6`; adds +2 to max SP `$FF1A72`).
  - `0x007C20..0x007C3C`: `ali_full_bag_audio_reject` (sets rejection state `state = 0x3C, anim = 0x44`, plays full bag buzzer error SFX `#$C00255`).
- **Evidence JSON:** `data/thor-evidence/rom_ali_item_pickup_inventory_m15_386.json`
- **Python Extractor & Tests:** `tools/extract_rom_ali_item_pickup_inventory.py`, `tests/rom_ali_item_pickup_inventory_test.py`
