#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include "./src/word_lock.c"

#define TEST_ERROR -1
#define TEST_SUCCESS 0

#define assert_test(test_name, test_result) \
	int saved_result = test_result; \
	if (saved_result == TEST_SUCCESS) { \
		printf("%s - success\n", test_name); \
	} else if (saved_result == TEST_ERROR) { \
		printf("%s - error\n", test_name); \
	} else { \
		printf("%s - did not understand test response\n", test_name); \
	}  \


void *child_thread_one_fn(void *tp) {
	tp = (struct thread_parker *)tp;

	tp_prepare_park(tp);
	tp_park(tp);

	return NULL;
}

void *child_thread_two_fn(void *tp) {
	tp = (struct thread_parker *)tp;
	sleep(2);
	struct unpark_handle unparker = tp_unpark(tp);
	uh_unpark(&unparker);
	return NULL;
}

int test_thread_parker() {
	pthread_t child_t1;
	pthread_t child_t2;

	struct thread_parker tp_1 = tp_init();

	pthread_create(&child_t1, NULL, child_thread_one_fn, (void *)&tp_1);
	pthread_create(&child_t2, NULL, child_thread_two_fn, (void *)&tp_1);

	pthread_join(child_t1, NULL);
	pthread_join(child_t2, NULL);

	return TEST_SUCCESS;
}

int main() {
	assert_test("thread_parker", test_thread_parker());
}