#include <unistd.h>
#include <stdatomic.h>
#include <errno.h>
#include <stdio.h>
#include "futex.c"

// Every thread gets its own parker to facilitate sleeping and waking
struct thread_parker {
	_Atomic(int) futex;
};

struct unpark_handle {
	_Atomic(int) *futex;
};

struct thread_parker tp_init()
{
	struct thread_parker tp = {};
	atomic_init(&tp.futex, 0);
	return tp;
}

int tp_prepare_park(struct thread_parker *tp)
{
	atomic_store_explicit(&tp->futex, 1, memory_order_release);
	return 0;
}

int tp_park(struct thread_parker *tp)
{
	while (atomic_load_explicit(&tp->futex, memory_order_acquire) != 0) {
		if (futex_wait(&tp->futex, NULL) == -1) {
			perror("tp_park futex_wait");
		}
	}
	return 0;
}

// Should be called while holding the queue lock
struct unpark_handle tp_unpark(struct thread_parker *tp)
{
	atomic_store_explicit(&tp->futex, 0, memory_order_release);
	struct unpark_handle handle = { .futex = &tp->futex };
	return handle;
}

int uh_unpark(struct unpark_handle *uh)
{
	if (futex_wake(uh->futex) == -1) {
		perror("futex_wake unpark");
	}
	return 0;
}