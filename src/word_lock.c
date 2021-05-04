#include <unistd.h>
#include <stdbool.h>
#include <stdalign.h>
#include <assert.h>
#include "thread_parker.c"
#include "spinwait.c"

const size_t LOCKED_BIT = 1;
const size_t QUEUE_LOCKED_BIT = 2;
const size_t QUEUE_MASK = ~3;

bool ls_is_locked(size_t lock_state)
{
	return (lock_state & LOCKED_BIT) != 0;
}

bool ls_is_queue_locked(size_t lock_state)
{
	return (lock_state & QUEUE_LOCKED_BIT) != 0;
}

struct thread_data *ls_get_queue_head(size_t lock_state)
{
	return (struct thread_data *)(lock_state & QUEUE_MASK);
}

size_t ls_set_queue_head(size_t lock_state, struct thread_data *queue_head)
{
	return (lock_state & ~QUEUE_MASK) | (size_t)queue_head;
}

struct thread_data {
	struct thread_parker parker;
	struct thread_data *queue_tail;
	struct thread_data *prev;
	struct thread_data *next;
};

struct thread_data td_init()
{
	// By ensuring alignment, we can safely store lock state within the least
	// significant bits of its pointer
	assert(alignof(struct thread_data) > ~QUEUE_MASK);
	struct thread_data td = {
		.parker = tp_init(),
		.queue_tail = NULL,
		.prev = NULL,
		.next = NULL,
	};
	return td;
}

struct word_lock {
	_Atomic(size_t) lock_state;
};

struct word_lock wl_init()
{
	struct word_lock wl = {};
	atomic_init(&wl.lock_state, 0);
	return wl;
}

// Barge lock technique
void wl_lock(struct word_lock *wl)
{
	// Fast lock
	size_t fresh_lock = 0;
	if (atomic_compare_exchange_weak(&wl->lock_state, &fresh_lock, LOCKED_BIT) == true) {
		return;
	}

	// Slow lock
	struct spin_wait sw = sw_init();
	size_t lock_state = atomic_load(&wl->lock_state);
	while (true) {
		// Spin until lock bit is set either by this thread (in which case
		// we immediately return) or another thread
		if (!ls_is_locked(lock_state)) {
			if (atomic_compare_exchange_weak(&wl->lock_state, &lock_state,
											 lock_state | LOCKED_BIT) == true) {
				return;
			}
			continue;
		}

		// If locked, spin until a queue head gets set. A queue head
		// gets set only if a parking occurs after spinning several
		// times
		if (ls_get_queue_head(lock_state) == NULL && sw_spin(&sw)) {
			lock_state = atomic_load(&wl->lock_state);
			continue;
		}

		// Add to queue
		struct thread_data td = td_init();
		tp_prepare_park(&td.parker);

		struct thread_data *queue_head = ls_get_queue_head(lock_state);
		if (queue_head == NULL) {
			td.queue_tail = &td;
			// Not currently necessary since td isn't stored in
			// thread local storage yet
			td.prev = NULL;
		} else {
			td.queue_tail = NULL;
			td.prev = NULL;
			td.next = queue_head;
		}

		if (atomic_compare_exchange_weak(&wl->lock_state, &lock_state,
										 ls_set_queue_head(lock_state, &td)) == false) {
			continue;
		}

		// Park thread
		tp_park(&td.parker);

		sw_reset(&sw);
		lock_state = atomic_load(&wl->lock_state);
	}
}

void wl_unlock(struct word_lock *wl)
{
	// Fast unlock
	size_t lock_state = atomic_fetch_sub(&wl->lock_state, LOCKED_BIT);
	if (ls_is_queue_locked(lock_state) == true || ls_get_queue_head(lock_state) == NULL) {
		return;
	}

	// Slow unlock
	lock_state = atomic_load(&wl->lock_state);
	while (true) {
		// If another thread is about to unpark a thread or there is no queue, return
		if (ls_is_queue_locked(lock_state) == true || ls_get_queue_head(lock_state) == NULL) {
			return;
		}

		if (atomic_compare_exchange_weak(&wl->lock_state, &lock_state,
										 lock_state | QUEUE_LOCKED_BIT) == true) {
			break;
		}
	}

	// At this point, the queue is locked and we know threads are waiting.
	// No other threads can unlock but can lock
outer_loop:
	while (true) {
		struct thread_data *queue_head = ls_get_queue_head(lock_state);
		struct thread_data *queue_tail = NULL;
		struct thread_data *current = queue_head;

		while (true) {
			queue_tail = current->queue_tail;
			if (queue_tail != NULL) {
				break;
			}
			struct thread_data *next = current->next;
			next->prev = current;
			current = next;
		}

		if (ls_is_locked(lock_state)) {
			if (atomic_compare_exchange_weak(&wl->lock_state, &lock_state,
											 lock_state | !QUEUE_LOCKED_BIT) == true) {
				return;
			} else {
				atomic_thread_fence(memory_order_acquire);
				continue;
			}
		}

		struct thread_data *new_tail = queue_tail->prev;
		if (new_tail == NULL) {
			while (true) {
				if (atomic_compare_exchange_weak(&wl->lock_state, &lock_state,
												 lock_state & LOCKED_BIT) == false) {
					if (ls_get_queue_head(lock_state) == NULL) {
						continue;
					} else {
						atomic_thread_fence(memory_order_acquire);
						goto outer_loop;
					}
				} else {
					break;
				}
			}
		} else {
			atomic_fetch_and(&wl->lock_state, ~QUEUE_LOCKED_BIT);
			queue_head->queue_tail = new_tail;
		}

		struct unpark_handle uh = tp_unpark(&queue_tail->parker);
		uh_unpark(&uh);

		break;
	}
}
