"""Compatibility migration for the bounded AUTO67 live-chain table."""


def ensure_live_chain_schema(connection):
    columns = {row["name"] for row in connection.execute("PRAGMA table_info(live_chain)")}
    if "record_class" not in columns:
        with connection:
            connection.execute("ALTER TABLE live_chain ADD COLUMN record_class TEXT NOT NULL DEFAULT 'SEED_ONLY'")
    connection.execute("CREATE INDEX IF NOT EXISTS live_chain_class ON live_chain(record_class)")
