# THOR M12 AUTO67 LIVE-MAP-1 — real runtime proof

- **Classification:** `PASS_LIVE_RUNTIME_MAP_GROWTH`
- **Baseline / code final:** `cad832fc7f5dfebde41695fdf8863db0679fc5b9` (experiment changed no production code)
- **Canonical ROM:** `local-roms/Beyond Oasis (USA).md`, SHA-256 `eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`
- **BizHawk:** `C:\Dev\SegaThorTools\BizHawk-2.11.1-win-x64\EmuHawk.exe`, 2.11.1 / GenPlus-gx
- **State:** `Beyond Oasis (U) [!].Genplus-gx.QuickSave1.State`
- **Lua:** `src/tools/thor_evidence/capture/live_capsule.lua`
- **Fresh DB:** `build/thor-evidence/live-map-1-pass-a.sqlite` (empty before PASS A)

## PASS A and replay

PASS A executed actual BizHawk CPU/Lua runtime. It produced 164425 Lua events, 1584 transport observations, 240 leases, 224 returns and 224 local chains. The Cartographer gate accepted 16 candidates and rejected 208 without accepted proof. Durable growth was **8 nodes / 7 proven edges**, with zero conflicts and one component; graph hash changed from `afa9da796f4e68d27173278ea962cfe3dc266ca06fc3304de534860d5e99977c` to `5707e3109cec076d1f82059973d196fbbc546f82e1ae97afc1b10acb8a4d3ddf`. Queue drops and write errors were both zero.

The first actual chain-step sample is preserved in ignored artifact `build/auto67-live-map-1-sample-chain.json`: occurrence `epoch=0:seq=1044`, window item `138`, investigation `INV-AUTO67-9e9d9d7ba2e011a10da2d5dc`, lease `L0000001A`, schema `oasis.m12.auto67.local-chain.v1`, one `REGISTER_REACHING_DEFINITION` step (`A4`, producer `0x0027BE`, consumer `0x0027D2`), complete interval and no intervening register write. Provenance resolved A4; A6 remains explicitly unresolved.

PASS B replayed QuickSave1 against the same DB: 31222 Lua events, 80 leases, 64 returns, 6 accepted chains, and **0 new nodes / 0 new edges / 0 promotions**, with unchanged graph hash. The same `BUS_WRITE_PC pc=0x0027D2 address=0xC00004` class remained observable (including exact prehistory joins), proving known map entries do not blacklist runtime observation.

## Layer and ring evidence

- Lua transport: bounded capacity 256; PASS A utilization/overwrites `256/1328`; Python RollingWindow capacity 256 with `1328` overwrites; `raw_event_backlog=0` (`NONEXISTENT`).
- Dispatcher: `duplicate_active_claims=0`, peak busy/working `16/16`, bounded worker returns, no future-PC task or branch suppression (`OBSOLETE_NO_SCHEDULING`).
- LiveMapSink: one queue, capacity `16384`, depth 0, drops 0, write errors 0; Cartographer writer produced the durable graph above.

## Where it slows

The stall is in BizHawk/Lua capture during `frameadvance`, not in Python dispatch or SQLite. PASS A frameadvance host time was p50/p95/p99/max `690000/750000/764999/912000` µs with `159210` `targeted_bus_exec` callbacks and `5891` targeted bus-write callbacks across `24007` target PCs. The no-capture control was p50/p95/p99/max `16000/17000/17000/24000` µs, with zero targeted callbacks: roughly **43x p50 overhead**. Python dispatch `T0→T8` p50 was `38` µs and Worker CPU was `1.36` s total. The actionable hotspot is the targeted hook registration/dispatch surface, especially the 24,007-PC execution target set; no architecture change was made.

## Ownership and validation

- `SOURCE_OWNED`: `1,475,600 → 1,475,600` (delta `0`); graph references did not promote bytes.
- Focused AUTO67/Dispatcher regressions: **109 passed**.
- Source-limit: **PASS** (`667` governed files, inventory `679`); the focused Debug CTest source-limit test passed. Debug/Release full CTest was not rerun because production files were unchanged; prior baseline validation remains authoritative.
- `git diff --check` passed; exact final GitHub Actions SHA is recorded after publication.

The one limitation is operational: PASS A was externally stopped at frame 496 after the measured callback stall; the runner still performed final status drain, `dispatcher.stop`, and `LiveMapSink.stop` and retained all runtime/map evidence. PASS B and the no-capture control exited cleanly with return code 0.
