#include <stdlib.h>
#include <stdbool.h>

const size_t LOCKED_BIT = 1;
const size_t QUEUE_LOCKED_BIT = 2;
const size_t QUEUE_MASK = !3;

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

size_t ls_set_queue_head(size_t lock_state, void *queue_head)
{
	return (lock_state & !QUEUE_MASK) | (size_t)queue_head;
}

