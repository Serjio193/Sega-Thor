# Current Task

TASK: M11.19 Instrumented Genesis Plus GX live coverage + RL feasibility spike
WHY: prove whether one developer-only emulator core can support both human-driven Motorola 68000 coverage discovery and future headless reinforcement-learning workers before changing Sega-Thor's main reconstruction strategy.
CURRENT MILESTONE: M11.19 experimental discovery-core feasibility; M12 remains TODO and is not started.
SLICE MODE: RE_TOOLING_ONLY / EXTERNAL_CORE_SPIKE
STATUS: ACTIVE
BRANCH: experiment/gpgx-live-coverage
ISSUE: #3
BASELINE: M11.18 `NATIVE_VERTICAL_SLICE_PLAYABLE` on main; existing verified C++ translations remain intact and are not expanded in this spike.

MILESTONE UNDERSTANDING CONFIDENCE: 92%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 96% for the experiment boundaries and measurements; 0% claim that Genesis Plus GX is the final discovery core until all gates are measured.
SLICE CONFIDENCE EVIDENCE: current Genesis Plus GX exposes an M68K CPU hook path, Sega-Thor already owns external-trace/decoder/Atlas tooling, and Stable-Retro provides a plausible Genesis/Gymnasium route. Performance, determinism and integration are still empirical questions.

PRIMARY QUESTION:
Can an instrumented Genesis Plus GX core collect address-level executed-M68K coverage cheaply enough for normal human play, persist/merge that coverage into Sega-Thor's Atlas, and also support deterministic headless reset/state/action stepping for future RL workers?

GO/NO-GO ACCEPTANCE:
- [ ] TEST A: build baseline and instrumented Genesis Plus GX, capture each executed cartridge-ROM M68K instruction-start PC with bounded in-memory work, measure overhead, and prove deterministic normalized coverage for an identical state/input sequence.
- [ ] TEST B: show a persistent live 192x64 ROM coverage map (0x100-byte display cells over the 0x300000-byte canonical ROM) while a human plays for at least 10 minutes; report unique starts, session delta, 10 s / 60 s discovery rate, touched blocks and current PC.
- [ ] TEST C: feed newly executed PCs offline into the existing Sega-Thor decoder/Atlas, retain dynamic-vs-static provenance, follow only safe direct edges, and never invent indirect targets.
- [ ] TEST D: prove headless Genesis reset/action stepping, deterministic state restoration and at least four isolated workers using Stable-Retro or an equivalent Genesis Plus GX frontend; record steps/s and memory use before any claim about large-scale training.
- [ ] No ROM, extracted commercial assets or copyrighted savestates committed.
- [ ] No production gameplay C++ translation added during the spike.
- [ ] No production emulator dependency added to oasis/oasis_core.
- [ ] Full locally available build/test/file-limit/diff validation green before any implementation commit is pushed.

IMPLEMENTATION ORDER:
1. Test A only: establish exact upstream Genesis Plus GX revision and baseline performance.
2. Add the minimal hook-local coverage bitmap/counters in the external core; export only at frame/batch cadence.
3. Repeat the identical deterministic segment and make the Test A PASS/FAIL decision.
4. Only after Test A PASS, implement the Sega-Thor-side persistent coverage format and live viewer for Test B.
5. Only after Test B PASS, connect coverage to the existing decoder/Atlas for Test C.
6. Only after Test C PASS, test Stable-Retro/headless/reset/savestate/parallel workers for Test D.
7. Only after all four PASS, propose an ADR/roadmap migration. No early architecture switch.

PER-INSTRUCTION PERFORMANCE RULE:
The M68K callback may only update fixed-size in-memory coverage state/counters. It must not perform file I/O, JSON, disassembly, AI inference, UI work, heap allocation, cross-process IPC, locks, or console logging per instruction. Export/merge happens at frame or bounded batch cadence.

COVERAGE REPRESENTATION:
- exact instruction-start evidence is address-level and keyed by canonical ROM SHA-256;
- the 192x64 map is visualization only (one cell = 0x100 ROM bytes);
- do not use `touched cells / 12288` as `% code recovered` because total executable-code size is unknown;
- persistent state stores only derived execution metadata, not ROM bytes;
- session coverage and lifetime coverage are separate so the viewer can highlight what the current play session discovers.

EXPECTED DATA FLOW:

```text
Genesis Plus GX M68K hook
    -> local instruction-start bitmap + coarse counters
    -> once-per-frame/batched delta export
    -> Sega-Thor coverage collector
    -> persistent per-ROM coverage database
    -> live map/statistics
    -> offline decoder/static direct-edge expansion
    -> Atlas with explicit provenance
```

DEEP TRACE IS SEPARATE FROM NORMAL COVERAGE:
Normal human play records cheap PC coverage. A bounded deep-trace mode may later capture opcode bytes, register snapshots, branches/calls/returns and memory accesses for selected ranges/scenarios. Deep trace must never be the always-on per-instruction path used for long play sessions.

FUTURE RL BOUNDARY IF GO:

```text
same Genesis Plus GX core family
    -> headless reset/load-state
    -> controller action
    -> frame/observation/RAM
    -> reward/termination integration
    -> parallel isolated workers
```

The gameplay-learning reward and RE-exploration reward must remain separate. A future RE explorer may reward newly executed code/basic blocks/control-flow targets, but no AI training is part of M11.19.

GO RESULT:
Only if A+B+C+D pass, create a separate ADR/roadmap change proposing `human/AI execution -> persistent 68000 Atlas -> saturation -> later bulk C++ translation`. Existing native C++ work is retained as verified reference/regression material.

NO-GO RESULT:
If a critical gate fails, record the measured reason and test the next emulator candidate instead of forcing this architecture.

EXACT NEXT ACTION:
On a local development machine, build an unmodified current Genesis Plus GX libretro core and an otherwise identical `HOOK_CPU` instrumented build. Run the same fixed Beyond Oasis segment, capture baseline/instrumented frame-time and normalized instruction-start coverage, and stop at the Test A PASS/FAIL decision before implementing the live viewer.

LOCAL-VALIDATION RULE:
Do not push implementation code for this experiment until it has been built/tested locally according to AGENTS.md. GitHub Actions is a post-push gate, not the first compiler. Documentation-only task setup may live on the experiment branch before Test A.

DO_NOT_WORK_ON:
M12 inventory/UI/save, new room renderer, new gameplay systems, broad C++ semantic translation, production emulator integration, RL training, random-input swarm, or any architecture migration before Test A-D produce a GO result.


HISTORICAL CHECKPOINTS:

The prior M11.18 and earlier task records remain in Git history and project documentation. `main` remains the authoritative completed M11.18 baseline while this experiment is evaluated.
