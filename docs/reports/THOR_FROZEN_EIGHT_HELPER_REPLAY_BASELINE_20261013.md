# Frozen eight-helper replay baseline (2026-10-13)

## Baseline identity

The immutable development baseline ID is `THOR_REPLAY_NATIVE_ISLAND_EIGHT_V1_20261013`. Its ROM, GPGX core, Debug and Release runner, configuration, initial-state, effective-input and canonical-hash identities are recorded in the paired JSON receipt. The canonical state hash uses the existing checkpoint identity routine: the supported `GENPLUS-GX 1.7.6` savestate representation is copied, the layout-declared host representation spans are zeroed, then SHA-256 is computed. Raw serializer hashes remain diagnostic only.

The frozen native set is exactly `{0x9D70, 0x9D4E, 0x9DB4, 0x9D92, 0x94A2, 0x9C40, 0x8EF8, 0xE268}`. Each entry retains its accepted caller gate and contract ID. Any helper-set change requires a new baseline ID and a new full replay acceptance; no helper silently joins or leaves this set.

## Reproducibility

Two Debug and two Release replay runs completed 3,600 frames each. All four matched the pinned reference artifacts: canonical frame state, 60 periodic hashes, 128 architectural and scheduler sync records, Z80 RAM/state, normalized helper events, external event order, and effective input polls. The first differing frame was reported as 0 by the comparator, with zero differing frames. Native counts and per-frame native-call count hashes were identical across all four runs.

Each run recorded 45,968 native calls and 410,911 ROM fallbacks from 456,879 helper target entries. There were 410,911 provenance rejects, zero precondition rejects, zero contract rejects, and zero native mismatches. These are the observed pinned-fixture counts, not predictions for other sessions.

E268 supplied the available per-helper scheduler counters and they matched in all four runs: 73 IRQ pause/resume pairs, 61,583 refresh events, 45,046 refresh-variant calls, and 15,825 scheduler resumes. The current runner does not emit equivalent IRQ/refresh counters for the other seven helpers. Those fields are marked `NOT_INSTRUMENTED_PER_HELPER` in the receipt and are not interpreted as zero.

## Build and repository gates

A fresh CMake configuration and fresh Debug and Release `oasis_gameplay_session` builds passed in `build/frozen-island-debug-clean-20261013`. The four replay runs used the current identified Debug/Release runners. Focused Debug and Release tests passed 6/6 in the prior validation for this implementation state; they were not rerun in the fresh build directory during this freeze task.

This was a fresh build directory, not a clean Git checkout. The source checkout is dirty and contains uncommitted implementation state, so clean-checkout reproducibility remains unproven. The island implementation files checked for this baseline satisfy the 500-line source limit. The repository-wide gate remains blocked by three pre-existing files: `src/tools/re_slice_decoder.cpp` (506 lines), `src/tools/thor_evidence/audio_mode1_structural_closure.py` (519), and `src/tools/thor_evidence/helper_candidate_index.py` (518). The global diff check also encounters line-ending noise in the already dirty tree. No commit or push was made because the project pre-push validation is not green.

## Performance

These are full instrumented replay wall times, not isolated stepper benchmarks. Debug ROM-only replay took 43.46 seconds; the two island runs averaged 46.505 seconds, about 7.01% higher. Release ROM-only took 12.17 seconds; island runs averaged 12.715 seconds, about 4.48% higher. The island averaged about 988 native calls/second in Debug and 3,617 in Release. E268 accounted for about 969 and 3,542 calls/second respectively. Instrumentation and replay I/O are included, so the values are diagnostic.

## Scope and recommendation

`REPLAY_NATIVE_ISLAND_VALIDATED=YES` applies only to the pinned 3,600-frame development session. All eight helpers remain path-specific; global helper verification is NO, production runtime activation is NO, and full natural CFG coverage is NO where not established. Runtime, structural, SOURCE_OWNED and canonical SQLite authorities are unchanged.

Defer a production-grade hybrid execution layer. The island handles 10.06% of this replay's helper target entries; 89.94% remain ROM-backed, mostly because E268 calls from unaccepted contexts correctly fall back. Further development-only coverage of already verified caller paths, paired with better per-helper IRQ/refresh telemetry, is the better next step. This mission freezes the baseline and implements neither expansion nor production integration.

Machine receipt: [THOR_FROZEN_EIGHT_HELPER_REPLAY_BASELINE_20261013.json](THOR_FROZEN_EIGHT_HELPER_REPLAY_BASELINE_20261013.json).
