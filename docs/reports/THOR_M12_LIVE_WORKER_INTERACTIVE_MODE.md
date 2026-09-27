# M12 Live Worker interactive mode

The desktop Worker Control launcher runs repeated native Worker captures
without a configured cycle count or wall-clock timeout. The operator configures
only Worker count and chain depth; those values apply to the next run. Every
accepted capture is independently checked by the host and ACKed before its
Worker is reused. The 2H status window remains a separate, replaceable snapshot
consumer.

The mode ends when EmuHawk exits. To protect the evidence disk, the host asks
Lua to stop between complete Worker rounds when free space reaches a 1 GiB
reserve. This is free **disk space**, not RAM. Available RAM and native/process
budgets are measured and calculated separately at launch. Startup preflight
also requires the disk reserve. The Worker Control window labels the disk
growth and automatic stop explicitly. Runtime FLOW_V1 bytes and their segment
index are appended to `continuous-runtime-evidence/`; host-side per-cycle state
and binary Wave files are bounded. The measured evidence-growth rate appears
in the Worker Control window.

This interactive mode deliberately does not run Cartographer, Archivist, or the
canonical ROM-range post-run audit. Its receipt is
`live-worker-interactive-receipt.json` with status
`STOPPED_AFTER_EMUHAWK_EXIT` or `STOPPED_DISK_RESERVE`; it is not a bounded
campaign `PASS`. The existing 100-cycle-per-Worker 2B proof path remains the
acceptance path and is unchanged. Native Worker/FLOW behavior, 1B scaling
semantics, production AUTO67, predecessor logic, and SOURCE_OWNED are unchanged.

## Validation

- Focused Worker Control tests: 20/20 PASS, including constant-memory status
  tailing and exact disk-spooled FLOW_V1 records.
- The first desktop use exposed two defects: the dashboard publisher used
  `format_bytes` without importing it, and the ACK loop failed if the CPU stream
  did not advance in the very first frame after the last ACK. The publisher
  import is fixed and the Lua loop now waits up to 120 frames for progress,
  while still failing if the stream remains stationary.
- Exact ROM linkage regression: PASS.
- Python compilation and runtime CLI help: PASS.
- Actual BizHawk stop-signal smokes: two clean `STOPPED_DISK_RESERVE` runs,
  with 8 Workers × 6 cycles (48/48 segments) and 8 Workers × 5 cycles
  (40/40 segments), followed by a regression run after the two fixes with 8
  Workers × 356 cycles (2,848/2,848 segments). Every Worker returned FREE in
  all three runs, and none of the receipts claims a bounded-campaign PASS.
- A prior interactive smoke crossed 100 captures per Worker and continued to
  106 captures per Worker (848 segments); the process was then forcibly
  terminated for cleanup. Its lifecycle counters confirm 105 full ACKed cycles
  per Worker, with the final capture in flight, so it is not a clean-close
  acceptance proof.
- No 100-cycle proof campaign was rerun. Manual clicking of the EmuHawk close
  button was not part of these headless process-level smoke checks; a nonzero
  EmuHawk process exit is reported as an error rather than a normal close.
- Source-file limit and `git diff --check`: PASS.
