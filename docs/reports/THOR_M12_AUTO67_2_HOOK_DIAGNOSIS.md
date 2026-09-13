# HISTORICAL — THOR M12 AUTO67.2 hook diagnosis

Authoritative final checkpoint: `THOR_M12_AUTO67_3_FINAL_CANONICAL_CHAIN_IDENTITY_CHECKPOINT.md`.

Date: 2026-09-13
Scope: targeted BizHawk capture and continuous worker hunting only. No AUTO68.

## Finding

The original 142 ms frame was not reproduced by the instrumented 600-frame
real run. The evidence does not show an individually expensive callback or a
registration/unregistration hitch: targeted registration reached 1 ms,
discovery registration 8 ms, and callback self-time was at most the profiler's
1 ms clock quantum per measured frame. The dispatcher remained at the already
measured 1.176 ms maximum. Therefore the 142 ms event has no proven exact
cause in this run and is not attributed speculatively.

## Focused-capture scaling

These are identical real BizHawk 300-frame runs with hook telemetry and native
operator window. Frame p95/p99/max are milliseconds.

| Focused slots | Frame p95 | Frame p99 | Max | >33 ms | >50 ms | Callback/frame p95 |
|---:|---:|---:|---:|---:|---:|---:|
| 0 | 17 | 22 | 25 | 0 | 0 | 0 |
| 1 | 17 | 21 | 23 | 0 | 0 | 65 |
| 2 | 18 | 22 | 29 | 0 | 0 | 130 |
| 3 | 18 | 22 | 26 | 0 | 0 | 195 |
| 4 | 18 | 22 | 35 | 1 | 0 | 195 |

The four-slot run is the only 300-frame run crossing 33 ms. No run crossed
50 ms or 100 ms. The default is therefore one focused slot while the other
workers continue hunting and analyzing.

## Final real validation

`build/auto67-hook-profile-hunt-1-600-v4.json` ran 600 real frames in 12.797 s
with 16 prestarted workers, one focused slot, 323 leases and 323 returns, peak
busy 16, 1,217 hunt attempts, 168 hunt-success assignments after returns, 286
capture-slot waits, and one completed/frozen focused capture. The native
operator window was launched with BizHawk. Raw backlog was `NONEXISTENT` and
active hooks at close were zero. The run had no frame over 33, 50, or 100 ms;
largest frame was 26 ms.

The operator proof showed two workers working, a worker returning to idle, a
known/merge result, and an active rolling window. Slot exhaustion did not stop
workers: candidates continued through `WAITING_CAPTURE_SLOT`/`QUICK_CHECK` and
returned to the pool.

## Hook inventory and filtering

| Class | BizHawk API | Filter/scope | 600-frame calls | Registration max |
|---|---|---|---:|---:|
| discovery bus write | `event.on_bus_write` | global (`nil`), burst-budgeted | 1,152 | 8 ms |
| targeted bus write | `event.on_bus_write` | native address argument | 1,048 in the 4-slot diagnostic run | 1 ms |
| targeted bus exec | `event.on_bus_exec` | native PC argument | 7,419 in the 4-slot diagnostic run | 1 ms |
| frame advance | `emu.frameadvance` | host operation, not a callback | 600 | n/a |

Targeted filters are installed in BizHawk's registration call. The discovery
hook is intentionally global and is limited to 64 callbacks per burst. The
profiler keeps only bounded callback samples and worst-frame records; it does
not create a raw-event backlog.

Registration and unregistration timing in the final one-slot run was:

| Hook class | Register min/p50/p95/p99/max us | Unregister min/p50/p95/p99/max us |
|---|---:|---:|
| discovery bus write | 0 / 0 / 7000 / 7000 / 7000 | 0 / 0 / 2000 / 2000 / 2000 |
| targeted bus write | 0 / 0 / 0 / 0 / 0 | 0 / 0 / 0 / 0 / 0 |
| targeted bus exec | 0 / 0 / 0 / 1000 / 1000 | 0 / 0 / 0 / 1000 / 1000 |

Callback class totals in that run were discovery bus write 1,152 calls /
16,000 us, targeted bus write 144 calls / 7,000 us, and targeted bus exec
3,585 calls / 2,000 us. Their calls-per-frame p95/max were 0/64, 1/1, and
65/65 respectively. The Lua `os.clock()` source has approximately 1 ms
resolution on this host, so sub-millisecond callback percentiles are reported
as zero rather than treated as exact nanosecond measurements.

The continuous-hunt correction is now active: focused capture is optional for
a lease, free workers hunt the current rolling context, and a busy focused
slot does not serialize the remaining worker pool. Sampling remains lossy and
must not be interpreted as absence proof.

The exact 142 ms root cause remains open; no further hook redesign or commit is
authorized by this diagnostic result.
