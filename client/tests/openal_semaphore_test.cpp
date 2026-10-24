// SPDX-License-Identifier: LGPL-2.1-or-later
// Link this test with patched OpenAL Soft 1.23.1 common/threads.cpp. Its
// al::semaphore and the production Ps4Semaphore are both compiled unchanged;
// only the SceKernel entry points below are modeled for a host executable.
#include "threads.h"
#include <atomic>
#include <condition_variable>
#include <cstdarg>
#include <cstdio>
#include <functional>
#include <mutex>
#include <system_error>
#include <thread>
#include <type_traits>
#include <sys/wait.h>
#include <unistd.h>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); std::abort(); } } while (false)

struct MockNativeSemaphore {
 std::mutex mutex;
 std::condition_variable wake;
 int count;
 explicit MockNativeSemaphore(int initial):count(initial){}
};
namespace {
std::atomic<int> live{0},created{0},destroyed{0},wait_calls{0},poll_calls{0},signal_calls{0};
std::atomic<int> initial_count{-1};
std::atomic<bool> interrupt_wait{false},interrupt_poll{false},fail_create{false},fail_poll{false};
int32_t error(uint32_t result){return static_cast<int32_t>(result);}
}

namespace ps4port {
void log(const char *, ...) { errno=EIO; }
}

int32_t sceKernelCreateSema(OrbisKernelSema *out,const char *name,uint32_t attr,
 int32_t initial,int32_t maximum,const OrbisKernelSemaOptParam *options)
{
 CHECK(name && attr==ORBIS_KERNEL_SEMA_ATTR_TH_FIFO && !options);
 CHECK(initial>=0 && maximum==INT_MAX);
 if(fail_create) return error(ORBIS_KERNEL_ERROR_EINVAL);
 *out=new MockNativeSemaphore(initial);
 initial_count=initial;++created;++live;errno=EIO;return 0;
}
int32_t sceKernelDeleteSema(OrbisKernelSema sem)
{
 CHECK(sem);delete sem;++destroyed;--live;errno=EIO;return 0;
}
int32_t sceKernelSignalSema(OrbisKernelSema sem,int count)
{
 CHECK(count==1);++signal_calls;
 std::lock_guard<std::mutex> lock(sem->mutex);
 CHECK(sem->count<INT_MAX);sem->count+=count;
 sem->wake.notify_one();errno=EIO;return 0;
}
int32_t sceKernelPollSema(OrbisKernelSema sem,int count)
{
 CHECK(count==1);++poll_calls;errno=EIO;
 if(fail_poll) return error(ORBIS_KERNEL_ERROR_EINVAL);
 if(interrupt_poll.exchange(false)) return error(ORBIS_KERNEL_ERROR_EINTR);
 std::lock_guard<std::mutex> lock(sem->mutex);
 if(!sem->count)return error(ORBIS_KERNEL_ERROR_EBUSY);
 --sem->count;return 0;
}
int32_t sceKernelWaitSema(OrbisKernelSema sem,int32_t count,OrbisKernelUseconds *timeout)
{
 CHECK(count==1 && !timeout);++wait_calls;
 if(interrupt_wait.exchange(false))return error(ORBIS_KERNEL_ERROR_EINTR);
 std::unique_lock<std::mutex> lock(sem->mutex);
 sem->wake.wait(lock,[&]{return sem->count>0;});
 --sem->count;errno=EIO;return 0;
}

static void expect_abort(const std::function<void()> &action)
{
 const pid_t pid=fork();CHECK(pid>=0);
 if(!pid){action();_exit(0);}
 int status=0;CHECK(waitpid(pid,&status,0)==pid);
 CHECK(WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT);
}

int main()
{
 static_assert(!std::is_copy_constructible<al::semaphore>::value,"no handle copies");
 static_assert(noexcept(std::declval<al::semaphore &>().wait()),"OpenAL wait contract");
 static_assert(noexcept(std::declval<al::semaphore &>().try_wait()),"OpenAL poll contract");
 {
  errno=ENOENT;al::semaphore sem;
  CHECK(initial_count==0 && live==1 && errno==ENOENT);
  CHECK(!sem.try_wait() && errno==EAGAIN);
  errno=ENOENT;sem.post();CHECK(errno==ENOENT);
  CHECK(sem.try_wait() && errno==ENOENT);
  CHECK(!sem.try_wait() && errno==EAGAIN);
  errno=ENOENT;
 }
 CHECK(live==0 && errno==ENOENT);
 {
  errno=ENOSPC;al::semaphore sem{3};
  CHECK(initial_count==3 && errno==ENOSPC);
  sem.wait();CHECK(errno==ENOSPC);
  CHECK(sem.try_wait() && errno==ENOSPC);
  CHECK(sem.try_wait() && errno==ENOSPC);
  CHECK(!sem.try_wait() && errno==EAGAIN);
  sem.post();errno=ENOENT;interrupt_poll=true;
  CHECK(sem.try_wait() && errno==ENOENT);
  CHECK(!sem.try_wait() && errno==EAGAIN);
 }
 CHECK(live==0);
 {
  al::semaphore sem;
  const int previous_waits=wait_calls;
  std::atomic<bool> complete{false};
  interrupt_wait=true;
  std::thread consumer([&]{errno=ENOENT;sem.wait();CHECK(errno==ENOENT);complete=true;});
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
  while(wait_calls<previous_waits+2 && std::chrono::steady_clock::now()<deadline)
   std::this_thread::yield();
  CHECK(wait_calls>=previous_waits+2 && !complete);
  errno=ENOSPC;sem.post();CHECK(errno==ENOSPC);
  consumer.join();CHECK(complete);
  CHECK(!sem.try_wait() && errno==EAGAIN);
 }
 CHECK(live==0 && created==destroyed && signal_calls>=3 && poll_calls>=8);
 {
  // The largest representable native count reaches the backend unchanged.
  errno=ENOENT;al::semaphore maximum{static_cast<unsigned>(INT_MAX)};
  CHECK(initial_count==INT_MAX && errno==ENOENT);
  CHECK(maximum.try_wait() && errno==ENOENT);
 }
 const int previous_creates=created;
 for(unsigned value:{static_cast<unsigned>(INT_MAX)+1u,UINT_MAX}) {
  bool rejected=false;
  try {al::semaphore invalid{value};}
  catch(const std::system_error &e) {
   rejected=true;CHECK(e.code()==std::make_error_code(std::errc::value_too_large));
  }
  CHECK(rejected && created==previous_creates && live==0);
 }
 // Unexpected native failures keep the established production fail-closed
 // behavior rather than falling back to the unsupported POSIX semaphore ABI.
 expect_abort([]{fail_create=true;al::semaphore sem;});
 expect_abort([]{al::semaphore sem;fail_poll=true;sem.try_wait();});
 CHECK(live==0 && created==destroyed);
 std::puts("PASS patched OpenAL semaphore: counts, poll, wake, EINTR, errno, cleanup, range rejection, fail-closed native errors");
}
