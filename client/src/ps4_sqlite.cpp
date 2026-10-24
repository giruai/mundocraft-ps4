// SPDX-License-Identifier: MIT
#include "ps4_sqlite.h"
#include "ps4_platform.h"

#include <sqlite3.h>

#include <cerrno>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

constexpr const char *probe_path = "/data/luanti/cache/port-preflight.sqlite";

bool fail(const char *stage, int code)
{
	// Do not print file contents, database messages, or user-supplied paths.
	ps4port::log("SQLITE_PREPARE_FAIL stage=%s code=%d errno=%d", stage, code, errno);
	return false;
}

bool sql_fail(const char *stage, sqlite3 *db, int code)
{
	ps4port::log("SQLITE_PREPARE_FAIL stage=%s code=%d extended=%d syserr=%d",
			stage, code, db ? sqlite3_extended_errcode(db) : 0,
			db ? sqlite3_system_errno(db) : 0);
	return false;
}

bool lock_is_busy(int fd)
{
	errno = 0;
	if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
		flock(fd, LOCK_UN);
		return false;
	}
	return errno == EWOULDBLOCK || errno == EAGAIN;
}

bool native_lock_probe()
{
	const int a = open(probe_path, O_RDWR | O_CREAT, 0600);
	if (a < 0)
		return fail("native-open-a", -1);
	const int b = open(probe_path, O_RDWR, 0600);
	if (b < 0) {
		const int error = errno;
		close(a);
		errno = error;
		return fail("native-open-b", -1);
	}

	bool ok = false;
	do {
		if (flock(a, LOCK_EX | LOCK_NB) != 0) {
			fail("native-lock-a", -1);
			break;
		}
		if (!lock_is_busy(b)) {
			fail("native-contention-before-stat", -1);
			break;
		}
		// The port's stat adapter opens and closes another descriptor to read
		// an inode. POSIX fcntl locks would be lost here; flock must survive.
		struct stat by_path{};
		struct stat by_fd{};
		if (stat(probe_path, &by_path) != 0 || fstat(a, &by_fd) != 0) {
			fail("native-stat", -1);
			break;
		}
		if (!by_path.st_ino || by_path.st_ino != by_fd.st_ino ||
				by_path.st_dev != by_fd.st_dev) {
			fail("native-file-identity", -1);
			break;
		}
		if (!lock_is_busy(b)) {
			fail("native-contention-after-stat", -1);
			break;
		}
		if (flock(a, LOCK_UN) != 0) {
			fail("native-unlock-a", -1);
			break;
		}
		if (flock(b, LOCK_EX | LOCK_NB) != 0) {
			fail("native-lock-b-after-unlock", -1);
			break;
		}
		if (flock(b, LOCK_UN) != 0) {
			fail("native-unlock-b", -1);
			break;
		}
		ok = true;
	} while (false);

	// Closing an fd also releases its flock if an earlier check failed.
	const int close_b = close(b);
	const int close_a = close(a);
	if (close_b != 0 || close_a != 0)
		return fail("native-close", -1);
	return ok;
}

int execute(sqlite3 *db, const char *sql)
{
	return sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
}

bool sqlite_lock_probe()
{
	sqlite3 *a = nullptr;
	sqlite3 *b = nullptr;
	int rc = sqlite3_open_v2(probe_path, &a,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
	if (rc != SQLITE_OK) {
		sql_fail("sqlite-open-a", a, rc);
		if (a)
			sqlite3_close(a);
		return false;
	}
	rc = sqlite3_open_v2(probe_path, &b,
			SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
	if (rc != SQLITE_OK) {
		sql_fail("sqlite-open-b", b, rc);
		if (b)
			sqlite3_close(b);
		sqlite3_close(a);
		return false;
	}

	bool ok = false;
	bool a_transaction = false;
	bool b_transaction = false;
	do {
		rc = execute(a, "CREATE TABLE IF NOT EXISTS port_probe(id INTEGER PRIMARY KEY, value TEXT)");
		if (rc != SQLITE_OK) {
			sql_fail("sqlite-create", a, rc);
			break;
		}
		rc = execute(a, "BEGIN IMMEDIATE");
		if (rc != SQLITE_OK) {
			sql_fail("sqlite-begin-a", a, rc);
			break;
		}
		a_transaction = true;
		rc = execute(a, "INSERT OR REPLACE INTO port_probe VALUES(1, 'first')");
		if (rc != SQLITE_OK) {
			sql_fail("sqlite-write-a", a, rc);
			break;
		}
		rc = execute(b, "BEGIN IMMEDIATE");
		if (rc == SQLITE_OK) {
			b_transaction = true;
			sql_fail("sqlite-contention-before-stat", b, rc);
			break;
		}
		if ((rc & 0xff) != SQLITE_BUSY) {
			sql_fail("sqlite-contention-before-stat", b, rc);
			break;
		}
		struct stat metadata{};
		if (stat(probe_path, &metadata) != 0) {
			fail("sqlite-stat-during-lock", -1);
			break;
		}
		rc = execute(b, "BEGIN IMMEDIATE");
		if (rc == SQLITE_OK) {
			b_transaction = true;
			sql_fail("sqlite-contention-after-stat", b, rc);
			break;
		}
		if ((rc & 0xff) != SQLITE_BUSY) {
			sql_fail("sqlite-contention-after-stat", b, rc);
			break;
		}
		rc = execute(a, "COMMIT");
		if (rc != SQLITE_OK) {
			sql_fail("sqlite-commit-a", a, rc);
			break;
		}
		a_transaction = false;
		rc = execute(b, "BEGIN IMMEDIATE");
		if (rc != SQLITE_OK) {
			sql_fail("sqlite-begin-b-after-commit", b, rc);
			break;
		}
		b_transaction = true;
		rc = execute(b, "INSERT OR REPLACE INTO port_probe VALUES(2, 'second')");
		if (rc != SQLITE_OK) {
			sql_fail("sqlite-write-b", b, rc);
			break;
		}
		rc = execute(b, "COMMIT");
		if (rc != SQLITE_OK) {
			sql_fail("sqlite-commit-b", b, rc);
			break;
		}
		b_transaction = false;
		ok = true;
	} while (false);

	if (b_transaction)
		if (execute(b, "ROLLBACK") != SQLITE_OK)
			ok = sql_fail("sqlite-rollback-b", b, sqlite3_errcode(b));
	if (a_transaction)
		if (execute(a, "ROLLBACK") != SQLITE_OK)
			ok = sql_fail("sqlite-rollback-a", a, sqlite3_errcode(a));
	if (sqlite3_close(b) != SQLITE_OK)
		ok = sql_fail("sqlite-close-b", b, sqlite3_errcode(b));
	if (sqlite3_close(a) != SQLITE_OK)
		ok = sql_fail("sqlite-close-a", a, sqlite3_errcode(a));
	return ok;
}

} // namespace

bool ps4_sqlite_prepare()
{
	if (!native_lock_probe())
		return false;
	// SQLITE_ENABLE_LOCKING_STYLE=1 must be used for the bundled SQLite build.
	// unix-flock owns locks per open file description, so the stat adapter's
	// temporary descriptor cannot release a database connection's lock.
	sqlite3_vfs *vfs = sqlite3_vfs_find("unix-flock");
	if (!vfs)
		return fail("sqlite-unix-flock-unavailable", SQLITE_NOTFOUND);
	const int rc = sqlite3_vfs_register(vfs, 1);
	if (rc != SQLITE_OK || sqlite3_vfs_find(nullptr) != vfs)
		return fail("sqlite-register-unix-flock", rc);
	if (!sqlite_lock_probe())
		return false;
	ps4port::log("SQLITE_PREPARE_PASS vfs=unix-flock");
	return true;
}
