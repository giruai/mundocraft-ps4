// Copyright © 2026 Mikołaj Mikołajczyk
// SPDX-License-Identifier: MIT
// Native stat layout and dual-size translation follow the MIT-licensed
// orbis_stat.cpp in the pinned OpenOrbis compatibility source.

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>

extern "C" int _open(const char *path, int flags, int mode);
extern "C" int _fstat(int fd, void *out);

namespace orbis {
const char *anchorPath(const char *path, char *buffer, size_t buffer_size);
}

namespace {

template <typename T>
T read_native(const unsigned char *raw, size_t offset)
{
	T value{};
	std::memcpy(&value, raw + offset, sizeof(value));
	return value;
}

void read_time(const unsigned char *raw, size_t offset, timespec &out)
{
	out.tv_sec = static_cast<decltype(out.tv_sec)>(read_native<int64_t>(raw, offset));
	out.tv_nsec = static_cast<decltype(out.tv_nsec)>(read_native<int64_t>(raw, offset + 8));
}

void translate(const unsigned char *raw, struct stat *out)
{
	std::memset(out, 0, sizeof(*out));
	out->st_dev = static_cast<decltype(out->st_dev)>(read_native<uint32_t>(raw, 0x00));
	out->st_ino = static_cast<decltype(out->st_ino)>(read_native<uint32_t>(raw, 0x04));
	out->st_mode = static_cast<decltype(out->st_mode)>(read_native<uint16_t>(raw, 0x08));
	out->st_nlink = static_cast<decltype(out->st_nlink)>(read_native<uint16_t>(raw, 0x0a));
	out->st_uid = static_cast<decltype(out->st_uid)>(read_native<uint32_t>(raw, 0x0c));
	out->st_gid = static_cast<decltype(out->st_gid)>(read_native<uint32_t>(raw, 0x10));
	out->st_rdev = static_cast<decltype(out->st_rdev)>(read_native<uint32_t>(raw, 0x14));
	read_time(raw, 0x18, out->st_atim);
	read_time(raw, 0x28, out->st_mtim);
	read_time(raw, 0x38, out->st_ctim);
	const int64_t size = read_native<int64_t>(raw, 0x48);
	out->st_size = static_cast<decltype(out->st_size)>(size);
	out->st_blocks = static_cast<decltype(out->st_blocks)>(read_native<int64_t>(raw, 0x50));
	out->st_blksize = static_cast<decltype(out->st_blksize)>(read_native<uint32_t>(raw, 0x58));
	out->st_flags = read_native<uint32_t>(raw, 0x5c);
	out->st_gen = read_native<uint32_t>(raw, 0x60);
	read_time(raw, 0x68, out->st_birthtim);
	// SDK libc++ filesystem was built against native offsets and reads st_size
	// at 0x48, while code compiled against public sys/stat.h reads it at 0x50.
	// Mirror the existing compatibility translation for both consumers.
	std::memcpy(reinterpret_cast<unsigned char *>(out) + 0x48, &size, sizeof(size));
}

static_assert(sizeof(struct stat) == 128, "SDK stat size changed");
static_assert(offsetof(struct stat, st_ino) == 0x04, "SDK inode offset changed");
static_assert(offsetof(struct stat, st_size) == 0x50, "SDK size offset changed");
static_assert(offsetof(struct stat, st_ctim) + 8 == 0x48,
		"dual-size compatibility offset changed");

} // namespace

extern "C" int __wrap_stat(const char *path, struct stat *out)
{
	if (!path || !out) {
		errno = EFAULT;
		return -1;
	}
	char anchored[512] = {};
	const char *native_path = orbis::anchorPath(path, anchored, sizeof(anchored));
	// Both sceKernelStat and the POSIX stat import zero file identity on the
	// title data mount. An opened path's native _fstat reports the inode that
	// SQLite also records on its database fd. O_NONBLOCK prevents a FIFO or
	// device path from hanging this metadata query. This requires read access.
	const int fd = _open(native_path, O_RDONLY | O_NONBLOCK, 0);
	if (fd < 0)
		return -1;
	unsigned char raw[256] = {};
	const int rc = _fstat(fd, raw);
	const int stat_errno = errno;
	const int close_rc = close(fd);
	if (rc != 0) {
		errno = stat_errno;
		return rc;
	}
	if (close_rc != 0)
		return -1;
	translate(raw, out);
	return 0;
}

extern "C" int __wrap_lstat(const char *path, struct stat *out)
{
	// The title's data and app mounts do not contain symbolic links; this is
	// the same documented behavior as the pinned compatibility lstat adapter.
	return __wrap_stat(path, out);
}
