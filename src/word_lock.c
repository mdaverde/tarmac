#include <unistd.h>
#include <stdbool.h>
#include "./lock_state.c"
#include "./thread_parker.c"
#include "./spinwait.c"

struct thread_data {
	struct thread_parker parker;
	struct thread_data *queue_tail;
	struct thread_data *prev;
	struct thread_data *next;
};

struct thread_data td_init()
{
	struct thread_data td = {
		.parker = tp_init(),
		.queue_tail = NULL,
		.prev = NULL,
		.next = NULL,
	};
	return td;
}

void with_thread_data(void func(struct thread_data *)) {
	struct thread_data *ptr = NULL;
	func(ptr);
}

struct word_lock {
	_Atomic(size_t) lock_state;
};

struct word_lock wl_init() {
	struct word_lock wl;
	atomic_init(&wl.lock_state, 0);
	return wl;
}

// Barge lock technique
void wl_lock(struct word_lock *wl)
{
	size_t expected = 0;
	if (atomic_compare_exchange_weak(&wl->lock_state, &expected, LOCKED_BIT) == 0) {
		return;
	}

	// Lock slow
	struct spin_wait sw = sw_init();
	size_t lock_state = atomic_load(&wl->lock_state);
	while (true) {
		if (!ls_is_locked(lock_state)) {
			if (atomic_compare_exchange_weak(&wl->lock_state, &lock_state, lock_state | LOCKED_BIT) == 0) {
				return;
			} else {
				lock_state = atomic_load(&wl->lock_state);
			}
			continue;
		}

	}
}

