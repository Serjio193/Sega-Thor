# AUTO67 operator test and capture optimization

Date: 2026-09-13. Baseline: `124c80c0a1c3cef95f7d646f3a2c35ceae472b74`.

**Manual acceptance: FAIL (operator-reported severe slowdown).**
**Sparse-capture performance experiment: PASS; full causal chains: UNPROVEN.**

The manual/profile run lasted 122.36 s; its 16 workers issued and returned
4,268 leases, with peak busy 16 and raw backlog zero. The forced close used
in that earlier diagnosis prevented Lua's final receipt; the surviving status
is a last observation, not a sealed full-session capture. No whole-session
duplicate-claim metric was recorded, so the JSON reports null rather than
inventing zero. The earlier AUTO67 acceptance report is superseded.

## Performance experiment

All comparison runs used the canonical ROM, reset startup, 16 configured
workers, a 256-event rolling window, and no injected demo inputs. Times include
emulator startup, throttling and shutdown. These are actual BizHawk runs, not
synthetic worker transitions; they are not a completed manual gameplay retest.

| Run | Frames | Wall seconds | Bus callbacks | Final UI bytes |
| --- | ---: | ---: | ---: | ---: |
| Original code | 1,800 | 63.453 | 751,620 | 933,936 |
| Burst capture only | 1,800 | 33.234 | 3,648 | 202,972 |
| Burst + bounded publisher | 1,800 | 33.219 | 3,648 | 73,368 |
| Capture-disabled control | 1,800 | 33.250 | 0 | 5,749 |
| Verified burst repeat | 1,800 | 32.719 | 3,648 | 73,320 |
| Final opt-in burst CLI | 1,800 | 32.766 | 3,648 | 73,376 |

In the verified repeat, frames 465 to 1065 advanced in 10.0132434 seconds:
**59.9206 FPS**. Process CPU deltas divided by that same wall interval were
23.25% of one core for EmuHawk and 2.185% for Python. This replaces the invalid
earlier comparison of cumulative CPU times from different process lifetimes.
The small difference from the disabled control is not evidence of negative
capture overhead; timings include startup and ordinary run variation.

## Changes and cost

The existing launcher and dispatcher remain in use. `--capture-mode burst`
installs the broad write hook once per 30 frames, removes it after at most 64
callback entries or the frame boundary, and keeps per-frame PC observations.
This reduces callback entry overhead instead of merely discarding data after
crossing into Lua. Hook removal succeeded on the installed BizHawk 2.11.1/GPGX.
The [upstream event API](https://github.com/TASEmulators/BizHawk/blob/2.11.1/src/BizHawk.Client.Common/lua/LuaHelperLibs/EventsLuaLibrary.cs)
and installed Lua documentation specify callback registration/removal.

**Burst is opt-in, because skipped writes can break a causal chain.** Default
`continuous` preserves the previous every-16th-write policy, which was already
lossy. A final default-mode check completed 600 frames in 19.937 s with 177,663
callbacks and 1,180 leases/returns. This mode is still costly. Neither mode
claims complete write coverage (`causal_chain_complete=false`). Burst also
biases observation toward the first writes in each selected frame.

The existing dashboard now reads a cached byte snapshot produced at most four
times per second. It includes only 16 recent investigations and the last 24
transitions per worker; at 16 workers that is at most 384 transitions. It
excludes the raw Lua event ring. Disk publication runs separately, uses an
atomic replacement, and drops a status update on contention/I/O failure. A
slow browser cannot hold the dispatcher lock or stall the worker pool. The UI
renders observed history with chain identity, not static example transitions.
Full investigation records remain in dispatcher memory/final evidence; this
repair bounds UI work, not the entire lifetime knowledge store.

## What the real worker evidence proves

The final opt-in run recorded 556 leases and 556 returns, peak busy 16 and
peak **WORKING** 16, with all workers IDLE at close. It observed 5,448 events,
5,192 producer-window overwrites and raw backlog zero/nonexistent. It rejected
4,783 already-covered observations and merged 109 active collisions before
dispatch. KNOWN worker returns: 0; MERGED worker returns: 0. A pre-dispatch
merge is not a worker return.

The accompanying [bounded JSON proof](THOR_M12_AUTO67_LIVE_OPERATOR_TEST.json)
contains 13 consecutive recorded transitions for each of W0, W1, W2 and W3:
IDLE followed by three complete LEASED → WORKING → RETURNING → IDLE cycles
with distinct chain fingerprints. These prove pool reuse for unresolved
observations. Average/max recorded seed age was 0.002671/0.016 seconds, but
that clock begins at dispatcher admission and does not measure the complete
gameplay-to-lease age; freshness acceptance remains incomplete.

The worker implementation does not invoke AUTO65's full static/provenance
engine: it records a live event as BOUNDED_UNRESOLVED. Its branch/edge counters
are not a validated causal graph. Therefore this experiment cannot prove the
original requirement of full chain investigation, KNOWN/MERGED worker return
paths, or ownership promotion. No fake resolution was supplied to the live
sampler. The report JSON contains source artifact hashes and separates the
failed manual session from these benchmark histories.

## Validation and remaining work

AUTO65/66/67 unit tests: 15 passed. Windows Debug (MSVC) and Release (MinGW)
builds plus CTest: 186/186 each. The final AUTO67 changes were rechecked in both
configurations, and its 8 tests also passed with WSL Python. No CMake, native
linking or production-runtime changes were made; a new GNU/Linux C++ build
and a new full-ROM assembly reconstruction were not needed for this patch.
File-limit and diff checks are run before publication. Browser visual review
was not completed because the bounded launcher had already exited; actual
bounded JSON history, rather than screenshots, is the retained evidence.

To prove a causal chain, the RE engine needs a complete, explicitly bounded
dependency capture, including relevant writers/readers, initial state and
handoff context, or a reproducible replay with validated state. Sparse
discovery can select such an investigation but cannot substitute for it.
That integration and a new manual responsiveness/causal-evidence acceptance
remain outstanding. No AUTO68 or M13 work was started.
