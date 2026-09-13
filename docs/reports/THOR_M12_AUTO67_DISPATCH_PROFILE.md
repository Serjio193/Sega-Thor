# THOR M12 AUTO67 dispatch critical-path profile

Date: 2026-09-13
Baseline: `019fed68d7e906daabe184f3b74017c853b78ce3`
Scope: dispatch path only; no AUTO68 or architecture expansion.

## Result

The automated NEW-lease burst passes the dispatcher gate. The real BizHawk
run does not pass smooth-gameplay acceptance: a 142 ms frame spike remains
while targeted captures are active, but the measured Python dispatcher path
stays below 1.176 ms maximum. The no-capsule control reached 600 frames with
23 ms maximum and no >33 ms or >50 ms spike. The remaining problem is therefore
in the BizHawk-facing targeted capture/hook path, not claim-lock or worker
dispatch.

## Real run

The native operator window was launched with the real BizHawk run. It covered
600 frames in 12.844 s with 16 prestarted workers, 323 leases, 323 returns,
peak busy 11, 33 active collisions, 33 merges, 1,496 rolling-window
overwrites, and raw backlog `NONEXISTENT`. Average/max seed age was
0.069344/0.219 s. Duplicate active claims: 0.

Frame spikes were 443 above 16 ms, one above 33 ms, and one above 50 ms. The
largest was frame 424 at 142 ms, correlated with leases `L0000022F`,
`L00000227`, `L00000231`, and `L0000022A`.

## T0-T8 timings

All values are microseconds and are min / p50 / p95 / p99 / max.

| Stage | Timings |
|---|---:|
| T0→T1 knowledge lookup | 2 / 12 / 27 / 36 / 256 |
| T1→T2 claim acquire | 1 / 4 / 8 / 10 / 18 |
| T2→T3 capsule allocate/reset | 4 / 16 / 35 / 73 / 456 |
| T3→T4 pre-context copy | 0 / 0 / 1 / 4 / 7 |
| T4→T5 filter descriptor submit | 0 / 0 / 0 / 0 / 1 |
| T5→T6 worker message submit | 0 / 0 / 2 / 4 / 14 |
| T6→T7 claim-lock release | 2 / 19 / 50 / 96 / 135 |
| T7→T8 worker start | 2 / 9 / 760 / 978 / 1,105 |
| T0→T8 total | 29 / 94 / 824 / 1,023 / 1,176 |

Claim-lock hold time was 6 / 8 / 34 / 67 / 112 us. Local Lua filtered-hook
installation timing was 0 / 0 / 0 / 1 / 1 ms. The filtered-hook change reduced
callbacks from 91,348 in the prior run to 13,501.

## Automated gate

A burst of 512 unique NEW events produced 512 leases without a large
dispatch-correlated stall: T0→T8 p95 27 us, max 2,067 us; claim-lock p95
32 us, max 80 us.

Conclusion: the dispatcher/worker pool is hot and bounded, but the real
BizHawk capture path still needs a separately authorized follow-up. No user
acceptance test is requested from this result, and no commit or push was made.
