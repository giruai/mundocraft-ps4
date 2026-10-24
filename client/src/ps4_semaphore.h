// PS4 kernel semaphore backend for Luanti.
// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

// Tests substitute declarations, never the implementation below.
#ifdef PS4_SEMAPHORE_NATIVE_HEADER
#include PS4_SEMAPHORE_NATIVE_HEADER
#else
#include <orbis/libkernel.h>
#endif
#include "ps4_platform.h"
#include <atomic>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <limits>

class Ps4Semaphore {
public:
	explicit Ps4Semaphore(int initial = 0)
	{
		const int saved = errno;
		if (initial < 0)
			fail("initial-count", static_cast<int32_t>(ORBIS_KERNEL_ERROR_EINVAL));
		const int32_t result = sceKernelCreateSema(&handle_, "LuantiSemaphore",
				ORBIS_KERNEL_SEMA_ATTR_TH_FIFO, initial, INT_MAX, nullptr);
		if (result != 0)
			fail("create", result);
		trace(Operation::Create, result, static_cast<unsigned>(initial));
		errno = saved;
	}

	~Ps4Semaphore()
	{
		const int saved = errno;
		const int32_t result = sceKernelDeleteSema(handle_);
		if (result != 0)
			fail("delete", result);
		errno = saved;
	}

	Ps4Semaphore(const Ps4Semaphore &) = delete;
	Ps4Semaphore &operator=(const Ps4Semaphore &) = delete;
	Ps4Semaphore(Ps4Semaphore &&) = delete;
	Ps4Semaphore &operator=(Ps4Semaphore &&) = delete;

	void post(unsigned num = 1)
	{
		const int saved = errno;
		if (!num || num > static_cast<unsigned>(INT_MAX))
			fail("post-count", static_cast<int32_t>(ORBIS_KERNEL_ERROR_EINVAL));
		const int32_t result = sceKernelSignalSema(handle_, static_cast<int>(num));
		if (result != 0)
			fail("signal", result);
		trace(Operation::Post, result, num);
		errno = saved;
	}

	void wait()
	{
		const int saved = errno;
		int32_t result;
		do {
			result = sceKernelWaitSema(handle_, 1, nullptr);
		} while (is(result, ORBIS_KERNEL_ERROR_EINTR));
		if (result != 0)
			fail("wait", result);
		trace(Operation::Wait, result, 0);
		errno = saved;
	}

	bool wait(unsigned time_ms)
	{
		const int saved = errno;
		if (!time_ms) {
			int32_t result;
			do {
				result = sceKernelPollSema(handle_, 1);
			} while (is(result, ORBIS_KERNEL_ERROR_EINTR));
			// Native PollSema reports insufficient count as EBUSY, whereas
			// Luanti's POSIX-facing nonblocking contract uses errno EAGAIN.
			if (result != 0 && !is(result, ORBIS_KERNEL_ERROR_EBUSY))
				fail("poll", result);
			trace(Operation::Poll, result, 0);
			errno = result == 0 ? saved : EAGAIN;
			return result == 0;
		}

		// Native timeout is relative uint32 microseconds, not milliseconds.
		// A monotonic deadline covers interrupted waits and chunks beyond
		// UINT32_MAX us without overflow, truncation or early expiry.
		using Clock = std::chrono::steady_clock;
		const auto deadline = Clock::now() + std::chrono::milliseconds(time_ms);
		// POSIX timed waits still acquire an available token when preempted
		// past their deadline. Try once before testing elapsed time; the
		// deadline above includes time spent polling or descheduled here.
		if (wait(0)) {
			errno = saved;
			return true;
		}
		for (;;) {
			const auto now = Clock::now();
			if (now >= deadline) {
				trace(Operation::TimedWait,
					static_cast<int32_t>(ORBIS_KERNEL_ERROR_ETIMEDOUT), time_ms);
				errno = ETIMEDOUT;
				return false;
			}
			const auto remaining = deadline - now;
			auto micros = std::chrono::duration_cast<std::chrono::microseconds>(remaining);
			if (micros < remaining)
				micros += std::chrono::microseconds(1); // round up, never early
			const uint64_t count = static_cast<uint64_t>(micros.count());
			OrbisKernelUseconds timeout = static_cast<OrbisKernelUseconds>(
				count > std::numeric_limits<OrbisKernelUseconds>::max() ?
				std::numeric_limits<OrbisKernelUseconds>::max() : count);
			const int32_t result = sceKernelWaitSema(handle_, 1, &timeout);
			if (result == 0) {
				trace(Operation::TimedWait, result, time_ms);
				errno = saved;
				return true;
			}
			if (!is(result, ORBIS_KERNEL_ERROR_ETIMEDOUT) &&
					!is(result, ORBIS_KERNEL_ERROR_EINTR))
				fail("timed-wait", result);
		}
	}

private:
	static_assert(sizeof(OrbisKernelUseconds) == 4 &&
		std::numeric_limits<OrbisKernelUseconds>::is_integer &&
		!std::numeric_limits<OrbisKernelUseconds>::is_signed,
		"OpenOrbis timeout ABI must be uint32 microseconds");
	enum class Operation { Create, Post, Wait, Poll, TimedWait };
	OrbisKernelSema handle_{};

	static bool is(int32_t result, uint32_t code)
	{
		return static_cast<uint32_t>(result) == code;
	}

	[[noreturn]] static void fail(const char *operation, int32_t result)
	{
		ps4port::log("PS4_SEMAPHORE fatal backend=native operation=%s result=0x%08x",
			operation, static_cast<unsigned>(static_cast<uint32_t>(result)));
		std::abort(); // also fail closed in NDEBUG builds
	}

	static void trace(Operation operation, int32_t result, unsigned value)
	{
		static std::atomic<unsigned> tickets[5]{};
		static const char *names[] = {"create", "post", "wait", "poll", "timed-wait"};
		const unsigned index = static_cast<unsigned>(operation);
		unsigned count = tickets[index].load(std::memory_order_relaxed);
		while (count < 4) {
			if (tickets[index].compare_exchange_weak(count, count + 1,
					std::memory_order_relaxed)) {
				const int saved = errno;
				ps4port::log("PS4_SEMAPHORE backend=native operation=%s result=0x%08x value=%u",
					names[index], static_cast<unsigned>(static_cast<uint32_t>(result)), value);
				errno = saved;
				break;
			}
		}
	}
};
