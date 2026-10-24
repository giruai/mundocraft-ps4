// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include <cstddef>
#include <cstdint>

#ifndef PS4_HEAP_SERIALIZE_REAL_ALLOCATOR
#define PS4_HEAP_SERIALIZE_REAL_ALLOCATOR 0
#endif

namespace ps4_heap_guard {
constexpr bool kSerializeRealAllocator = PS4_HEAP_SERIALIZE_REAL_ALLOCATOR != 0;

// Fixed bounds make the allocator hooks independent of heap allocation.
// Host saturation tests can override this without shrinking the target table.
#if defined(PS4_HEAP_GUARD_TEST_CAPACITY)
constexpr std::size_t kRegistryCapacity = PS4_HEAP_GUARD_TEST_CAPACITY;
#else
constexpr std::size_t kRegistryCapacity = 524288;
#endif

struct Stats {
	uint64_t guarded_allocations;
	uint64_t bypassed_allocations;
	uint64_t live_allocations;
	uint64_t peak_live_allocations;
	uint64_t live_requested_bytes;
	uint64_t peak_requested_bytes;
	uint64_t real_allocator_calls;
	uint64_t real_allocator_contentions;
};

Stats get_stats() noexcept;
void check(const char *stage, std::uintptr_t detection_site) noexcept;

} // namespace ps4_heap_guard

// Emits fixed-buffer statistics through a low-level platform output function.
extern "C" void ps4_heap_guard_report(const char *stage) noexcept;
extern "C" void ps4_heap_guard_check(const char *stage) noexcept;
