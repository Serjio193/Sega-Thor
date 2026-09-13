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
CREATE TABLE IF NOT EXISTS operation_instance (
  id TEXT PRIMARY KEY, trace_id TEXT NOT NULL REFERENCES trace(id), epoch_no INTEGER NOT NULL,
  exec_seq INTEGER NOT NULL, pc INTEGER NOT NULL, rule_id TEXT NOT NULL, payload TEXT NOT NULL,
  UNIQUE(trace_id,epoch_no,exec_seq)
);
CREATE TABLE IF NOT EXISTS provenance_dependency (
  id TEXT PRIMARY KEY, trace_id TEXT NOT NULL REFERENCES trace(id), source_id TEXT NOT NULL,
  target_id TEXT NOT NULL, role TEXT NOT NULL CHECK(role IN ('VALUE','ADDRESS','CONTROL','EXECUTION')),
  status TEXT NOT NULL CHECK(status IN ('PROVEN','UNKNOWN_TRANSFORM','UNKNOWN_COMPLETENESS')),
  rule_id TEXT NOT NULL, witness_event_id TEXT, payload TEXT NOT NULL,
  UNIQUE(trace_id,source_id,target_id,role,rule_id)
);
CREATE TABLE IF NOT EXISTS ram_byte_version (
  id TEXT PRIMARY KEY, trace_id TEXT NOT NULL REFERENCES trace(id), epoch_no INTEGER NOT NULL,
  address INTEGER NOT NULL, version_no INTEGER NOT NULL, temporal_seq INTEGER NOT NULL,
  value INTEGER NOT NULL, status TEXT NOT NULL, origin TEXT NOT NULL, operation_id TEXT,
  previous_version_id TEXT, payload TEXT NOT NULL,
  UNIQUE(trace_id,epoch_no,address,version_no)
);
CREATE TABLE IF NOT EXISTS ram_write_operation (
  id TEXT PRIMARY KEY, trace_id TEXT NOT NULL REFERENCES trace(id), epoch_no INTEGER NOT NULL,
  temporal_seq INTEGER NOT NULL, execution_instance TEXT NOT NULL, pc INTEGER NOT NULL,
  rule_id TEXT NOT NULL, width INTEGER NOT NULL, effective_address INTEGER NOT NULL,
  byte_start INTEGER NOT NULL, byte_end INTEGER NOT NULL, payload TEXT NOT NULL,
  UNIQUE(trace_id,epoch_no,temporal_seq,id)
);
CREATE TABLE IF NOT EXISTS ram_write_output (
  operation_id TEXT NOT NULL REFERENCES ram_write_operation(id), version_id TEXT NOT NULL REFERENCES ram_byte_version(id),
  address INTEGER NOT NULL, byte_offset INTEGER NOT NULL, previous_version_id TEXT,
  PRIMARY KEY(operation_id,address)
);
CREATE TABLE IF NOT EXISTS ram_coverage (
  certificate_id TEXT PRIMARY KEY, trace_id TEXT NOT NULL REFERENCES trace(id), epoch_no INTEGER NOT NULL,
  start_seq INTEGER NOT NULL, end_seq INTEGER NOT NULL, addresses TEXT NOT NULL,
  evidence_hash TEXT NOT NULL, status TEXT NOT NULL, complete INTEGER NOT NULL, payload TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS v3_execution_instance (
  id TEXT PRIMARY KEY, trace_id TEXT NOT NULL REFERENCES trace(id), epoch_no INTEGER NOT NULL,
  exec_seq INTEGER NOT NULL, pc INTEGER NOT NULL, rule_id TEXT NOT NULL, payload TEXT NOT NULL,
  UNIQUE(trace_id,epoch_no,exec_seq)
);
CREATE TABLE IF NOT EXISTS v3_register_version (
  id TEXT PRIMARY KEY, trace_id TEXT NOT NULL REFERENCES trace(id), epoch_no INTEGER NOT NULL,
  register_name TEXT NOT NULL, bit_offset INTEGER NOT NULL, bit_width INTEGER NOT NULL,
  value INTEGER, version_no INTEGER NOT NULL, temporal_seq INTEGER NOT NULL,
  execution_instance TEXT, operation_id TEXT, status TEXT NOT NULL,
  previous_version_id TEXT, payload TEXT NOT NULL,
  UNIQUE(trace_id,epoch_no,register_name,version_no)
);
CREATE TABLE IF NOT EXISTS v3_register_operation (
  id TEXT PRIMARY KEY, trace_id TEXT NOT NULL REFERENCES trace(id), epoch_no INTEGER NOT NULL,
  temporal_seq INTEGER NOT NULL, execution_instance TEXT NOT NULL, pc INTEGER NOT NULL,
  rule_id TEXT NOT NULL, destination TEXT NOT NULL, bit_offset INTEGER NOT NULL,
  bit_width INTEGER NOT NULL, status TEXT NOT NULL, payload TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS v3_control_fact (
  id TEXT PRIMARY KEY, trace_id TEXT NOT NULL REFERENCES trace(id), execution_id TEXT NOT NULL,
  condition_rule TEXT NOT NULL, branch_execution_id TEXT NOT NULL, taken INTEGER,
  status TEXT NOT NULL, payload TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS v3_execution_relation (
  id TEXT PRIMARY KEY, trace_id TEXT NOT NULL REFERENCES trace(id), kind TEXT NOT NULL,
  source_id TEXT NOT NULL, target_id TEXT NOT NULL, status TEXT NOT NULL, payload TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS v3_dependency (
  id TEXT PRIMARY KEY, trace_id TEXT NOT NULL REFERENCES trace(id), source_id TEXT NOT NULL,
  target_id TEXT NOT NULL, role TEXT NOT NULL CHECK(role IN ('VALUE','ADDRESS','CONTROL','EXECUTION')),
  rule_id TEXT NOT NULL, status TEXT NOT NULL, payload TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS event_order ON event(trace_id,epoch_no,seq);
CREATE INDEX IF NOT EXISTS version_location ON value_version(location_id,event_id);
CREATE INDEX IF NOT EXISTS incoming_link ON temporal_link(target);
CREATE INDEX IF NOT EXISTS provenance_target ON provenance_dependency(target_id);
CREATE INDEX IF NOT EXISTS ram_version_lookup ON ram_byte_version(trace_id,epoch_no,address,temporal_seq);
CREATE INDEX IF NOT EXISTS ram_operation_lookup ON ram_write_operation(trace_id,epoch_no,temporal_seq);
CREATE INDEX IF NOT EXISTS v3_register_lookup ON v3_register_version(trace_id,epoch_no,register_name,temporal_seq);
CREATE INDEX IF NOT EXISTS v3_dependency_target ON v3_dependency(target_id);
