# Standalone Handoff — M15.333: Master Y-Sorted SAT Render Pipeline (`0x00A196..0x00A27E`)

## Summary
- **Source routine in Beyond Oasis (USA):** `0x00A196..0x00A27E`
- **Evidence JSON:** `data/thor-evidence/rom_ysort_sat_pipeline_m15_333.json`
- **Python Extractor & Tests:** `tools/extract_rom_ysort_sat_pipeline.py`, `tests/rom_ysort_sat_pipeline_test.py`

## 5 Render Passes (Front-to-Back in Mega Drive SAT)
1. High-priority FX (`$FF2954`, 4 slots, stride `0x5A`)
2. Hit-sparks (`$FF19C8`, 4 slots, stride `0x08`)
3. 21 Entities (`$FF19E8`, stride `0xBC`) sorted by **descending Y** (`0x0C(a6)` > `-1`)
4. Ground/shadow FX (`$FF2ABC`, 8 slots, stride `0x5A`)
5. Floor FX (`$FF2D8C`, 6 slots, stride `0x5A`)
