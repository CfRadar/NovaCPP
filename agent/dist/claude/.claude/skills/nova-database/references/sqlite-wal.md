# SQLite 3.49.1 WAL Concurrency & Tuning Reference

## WAL Mode Architecture

NovaCPP embeds SQLite 3.49.1 directly via `novacpp/sqlite/sqlite3.c`.
Instead of locking the entire database during write transactions, SQLite in Write-Ahead Logging (`WAL`) mode appends changes to a separate `*.db-wal` file.

### Active WAL Pragmas in NovaCPP
```sql
PRAGMA journal_mode = WAL;
PRAGMA synchronous = NORMAL;
PRAGMA busy_timeout = 5000;
PRAGMA foreign_keys = ON;
PRAGMA temp_store = MEMORY;
```

### Benefits:
1. **Readers Never Block Writers**: Concurrent HTTP requests reading data proceed unimpeded while background threads write updates.
2. **Writers Never Block Readers**: Writes proceed asynchronously in the WAL log.
3. **5000ms Busy Timeout**: If another thread is actively committing a transaction, new writers automatically retry with exponential backoff up to 5 seconds before failing.
4. **Checkpointing**: Passive checkpoints (`PRAGMA wal_checkpoint(PASSIVE);`) flush WAL frames into the main database without locking active readers.

