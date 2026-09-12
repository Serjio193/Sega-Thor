# M12 controlled runtime root capture — neutral causal result (2026-09-12)

Baseline: `origin/main = 45ec16a`.

This is a developer-only, one-frame runtime capture using the now-verified
BizHawk process harness. It does not perform RE address discovery, create a
savestate, write guest RAM/registers, run a natural replay, or promote ROM
bytes. The baseline launcher was run immediately before extending the probe;
it returned `result=PASS`.

## Question and bounded method

The caller-join report ranked a controlled `A196 -> A342 -> A372` register/root
capture as the highest information fork. The exact external ROM and QuickSave1
were SHA-256 checked by `run.ps1`. Lua loaded the state explicitly with
`savestate.load(EXACT_STATE_PATH, true)`, required frame `2117`, installed
execution hooks at `A196`, `A342`, `A372` and a write hook at `FF1858`, settled
three neutral frames, then advanced exactly one frame with either:

```lua
joypad.set({ Right = true }, 1)
```

or an all-false controller table. The probe recorded only frame, D2, D5, A5,
PC, `FF1858`, `FF188A`, `FF188C`, Right controller state, the four SAT bytes,
and callback counts.

## Result

Both arms entered `A196` and `A342` once and `A372` six times on frame `2120`.
Every A372 event had the same values in both arms:

```text
FF1858=00, FF188A=0000, FF188C=0000
A5=FF13CC, FF13D4, FF13DC, FF13E4, FF13EC, FF13F4
D2=00880901, 00980302, 00B80203, 00D00904, 00E00305, 01000206
D5=1, 2, 3, 4, 5, 6
```

The Right arm reported `Right=true` at the A196/A342/A372 hooks; the neutral
arm reported `Right=false`. No `FF1858` write occurred in either arm. After
the one frame, both arms were at frame `2121` with:

```text
SAT      = 00 88 09 01
FF1858   = 00
FF188A   = 0010
FF188C   = 0080
events   = 16 (A196 2, A342 2, A372 12 including the settle frame)
```

The normalized event traces are identical after removing the controller-state
field. This independently confirms that the controlled Right arm and neutral
arm execute the same producer path and state transitions for this frame.

## Evidence boundary

Proven:

- The restored harness reaches the controlled `A196 -> A342 -> A372` path.
- `FF1858=0` selects the default root in this state.
- `A5` begins at `FF13CC`; six finite records are produced in the first loop.
- The producer updates `FF188A` to `0x0010` and `FF188C` to `0x0080` by frame end.
- The existing source-side SAT oracle is reproducible without UI automation.

Not proven:

- Right causes the A372 execution, FF13CC write, or counter updates.
- The game polled Right on this frame; `emu.islagged()` remains true and the
  prior probe observed no input-poll callback.
- The produced records represent a named object, animation, frame or sprite.
- Same-frame SAT VRAM publication; the four-byte shadow source is the observed
  change, while VRAM remains outside this probe.

No `SOURCE_OWNED` promotion is justified. The static producer and scheduler
joins remain the authoritative structural evidence; this runtime pass supplies
bounded values and a negative causal control.

## Re-ranked M12 forks

1. **Static FF1858/FF188A/FF188C access closure — highest.** The controlled
   values now identify a concrete default-root path and post-call counters.
   The existing direct absolute census has 12 references: two producer readers,
   one `SF.B` zero writer, and nine `ST.B` `0xFF` writers. Close only caller
   boundaries that have exact ROM contracts; retain unresolved indirect/local
   paths as blockers. No semantic labels or promotion follow automatically.
2. **Static `0x03BDA6` consumer boundary — medium.** The table grammar is
   proven, but the `0x03BDD8` stream remains unterminated before code boundary
   `0x03BF86`. This is independent of the now-closed controlled root path.
3. **Input causality runtime fork — currently blocked/low value.** The exact
   QuickSave1 frame is lagged and has no input-poll callback; repeating Right,
   neutral, or direction arms cannot establish causality. A future attempt needs
   a separately validated state/frame with an observable input poll, with no
   state fabrication or long replay.

The first fork is the next autonomous M12 direction. It is a static consumer
closure, not another equivalent runtime replay. M13, broad C++ migration,
manual gameplay and RAM-forced experiments remain out of scope.

Artifacts are local and ignored under
`build/bizhawk-controlled-harness/root-right` and `root-neutral`. Original ROM
and QuickSave1 hashes remained unchanged.
