// Native startup conformance check; host mocks do not establish console behavior.
// SPDX-License-Identifier: MIT
#include "ps4_semaphore_check.h"
#include "ps4_semaphore.h"
#include "ps4_platform.h"
#ifndef PS4_SEMAPHORE_NATIVE_HEADER
#include <orbis/libkernel.h>
#endif
#include <pthread.h>
#include <atomic>
#include <cerrno>

namespace {
struct WakeTest {
    Ps4Semaphore ready{0};
    Ps4Semaphore wake{0};
    Ps4Semaphore done{0};
    std::atomic<bool> received{false};
};
void *wake_worker(void *value)
{
    auto &test = *static_cast<WakeTest *>(value);
    test.ready.post();
    test.received.store(test.wake.wait(500), std::memory_order_relaxed);
    test.done.post();
    return nullptr;
}
bool failure(const char *stage)
{
    ps4port::log("SEMAPHORE_CHECK FAIL stage=%s errno=%d", stage, errno);
    return false;
}
}

bool ps4_semaphore_selftest()
{
    ps4port::log("SEMAPHORE_CHECK begin backend=sceKernel");
    {
    Ps4Semaphore tokens(2);
    if (!tokens.wait(0) || !tokens.wait(0))
        return failure("initial_count");
    if (tokens.wait(0) || errno != EAGAIN)
        return failure("empty_poll");
    tokens.post(3);
    for (int i = 0; i < 3; ++i)
        if (!tokens.wait(0)) return failure("multi_post");
    if (tokens.wait(0)) return failure("count_exhausted");
    const auto begin = sceKernelGetProcessTime();
    const bool unexpected_token = tokens.wait(20);
    const int wait_errno = errno;
    const auto elapsed = sceKernelGetProcessTime() - begin;
    ps4port::log("SEMAPHORE_CHECK timed_wait token=%u errno=%d elapsed_us=%llu",
        unsigned(unexpected_token), wait_errno,
        static_cast<unsigned long long>(elapsed));
    if (unexpected_token || wait_errno != ETIMEDOUT || elapsed < 5000)
        return failure("timed_wait");
    WakeTest test;
    pthread_t thread{};
    const int start_error = pthread_create(&thread, nullptr, wake_worker, &test);
    if (start_error != 0) {
        ps4port::log("SEMAPHORE_CHECK thread_create rc=%d", start_error);
        return false;
    }
    const bool ready = test.ready.wait(1000);
    test.wake.post();
    const bool done = test.done.wait(1000);
    const int join_error = pthread_join(thread, nullptr);
    if (!ready || !done || join_error != 0 ||
        !test.received.load(std::memory_order_relaxed))
        return failure("worker_wake");
    } // native handles have been successfully deleted before PASS
    ps4port::log("SEMAPHORE_CHECK PASS backend=sceKernel count=2 post=3 poll=empty timeout=20ms worker_wake=ok");
    return true;
}
