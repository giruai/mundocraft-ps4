// Translate OpenOrbis SDK flock arguments to the PS4 native fcntl ABI.
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// libkernel exports _fcntl. The SDK libc fcntl wrapper passes its argument
// through unchanged, but SDK fcntl.h describes a different flock layout and
// lock-type values from the native FreeBSD-derived kernel interface.
extern int _fcntl(int fd, int command, ...);

struct native_flock {
	int64_t l_start;
	int64_t l_len;
	int32_t l_pid;
	int16_t l_type;
	int16_t l_whence;
	int32_t l_sysid;
};

enum {
	NATIVE_F_RDLCK = 1,
	NATIVE_F_UNLCK = 2,
	NATIVE_F_WRLCK = 3,
	NATIVE_F_DUPFD_CLOEXEC = 17
};

_Static_assert(sizeof(struct flock) == 32, "unexpected SDK flock size");
_Static_assert(offsetof(struct flock, l_type) == 0, "unexpected SDK flock type");
_Static_assert(offsetof(struct flock, l_whence) == 2, "unexpected SDK flock whence");
_Static_assert(offsetof(struct flock, l_start) == 8, "unexpected SDK flock start");
_Static_assert(offsetof(struct flock, l_len) == 16, "unexpected SDK flock length");
_Static_assert(offsetof(struct flock, l_pid) == 24, "unexpected SDK flock pid");
_Static_assert(sizeof(struct native_flock) == 32, "unexpected native flock size");
_Static_assert(offsetof(struct native_flock, l_start) == 0, "unexpected native flock start");
_Static_assert(offsetof(struct native_flock, l_len) == 8, "unexpected native flock length");
_Static_assert(offsetof(struct native_flock, l_pid) == 16, "unexpected native flock pid");
_Static_assert(offsetof(struct native_flock, l_type) == 20, "unexpected native flock type");
_Static_assert(offsetof(struct native_flock, l_whence) == 22, "unexpected native flock whence");
_Static_assert(offsetof(struct native_flock, l_sysid) == 24, "unexpected native flock sysid");
_Static_assert(F_GETLK == 11 && F_SETLK == 12 && F_SETLKW == 13,
		"unexpected lock command values");
_Static_assert(F_RDLCK == 0 && F_WRLCK == 1 && F_UNLCK == 2,
		"unexpected SDK lock type values");

static int to_native_lock_type(short type)
{
	switch (type) {
	case F_RDLCK: return NATIVE_F_RDLCK;
	case F_WRLCK: return NATIVE_F_WRLCK;
	case F_UNLCK: return NATIVE_F_UNLCK;
	default: return -1;
	}
}

static int from_native_lock_type(short type)
{
	switch (type) {
	case NATIVE_F_RDLCK: return F_RDLCK;
	case NATIVE_F_WRLCK: return F_WRLCK;
	case NATIVE_F_UNLCK: return F_UNLCK;
	default: return -1;
	}
}

static int translate_lock(int fd, int command, struct flock *requested)
{
	if (!requested) {
		errno = EINVAL;
		return -1;
	}
	const int native_type = to_native_lock_type(requested->l_type);
	if (native_type < 0) {
		errno = EINVAL;
		return -1;
	}
	struct native_flock native = {
		.l_start = requested->l_start,
		.l_len = requested->l_len,
		.l_pid = requested->l_pid,
		.l_type = (int16_t)native_type,
		.l_whence = requested->l_whence,
		.l_sysid = 0
	};
	const int previous_errno = errno;
	const int rc = _fcntl(fd, command, &native);
	if (rc < 0)
		return rc;
	if (command == F_GETLK) {
		const int sdk_type = from_native_lock_type(native.l_type);
		if (sdk_type < 0) {
			errno = EIO;
			return -1;
		}
		requested->l_start = native.l_start;
		requested->l_len = native.l_len;
		requested->l_pid = native.l_pid;
		requested->l_type = (short)sdk_type;
		requested->l_whence = native.l_whence;
	}
	errno = previous_errno;
	return rc;
}

static int forward_fcntl(int fd, int command, va_list args)
{
	int rc;
	const int previous_errno = errno;
	switch (command) {
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		return translate_lock(fd, command, va_arg(args, struct flock *));
	case F_GETFD:
	case F_GETFL:
	case F_GETOWN:
		rc = _fcntl(fd, command);
		break;
	case F_DUPFD:
	case F_SETFD:
	case F_SETFL:
	case F_SETOWN:
		rc = _fcntl(fd, command, va_arg(args, int));
		break;
	case F_DUPFD_CLOEXEC:
		rc = _fcntl(fd, NATIVE_F_DUPFD_CLOEXEC, va_arg(args, int));
		break;
	default:
		// Other SDK commands include Linux-only values with incompatible
		// native meanings. Never guess whether a variadic argument exists.
		errno = EINVAL;
		return -1;
	}
	if (rc >= 0)
		errno = previous_errno;
	return rc;
}

int fcntl(int fd, int command, ...)
{
	va_list args;
	va_start(args, command);
	const int rc = forward_fcntl(fd, command, args);
	va_end(args);
	return rc;
}

int fcntl64(int fd, int command, ...)
{
	va_list args;
	va_start(args, command);
	const int rc = forward_fcntl(fd, command, args);
	va_end(args);
	return rc;
}
