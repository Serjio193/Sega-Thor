PRAGMA foreign_keys = ON;
CREATE TABLE IF NOT EXISTS metadata (key TEXT PRIMARY KEY, value TEXT NOT NULL);
CREATE TABLE IF NOT EXISTS environment (id TEXT PRIMARY KEY, payload TEXT NOT NULL);
CREATE TABLE IF NOT EXISTS scenario (
  id TEXT PRIMARY KEY, environment_id TEXT NOT NULL REFERENCES environment(id), payload TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS trace (
  id TEXT PRIMARY KEY, scenario_id TEXT NOT NULL REFERENCES scenario(id), schema TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS receipt (
  raw_hash TEXT NOT NULL, source TEXT NOT NULL, trace_id TEXT NOT NULL REFERENCES trace(id),
  PRIMARY KEY(raw_hash,source)
);
CREATE TABLE IF NOT EXISTS epoch (
  trace_id TEXT NOT NULL REFERENCES trace(id), number INTEGER NOT NULL,
  state_hash TEXT NOT NULL, PRIMARY KEY(trace_id,number)
);
CREATE TABLE IF NOT EXISTS event (
  id TEXT PRIMARY KEY, trace_id TEXT NOT NULL, epoch_no INTEGER NOT NULL, seq INTEGER NOT NULL,
  kind TEXT NOT NULL, payload TEXT NOT NULL, UNIQUE(trace_id,seq),
  FOREIGN KEY(trace_id,epoch_no) REFERENCES epoch(trace_id,number)
);
CREATE TABLE IF NOT EXISTS location (id TEXT PRIMARY KEY, payload TEXT NOT NULL);
CREATE TABLE IF NOT EXISTS value_version (
  id TEXT PRIMARY KEY, event_id TEXT NOT NULL REFERENCES event(id),
  location_id TEXT NOT NULL REFERENCES location(id), slot INTEGER NOT NULL,
  bit_offset INTEGER NOT NULL, bit_width INTEGER NOT NULL,
  value_hex TEXT, status TEXT NOT NULL, UNIQUE(event_id,slot)
);
CREATE TABLE IF NOT EXISTS temporal_link (
  id TEXT PRIMARY KEY, source TEXT NOT NULL REFERENCES value_version(id),
  target TEXT NOT NULL REFERENCES value_version(id), kind TEXT NOT NULL,
  status TEXT NOT NULL CHECK(status='UNKNOWN')
);
CREATE TABLE IF NOT EXISTS relation (
  id TEXT PRIMARY KEY, source TEXT NOT NULL REFERENCES location(id),
  target TEXT NOT NULL REFERENCES location(id), kind TEXT NOT NULL, context TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS witness (
  relation_id TEXT NOT NULL REFERENCES relation(id), event_id TEXT NOT NULL REFERENCES event(id),
  status TEXT NOT NULL, PRIMARY KEY(relation_id,event_id)
);
CREATE INDEX IF NOT EXISTS event_order ON event(trace_id,epoch_no,seq);
CREATE INDEX IF NOT EXISTS version_location ON value_version(location_id,event_id);
CREATE INDEX IF NOT EXISTS incoming_link ON temporal_link(target);
