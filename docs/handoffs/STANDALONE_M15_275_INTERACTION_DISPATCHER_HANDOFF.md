# STANDALONE M15.275 UNIVERSAL INTERACTION DISPATCHER HANDOFF

**Milestone:** SEGA-THOR HYBRID — M15.275  
**Repository:** `C:/Github/Sega-Thor`  
**Standalone Repository (`C:/Github/Sega-Thor-Standalone`):** STRICTLY READ-ONLY / UNTOUCHED  
**Machine-Readable Evidence Output:** `data/thor-evidence/rom_interaction_dispatcher_m15_275.json`  
**Extraction Tool:** `tools/extract_rom_interaction_dispatcher.py`  
**Deterministic Test Suite:** `tests/rom_interaction_dispatcher_test.py`

---

## 1. Universal Interaction Dispatcher (`0x008330..0x0083D2`)

- **Gate:** `16` px proximity (`0x00B852`) + facing check (`0x00E338`).
- **Routes by Entity Type (`$0(A0)`):**
  - `0x0032` -> `0x018BBC` (Town NPCs / Village Elder)
  - `0x0020` -> `0x018CAA` (Spirit Awakening Dispatcher)
  - `0x0094`, `0x0096`, `0x00BE` -> `0x018EBE` (Story NPCs, message `$96(A0)`, post-dialogue hooks `0x24`, `0x2F`, `0x30`, `0x39`)
  - `0x00B0` -> `0x00839E` (Signpost / Tablet: SFX `0xA00264`, freeze `0x3F`, `0x00C2EC`)
