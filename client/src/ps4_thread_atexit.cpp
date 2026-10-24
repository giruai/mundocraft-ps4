// Run C++ thread_local destructors on PS4, where SDK libc lacks this hook.
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <pthread.h>

#include <cerrno>
#include <cstdlib>

namespace {

struct Destructor {
	void (*function)(void *);
	void *object;
	Destructor *next;
};

struct ThreadState {
	Destructor *head;
};

pthread_key_t key;
pthread_once_t key_once = PTHREAD_ONCE_INIT;
int key_error = 0;

void drain_thread(void *value)
{
	auto *state = static_cast<ThreadState *>(value);
	if (!state)
		return;

	// POSIX clears the key before invoking its destructor. Put the state
	// back so destructors may register more thread_local destructors here.
	if (pthread_setspecific(key, state) != 0)
		std::abort();
	while (state->head) {
		Destructor *item = state->head;
		state->head = item->next;
		item->function(item->object);
		std::free(item);
	}
	if (pthread_setspecific(key, nullptr) != 0)
		std::abort();
	std::free(state);
}

void drain_exiting_main_thread()
{
	// pthread key destructors do not run when the initial thread returns
	// from main and libc calls exit. Run that thread's list at normal exit.
	drain_thread(pthread_getspecific(key));
}

void initialize_key()
{
	key_error = pthread_key_create(&key, drain_thread);
	if (key_error != 0)
		return;
	if (std::atexit(drain_exiting_main_thread) != 0) {
		pthread_key_delete(key);
		key_error = ENOMEM;
	}
}

} // namespace

extern "C" void ps4_thread_atexit_cleanup_main()
{
	// The PS4 entry path can return to Home without going through CRT exit.
	const int once_error = pthread_once(&key_once, initialize_key);
	if (once_error || key_error)
		std::abort();
	drain_exiting_main_thread();
}

extern "C" int __cxa_thread_atexit_impl(void (*destructor)(void *),
		void *object, void *dso_handle)
{
	(void)dso_handle; // This executable does not unload code at runtime.
	if (!destructor) {
		errno = EINVAL;
		return -1;
	}
	const int once_error = pthread_once(&key_once, initialize_key);
	if (once_error || key_error) {
		errno = once_error ? once_error : key_error;
		return -1;
	}

	auto *state = static_cast<ThreadState *>(pthread_getspecific(key));
	bool new_state = false;
	if (!state) {
		state = static_cast<ThreadState *>(std::calloc(1, sizeof(*state)));
		if (!state) {
			errno = ENOMEM;
			return -1;
		}
		new_state = true;
	}
	auto *item = static_cast<Destructor *>(std::malloc(sizeof(Destructor)));
	if (!item) {
		if (new_state)
			std::free(state);
		errno = ENOMEM;
		return -1;
	}
	item->function = destructor;
	item->object = object;
	item->next = state->head;
	if (new_state) {
		const int set_error = pthread_setspecific(key, state);
		if (set_error) {
			std::free(item);
			std::free(state);
			errno = set_error;
			return -1;
		}
	}
	state->head = item;
	return 0;
}
