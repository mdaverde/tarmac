#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include "./src/word_lock.c"

#define TEST_ERROR -1
#define TEST_SUCCESS 0

#define run_test(test_name, test_result)                                                           \
	{                                                                                              \
		int saved_result = test_result;                                                            \
		if (saved_result == TEST_SUCCESS) {                                                        \
			printf("%s - success\n", test_name);                                                   \
		} else if (saved_result == TEST_ERROR) {                                                   \
			printf("%s - ERROR\n", test_name);                                                     \
		} else {                                                                                   \
			printf("%s - did not understand test response\n", test_name);                          \
		}                                                                                          \
	}

void *child_thread_one_fn(void *tp)
{
	tp = (struct thread_parker *)tp;

	tp_prepare_park(tp);
	tp_park(tp);

	return NULL;
}

void *child_thread_two_fn(void *tp)
{
	tp = (struct thread_parker *)tp;
	sleep(1); // random work
	struct unpark_handle unparker = tp_unpark(tp);
	uh_unpark(&unparker);
	return NULL;
}

int test_thread_parker()
{
	pthread_t child_t1;
	pthread_t child_t2;

	struct thread_parker tp_1 = tp_init();

	pthread_create(&child_t1, NULL, child_thread_one_fn, (void *)&tp_1);
	pthread_create(&child_t2, NULL, child_thread_two_fn, (void *)&tp_1);

	pthread_join(child_t1, NULL);
	pthread_join(child_t2, NULL);

	return TEST_SUCCESS;
}

int test_lock_state()
{
	size_t state = 0;
	assert(ls_is_locked(state) == false);
	state = state | LOCKED_BIT;
	assert(ls_is_locked(state) == true);

	state = 0;
	assert(ls_is_queue_locked(state) == false);
	state = state | QUEUE_LOCKED_BIT;
	assert(ls_is_queue_locked(state) == true);

	state = 0;
	assert(ls_get_queue_head(state) == NULL);
	struct thread_data td = td_init();
	state = ls_set_queue_head(state, &td);
	assert(ls_get_queue_head(state) == &td);

	state = 0;
	state = state | LOCKED_BIT | QUEUE_LOCKED_BIT;
	struct thread_data td2 = td_init();
	state = ls_set_queue_head(state, &td2);
	assert(ls_get_queue_head(state) == &td2);
	assert(ls_is_locked(state) == true);
	assert(ls_is_queue_locked(state) == true);

	return TEST_SUCCESS;
}

struct thread_stack_notifier {
	_Atomic(int) *futex_addr;
	void *stack_addr;
};

void *thread_with_random_str_on_stack(void *stack_notifier)
{
	struct thread_stack_notifier *notifier = (struct thread_stack_notifier *)stack_notifier;

	char *random_str = "hello from child thread";
	notifier->stack_addr = &random_str;

	// If thread exits then stack memory gets blown away so sleep it
	atomic_store(notifier->futex_addr, 1);
	if (futex_wait(notifier->futex_addr, NULL) == -1) {
		perror("futex_wait wrong");
	}
	return NULL;
}

int test_access_thread_stack_data()
{
	pthread_t child_thread;

	_Atomic(int) futex_atom;
	atomic_init(&futex_atom, 0);

	struct thread_stack_notifier stack_notifier = { &futex_atom, NULL };

	pthread_create(&child_thread, NULL, thread_with_random_str_on_stack, (void *)&stack_notifier);

	// Ensure child thread runs before looping on stack_addr
	sleep(1);
	while (stack_notifier.stack_addr == NULL) {
	}

	assert(strcmp(*(char **)stack_notifier.stack_addr, "hello from child thread") == 0);

	futex_wake(&futex_atom);
	pthread_join(child_thread, NULL);

	return TEST_SUCCESS;
}

int test_atomic_compare_exchange_weak_ptr()
{
	_Atomic(int) atom = 0;

	int expected = 1;
	int new_value = 2;
	if (atomic_compare_exchange_weak(&atom, &expected, new_value) == false) {
		assert(expected == 0);
	}

	return TEST_SUCCESS;
}

int word_lock_counter = 0;

void *word_lock_child_add(void *void_wl)
{
	struct word_lock *wl = (struct word_lock *)void_wl;

	wl_lock(wl);
	for (int i = 0; i < 1000; i++) {
		word_lock_counter = word_lock_counter + 1;
	}
	wl_unlock(wl);

	return NULL;
}

void *word_lock_child_sub(void *void_wl)
{
	struct word_lock *wl = (struct word_lock *)void_wl;

	wl_lock(wl);
	for (int i = 0; i < 1000; i++) {
		word_lock_counter = word_lock_counter - 1;
	}
	wl_unlock(wl);

	return NULL;
}

int test_word_lock()
{
	int NUM_THREADS = 20;

	for (int j = 0; j < 1000; j++) {
		struct word_lock wl = wl_init();

		word_lock_counter = 0;

		pthread_t adding_threads[NUM_THREADS];
		pthread_t subtracting_threads[NUM_THREADS];

		for (int i = 0; i < NUM_THREADS; i++) {
			pthread_create(&adding_threads[i], NULL, word_lock_child_add, (void *)&wl);
			pthread_create(&subtracting_threads[i], NULL, word_lock_child_sub, (void *)&wl);
		}

		for (int i = 0; i < NUM_THREADS; i++) {
			pthread_join(adding_threads[i], NULL);
			pthread_join(subtracting_threads[i], NULL);
		}

		assert(word_lock_counter == 0);
	}

	return TEST_SUCCESS;
}

int main()
{
	run_test("thread_parker", test_thread_parker());
	run_test("lock_state", test_lock_state());
	run_test("access_thread_stack_data", test_access_thread_stack_data());
	run_test("atomic_compare_exchange_weak_ptr", test_atomic_compare_exchange_weak_ptr());
	run_test("test_word_lock", test_word_lock());
}