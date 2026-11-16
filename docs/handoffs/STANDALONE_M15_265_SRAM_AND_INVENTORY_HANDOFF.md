# STANDALONE M15.265 SRAM SAVE/LOAD & PERSISTENT INVENTORY HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.265  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_sram_and_inventory_m15_265.json`  
**Extraction Tool:** `tools/extract_rom_sram_and_inventory.py`  
**Deterministic Test Suite:** `tests/rom_sram_and_inventory_test.py`

---

## 1. SRAM Slot Architecture (`0x001182..0x001298`)

- **Control Register:** `$00A130F1` (`0x03` = unlock SRAM, `0x02` = lock SRAM).
- **4 Save Slots (odd-byte SRAM, primary + mirror backup, `0x4B8` SRAM bytes each):**
  - Slot 0: Primary `$200009`, Backup `$2012E9` (`$FF10AA` bit 0)
  - Slot 1: Primary `$2004C1`, Backup `$2017A1` (`$FF10AA` bit 1)
  - Slot 2: Primary `$200979`, Backup `$201C59` (`$FF10AA` bit 2)
  - Slot 3: Primary `$200E31`, Backup `$202111` (`$FF10AA` bit 3)
  - Last selected slot: `$2025C9` -> `$FF10A8.W` (clamped `<= 3`).
- **Validation (`0x00129A`) & Repair (`0x0012E8`):**
  - 6-byte magic signature at ROM `0x001754`: `1A BC 00 02 2A 4E`.
  - **596-byte (`0x254`)** payload copied from/to `$FF0BFE..$FF0E51`.
  - 16-bit unsigned byte-sum checksum stored immediately after the 596-byte payload. If Primary fails and Backup passes, `0x0012E8` repairs Primary from Backup.

---

## 2. Persistent WRAM Block (`$FF0BFE..$FF0E51`, 596 Bytes)

Everything saved/loaded across sessions lives inside `$FF0BFE..$FF0E51` (cleared on New Game at `0x001300`):
- `$FF0D7A`: Active/unlocked spirit bitmask (bits 0..3: Dytto, Efreet, Shade, Bow)
- `$FF0D7E`: Power/Score word (copied to Player `+$06`)
- `$FF0D80`: Equipment/status word (`0x01FF` on init)
- `$FF0D82`: **7 Weapon Slots** (2 bytes each: `(weapon_id << 8) | ammo_or_0xFE`, e.g., `0x05FE` Omega Sword, `0x07FE..0x0CFE` Bows/Bombs)
- `$FF0D90`: **16 Item/Consumable Slots** (2 bytes each: `item_id, count`, IDs `0x1A..0x28` + `0x2A`)
- `$FF0DB6`: 4 Spirit Level bytes (`0x0F0F0F0F`)
- `$FF0DBA`: Spirit slot flags (bits 0..3)
- `$FF0DBC`: Global Story Flags (32 bytes)
- `$FF0E1C`: Area Flags (16 bytes, cleared on area change)
- `$FF0E2C`: Room Flags (16 bytes, cleared on room change)
- `$FF0E40`: Current Room ID (`0xMMRR`)
