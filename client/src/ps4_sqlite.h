// SPDX-License-Identifier: MIT
#pragma once

// Call once on the main thread, before any Luanti SQLite connection is opened.
// Returns false if native file locking or SQLite's unix-flock VFS is unusable.
// All connections to a database must use this VFS. It serializes readers and
// has no shared-memory methods, so normal WAL mode is unsupported.
bool ps4_sqlite_prepare();
