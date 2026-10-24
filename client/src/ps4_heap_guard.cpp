// SPDX-License-Identifier: LGPL-2.1-or-later
#include "ps4_heap_guard.h"

#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>

#if defined(__ORBIS__)
#include <orbis/libkernel.h>
#else
#include <unistd.h>
#endif

extern "C" {
void *__real_malloc(std::size_t);
void *__real_calloc(std::size_t, std::size_t);
void *__real_realloc(void *, std::size_t);
void __real_free(void *);
void *__real_aligned_alloc(std::size_t, std::size_t);
int __real_posix_memalign(void **, std::size_t, std::size_t);
void *__real_valloc(std::size_t);
std::size_t __real_malloc_usable_size(void *);
}

namespace {

// Isolate the SDK allocator's concurrent entry separately from registry locking.
// No registry lock is held across these calls. A native allocator may call
// another allocation API internally, so same-thread nesting must not relock.
std::atomic<unsigned> g_real_allocator_lock{0};
std::atomic<uint64_t> g_real_allocator_calls{0};
std::atomic<uint64_t> g_real_allocator_contentions{0};
thread_local unsigned g_real_allocator_depth = 0;

class RealAllocatorScope {
public:
 RealAllocatorScope() noexcept
 {
  if (g_real_allocator_depth++ != 0)
   return;
  g_real_allocator_calls.fetch_add(1, std::memory_order_relaxed);
  if (!ps4_heap_guard::kSerializeRealAllocator)
   return;
  unsigned expected = 0;
  if (g_real_allocator_lock.compare_exchange_strong(expected, 1,
    std::memory_order_acquire, std::memory_order_relaxed))
   return;
  g_real_allocator_contentions.fetch_add(1, std::memory_order_relaxed);
  do {
   while (g_real_allocator_lock.load(std::memory_order_relaxed) != 0) {
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#endif
   }
   expected = 0;
  } while (!g_real_allocator_lock.compare_exchange_weak(expected, 1,
    std::memory_order_acquire, std::memory_order_relaxed));
 }
 ~RealAllocatorScope()
 {
  if (--g_real_allocator_depth == 0 && ps4_heap_guard::kSerializeRealAllocator)
   g_real_allocator_lock.store(0, std::memory_order_release);
 }
 RealAllocatorScope(const RealAllocatorScope &) = delete;
 RealAllocatorScope &operator=(const RealAllocatorScope &) = delete;
};

template <typename Function, typename... Args>
auto call_real(Function function, Args... args) noexcept -> decltype(function(args...))
{
 RealAllocatorScope scope;
 return function(args...);
}

using ps4_heap_guard::Stats;
constexpr std::size_t kCanarySize = 16;
constexpr std::size_t kBucketCount = ps4_heap_guard::kRegistryCapacity * 4;
constexpr uint32_t kEmpty = 0;
constexpr uint32_t kTombstone = UINT32_MAX;
constexpr uint8_t kMalloc = 1;
constexpr uint8_t kCalloc = 2;
constexpr uint8_t kAligned = 3;
constexpr uint8_t kPosixAligned = 4;
constexpr uint8_t kValloc = 5;

struct Entry {
	void *pointer;
	size_t requested;
	size_t alignment;
	uintptr_t allocation_site;
	bool live;
	bool reserved;
	uint8_t kind;
};

Entry g_entries[ps4_heap_guard::kRegistryCapacity]{};
uint32_t g_buckets[kBucketCount]{};
uint32_t g_free_slots[ps4_heap_guard::kRegistryCapacity]{};
size_t g_free_count = 0;
size_t g_tombstones = 0;
bool g_initialized = false;
std::atomic_flag g_registry_lock = ATOMIC_FLAG_INIT;
uint64_t g_guarded_allocations = 0;
uint64_t g_bypassed_allocations = 0;
uint64_t g_live_allocations = 0;
uint64_t g_peak_live_allocations = 0;
uint64_t g_live_requested_bytes = 0;
uint64_t g_peak_requested_bytes = 0;
thread_local unsigned g_hook_depth = 0;

class DepthGuard {
public:
	DepthGuard() noexcept { ++g_hook_depth; }
	~DepthGuard() { --g_hook_depth; }
	DepthGuard(const DepthGuard &) = delete;
	DepthGuard &operator=(const DepthGuard &) = delete;
};

class RegistryLock {
public:
	RegistryLock() noexcept
	{
		while (g_registry_lock.test_and_set(std::memory_order_acquire)) {
		}
		m_locked = true;
	}
	~RegistryLock() { unlock(); }
	void unlock() noexcept
	{
		if (m_locked) {
			g_registry_lock.clear(std::memory_order_release);
			m_locked = false;
		}
	}
	RegistryLock(const RegistryLock &) = delete;
	RegistryLock &operator=(const RegistryLock &) = delete;
	private:
	bool m_locked = false;
};

static_assert(ps4_heap_guard::kRegistryCapacity >= 16 &&
		(ps4_heap_guard::kRegistryCapacity &
		(ps4_heap_guard::kRegistryCapacity - 1)) == 0,
		"heap guard capacity must be a power of two and at least 16");

void initialize_registry_locked() noexcept
{
	if (g_initialized)
		return;
	for (size_t i = 0; i < ps4_heap_guard::kRegistryCapacity; ++i)
		g_free_slots[i] = uint32_t(ps4_heap_guard::kRegistryCapacity - i - 1);
	g_free_count = ps4_heap_guard::kRegistryCapacity;
	g_initialized = true;
}

size_t hash_pointer(const void *pointer) noexcept
{
	uintptr_t value = reinterpret_cast<uintptr_t>(pointer);
	value ^= value >> 17;
	value *= uintptr_t(0xed5ad4bbU);
	value ^= value >> 11;
	return size_t(value) & (kBucketCount - 1);
}

size_t find_bucket_locked(const void *pointer) noexcept
{
	const size_t start = hash_pointer(pointer);
	for (size_t probe = 0; probe < kBucketCount; ++probe) {
		const size_t slot = (start + probe) & (kBucketCount - 1);
		const uint32_t value = g_buckets[slot];
		if (value == kEmpty)
			return kBucketCount;
		if (value != kTombstone &&
				g_entries[value - 1].pointer == pointer)
			return slot;
	}
	return kBucketCount;
}

uint32_t reserve_entry() noexcept
{
	RegistryLock lock;
	initialize_registry_locked();
	if (g_free_count == 0)
		return UINT32_MAX;
	const uint32_t index = g_free_slots[--g_free_count];
	g_entries[index].reserved = true;
	return index;
}

void release_reservation(uint32_t index) noexcept
{
	RegistryLock lock;
	g_entries[index] = {};
	g_free_slots[g_free_count++] = index;
}

void insert_bucket_locked(uint32_t index) noexcept
{
	const size_t start = hash_pointer(g_entries[index].pointer);
	size_t tombstone = kBucketCount;
	for (size_t probe = 0; probe < kBucketCount; ++probe) {
		const size_t slot = (start + probe) & (kBucketCount - 1);
		if (g_buckets[slot] == kTombstone && tombstone == kBucketCount)
			tombstone = slot;
		if (g_buckets[slot] == kEmpty) {
			const size_t destination = tombstone == kBucketCount ? slot : tombstone;
			if (tombstone != kBucketCount)
				--g_tombstones;
			g_buckets[destination] = index + 1;
			return;
		}
	}
	// Four buckets per registry record makes this unreachable in normal use.
	if (tombstone != kBucketCount) {
		--g_tombstones;
		g_buckets[tombstone] = index + 1;
	}
}

void compact_buckets_locked() noexcept
{
	for (size_t i = 0; i < kBucketCount; ++i)
		g_buckets[i] = kEmpty;
	g_tombstones = 0;
	for (uint32_t i = 0; i < ps4_heap_guard::kRegistryCapacity; ++i) {
		if (!g_entries[i].live)
			continue;
		const size_t start = hash_pointer(g_entries[i].pointer);
		for (size_t probe = 0; probe < kBucketCount; ++probe) {
			const size_t slot = (start + probe) & (kBucketCount - 1);
			if (g_buckets[slot] == kEmpty) {
				g_buckets[slot] = i + 1;
				break;
			}
		}
	}
}

void publish_entry(uint32_t index, void *pointer, size_t requested,
		size_t alignment, uint8_t kind, uintptr_t site) noexcept
{
	RegistryLock lock;
	Entry &entry = g_entries[index];
	entry.pointer = pointer;
	entry.requested = requested;
	entry.alignment = alignment;
	entry.allocation_site = site;
	entry.kind = kind;
	entry.reserved = false;
	entry.live = true;
	insert_bucket_locked(index);
	++g_guarded_allocations;
	++g_live_allocations;
	g_live_requested_bytes += requested;
	if (g_live_allocations > g_peak_live_allocations)
		g_peak_live_allocations = g_live_allocations;
	if (g_live_requested_bytes > g_peak_requested_bytes)
		g_peak_requested_bytes = g_live_requested_bytes;
}

void note_bypass() noexcept
{
	RegistryLock lock;
	++g_bypassed_allocations;
}

bool add_canary_size(size_t size, size_t &total) noexcept
{
	if (size > std::numeric_limits<size_t>::max() - kCanarySize)
		return false;
	total = size + kCanarySize;
	return true;
}

bool round_to_alignment(size_t size, size_t alignment, size_t &rounded) noexcept
{
	const size_t remainder = size % alignment;
	if (!remainder) {
		rounded = size;
		return true;
	}
	const size_t extra = alignment - remainder;
	if (size > std::numeric_limits<size_t>::max() - extra)
		return false;
	rounded = size + extra;
	return true;
}

void *real_allocate(uint8_t kind, size_t alignment, size_t requested,
		size_t physical, bool zero, bool &success) noexcept
{
	(void)requested;
	success = true;
	if (kind == kPosixAligned) {
		void *pointer = nullptr;
		const int error = call_real(__real_posix_memalign, &pointer, alignment, physical);
		if (error) {
			errno = error;
			success = false;
			return nullptr;
		}
		return pointer;
	}
	if (kind == kAligned) {
		size_t padded = 0;
		if (!round_to_alignment(physical, alignment, padded)) {
			errno = ENOMEM;
			success = false;
			return nullptr;
		}
		void *pointer = call_real(__real_aligned_alloc, alignment, padded);
		if (!pointer)
			success = false;
		return pointer;
	}
	if (kind == kValloc) {
		void *pointer = call_real(__real_valloc, physical);
		if (!pointer)
			success = false;
		return pointer;
	}
	void *pointer = zero ? call_real(__real_calloc, 1, physical) : call_real(__real_malloc, physical);
	if (!pointer)
		success = false;
	return pointer;
}

#define PS4_CALLER_SITE reinterpret_cast<uintptr_t>(__builtin_extract_return_addr( \
		__builtin_return_address(0)))

uint8_t canary_byte(const void *pointer, size_t size, size_t offset) noexcept
{
	uintptr_t value = reinterpret_cast<uintptr_t>(pointer) ^
			(size * uintptr_t(0x9e3779b1U));
	value ^= value >> 13;
	return uint8_t(0xa7U ^ uint8_t(value >> ((offset & 7) * 8)) ^
			uint8_t(offset * 29U));
}

void write_canary(void *pointer, size_t size) noexcept
{
	auto *tail = static_cast<uint8_t *>(pointer) + size;
	for (size_t i = 0; i < kCanarySize; ++i)
		tail[i] = canary_byte(pointer, size, i);
}

bool canary_is_intact(const void *pointer, size_t size) noexcept
{
	const auto *tail = static_cast<const uint8_t *>(pointer) + size;
	for (size_t i = 0; i < kCanarySize; ++i) {
		if (tail[i] != canary_byte(pointer, size, i))
			return false;
	}
	return true;
}

class Buffer {
public:
	explicit Buffer(char *data, size_t capacity) : m_data(data), m_capacity(capacity) {}
	void literal(const char *text) noexcept
	{
		while (*text && m_size < m_capacity)
			m_data[m_size++] = *text++;
	}
	void decimal(uint64_t value) noexcept
	{
		char digits[24];
		size_t count = 0;
		do {
			digits[count++] = char('0' + value % 10);
			value /= 10;
		} while (value && count < sizeof(digits));
		while (count && m_size < m_capacity)
			m_data[m_size++] = digits[--count];
	}
	void hex(uintptr_t value) noexcept
	{
		static constexpr char digits[] = "0123456789abcdef";
		literal("0x");
		bool started = false;
		for (int shift = int(sizeof(value) * 8) - 4; shift >= 0; shift -= 4) {
			const unsigned nibble = unsigned((value >> shift) & 0xfU);
			if (nibble || started || shift == 0) {
				if (m_size < m_capacity)
					m_data[m_size++] = digits[nibble];
				started = true;
			}
		}
	}
	size_t size() const noexcept { return m_size; }
	void terminate() noexcept
	{
		if (m_size < m_capacity)
			m_data[m_size] = '\0';
	}
private:
	char *m_data;
	size_t m_capacity;
	size_t m_size = 0;
};

void output(const char *buffer) noexcept
{
#if defined(__ORBIS__)
	(void)sceKernelDebugOutText(0, buffer);
#else
	size_t length = 0;
	while (buffer[length])
		++length;
	(void)::write(STDERR_FILENO, buffer, length);
#endif
}

[[noreturn]] void fail_canary(const Entry &entry, uintptr_t detection_site,
		const char *operation) noexcept
{
	char text[256];
	Buffer buffer(text, sizeof(text) - 1);
	buffer.literal("PS4_HEAP_GUARD tail-corruption op=");
	buffer.literal(operation);
	buffer.literal(" alloc_site=");
	buffer.hex(entry.allocation_site);
	buffer.literal(operation[0] == 'f' ? " free_site=" : " detect_site=");
	buffer.hex(detection_site);
	buffer.literal(" ptr=");
	buffer.hex(reinterpret_cast<uintptr_t>(entry.pointer));
	buffer.literal(" size=");
	buffer.decimal(entry.requested);
	buffer.literal("\n");
	buffer.terminate();
	output(text);
	std::abort();
}

bool get_entry(void *pointer, Entry &copy, uint32_t *index_out = nullptr) noexcept
{
	RegistryLock lock;
	const size_t bucket = find_bucket_locked(pointer);
	if (bucket == kBucketCount)
		return false;
	const uint32_t index = g_buckets[bucket] - 1;
	copy = g_entries[index];
	if (index_out)
		*index_out = index;
	return true;
}

void check_all(const char *stage, uintptr_t detection_site) noexcept
{
	Entry bad{};
	uint64_t checked = 0;
	{
		RegistryLock lock;
		initialize_registry_locked();
		for (size_t i = 0; i < ps4_heap_guard::kRegistryCapacity; ++i) {
			const Entry &entry = g_entries[i];
			if (!entry.live)
				continue;
			++checked;
			if (!canary_is_intact(entry.pointer, entry.requested)) {
				bad = entry;
				break;
			}
		}
		if (bad.pointer)
			lock.unlock();
	}
	if (bad.pointer)
		fail_canary(bad, detection_site, "scan");
	char text[192];
	Buffer buffer(text, sizeof(text) - 1);
	buffer.literal("PS4_HEAP_GUARD scan stage=");
	if (stage)
		buffer.literal(stage);
	buffer.literal(" checked=");
	buffer.decimal(checked);
	buffer.literal(" result=pass\n");
	buffer.terminate();
	output(text);
}

bool erase_entry(void *pointer, uint32_t expected_index, Entry &copy) noexcept
{
	RegistryLock lock;
	const size_t bucket = find_bucket_locked(pointer);
	if (bucket == kBucketCount || g_buckets[bucket] - 1 != expected_index)
		return false;
	copy = g_entries[expected_index];
	g_buckets[bucket] = kTombstone;
	++g_tombstones;
	g_live_requested_bytes -= copy.requested;
	--g_live_allocations;
	g_entries[expected_index] = {};
	g_free_slots[g_free_count++] = expected_index;
	if (g_tombstones > kBucketCount / 4)
		compact_buckets_locked();
	return true;
}

void *allocate_guarded(size_t requested, uint8_t kind, size_t alignment,
		bool zero, uintptr_t site) noexcept
{
	size_t physical = 0;
	if (!add_canary_size(requested, physical)) {
		errno = ENOMEM;
		return nullptr;
	}
	const uint32_t index = reserve_entry();
	if (index == UINT32_MAX) {
		bool success = true;
		void *pointer = real_allocate(kind, alignment, requested, requested,
				zero, success);
		if (success && pointer)
			note_bypass();
		return pointer;
	}
	bool success = true;
	void *pointer = real_allocate(kind, alignment, requested, physical,
			zero, success);
	const int saved_errno = errno;
	if (!success || !pointer) {
		release_reservation(index);
		errno = saved_errno;
		return nullptr;
	}
	write_canary(pointer, requested);
	publish_entry(index, pointer, requested, alignment, kind, site);
	errno = saved_errno;
	return pointer;
}

void free_tracked(void *pointer, uintptr_t site, bool &tracked) noexcept
{
	tracked = false;
	Entry entry{};
	uint32_t index = 0;
	if (!get_entry(pointer, entry, &index))
		return;
	tracked = true;
	if (!canary_is_intact(pointer, entry.requested))
		fail_canary(entry, site, "free");
	Entry removed{};
	if (!erase_entry(pointer, index, removed))
		return;
	call_real(__real_free, pointer);
}

void *reallocate_tracked(void *pointer, size_t requested, uintptr_t site) noexcept
{
	Entry old{};
	uint32_t index = 0;
	if (!get_entry(pointer, old, &index))
		return call_real(__real_realloc, pointer, requested);
	if (!canary_is_intact(pointer, old.requested))
		fail_canary(old, site, "realloc");
	if (requested == 0) {
		bool tracked = false;
		free_tracked(pointer, site, tracked);
		return nullptr;
	}
	size_t physical = 0;
	if (!add_canary_size(requested, physical)) {
		errno = ENOMEM;
		return nullptr;
	}
	// Remove the record while realloc may move/free the raw allocation. The
	// reserved slot cannot be reused, and no registry lock spans allocator code.
	bool detached = false;
	{
		RegistryLock lock;
		const size_t old_bucket = find_bucket_locked(pointer);
		if (old_bucket == kBucketCount || g_buckets[old_bucket] - 1 != index) {
			// Concurrent realloc/free of one pointer is caller-side undefined.
		} else {
			g_buckets[old_bucket] = kTombstone;
			++g_tombstones;
			g_entries[index].live = false;
			g_entries[index].reserved = true;
			detached = true;
		}
	}
	if (!detached) {
		errno = EINVAL;
		return nullptr;
	}
	void *replacement = call_real(__real_realloc, pointer, physical);
	const int saved_errno = errno;
	if (!replacement) {
		// realloc failure leaves the old raw allocation intact.
		write_canary(pointer, old.requested);
		RegistryLock lock;
		Entry &entry = g_entries[index];
		entry = old;
		entry.live = true;
		entry.reserved = false;
		insert_bucket_locked(index);
		if (g_tombstones > kBucketCount / 4)
			compact_buckets_locked();
		errno = saved_errno;
		return nullptr;
	}
	write_canary(replacement, requested);
	{
		RegistryLock lock;
		Entry &entry = g_entries[index];
		entry.pointer = replacement;
		entry.requested = requested;
		entry.allocation_site = site;
		entry.live = true;
		entry.reserved = false;
		insert_bucket_locked(index);
		g_live_requested_bytes = g_live_requested_bytes - old.requested + requested;
		++g_guarded_allocations;
		if (g_live_requested_bytes > g_peak_requested_bytes)
			g_peak_requested_bytes = g_live_requested_bytes;
		if (g_tombstones > kBucketCount / 4)
			compact_buckets_locked();
	}
	errno = saved_errno;
	return replacement;
}

} // namespace

namespace ps4_heap_guard {

Stats get_stats() noexcept
{
	RegistryLock lock;
	initialize_registry_locked();
	return {g_guarded_allocations, g_bypassed_allocations, g_live_allocations,
			g_peak_live_allocations, g_live_requested_bytes,
			g_peak_requested_bytes,
			g_real_allocator_calls.load(std::memory_order_relaxed),
			g_real_allocator_contentions.load(std::memory_order_relaxed)};
}

void check(const char *stage, std::uintptr_t detection_site) noexcept
{
	DepthGuard depth;
	check_all(stage, detection_site);
}

} // namespace ps4_heap_guard

extern "C" void ps4_heap_guard_report(const char *stage) noexcept
{
	DepthGuard depth;
	const Stats stats = ps4_heap_guard::get_stats();
	char text[256];
	Buffer buffer(text, sizeof(text) - 1);
	buffer.literal("PS4_HEAP_GUARD stage=");
	if (stage)
		buffer.literal(stage);
	buffer.literal(" live=");
	buffer.decimal(stats.live_allocations);
	buffer.literal(" live_bytes=");
	buffer.decimal(stats.live_requested_bytes);
	buffer.literal(" peak_live=");
	buffer.decimal(stats.peak_live_allocations);
	buffer.literal(" peak_bytes=");
	buffer.decimal(stats.peak_requested_bytes);
	buffer.literal(" guarded=");
	buffer.decimal(stats.guarded_allocations);
	buffer.literal(" bypassed=");
	buffer.decimal(stats.bypassed_allocations);
	buffer.literal("\n");
	buffer.terminate();
	output(text);
 Buffer real(text, sizeof(text) - 1);
 real.literal("PS4_HEAP_REAL stage=");
 if (stage) real.literal(stage);
 real.literal(" serialize=");
 real.decimal(ps4_heap_guard::kSerializeRealAllocator ? 1 : 0);
 real.literal(" calls="); real.decimal(stats.real_allocator_calls);
 real.literal(" contended="); real.decimal(stats.real_allocator_contentions);
 real.literal("\n"); real.terminate(); output(text);
}

extern "C" __attribute__((noinline)) void ps4_heap_guard_check(const char *stage) noexcept
{
	ps4_heap_guard::check(stage, PS4_CALLER_SITE);
}

extern "C" __attribute__((noinline)) void *__wrap_malloc(size_t size)
{
	if (g_hook_depth)
		return call_real(__real_malloc, size);
	DepthGuard depth;
	return allocate_guarded(size, kMalloc, alignof(std::max_align_t), false,
			PS4_CALLER_SITE);
}

extern "C" __attribute__((noinline)) void *__wrap_calloc(size_t count, size_t size)
{
	if (g_hook_depth)
		return call_real(__real_calloc, count, size);
	DepthGuard depth;
	if (size && count > std::numeric_limits<size_t>::max() / size) {
		errno = ENOMEM;
		return nullptr;
	}
	const size_t requested = count * size;
	return allocate_guarded(requested, kCalloc, alignof(std::max_align_t), true,
			PS4_CALLER_SITE);
}

extern "C" __attribute__((noinline)) void *__wrap_realloc(void *pointer, size_t size)
{
	if (g_hook_depth)
		return call_real(__real_realloc, pointer, size);
	DepthGuard depth;
	if (!pointer)
		return allocate_guarded(size, kMalloc, alignof(std::max_align_t), false,
				PS4_CALLER_SITE);
	return reallocate_tracked(pointer, size, PS4_CALLER_SITE);
}

extern "C" __attribute__((noinline)) void __wrap_free(void *pointer)
{
	if (!pointer)
		return;
	if (g_hook_depth) {
		call_real(__real_free, pointer);
		return;
	}
	DepthGuard depth;
	const int saved_errno = errno;
	bool tracked = false;
	free_tracked(pointer, PS4_CALLER_SITE, tracked);
	if (!tracked)
		call_real(__real_free, pointer);
	errno = saved_errno;
}

extern "C" __attribute__((noinline)) void *__wrap_aligned_alloc(size_t alignment,
		size_t size)
{
	if (g_hook_depth)
		return call_real(__real_aligned_alloc, alignment, size);
	DepthGuard depth;
	if (!alignment || (alignment & (alignment - 1)) || size % alignment) {
		errno = EINVAL;
		return nullptr;
	}
	return allocate_guarded(size, kAligned, alignment, false, PS4_CALLER_SITE);
}

extern "C" __attribute__((noinline)) int __wrap_posix_memalign(void **memptr,
		size_t alignment, size_t size)
{
	if (g_hook_depth)
		return call_real(__real_posix_memalign, memptr, alignment, size);
	DepthGuard depth;
	if (!memptr || alignment < sizeof(void *) || (alignment & (alignment - 1)))
		return EINVAL;
	size_t total = 0;
	if (!add_canary_size(size, total))
		return ENOMEM;
	const uint32_t index = reserve_entry();
	if (index == UINT32_MAX) {
		void *pointer = nullptr;
		const int error = call_real(__real_posix_memalign, &pointer, alignment, size);
		if (!error) {
			*memptr = pointer;
			note_bypass();
		}
		return error;
	}
	void *pointer = nullptr;
	const int error = call_real(__real_posix_memalign, &pointer, alignment, total);
	if (error) {
		release_reservation(index);
		return error;
	}
	write_canary(pointer, size);
	publish_entry(index, pointer, size, alignment, kPosixAligned, PS4_CALLER_SITE);
	*memptr = pointer;
	return 0;
}

extern "C" __attribute__((noinline)) void *__wrap_valloc(size_t size)
{
	if (g_hook_depth)
		return call_real(__real_valloc, size);
	DepthGuard depth;
	return allocate_guarded(size, kValloc, 0, false, PS4_CALLER_SITE);
}

extern "C" __attribute__((noinline)) size_t __wrap_malloc_usable_size(void *pointer)
{
	if (g_hook_depth)
		return call_real(__real_malloc_usable_size, pointer);
	DepthGuard depth;
	Entry entry{};
	if (pointer && get_entry(pointer, entry))
		return entry.requested;
	return call_real(__real_malloc_usable_size, pointer);
}
